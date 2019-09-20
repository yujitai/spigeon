/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file worker.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/

#include "server/worker.h"

#include <fcntl.h>
#include <sys/time.h>

#include "server/event.h"
#include "server/network.h"
#include "util/status.h"
#include "util/zmalloc.h"
#include "util/scoped_ptr.h"

namespace zf {

static const size_t INITIAL_FD_NUM = 1024;

static void recv_notify(EventLoop *el, 
        IOWatcher *w, 
        int fd, 
        int revents, 
        void *data) 
{
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

/**
 * @brief Callback for connection io events 
 */
void tcp_conn_io_cb(EventLoop *el, 
        IOWatcher *w, 
        int fd, 
        int revents, 
        void *data) 
{
    UNUSED(w);
    UNUSED(data);

    GenericWorker* worker = (GenericWorker*)(el->owner);
    if (revents & EventLoop::READ) {
        worker->tcp_read_io(fd);
    }
    if (revents & EventLoop::WRITE) {
        worker->tcp_write_io(fd);
    } 
}

void udp_conn_io_cb(EventLoop *el, 
        IOWatcher *w, 
        int fd, 
        int revents, 
        void *data) 
{
    UNUSED(w);
    UNUSED(data);

    GenericWorker* worker = (GenericWorker*)(el->owner);
    if (revents & EventLoop::READ) {
        worker->udp_read_io(fd);
    }
    if (revents & EventLoop::WRITE) {
        worker->udp_write_io(fd);
    } 
}

/**
 * Connection timeout callback
 **/
void timeout_cb(EventLoop* el, TimerWatcher* w, void* data) {
    UNUSED(w);

    GenericWorker* worker = (GenericWorker*)(el->owner);
    Connection* c = (Connection*)data;
    worker->process_timeout(c);
}

/**
 * Callback for cron work
 **/
void cron_cb(EventLoop* el, TimerWatcher* w, void* data) 
{
    UNUSED(w);
    UNUSED(el);

    GenericWorker *worker = (GenericWorker*)data;
    worker->process_cron_work();
}

void GenericWorker::tcp_read_io(int fd) {
    assert(fd >= 0);
    
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }

    TCPConnection* c = (TCPConnection*)conns[fd];
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }
  
    int rlen = c->_bytes_expected;
    size_t iblen = sdslen(c->_io_buffer);
    c->_io_buffer = sdsMakeRoomFor(c->_io_buffer, rlen);
    int r = sock_read_data(fd, c->_io_buffer + iblen, rlen);
    if (r == NET_ERROR) {
        log_debug("sock_read_data: return error, close connection");
        close_conn(c);
        return;
    } else if (r == NET_PEER_CLOSED) {
        log_debug("sock_read_data: return 0, peer closed");
        close_conn(c);
        return;
    } else if (r > 0) {
        sdsIncrLen(c->_io_buffer, r);
    }
    c->_last_interaction = el->now();

    // Upper process.
    if (sdslen(c->_io_buffer) - c->_bytes_processed >= c->_bytes_expected) {
        if (WORKER_OK != this->process_io_buffer(c)) {
            log_debug("read_io: user return error, close connection");
            close_conn(c);
        }
    }
}

void GenericWorker::udp_read_io(int fd) {
    assert(fd >= 0);
    
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }

    UDPConnection* c = (UDPConnection*)conns[fd];
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }

    c->_io_buffer = sdsMakeRoomFor(c->_io_buffer, 1024);
    int r = sock_read_data(fd, c->_io_buffer, 1024);
    if (r > 0) {
        sdsIncrLen(c->_io_buffer, r);
    }
    c->_last_interaction - el->now();

    this->process_io_buffer(c);
}

int GenericWorker::add_reply(Connection* c, const Slice& reply) {
    if (dynamic_cast<TCPConnection*>(c)) {
        TCPConnection* tc = dynamic_cast<TCPConnection*>(c);
        tc->_reply_list.push_back(reply);
        tc->_reply_list_size++;
        enable_events(tc, EventLoop::WRITE);
        return tc->_reply_list_size;
    } else if (dynamic_cast<UDPConnection*>(c)) {
        UDPConnection* uc = dynamic_cast<UDPConnection*>(c);
        uc->_reply_list.push_back(reply);
        uc->_reply_list_size++;
        enable_events(uc, EventLoop::WRITE);
        return uc->_reply_list_size;
    }
}

int GenericWorker::reply_list_size(Connection* c) {
    // return c->_reply_list_size;
}

void GenericWorker::tcp_write_io(int fd) {
    assert(fd >= 0);
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }
    TCPConnection* c = (TCPConnection*)conns[fd];
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }

    while (! c->_reply_list.empty()) {
        Slice reply = c->_reply_list.front();
        int w = sock_write_data(fd,
                reply.data() + c->_bytes_written,
                reply.size() - c->_bytes_written);

        if (w == NET_ERROR) {
            log_debug("sock_write_data: return error, close connection");
            close_conn(c);
            return;
        } else if(w == 0) {         /* would block */
            log_warning("write zero bytes, want[%d] fd[%d] ip[%s]", int(reply.size() - c->_bytes_written), c->_fd, c->_ip);
            return;
        } else if ((w + c->_bytes_written) == reply.size()) { /* finish */
            c->_reply_list.pop_front();
            c->_reply_list_size--;
            c->_bytes_written = 0;
            zfree((void*)reply.data());
        } else {
            c->_bytes_written += w;
        }
    }
    c->_last_interaction = el->now();

    /* no more replies to write */
    if (c->_reply_list.empty())
        disable_events(c, EventLoop::WRITE);
}

