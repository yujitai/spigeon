#include "server/db_worker.h"
#include "server/db_dispatcher.h"
#include "server/event.h"
#include "inc/env.h"
#include "db/db.h"
#include "db/request.h"
#include "replication/replication.h"
#include "util/status.h"
#include "util/scoped_ptr.h"

namespace store {
static const uint64_t RETRY_TIMEOUT = 5000000; // 5 second

struct RepMsg {
    int fd;                               // slave fd
    RepStatus *stat;                        // replication stats
};

// DBWorker implementation

DBWorker::DBWorker(const DBServerOptions &o,
               DBDispatcher *owner, DB *db, Replication *rep)
        : NsheadWorker((GenericServerOptions&)o),
          db_options(o), dispatcher(owner), db_handler(db), rep_handler(rep) {}

DBWorker::~DBWorker() {
}

// recving a 'c' from fd means we have a new connection in msg to process 
void DBWorker::process_notify(int msg) {
    switch (msg) {
        case NEWCONNECTION:                           // new connection
            int client_fd;
            if (mq_pop((void**)&client_fd)) {
                new_conn(client_fd);
            }
            break;
        default:
            GenericWorker::process_notify(msg);
    }
}

#define MONITOR_PROVIDER "__MONITOR__"

int DBWorker::process_request(Connection *conn,
                              const Slice &header, const Slice &body) {
    nshead_t* head = (nshead_t*)header.data();
    Slice reply;
    if (!strncmp(head->provider, MONITOR_PROVIDER, strlen(MONITOR_PROVIDER))) {
        Status s = db_handler->process_monitor_request(&reply);
        add_reply(conn, reply);
        return WORKER_OK;
    }
    if (head->body_len == 0) {
        return WORKER_ERROR;
    }
    if (body.size() > (unsigned long)options.max_query_buffer_size) {
        Status s = db_handler->build_error_response(
            Status::EntityTooLarge("request to large"), &reply);
        if (s.ok()) {
            add_reply(conn, reply);
            return WORKER_OK;
        } else {
            return WORKER_ERROR;
        }
    }
    scoped_ptr<Request> req(Request::from_raw(body, head->log_id));
    if (!req.get()) {
        log_warning("can not parse request");
        return WORKER_ERROR;
    }
        
    if (req->method() == STORE_COMMAND_SYNC) {
        log_notice("REPL_SYNC_RECEIVED %s:%d", conn->ip, conn->port);
        RepMsg *msg = new RepMsg;
        msg->fd = conn->fd;
        msg->stat = new RepStatus;
        if (rep_handler->process_sync(*req, msg->stat) == REPLICATION_OK) {
            remove_conn(conn);
            dispatcher->new_slave(msg);
            return WORKER_CONNECTION_REMOVED;
        } else {
            delete msg;
            return WORKER_ERROR;
        }
    } else {
        log_data_t* ld = log_data_new();
        log_set_thread_log_data(ld);

        unsigned long ts_start = EventLoop::current_time();
        Status s = db_handler->process_request(*req, &reply);
        unsigned long ts_end = EventLoop::current_time();

        log_data_push_str(ld, "result", s.toString().c_str());
        log_data_push_int(ld, "cost", ts_end - ts_start);
        log_notice("finished");
        log_data_free(ld);
        log_set_thread_log_data(NULL);

        if (s.isInternalError()) {
            Env::suicide();
            return WORKER_ERROR;
        } else if (!s.ok()) {
            return WORKER_ERROR;
        } else{
            add_reply(conn, reply);
            return WORKER_OK;
        }
    }
}


RepMasterWorker::RepMasterWorker(const DBServerOptions &o, Replication *rep)
        : NsheadWorker((GenericServerOptions&)o),
          db_options(o), rep_handler(rep) {}

RepMasterWorker::~RepMasterWorker() {}  

static void rep_stat_destructor(void *data) {
    delete (RepStatus*)data;
}

void RepMasterWorker::process_notify(int msg) {
    switch (msg) {
        case NEWCONNECTION:
            RepMsg *rep_msg;
            if (mq_pop((void**)&rep_msg)) {
                Connection *c = new_conn(rep_msg->fd); // connect with slave
                c->priv_data = (void*)(rep_msg->stat); 
                c->priv_data_destructor = rep_stat_destructor;
                disable_events(c, EventLoop::READ);
                Slice reply;
                rep_handler->build_handshake_msg(&reply);
                log_notice("REPL_HANDSHAKE %s:%d", c->ip, c->port);
                add_reply(c, reply);
                delete rep_msg;
            }
            break;
        default:
            GenericWorker::process_notify(msg);
    }
}

void RepMasterWorker::process_timeout(Connection *c) {
    RepStatus *stat = (RepStatus*)c->priv_data;
    Slice binlog;
    while ((reply_list_size(c) < options.max_reply_list_size)
           && rep_handler->get_binlog(stat, &binlog) == REPLICATION_OK) {
        add_reply(c, binlog);
    }
    
    unsigned long interval = el->now() - c->last_interaction;
    if ((interval > (unsigned long)db_options.heartbeat)
        && reply_list_size(c) == 0)
    {
        Slice ping;
        rep_handler->build_ping(&ping);
        add_reply(c, ping);
    }
}


RepSlaveWorker::RepSlaveWorker(const DBServerOptions &o, Replication *rep)
        : NsheadWorker((GenericServerOptions&)o),
        db_options(o), repl_state(REPL_CONNECT), rep_handler(rep),
        last_retry(0), last_heartbeat(0), repl_conn(NULL) {}

RepSlaveWorker::~RepSlaveWorker() {}

Connection* RepSlaveWorker::connect_master() {
    int s = tcp_connect(db_options.master_host, db_options.master_port);
    if (s == NET_ERROR) {
        log_fatal("couldn't connect to master");
        return NULL;
    }
    log_notice("connected with master");
    // set up events and timer on the connection
    Connection *c = new_conn(s);
    return c;
}

#define change_repl_state(from, to)             \
    do {                                        \
        repl_state = to;                        \
        log_notice("%s -> %s", #from, #to);     \
    } while (0)


int RepSlaveWorker::process_request(Connection *c,
                              const Slice &header, const Slice &body) {
    UNUSED(c);
    UNUSED(header);
    log_debug("receive data from master ");
    // update the last heartbeat time
    last_heartbeat = el->now();   
    scoped_ptr<Request> request(Request::from_raw(body, 0));
    if (!request.get()) {
        log_warning("can not parse request");
        return WORKER_ERROR;
    }
    if (request->method() == STORE_REPL_HANDSHAKE) {
        if (repl_state == REPL_SYNC_SENT) {
            change_repl_state(REPL_SYNC_SENT, REPL_HANDSHAKE_FINISHED);
        }
        return WORKER_OK;
    } else if (request->method() == STORE_REPL_PING) {
        return WORKER_OK;
    } else if (request->method() == STORE_REPL_DATA) {
        int ret = rep_handler->process_binlog(*request);
        if(ret == REPLICATION_INTERNAL_ERROR){
            Env::suicide();
            return WORKER_ERROR;
        }
        if (ret != REPLICATION_OK) {
            log_warning("process binlog error");
            return WORKER_ERROR;
        } else {
            return WORKER_OK;
        }        
    } else {
        log_warning("unknown repl method");
    }
    return WORKER_OK;
}

void RepSlaveWorker::send_sync(Connection *c) {
    Slice sync;
    rep_handler->build_sync(&sync);
    add_reply(c, sync);
}

void RepSlaveWorker::before_remove_conn(Connection *c) {
    if (repl_conn == c) {
        repl_conn = NULL;
        log_notice("connection to master closed");
    }
}
    
void RepSlaveWorker::process_cron_work() {
    if (repl_state == REPL_CONNECT) {
        if ((el->now()-last_retry) > RETRY_TIMEOUT) {
            if (repl_conn != NULL)
                close_conn(repl_conn);
            repl_conn = connect_master();
            last_retry = el->now();
            if (repl_conn) {
                change_repl_state(REPL_CONNECT, REPL_CONNECTED);
            }
        }
    } else if (repl_state == REPL_CONNECTED) {
        assert(repl_conn != NULL);
        send_sync(repl_conn);
        change_repl_state(REPL_CONNECTED, REPL_SYNC_SENT);
    } else if (repl_state == REPL_SYNC_SENT) {
        if ((el->now()-last_retry) > RETRY_TIMEOUT) {
            change_repl_state(REPL_SYNC_SENT, REPL_CONNECT);
        }
    } else if (repl_state == REPL_HANDSHAKE_FINISHED) {
        if (last_heartbeat > 0 &&
            ((el->now()-last_heartbeat) > (uint64_t)2*db_options.heartbeat))
        {
            change_repl_state(REPL_HANDSHAKE_FINISHED, REPL_CONNECT);
        }
    }
}
    
}
