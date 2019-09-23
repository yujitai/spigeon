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
class NetworkMgr;
class GenericWorker;

enum {
    DISPATCHER_OK = 0,
    DISPATCHER_ERROR = 1
};

enum {
    PROTOCOL_TCP = 0,
    PROTOCOL_UDP = 1
};

class GenericDispatcher {
public:
    enum {
        QUIT = 0
    };

    GenericDispatcher(GenericServerOptions& options);
    virtual ~GenericDispatcher();

    virtual int init();
    void run();
    int notify(int msg);
    void mq_push(void* msg);
    bool mq_pop(void** msg);
    int dispatch_new_conn(int fd, int protocol);
    virtual void process_notify(int msg);
    virtual int64_t get_clients_count(std::string& clients_detail);
    NetworkMgr* network_manager() const;
public:
    void process_internal_notify(int msg);

protected:
    void stop();
    virtual int spawn_worker();
    virtual int join_workers();

    GenericServerOptions& options;

    int listen_fd;
    int notify_recv_fd;
    int notify_send_fd;
    // owned by dispatcher.
    EventLoop* _el;
    IOWatcher* io_watcher;
    IOWatcher* pipe_watcher;
    // owned by dispatcher.
    NetworkMgr* _network_mgr;
    LockFreeQueue<void*> mq;
    std::vector<GenericWorker*> workers;
    std::vector<GenericWorker*>::size_type next_worker;
    Mutex lock;
};

} // namespace zf

#endif // __DISPATCHER_H_


