#ifndef _DB_DISPATCHER_H_
#define _DB_DISPATCHER_H_

#include "server/dispatcher.h"
#include "server/db_server.h"

namespace store {

class Env;
class DB;
class Replication;
struct RepMsg;

class DBDispatcher: public GenericDispatcher {
public:
    DBDispatcher(DBServerOptions &options, DB *db, Replication *replication);
    ~DBDispatcher();
    int init();
    int new_slave(RepMsg *msg);
private:
    int spawn_worker();
    int spawn_rep_worker();
    int join_workers();

    DBServerOptions &db_options;

    DB *db_handler;
    Replication *rep_handler;
    GenericWorker *rep_worker;
};

}

#endif

