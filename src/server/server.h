#ifndef _SERVER_MODULE_H_
#define _SERVER_MODULE_H_
#include "util/config_file.h"
#include "inc/module.h"

namespace store {

enum {
    SERVER_MODULE_OK = 0,
    SERVER_MODULE_ERROR = 1
};

class GenericDispatcher;
class GenericWorker;
class GenericServerOptions;

typedef GenericWorker *(*worker_factory_func_t)(GenericServerOptions &o);
struct GenericServerOptions {
    // network
    char *host;
    int port;
    int worker_num;
    long long connection_timeout;
    long long tick;
    long long max_query_buffer_size;
    int max_reply_list_size;
    worker_factory_func_t worker_factory_func;
    int ssl_open;
};

class GenericServer : public Module {
public:
    GenericServer();
    virtual ~GenericServer(); 

    int init_conf();
    int init_conf(const GenericServerOptions &o);
    int load_conf(const char *filename);
    int validate_conf();

    int init();

    void run();

    void stop();
protected:
    GenericServerOptions options;
    GenericDispatcher *dispatcher;
};
}

#endif
