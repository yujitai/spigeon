/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file dispatcher.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/

#ifndef __DISPATCHER_H_
#define __DISPATCHER_H_

#include <vector>
#include <string>
#include <sstream>

#include "server/server.h"
#include "util/lock.h"

namespace zf {

class EventLoop;
class IOWatcher;
class GenericWorker;
class NetworkManager;

enum {
    DISPATCHER_OK = 0,
    DISPATCHER_ERROR = 1
};

enum {
    PROTOCOL_TCP = 0,
    PROTOCOL_UDP = 1
};

typedef void (*accept_cb_t)(EventLoop*, IOWatcher*, int, int, void*);

class GenericDispatcher {
public:
    enum {
        QUIT = 0
    };

    GenericDispatcher(GenericServerOptions& options);
    virtual ~GenericDispatcher();

    virtual int initialize();
    int create_pipe();
    int create_server(uint8_t type, 
            char* ip, uint16_t port, 
            accept_cb_t accept_cb);

    void run();
    int notify(int msg);
    void mq_push(void* msg);
    bool mq_pop(void** msg);
    int dispatch_new_conn(int fd, int protocol);
    virtual void process_notify(int msg);
    virtual int64_t get_clients_count(std::string& clients_detail);
    NetworkManager* network_manager() const;
public:
    void process_internal_notify(int msg);
protected:
    void stop();
    /**
     * Spawn a new worker and push it into GenericDispatcher::workers.
     **/
    virtual int spawn_worker();
    virtual int join_workers();

    GenericServerOptions& options;

    int _notify_recv_fd;
    int _notify_send_fd;
    // owned by dispatcher.
    EventLoop* _el;
    IOWatcher* _io_watcher;
    IOWatcher* _pipe_watcher;
    // owned by dispatcher.
    NetworkManager* _network_manager;
    LockFreeQueue<void*> mq;
    std::vector<GenericWorker*> workers;
    std::vector<GenericWorker*>::size_type next_worker;
    Mutex lock;
};

} // namespace zf

#endif // __DISPATCHER_H_


