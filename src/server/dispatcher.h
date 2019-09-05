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
#include "server/thread.h"
#include "util/lock.h"

namespace zf {

enum {
    DISPATCHER_OK = 0,
    DISPATCHER_ERROR = 1
};

class Config;
class GenericWorker;
class EventLoop;
class IOWatcher;

class GenericDispatcher: public Runnable {
public:
    enum {
        QUIT = 0
    };
    GenericDispatcher(GenericServerOptions &options);
    virtual ~GenericDispatcher();
    virtual int init();
    void run();
    int notify(int msg);
    void mq_push(void *msg);
    bool mq_pop(void **msg);
    int dispatch_new_conn(int fd);        // dispatch a new conn
    virtual void process_notify(int msg);
    virtual int64_t get_clients_count(std::string &clients_detail);
public:
    void process_internal_notify(int msg);
protected:
    void stop();
    virtual int spawn_worker();
    virtual int join_workers();

    GenericServerOptions &options;

    int listen_fd;
    EventLoop *el;
    IOWatcher *io_watcher;
    int notify_recv_fd;
    int notify_send_fd;
    IOWatcher *pipe_watcher;
    LockFreeQueue<void*> mq;
    std::vector<GenericWorker*> workers;
    std::vector<GenericWorker*>::size_type next_worker;
    Mutex lock;
};

} // namespace zf

#endif // __DISPATCHER_H_


