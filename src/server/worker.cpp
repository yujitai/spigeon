#include <sys/time.h>
#include <fcntl.h>
#include "server/worker.h"
#include "server/event.h"
#include "util/network.h"
#include "util/zmalloc.h"
#include "util/stat.h"
#include "util/scoped_ptr.h"
#include "util/status.h"

namespace store {

static const size_t INITIAL_FD_NUM = 1024;

Connection::Connection(int client_fd)
        : sslConnected(false), fd(client_fd), reply_list_size(0), current_state(0),
          bytes_processed(0), bytes_expected(1), cur_resp_pos(0),ssl(NULL), watcher(NULL), 
          timer(NULL), priv_data(NULL), priv_data_destructor(NULL) {
    querybuf = sdsempty();
}

Connection::~Connection() {
    sdsfree(querybuf);
    std::list<Slice>::iterator it;
    for (it = reply_list.begin(); it != reply_list.end(); ++it) {
        zfree((void*)(*it).data());
    }
    reply_list.clear();
    if (ssl) {
        SSL_shutdown (ssl);
        SSL_free(ssl);
    }
}

static void recv_notify(EventLoop *el, IOWatcher *w, int fd, int revents, void *data) {
    UNUSED(el);
    UNUSED(w);
    UNUSED(revents);
    int msg;
    if (read(fd, &msg, sizeof(int)) != sizeof(int)) {
        log_warning("can't read from noitfy pipe");
        return;
    }
    GenericWorker *worker = (GenericWorker*)data;
    worker->process_internal_notify(msg);
}

// callback for connection io events 
void conn_io_cb(EventLoop *el, IOWatcher *w, int fd, int revents, void *data) {
    UNUSED(w);
    UNUSED(data);
    GenericWorker *worker = (GenericWorker*)(el->owner);
    if (revents & EventLoop::READ) {
        worker->read_query(fd);
    }
    if (revents & EventLoop::WRITE) {
        worker->write_reply(fd);
    } 
}

// connection timeout callback
void timeout_cb(EventLoop *el, TimerWatcher *w, void *data) {
    UNUSED(w);
    GenericWorker *worker = (GenericWorker*)(el->owner);
    Connection *c = (Connection*)data;
    worker->process_timeout(c);
}

// callback for cron work
void cron_cb(EventLoop *el, TimerWatcher *w, void *data) {
    UNUSED(w);
    UNUSED(el);
    GenericWorker *worker = (GenericWorker*)data;
    worker->process_cron_work();
}

void GenericWorker::read_query(int fd) {
    assert(fd >= 0);
    
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }
    Connection *c = conns[fd];
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }
  
    int nread = 0, readlen = c->bytes_expected;
    size_t qblen = sdslen(c->querybuf);
    stat_set_max("max_query_buffer_size", sdsAllocSize(c->querybuf));
    c->querybuf = sdsMakeRoomFor(c->querybuf, readlen);
    nread = sock_read_data(fd, c->querybuf+qblen, readlen);
    if (nread == NET_ERROR) {
        log_debug("sock_read_data: return error, close connection");
        close_conn(c);
        return;
    } else if (nread > 0) {
        sdsIncrLen(c->querybuf, nread);
    }
    c->last_interaction = el->now();

    int ret = process_read_query(c);
    if(WORKER_CONNECTION_REMOVED != ret  && WORKER_OK != ret){
        log_debug("process_read_query: return error, close connection");
        close_conn(c);
    }
}

int GenericWorker::process_read_query(Connection *c) {
    while (sdslen(c->querybuf) - c->bytes_processed >= c->bytes_expected) {
        int ret = process_query_buffer(c);
        if (ret) {
            return ret;
        }
    }
    return WORKER_OK;
}

int GenericWorker::add_reply(Connection *c, const Slice& reply) {
    c->reply_list.push_back(reply);
    c->reply_list_size++;
    stat_set_max("max_reply_size", reply.size());
    stat_set_max("max_reply_list_size", c->reply_list_size);
    enable_events(c, EventLoop::WRITE);
    return c->reply_list_size;
}

int GenericWorker::reply_list_size(Connection *c) {
    return c->reply_list_size;
}

void GenericWorker::write_reply(int fd) {
    assert(fd >= 0);
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }
    Connection *c = conns[fd];
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }

    while (!c->reply_list.empty()) {
        Slice reply = c->reply_list.front();
        int nwritten = sock_write_data(fd,
                                       reply.data() + c->cur_resp_pos,
                                       reply.size() - c->cur_resp_pos);
        if (nwritten == NET_ERROR) {
            log_debug("sock_write_data: return error, close connection");
            close_conn(c);
            return;
        } else if( nwritten == 0) {         /* would block */
            log_warning("write zero bytes, want[%d] fd[%d] ip[%s]", int(reply.size() - c->cur_resp_pos), c->fd, c->ip);
            return;
        } else if ((nwritten + c->cur_resp_pos) == reply.size()) { /* finish */
            c->reply_list.pop_front();
            c->reply_list_size--;
            c->cur_resp_pos = 0;
            zfree((void*)reply.data());
        } else {
            c->cur_resp_pos += nwritten;
        }
    }
    c->last_interaction = el->now();
    /* no more replies to write */
    if (c->reply_list.empty())
        disable_events(c, EventLoop::WRITE);
}

void GenericWorker::disable_events(Connection *c, int events) {
    el->stop_io_event(c->watcher, c->fd, events);
}

