/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file dispatcher.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/

#include "server/dispatcher.h"

#include <sys/time.h>

#include "server/event.h"
#include "server/worker.h"
#include "server/network_manager.h"
#include "util/log.h"
#include "util/store_define.h"

namespace zf {

/**
 * @brief Dispatcher pipe message hanler.
 **/
void recv_notify(EventLoop *el, IOWatcher *w, int fd, int revents, void *data) {
    UNUSED(el);
    UNUSED(w);
    UNUSED(revents);

    int msg;
    if (read(fd, &msg, sizeof(int)) != sizeof(int)) {
        log_warning("can't read from notify pipe");
        return;
    }
    GenericDispatcher *dispatcher = (GenericDispatcher*)data;
    dispatcher->process_internal_notify(msg);
}

/**
 * TCP and UDP acceptor
 * TODO:后续把这些回调写成仿函数 class Acceptor
 **/
void accept_new_conn(EventLoop* el, 
        IOWatcher* w, 
        int listen_fd, 
        int revents, 
        void* data)  
{
    UNUSED(el);
    UNUSED(w);
    UNUSED(revents);
    
    struct timeval start, end;
    Ipv4Address cli_addr;
    GenericDispatcher* dp = (GenericDispatcher*)data;
    NetworkManager* network_manager = dp->network_manager();
    if (!network_manager) 
        return;

    gettimeofday(&start, NULL);
    Socket* s = network_manager->generic_accept(listen_fd, cli_addr); 
    if (!s) {
        log_warning("[generic accept failed]");
        return;
    }
    if (dp->dispatch_new_conn(s) == DISPATCHER_ERROR) {
        log_warning("[dispatch new conn failed]");
        return;
    }
    gettimeofday(&end, NULL);
    log_debug("[accept new conn] cost[%luus] client_fd[%d]", TIME_US_DIFF(start, end), s->fd());
}

GenericDispatcher::GenericDispatcher(GenericServerOptions &o)
        : _options(o),
          _el(NULL), 
          _io_watcher(NULL), 
          _pipe_watcher(NULL),
          _next_worker(0)
{
    _el = new EventLoop((void*)this, false);
    _network_manager = new NetworkManager(_el);
}

GenericDispatcher::~GenericDispatcher() {
    for (size_t i = 0; i < _workers.size(); i++) {
        if (_workers[i] != NULL) {
            delete _workers[i];
            _workers[i] = NULL;
        }
    }

    delete _el;
    delete _network_manager;
}

int GenericDispatcher::initialize() {
    if (create_pipe() == DISPATCHER_ERROR)
        return DISPATCHER_ERROR;

    if (create_server(_options.server_type, 
                      _options.ip, _options.port,
                      accept_new_conn)) 
        return DISPATCHER_ERROR;

    for (int i = 0; i < _options.worker_num; i++) {
        if (spawn_worker() == DISPATCHER_ERROR) 
            return DISPATCHER_ERROR;
    }

    return DISPATCHER_OK;
}

int GenericDispatcher::create_pipe() {
    int fds[2];
    if (pipe(fds)) {
        log_fatal("create notify pipe failed");
        return DISPATCHER_ERROR;
    }
    _notify_recv_fd = fds[0];
    _notify_send_fd = fds[1];
    _pipe_watcher = _el->create_io_event(recv_notify, (void*)this);
    if (_pipe_watcher == NULL) {
        log_fatal("can't create io event for recv_notify");
        return DISPATCHER_ERROR;
    }
    _el->start_io_event(_pipe_watcher, _notify_recv_fd, EventLoop::READ);
    log_debug("[create notify pipe ok]");

    return DISPATCHER_OK;
}

int GenericDispatcher::create_server(uint8_t type, char* ip, uint16_t port, 
        accept_cb_t accept_cb) 
{
    Socket* s = _network_manager->create_server(type, ip, port);
    if (!s) {
        log_fatal("create %s server failed on %s:%d", type, ip, port);
        return DISPATCHER_ERROR;
    }
    _io_watcher = _el->create_io_event(accept_cb, (void*)this);
    if (_io_watcher == NULL) {
        log_fatal("can't create io event for accept_new_conn");
        return DISPATCHER_ERROR;
    }
    _el->start_io_event(_io_watcher, s->fd(), EventLoop::READ);

    return DISPATCHER_OK;
}
 
void GenericDispatcher::stop() {
    // stop pipe watcher
    if (_pipe_watcher)
        _el->delete_io_event(_pipe_watcher);

    // stop event loop
    if (_io_watcher)
        _el->delete_io_event(_io_watcher);

    _el->stop();
    log_notice("event loop stopped");

    // TODO:close socket
    // close(listen_fd);
    // log_notice("close listening socket");

    // tell workers to quit
    join_workers();
    log_notice("joined workers");
}

void GenericDispatcher::run() {
    log_notice("dispatcher start");
    _el->run();
}

int GenericDispatcher::spawn_worker() {
    if (_options.worker_factory_func == NULL) {
        log_fatal("don't specify worker_factory_func to create worker");
        return DISPATCHER_ERROR;
    }

    GenericWorker* new_worker = _options.worker_factory_func(_options);
    new_worker->initialize();
    new_worker->set_network(_network_manager);
    if (create_thread(new_worker) == THREAD_ERROR) {
        log_fatal("failed to create worker thread");
        return DISPATCHER_ERROR;
    }
    std::stringstream worker_id;
    worker_id << new_worker->_thread_name << "_" << new_worker->_thread_id;
    new_worker->set_worker_id(worker_id.str());
    _workers.push_back(new_worker);

    return DISPATCHER_OK;
}

int GenericDispatcher::join_workers() {
    for (auto worker : _workers) {
        worker->notify(GenericWorker::QUIT);
        if (join_thread(worker) == THREAD_ERROR) {
            log_fatal("failed to join worker thread");
            return DISPATCHER_ERROR;
        }
    }

    return DISPATCHER_OK;
}

int GenericDispatcher::dispatch_new_conn(Socket* s) {
    // Just use a round-robin right now.
    GenericWorker* worker = _workers[_next_worker];
    _next_worker = (_next_worker + 1) % _workers.size();
    worker->mq_push((void*)s);

    // Notify worker the arrival of a new connection.
    if (worker->notify(GenericWorker::NEW_CONNECTION) != WORKER_OK) {
        log_warning("[write worker notify pipe failed");
        return DISPATCHER_ERROR;             
    }

    log_debug("[dispatch new connection] fd[%d] worker_id[%s]", 
            s->fd(), worker->worker_id().c_str());

    return DISPATCHER_OK;
}

void GenericDispatcher::process_internal_notify(int msg) {
    switch (msg) {
        case QUIT:  
            stop();
            break;
        default:
            process_notify(msg);
            break;
    }    
}

void GenericDispatcher::process_notify(int msg) {
    log_warning("Unknow notification: %d", msg);
}

int GenericDispatcher::notify(int msg) {
    int w = write(_notify_send_fd, &msg, sizeof(int));     
    if (w == sizeof(int)) 
        return DISPATCHER_OK;
    else {
        return DISPATCHER_ERROR;
    }
}

void GenericDispatcher::mq_push(void *msg) {
    _mq.produce(msg);
}

bool GenericDispatcher::mq_pop(void **msg) {
    return _mq.consume(msg);
}

// TODO:测试
int64_t GenericDispatcher::get_clients_count(std::string& clients_detail) {
    std::stringstream temp;
    int64_t current_count = 0;
    temp << "[";
    for (size_t i = 0; i < _workers.size(); i++) {
        if (_workers[i] != NULL) {
            std::string worker_id_temp = _workers[i]->worker_id();
            int64_t count_temp = _workers[i]->get_clients_count();
            current_count += count_temp;
            temp << "," << worker_id_temp << ":" << count_temp;
        }
    }
    temp << "]";
    clients_detail = temp.str().c_str();
    return current_count;
}

NetworkManager* GenericDispatcher::network_manager() const {
    return _network_manager;
}

} // namespace zf


