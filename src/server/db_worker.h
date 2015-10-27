#ifndef _DB_WORKER_H_
#define _DB_WORKER_H_

#include <stdint.h>
#include "server/nshead_worker.h"
#include "server/db_server.h"

namespace store {

class DBWorker: public NsheadWorker {
  public:
    DBWorker(const DBServerOptions &options,
           DBDispatcher *dispatcher, DB *db, Replication *replication);
    ~DBWorker();
    void process_notify(int msg);
    int process_request(Connection *c,
                        const Slice &header, const Slice &body);
  private:
    const DBServerOptions &db_options;
    DBDispatcher *dispatcher;
    DB *db_handler;                               // reference to db handler
    Replication *rep_handler;
};

class RepMasterWorker: public NsheadWorker {
  public:
    RepMasterWorker(const DBServerOptions &options, Replication *rep);
    ~RepMasterWorker();
    void process_notify(int msg);
    void process_timeout(Connection *c);
  private:
    const DBServerOptions &db_options;
    Replication *rep_handler;
};

class RepSlaveWorker: public NsheadWorker {
  public:
    RepSlaveWorker(const DBServerOptions &options, Replication *rep);
    ~RepSlaveWorker();
    int process_request(Connection *c,
                        const Slice &header, const Slice &body);
    void process_cron_work();
    void before_remove_conn(Connection *c);
  private:
    void send_sync(Connection *c);
    Connection* connect_master();
    enum {
        REPL_NONE = 0,                      // no replication
        REPL_CONNECT = 1,                   // connect master
        REPL_CONNECTED = 2,                 // connected with master
        REPL_SYNC_SENT = 3,                  // sync command sent
        REPL_HANDSHAKE_FINISHED = 4                      // handshake finished
    };
    const DBServerOptions &db_options;
    int repl_state;
    Replication *rep_handler;
    uint64_t last_retry;
    uint64_t last_heartbeat;
    Connection *repl_conn;
};

}
#endif
