#ifndef _WORKER_H_
#define _WORKER_H_

#include <vector>
#include <list>
#include "server/thread.h"
#include "server/server.h"
#include "server/dispatcher.h"
#include "util/sds.h"
#include "util/slice.h"

namespace store {

enum {
    WORKER_OK = 0,
    WORKER_ERROR = -1,
    WORKER_CONNECTION_REMOVED = -2
};

class EventLoop;
class IOWatcher;
class TimerWatcher;
class DBDispatcher;

class Connection {
  public:
    int fd;
    char ip[20];
    int port;
    
    sds querybuf;
    // current porcessing state 
    int current_state;
    // the length we already processed. ie the header size
    size_t bytes_processed;
    // the length we expect the current header/body to be in 
    size_t bytes_expected;

    // current postion inside a reply 
    size_t cur_resp_pos;
    std::list<Slice> reply_list;
    int reply_list_size;

    IOWatcher *watcher;
    TimerWatcher *timer;

    void *priv_data;                           // user specific data
    void (*priv_data_destructor)(void*);

    unsigned long last_interaction;
  
    Connection(int fd);
    ~Connection();

    void reset(int initial_state, size_t initial_bytes_expected) {
        bytes_processed = 0;
        current_state   = initial_state;
        bytes_expected  = initial_bytes_expected;
    }

    void expect_next(int next_state, size_t next_bytes_expected) {
        bytes_processed += bytes_expected;
        current_state   = next_state;
        bytes_expected  = next_bytes_expected;
    }

    void shift_processed(int next_state, size_t next_bytes_expected) {
        /* shrink the query buffer */
        bytes_processed += bytes_expected;
        querybuf = sdsrange(querybuf, bytes_processed, -1);

        bytes_processed = 0;
        current_state   = next_state;
        bytes_expected  = next_bytes_expected;
    }
};

class GenericWorker: public Runnable {
  public:
    // notification messages
    enum {
        QUIT = 0,
        NEWCONNECTION = 1
    };
    
    GenericWorker(const GenericServerOptions &options);
    virtual ~GenericWorker();
    virtual int init();
    void run();
    void mq_push(void *msg);            // push into message queue
    bool mq_pop(void **msg);            // pop from message queue
    void read_query(int fd);
    void write_reply(int fd);
    int notify(int msg);
    virtual void process_notify(int msg);
    virtual void process_timeout(Connection *c);
    virtual void process_cron_work();
  protected:
    void stop();
    Connection *new_conn(int fd);
    void close_conn(Connection *c);
    virtual void before_remove_conn(Connection *c) { UNUSED(c); }
    virtual void after_remove_conn(Connection *c) { UNUSED(c); }
    void close_all_conns();
    void remove_conn(Connection *c);      // remove but not close the connection
    int add_reply(Connection *c, const Slice& reply);
    int reply_list_size(Connection *c);
    virtual int process_read_query(Connection *c);
    virtual int process_query_buffer(Connection *c) = 0;

    void disable_events(Connection *c, int events);
    void enable_events(Connection *c, int events);
    void start_timer(Connection *c);

    const GenericServerOptions options;

    LockFreeQueue<void*> mq;        // new connection queue
    EventLoop *el;
    IOWatcher *pipe_watcher;
    TimerWatcher *cron_timer;
    
    int notify_recv_fd;           // recving end of notify pipe
    int notify_send_fd;           // sending end of notify pipe
    std::vector<Connection*> conns; // connections currently alive
};

}
#endif
