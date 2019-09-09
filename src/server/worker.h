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

#ifndef __WORKER_H_
#define __WORKER_H_

#include <list>
#include <vector>
#include <openssl/ssl.h>

#include "server/thread.h"
#include "server/server.h"
#include "server/dispatcher.h"
#include "util/sds.h"
#include "util/slice.h"

namespace zf {

enum {
    WORKER_OK = 0,
    WORKER_ERROR = -1,
    WORKER_CONNECTION_REMOVED = -2
};

class EventLoop;
class IOWatcher;
class TimerWatcher;

/**
 * @brief Connection
 *
 **/
struct Connection {
    char         ip[20];            // client ip
    int          port;              // client port
    int          fd;                // client fd
    bool         sslConnected;
    int          reply_list_size;
    int          current_state;
    size_t       bytes_processed;
    size_t       bytes_expected;
    size_t       cur_resp_pos;
    SSL          *ssl;
    sds          io_buffer;
    IOWatcher    *watcher;
    TimerWatcher *timer;
    void         *priv_data;
    void (*priv_data_destructor)(void*);
    uint64_t begin_interaction;
    uint64_t last_interaction;
    uint64_t last_recv_request;
    std::list<Slice> reply_list; 
    std::vector<uint64_t> ping_times;
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
        // Shrink the io_buffer
        bytes_processed += bytes_expected;
        io_buffer = sdsrange(io_buffer, bytes_processed, -1);

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
    
    GenericWorker(const GenericServerOptions &options, 
            const std::string& thread_name);
    virtual ~GenericWorker();

    virtual int init();
    void run();
    void mq_push(void *msg);            // push into message queue
    bool mq_pop(void **msg);            // pop from message queue
    virtual void read_io(int fd);
    virtual void write_reply(int fd);
    virtual int notify(int msg);
    virtual void process_notify(int msg);
    virtual void process_timeout(Connection *c);
    virtual void process_cron_work();
    int64_t get_clients_count();
    void set_clients_count(int64_t count);
    void set_worker_id(const std::string& id);
    const std::string& get_worker_id();
public:
    void process_internal_notify(int msg);
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
    virtual int process_io_buffer(Connection *c) = 0;

    void disable_events(Connection *c, int events);
    void enable_events(Connection *c, int events);
    void start_timer(Connection *c);

    const GenericServerOptions options;

    LockFreeQueue<void*> mq;      // new connection queue
    int64_t online_count;

    EventLoop* el;
    IOWatcher* pipe_watcher;
    TimerWatcher* cron_timer;

    /**
     *  worker_id = thread_name + thread_id.
     */
    std::string worker_id;

    /**
     *  Notify pipe fd, used for thread communication. 
     */
    int notify_recv_fd;             
    int notify_send_fd;         
    /**
     *  Alive connections, index is fd.
     *  i.e. conns[fd] = conn;
     */
    std::vector<Connection*> conns;
};

} // namespace zf

#endif // __WORKER_H_