void GenericWorker::udp_write_io(int fd) {
    assert(fd >= 0);
    if ((unsigned int)fd >= conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }
    UDPConnection* c = (UDPConnection*)conns[fd]; 
    if (c == NULL) {
        log_warning("connection not exists");
        return;
    }

    while (! c->_reply_list.empty()) {
        Slice reply = c->_reply_list.front();
        int w = sock_write_data(fd, reply.data(), reply.size());
        if (w == reply.size()) {
            c->_reply_list.pop_front();
            c->_reply_list_size--;
            zfree((void*)reply.data());
        } 
    }
    c->_last_interaction = el->now();

    /* no more replies to write */
    if (c->_reply_list.empty())
        disable_events(c, EventLoop::WRITE);
}


void GenericWorker::disable_events(Connection *c, int events) {
    el->stop_io_event(c->_watcher, c->_fd, events);
}

void GenericWorker::enable_events(Connection *c, int events) {
    el->start_io_event(c->_watcher, c->_fd, events);
}

void GenericWorker::start_timer(Connection *c) {
    el->start_timer(c->_timer, options.tick);
}

Connection* GenericWorker::new_tcp_conn(int fd) {
    assert(fd >= 0);
    log_debug("[new connection] fd[%d]", fd);

    sock_setnonblock(fd);
    sock_setnodelay(fd);  
    Connection* c = new TCPConnection(fd);
    sock_peer_to_str(fd, c->_ip, &(c->_port));
    c->_last_interaction = el->now();

    // create io event and timer for the connection.
    c->_watcher = el->create_io_event(tcp_conn_io_cb, (void*)c);
    c->_timer = el->create_timer(timeout_cb, (void*)c, true);

    // start io and timer.
    if (options.ssl_open) {
        enable_events(c, EventLoop::READ | EventLoop::WRITE);
    } else {
        enable_events(c, EventLoop::READ);
    }
    start_timer(c);

    if ((unsigned int)fd >= conns.size())
        conns.resize(fd*2, NULL);

    conns[fd] = c;

    return c;
}

Connection* GenericWorker::new_udp_conn(int fd) {
    assert(fd >= 0);
    log_debug("[new udp connection] fd[%d]", fd);

    sock_setnonblock(fd);
    Connection* c = new UDPConnection(fd);
    c->_last_interaction = el->now();

    // create io event and timer for the connection.
    c->_watcher = el->create_io_event(udp_conn_io_cb, (void*)c);
    c->_timer = el->create_timer(timeout_cb, (void*)c, true);

    // start io and timer.
    if (options.ssl_open) {
        enable_events(c, EventLoop::READ | EventLoop::WRITE);
    } else {
        enable_events(c, EventLoop::READ);
    }
    start_timer(c);

    if ((unsigned int)fd >= conns.size())
        conns.resize(fd*2, NULL);

    conns[fd] = c;

    return c;
}

void GenericWorker::close_conn(Connection *c) {
    log_debug("close connection");
    int fd = c->_fd; 
    remove_conn(c);
    close(fd);
}

void GenericWorker::close_all_conns() {
    for (std::vector<Connection*>::iterator it = conns.begin();
         it != conns.end(); ++it)
    {
        if (*it != NULL) close_conn(*it);
    }
}

void GenericWorker::remove_conn(Connection *c) {
    c->_last_interaction = el->now();
    before_remove_conn(c);
    el->delete_io_event(c->_watcher);  
    el->delete_timer(c->_timer);
    /*
    if (c->priv_data_destructor) {
        c->priv_data_destructor(c->priv_data);
    }
    */
    conns[c->_fd] = NULL;
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

GenericWorker::GenericWorker(const GenericServerOptions &o, 
        const std::string& thread_name)
    : Runnable(thread_name), 
      options(o), 
      el(NULL), 
      pipe_watcher(NULL), 
      cron_timer(NULL)
{
    conns.resize(INITIAL_FD_NUM, NULL);
    el = new EventLoop((void*)this, false);
    online_count = 0;
    worker_id = "";
}

GenericWorker::~GenericWorker() {
    close_all_conns();
    delete el;
    //stat_destroy();
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
    struct timeval s, e; 
    gettimeofday(&s, NULL);
    int written = write(notify_send_fd, &msg, sizeof(int));
    gettimeofday(&e, NULL);
    log_debug("[write worker pipe] cost[%luus] msg_type[%d]", TIME_US_DIFF(s, e), msg);

    if (written == sizeof(int))
        return WORKER_OK;
    else
        return WORKER_ERROR;
}

void GenericWorker::process_internal_notify(int msg) {
    int* cfd;

    switch (msg) {
        case QUIT:          
            stop();
            break;
        case TCPCONNECTION:         
            if (mq_pop((void**)&cfd)) {
                new_tcp_conn(*cfd);
                delete cfd;
            }
            break;
        case UDPCONNECTION:
            if (mq_pop((void**)&cfd)) {
                new_udp_conn(*cfd);
                delete cfd;
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

void GenericWorker::process_timeout(Connection* c) {
    if ((el->now() - c->_last_interaction) > 
            (uint64_t)options.connection_timeout)
    {
        log_debug("[time out] close fd[%d]", c->_fd);
        close_conn(c);
    }
}

} // namespace zf


