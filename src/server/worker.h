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

#include "server/thread.h"
#include "server/server.h"
#include "server/dispatcher.h"
#include "server/tcp_connection.h"
#include "server/udp_connection.h"
#include "server/network_manager.h"

namespace zf {

enum {
    WORKER_OK = 0,
    WORKER_ERROR = -1,
    WORKER_CONNECTION_REMOVED = -2
};

class EventLoop;

class GenericWorker: public Runnable {
 public:
    // notification messages
    enum {
        QUIT = 0,
        TCPCONNECTION = 1,
        UDPCONNECTION = 2
    };

    GenericWorker(const GenericServerOptions& options, 
            const std::string& thread_name);

    virtual ~GenericWorker();

    virtual int init();
    void run();
    void mq_push(void *msg);            
    bool mq_pop(void **msg);    
    
    // process tcp io event
    virtual void tcp_read_io(int fd);
    virtual void tcp_write_io(int fd);

#if 0
    // process udp io event
    virtual void udp_read_io(int fd);
    virtual void udp_write_io(int fd);
#endif
    virtual int notify(int msg);
    virtual void process_notify(int msg);
    virtual void process_timeout(Connection *c);
    virtual void process_cron_work();
    int64_t get_clients_count();
    void set_clients_count(int64_t count);
    void set_worker_id(const std::string& id);
    const std::string& get_worker_id();
    void set_network(NetworkMgr* network_manager);
public:
    void process_internal_notify(int msg);
protected:
    void stop();
    Connection* new_tcp_conn(SOCKET s);
    //Connection* new_udp_conn(int fd);
    void close_conn(Connection *c);
    virtual void before_remove_conn(Connection *c) { UNUSED(c); }
    virtual void after_remove_conn(Connection *c) { UNUSED(c); }
    void close_all_conns();
    void remove_conn(Connection* c);      // remove but not close the connection
    int add_reply(Connection* c, const Slice& reply);
    int reply_list_size(Connection *c);
    virtual int process_io_buffer(Connection *c) = 0;

    void disable_events(Connection *c, int events);
    void enable_events(Connection *c, int events);
    void start_timer(Connection *c);

    const GenericServerOptions options;
    LockFreeQueue<void*> mq;      // new connection queue
    int64_t online_count;

    // owned by worker
    EventLoop* _el;
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
    
    // owned by dispatcher
    NetworkMgr* _network_manager;
};

} // namespace zf

#endif // __WORKER_H_


