#ifndef _DB_SERVER_H_
#define _DB_SERVER_H_
#include "util/config_file.h"
#include "inc/module.h"
#include "server.h"

namespace store {

struct DBServerOptions {
    // network
    char *host;
    int port;
    int worker_num;
    long long connection_timeout;
    long long tick;
    long long max_query_buffer_size;
    int max_reply_list_size;
    worker_factory_func_t worker_factory_func;
    // replication
    flag_t master;
    char *master_host;
    int master_port;  
    long long heartbeat;
};

class DBDispatcher;
class DB;
class Replication;

class DBServer : public GenericServer {
public:
    DBServer();
    ~DBServer(); 

    int init_conf();
    int load_conf(const char *filename);
    int validate_conf();

    int init(DB *db, Replication *rep);

    void run();

    void stop();
private:
    DBServerOptions options;
};
}


#endif

