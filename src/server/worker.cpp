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

#include "server/event.h"
#include "util/zmalloc.h"

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
 * Callback for Connection io events.
 */
void conn_io_cb(EventLoop* el, 
        IOWatcher* w, 
        SOCKET fd, 
        int revents, 
        void* data) 
{
    UNUSED(w);
    UNUSED(data);

    GenericWorker* worker = (GenericWorker*)(el->owner);
    if (revents & EventLoop::READ)
        worker->read_io(fd);

    if (revents & EventLoop::WRITE) 
        worker->write_io(fd);
}

/**
 * Connection timeout callback.
 **/
void timeout_cb(EventLoop* el, TimerWatcher* w, void* data) {
    UNUSED(w);

    GenericWorker* worker = (GenericWorker*)(el->owner);
    Connection* c = (Connection*)data;
    worker->process_timeout(c);
}

/**
 * Callback for cron work.
 **/
void cron_cb(EventLoop* el, TimerWatcher* w, void* data) 
{
    UNUSED(w);
    UNUSED(el);

    GenericWorker *worker = (GenericWorker*)data;
    worker->process_cron_work();
}

GenericWorker::GenericWorker(const GenericServerOptions& o, 
        const std::string& thread_name)
    : Runnable(thread_name), 
      _options(o), 
      _el(NULL), 
      pipe_watcher(NULL), 
      cron_timer(NULL)
{
    _conns.resize(INITIAL_FD_NUM, NULL);
    _el = new EventLoop((void*)this, false);
    _worker_id = "";
}

GenericWorker::~GenericWorker() {
    close_all_conns();
    delete _el;
}

int GenericWorker::initialize() {
    int fds[2];
    if (pipe(fds)) {
        log_fatal("can't create notify pipe");
        return WORKER_ERROR;
    }
    _notify_recv_fd = fds[0];
    _notify_send_fd = fds[1];

    // Listen for notifications from dispatcher thread.
    pipe_watcher = _el->create_io_event(recv_notify, (void*)this);
    if (pipe_watcher == NULL)
        return WORKER_ERROR;
    _el->start_io_event(pipe_watcher, _notify_recv_fd, EventLoop::READ);

    // Start cron timer.
    cron_timer = _el->create_timer(cron_cb, (void*)this, true);
    _el->start_timer(cron_timer, _options.tick);

    return WORKER_OK;
}

void GenericWorker::set_network(NetworkManager* network_manager) {
    _network_manager = network_manager;
}

void GenericWorker::read_io(SOCKET fd) {
    if (fd == INVALID_SOCKET) {
        log_fatal("Invalid socket. fd[%d]", fd);
        return;
    }

    if (fd >= _conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }

    Connection* c = _conns[fd];
    if (!c) {
        log_warning("connection not exists.");
        return;
    }

    if (dynamic_cast<TCPConnection*>(c)) {
        tcp_read_io(c);
    } else if (dynamic_cast<UDPConnection*>(c)) {
        udp_read_io(c);
    }
}

void GenericWorker::tcp_read_io(Connection* conn) {
    TCPConnection* c = (TCPConnection*)conn;

    int rlen = c->_bytes_expected;
    size_t iblen = sdslen(c->_io_buffer);
    c->_io_buffer = sdsMakeRoomFor(c->_io_buffer, rlen);
    int r = c->read(c->_io_buffer + iblen, rlen);
    if (r == SOCKET_ERR) {
        log_debug("socket read: return error, close connection");
        close_conn(c);
        return;
    } else if (r == SOCKET_PEER_CLOSED) {
        log_debug("socket read: return 0, peer closed");
        close_conn(c);
        return;
    } else if (r > 0) {
        sdsIncrLen(c->_io_buffer, r);
    }
    c->_last_interaction = _el->now();

    // Upper process.
    if (sdslen(c->_io_buffer) - c->_bytes_processed >= c->_bytes_expected) {
        if (WORKER_OK != this->process_io_buffer(c)) {
            log_debug("read_io: user return error, close connection");
            close_conn(c);
        }
    }
}

void GenericWorker::udp_read_io(Connection* conn) {
    UDPConnection* c = (UDPConnection*)conn;

    c->_io_buffer = sdsMakeRoomFor(c->_io_buffer, 1024);
    int r = c->read(c->_io_buffer, 1024);
    if (r > 0) {
        sdsIncrLen(c->_io_buffer, r);
    }
    c->_last_interaction = _el->now();

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
    return c->_reply_list_size;
}

void GenericWorker::write_io(SOCKET fd) {
    if (fd == INVALID_SOCKET) {
        log_fatal("Invalid socket. fd[%d]", fd);
        return;
    }

    if (fd >= _conns.size()) {
        log_warning("invalid fd: %d", fd);
        return;
    }

    auto& sockets = _network_manager->sockets();
    Socket* s = sockets[fd]; 
    if (!s) {
        log_warning("socket not exists");
        return;
    }

    Connection* c = _conns[fd];
    if (!c) {
        log_warning("connection not exists");
        return;
    }

    if (dynamic_cast<TCPConnection*>(c)) {
        tcp_write_io(c);
    } else if (dynamic_cast<UDPConnection*>(c)) {
        udp_write_io(c);
    }
}

