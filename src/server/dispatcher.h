#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include <vector>
#include <string>
#include "server/thread.h"
#include "util/lock.h"
#include "server/server.h"

namespace store {
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
    ~GenericDispatcher();
    virtual int init();
    void run();
    int notify(int msg);
    int dispatch_new_conn(int fd);        // dispatch a new conn
    virtual void process_notify(int msg);
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
    std::vector<GenericWorker*> workers;
    std::vector<GenericWorker*>::size_type next_worker;
    Mutex lock;
};

}
#endif