void GenericWorker::enable_events(Connection *c, int events) {
    el->start_io_event(c->watcher, c->fd, events);
}

void GenericWorker::start_timer(Connection *c) {
    el->start_timer(c->timer, options.tick);
}

Connection *GenericWorker::new_conn(int fd) {
    assert(fd >= 0);
    log_debug("new connection:%d", fd);
    sock_setnonblock(fd);
    sock_setnodelay(fd);  
    Connection *c = new Connection(fd);
    sock_peer_to_str(fd, c->ip, &(c->port));
    c->last_interaction = el->now();
    c->begin_interaction = c->last_interaction;
    c->last_recv_request = c->last_interaction;
    /* create io event and timeout for the connection */
    c->watcher = el->create_io_event(conn_io_cb, (void*)c);
    c->timer = el->create_timer(timeout_cb, (void*)c, true);
    /* start io and timeout */
    if (options.ssl_open) {
        enable_events(c, EventLoop::READ | EventLoop::WRITE);
    } else {
        enable_events(c, EventLoop::READ);
    }
    start_timer(c);

    if ((unsigned int)fd >= conns.size())
        conns.resize(fd*2, NULL);
    conns[fd] = c;
    stat_incr("current_connections");
    stat_incr("total_connections");
    return c;
}

void GenericWorker::close_conn(Connection *c) {
    log_debug("close connection");
    int socket_fd = c->fd; 
    remove_conn(c);
    close(socket_fd);
}

void GenericWorker::close_all_conns() {
    for (std::vector<Connection*>::iterator it = conns.begin();
         it != conns.end(); ++it)
    {
        if (*it != NULL) close_conn(*it);
    }
}

void GenericWorker::remove_conn(Connection *c) {
    c->last_interaction = el->now();
    before_remove_conn(c);
    el->delete_io_event(c->watcher);  
    el->delete_timer(c->timer);
    if (c->priv_data_destructor) {
        c->priv_data_destructor(c->priv_data);
    }
    stat_decr("current_connections");
    conns[c->fd] = NULL;
    after_remove_conn(c);
    delete c;
}

void GenericWorker::process_cron_work() {
}

int64_t GenericWorker::get_clients_count(){
    return online_count;
}
void GenericWorker::set_clients_count(int64_t count){
     online_count = count;
     return;
}
GenericWorker::GenericWorker(const GenericServerOptions &o, std::string thread_name)
        : Runnable(thread_name), options(o), el(NULL), pipe_watcher(NULL), cron_timer(NULL)
{
    conns.resize(INITIAL_FD_NUM, NULL);
    el = new EventLoop((void*)this, false);
    online_count = 0;
    worker_id = "";
}

GenericWorker::~GenericWorker() {
    close_all_conns();
    delete el;
    stat_destroy();
}

void GenericWorker::set_worker_id(const std::string& id) {
    worker_id = id;
    return;
}

const std::string& GenericWorker::get_worker_id() {
    return worker_id;
}

int GenericWorker::init() {
    int fds[2];
    if (pipe(fds)) {
        log_fatal("can't create notify pipe");
        return WORKER_ERROR;
    }
    notify_recv_fd = fds[0];
    notify_send_fd = fds[1];

    // Listen for notifications from dispatcher thread
    pipe_watcher = el->create_io_event(recv_notify, (void*)this);
    if (pipe_watcher == NULL)
        return WORKER_ERROR;
    el->start_io_event(pipe_watcher, notify_recv_fd, EventLoop::READ);
    // start cron timer
    cron_timer = el->create_timer(cron_cb, (void*)this, true);
    el->start_timer(cron_timer, options.tick);

    return WORKER_OK;
}

void GenericWorker::stop() {
    el->delete_timer(cron_timer);    
    el->delete_io_event(pipe_watcher);
    close(notify_recv_fd);
    close(notify_send_fd);
    el->stop();
    for (size_t i = 0; i < conns.size(); i++) {
        if (conns[i] != NULL)
            close_conn(conns[i]);
    }
}

void GenericWorker::mq_push(void *msg) {
    mq.produce(msg);
}

bool GenericWorker::mq_pop(void **msg) {
    return mq.consume(msg);
}

void GenericWorker::run() {
    el->run();
}

int GenericWorker::notify(int msg) {
    struct timeval s1, e1; 
    gettimeofday(&s1, NULL);
    int written = write(notify_send_fd, &msg, sizeof(int));
    gettimeofday(&e1, NULL);
    log_debug("[Cost_Info] [write worker pipe] cost[%lu(us)] msg[%d]", TIME_US_DIFF(s1, e1), msg);
    if (written == sizeof(int))
        return WORKER_OK;
    else
        return WORKER_ERROR;
}

void GenericWorker::process_internal_notify(int msg) {
    switch (msg) {
        case QUIT:                       // stop
            stop();
            break;
        case NEWCONNECTION:                           // new connection
            int* client_fd;
            if (mq_pop((void**)&client_fd)) {
                new_conn(*client_fd);
                delete client_fd;
            }
            break;
        default:
            process_notify(msg);
            break;
    }
}

void GenericWorker::process_notify(int msg) {
    log_warning("unknow notify: %d", msg);
}

void GenericWorker::process_timeout(Connection *c) {
    if ((el->now()-c->last_interaction) > (unsigned long)options.connection_timeout)
    {
        log_debug("close fd:%d on timeout", c->fd);
        close_conn(c);
    }
}

}
