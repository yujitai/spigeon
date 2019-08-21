#ifndef _SERVER_MODULE_H_
#define _SERVER_MODULE_H_

#include <string>
#include <stdint.h>

#include "module.h"
#include "util/config_file.h"

namespace zf {

enum {
    SERVER_MODULE_OK = 0,
    SERVER_MODULE_ERROR = 1
};

class GenericDispatcher;
class GenericWorker;
class GenericServerOptions;

typedef GenericWorker *(*worker_factory_func_t)(GenericServerOptions &o);
typedef enum generic_server_type {
    G_SERVER_TCP = 0,
    G_SERVER_UDP = 1,
    G_SERVER_PIPE = 2,
} G_SERVER_TYPE;
struct GenericServerOptions {
    // network
    char *host;
    int port;
    int worker_num;
    G_SERVER_TYPE server_type;
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
    int64_t get_clients_count(std::string &clients_detail);
protected:
    GenericServerOptions options;
    GenericDispatcher *dispatcher;
};

} // namespace zf

#endif