void GenericWorker::tcp_write_io(Connection* conn) {
    TCPConnection* c = (TCPConnection*)conn;

    while (! c->_reply_list.empty()) {
        Slice reply = c->_reply_list.front();
        int w = c->write(reply.data() + c->_bytes_written,
                         reply.size() - c->_bytes_written);
        if (w == SOCKET_ERR) {
            log_debug("sock_write_data: return error, close connection");
            close_conn(c);
            return;
        } else if(w == 0) { // would block.
            log_warning("write zero bytes, want[%d] fd[%d] ip[%s]", int(reply.size() - c->_bytes_written), c->fd(), c->_ip);
            return;
        } else if ((w + c->_bytes_written) == reply.size()) { // finish.
            c->_reply_list.pop_front();
            c->_reply_list_size--;
            c->_bytes_written = 0;
            zfree((void*)reply.data());
        } else {
            c->_bytes_written += w;
        }
    }
    c->_last_interaction = _el->now();

    // no more replies to write
    if (c->_reply_list.empty())
        disable_events(c, EventLoop::WRITE);
}

void GenericWorker::udp_write_io(Connection* c) {
    while (! c->_reply_list.empty()) {
        Slice reply = c->_reply_list.front();
        int w = c->write(reply.data(), reply.size());
        if (w == reply.size()) {
            c->_reply_list.pop_front();
            c->_reply_list_size--;
            zfree((void*)reply.data());
        } 
    }
    c->_last_interaction = _el->now();

    // no more replies to write
    if (c->_reply_list.empty())
        disable_events(c, EventLoop::WRITE);
}

void GenericWorker::disable_events(Connection* c, int events) {
    _el->stop_io_event(c->_watcher, c->fd(), events);
}

void GenericWorker::enable_events(Connection* c, int events) {
    _el->start_io_event(c->_watcher, c->fd(), events);
}

void GenericWorker::start_timer(Connection* c) {
    _el->start_timer(c->_timer, _options.tick);
}

Connection* GenericWorker::create_connection(Socket* s) {
    if (!s) {
        log_fatal("[Invalid Socket*]");
        return nullptr;
    }

    SOCKET fd = s->fd();
    if (fd == INVALID_SOCKET) {
        log_fatal("[invalid fd]");
        return nullptr;
    }

    Connection* c = nullptr;
    if (dynamic_cast<TCPSocket*>(s)) {
        c = new TCPConnection(s);
    } else if (dynamic_cast<UDPSocket*>(s)) {
        c = new UDPConnection(s);
    }

    c->_last_interaction = _el->now();
    c->_watcher = _el->create_io_event(conn_io_cb, (void*)c);
    c->_timer = _el->create_timer(timeout_cb, (void*)c, true);

    // start io and timer.
    if (_options.ssl_open) {
        enable_events(c, EventLoop::READ | EventLoop::WRITE);
    } else {
        enable_events(c, EventLoop::READ);
    }
    start_timer(c);

    if (fd >= _conns.size())
        _conns.resize(fd*2, NULL);
    _conns[fd] = c;

    log_debug("[new connection] fd[%d]", fd);
    return c;
}

void GenericWorker::close_conn(Connection* c) {
    log_debug("close connection");
    SOCKET fd = c->fd(); 
    remove_conn(c);
    close(fd);
}

void GenericWorker::close_all_conns() {
    std::vector<Connection*>::iterator it = _conns.begin();
    for (; it != _conns.end(); ++it) {
        if (*it != NULL) 
            close_conn(*it);
    }
}

void GenericWorker::remove_conn(Connection *c) {
    c->_last_interaction = _el->now();
    before_remove_conn(c);

    _el->delete_io_event(c->_watcher);  
    _el->delete_timer(c->_timer);
    if (c->_priv_data_destructor)
        c->_priv_data_destructor(c->_priv_data);
    _conns[c->fd()] = NULL;

    after_remove_conn(c);

    delete c;
}

void GenericWorker::process_cron_work() {
}

int GenericWorker::get_clients_count(){
    return _conns.size();
}

void GenericWorker::set_worker_id(const std::string& id) {
    _worker_id = id;
    return;
}

const std::string& GenericWorker::worker_id() {
    return _worker_id;
}

void GenericWorker::stop() {
    _el->delete_timer(cron_timer);    
    _el->delete_io_event(pipe_watcher);
    close(_notify_recv_fd);
    close(_notify_send_fd);
    _el->stop();
    for (size_t i = 0; i < _conns.size(); i++) {
        if (_conns[i] != NULL)
            close_conn(_conns[i]);
    }
}

void GenericWorker::mq_push(void* msg) {
    _mq.produce(msg);
}

bool GenericWorker::mq_pop(void** msg) {
    return _mq.consume(msg);
}

void GenericWorker::run() {
    _el->run();
}

int GenericWorker::notify(int msg) {
    struct timeval s, e; 
    gettimeofday(&s, NULL);
    int w = write(_notify_send_fd, &msg, sizeof(int));
    gettimeofday(&e, NULL);
    log_debug("[Write pipe] cost[%luus] msg_type[%d]", TIME_US_DIFF(s, e), msg);

    if (w == sizeof(int))
        return WORKER_OK;
    else
        return WORKER_ERROR;
}

void GenericWorker::process_internal_notify(int msg) {
    Socket* s;
    switch (msg) {
        case QUIT:          
            stop();
            break;
        case NEW_CONNECTION:         
            if (mq_pop((void**)&s)) 
                create_connection(s);
            break;
        default:
            process_notify(msg);
            break;
    }
}

void GenericWorker::process_notify(int msg) {
    log_warning("[Unknown notify] msg[%d]", msg);
}

void GenericWorker::process_timeout(Connection* c) {
    if ((_el->now() - c->_last_interaction) > _options.connection_timeout) {
        log_debug("[Time out] close fd[%d]", c->fd());
        close_conn(c);
    }
}

} // namespace zf


