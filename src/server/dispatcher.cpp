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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <pthread.h>
#include <sys/time.h>

#include "server/event.h"
#include "server/worker.h"
#include "server/network_manager.h"
#include "util/log.h"
#include "util/store_define.h"

namespace zf {

/**
 * @brief Dispatcher pipe message hanler
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
 * @brief Tcp server socket io handler
 */
void accept_tcp_conn(EventLoop *el, 
        IOWatcher* w, 
        int fd, 
        int revents, 
        void* data)  
{
    UNUSED(el);
    UNUSED(w);
    UNUSED(revents);
    
    struct timeval s, e, s1, e1; 
    int new_fd;
    Ipv4Address addr;
    GenericDispatcher* dp = (GenericDispatcher*)data;

    NetworkMgr* network_manager = dp->network_manager();
    if (!network_manager) 
        return;
    gettimeofday(&s, NULL);
    new_fd = network_manager->tcp_accept(fd, addr); 
    if (new_fd == NetworkMgr::NET_ERROR) {
        log_warning("[accept socket failed]");
        return;
    }
    
    gettimeofday(&s1, NULL);
    if (dp->dispatch_new_conn(new_fd, PROTOCOL_TCP) == DISPATCHER_ERROR) {
        log_warning("[dispatch new connection failed]");
        return;
    }

    gettimeofday(&e1, NULL);
    log_debug("[dispatch new conn] cost[%luus] cfd[%d]", TIME_US_DIFF(s1, e1), new_fd);

    gettimeofday(&e, NULL);
    log_debug("[accept new conn] cost[%luus] cfd[%d]", TIME_US_DIFF(s, e), new_fd);
}

/**
 * @brief Udp server socket io handler
 */
#if 0
void accept_udp_conn(EventLoop *el, 
        IOWatcher* w, 
        int fd, 
        int revents, 
        void* data)  
{
    UNUSED(el);
    UNUSED(w);
    UNUSED(revents);
    
    GenericDispatcher* dp = (GenericDispatcher*)data;

    struct sockaddr_in ca;
    memset(&ca, 0, sizeof(struct sockaddr_in));
    socklen_t calen = sizeof(ca);
    char buf[1024] = {0};

    size_t r = ::recvfrom(fd, buf, 1024, 0, (struct sockaddr*)&ca, &calen);
    if (r) {
        char ip[20];
        uint16_t port;
        cout << "udp recv" << r << endl;
        cout << "ip=" << ip << inet_ntoa(ca.sin_addr) << endl;
        cout << "port=" << ntohs(ca.sin_port) << endl;
    }

    int new_fd = create_udp_server(NULL, 8888);
    cout << "new udp fd =" << new_fd << endl;

    if (connect(new_fd, (struct sockaddr*)&ca, sizeof(struct sockaddr)) < 0) {
        perror("connect");
        return;
    } 

    dp->dispatch_new_conn(new_fd, PROTOCOL_UDP);
}
#endif

GenericDispatcher::GenericDispatcher(GenericServerOptions &o)
        : options(o),
          _el(NULL), 
          io_watcher(NULL), 
          pipe_watcher(NULL),
          next_worker(0), 
          listen_fd(0) 
{
    _el = new EventLoop((void*)this, false);
    _network_mgr = new NetworkMgr(_el);
}

GenericDispatcher::~GenericDispatcher() {
    for (size_t i = 0; i < workers.size(); i++) {
        if (workers[i] != NULL) {
            delete workers[i];
            workers[i] = NULL;
        }
    }
    
    delete _el;
}

int GenericDispatcher::init() {
    // TODO:switch case.
    // set up pipe fd
    if (options.server_type == G_SERVER_PIPE 
            || options.server_type == G_SERVER_TCP 
            || options.server_type == G_SERVER_UDP) 
    {
        int fds[2];
        if (pipe(fds)) {
            log_fatal("can't create notify pipe");
            return DISPATCHER_ERROR;
        }
        notify_recv_fd = fds[0];
        notify_send_fd = fds[1];
        pipe_watcher = _el->create_io_event(recv_notify, (void*)this);
        if (pipe_watcher == NULL)
            return DISPATCHER_ERROR;
        _el->start_io_event(pipe_watcher, notify_recv_fd, EventLoop::READ);
    }

    // set up the tcp server socket.
    if (options.server_type == G_SERVER_TCP) {
        SOCKET fd = _network_mgr->create_tcp_server(options.ip, options.port);
        if (fd == NetworkMgr::NET_ERROR) {
            log_fatal("Create tcp server failed on %s:%d", options.ip, options.port);
            return DISPATCHER_ERROR;
        }
        log_trace("start listen port %d", options.port);

        io_watcher = _el->create_io_event(accept_tcp_conn, (void*)this);
        if (io_watcher == NULL) {
            log_fatal("Can't create io event for accept_tcp_conn");
            return DISPATCHER_ERROR;
        }
        _el->start_io_event(io_watcher, fd, EventLoop::READ);
    }

#if 0
    // set up the udp server socket.
    if (options.server_type == G_SERVER_UDP) {
        int fd = create_udp_server(options.host, options.port);
        cout << "udp fd = "  << fd << endl;
        if (fd == NET_ERROR) {
            log_fatal("Can't create udp server on %s:%d",
                      options.host.c_str(), options.port);
            return DISPATCHER_ERROR;
        }

        io_watcher = el->create_io_event(accept_udp_conn, (void*)this);
        if (io_watcher == NULL) {
            log_fatal("Can't create io event for accept_tcp_conn");
            return DISPATCHER_ERROR;
        }
        el->start_io_event(io_watcher, fd, EventLoop::READ);
    }
#endif
    for (int i = 0; i < options.worker_num; i++) {
        if (spawn_worker() == DISPATCHER_ERROR) {
            return DISPATCHER_ERROR;
        }
    }

    return DISPATCHER_OK;
}

void GenericDispatcher::stop() {
    // stop pipe watcher
    if (pipe_watcher)
        _el->delete_io_event(pipe_watcher);

    // stop event loop
    if (io_watcher)
        _el->delete_io_event(io_watcher);

    _el->stop();
    log_notice("event loop stopped");

    // close socket
    close(listen_fd);
    log_notice("close listening socket");

    // tell workers to quit
    join_workers();
    log_notice("joined workers");
}

void GenericDispatcher::run() {
    log_notice("dispatcher start");
    _el->run();
}

// spawn a new worker and push it into the GenericDispatcher::workers
int GenericDispatcher::spawn_worker() {
    if (options.worker_factory_func == NULL) {
        log_fatal("you should specify worker_factory_func to create worker");
        return DISPATCHER_ERROR;
    }

    GenericWorker* new_worker = options.worker_factory_func(options);
    new_worker->init();
    new_worker->set_network(_network_mgr);
    if (create_thread(new_worker) == THREAD_ERROR) {
        log_fatal("failed to create worker thread");
        return DISPATCHER_ERROR;
    }
    std::stringstream worker_id;
    worker_id << new_worker->_thread_name << "_" << new_worker->_thread_id;
    new_worker->set_worker_id(worker_id.str());
    workers.push_back(new_worker);

    return DISPATCHER_OK;
}

int GenericDispatcher::join_workers() {
    for (size_t i = 0; i < workers.size(); i++) {
        if (workers[i] != NULL) {
            workers[i]->notify(GenericWorker::QUIT);
            if (join_thread(workers[i]) == THREAD_ERROR) {
                log_fatal("failed to join worker thread");
            }
        }
    }

    return DISPATCHER_OK;
}

int GenericDispatcher::dispatch_new_conn(int fd, int protocol) {
    log_debug("[dispatch new connection] fd[%d]", fd);

    // Just use a round-robin right now.
    GenericWorker* worker = workers[next_worker];
    next_worker = (next_worker + 1) % workers.size();
    int* nfd = new int(fd);
    worker->mq_push((void*)nfd);

    // Notify worker the arrival of a new connection.
    switch (protocol) {
        case PROTOCOL_TCP: {
            if (worker->notify(GenericWorker::TCPCONNECTION) != WORKER_OK) {
                log_warning("[write worker notify pipe failed");
                return DISPATCHER_ERROR;             
            }
            break;
        }
        case PROTOCOL_UDP: {
            if (worker->notify(GenericWorker::UDPCONNECTION) != WORKER_OK) {
                log_warning("[write worker notify pipe failed");
                return DISPATCHER_ERROR;             
            }
            break;
        }
        default:
            log_warning("[unknown protocol]");
            break;
    }
       
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
    log_warning("unknow notification: %d", msg);
}

int GenericDispatcher::notify(int msg) {
    int written = write(notify_send_fd, &msg, sizeof(int));     
    if (written == sizeof(int)) {
        return DISPATCHER_OK;
    } else {
        return DISPATCHER_ERROR;
    }
}

void GenericDispatcher::mq_push(void *msg) {
    mq.produce(msg);
}

bool GenericDispatcher::mq_pop(void **msg) {
    return mq.consume(msg);
}

int64_t GenericDispatcher::get_clients_count(std::string& clients_detail) {
    std::stringstream temp;
    int64_t current_count = 0;
    temp << "[";
    for (size_t i = 0; i < workers.size(); i++) {
        if (workers[i] != NULL) {
            std::string workerid_temp = workers[i]->get_worker_id();
            int64_t  count_temp = workers[i]->get_clients_count();
            current_count += count_temp;
            temp << "," << workerid_temp << ":" << count_temp;
        }
    }
    temp << "]";
    clients_detail = temp.str().c_str();
    return current_count;
}

NetworkMgr* GenericDispatcher::network_manager() const {
    return _network_mgr;
}

} // namespace zf


