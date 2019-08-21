#include "server/server.h"
#include "server/dispatcher.h"
#include "util/zmalloc.h"

namespace zf {

static command_t server_cmd_table[] = {
    { "host",
      conf_set_str_slot,
      offsetof(GenericServerOptions, host) },

    { "port",
      conf_set_num_slot,
      offsetof(GenericServerOptions, port) },

    { "worker_num",
      conf_set_num_slot,
      offsetof(GenericServerOptions, worker_num) },

    { "server_type",
      conf_set_num_slot,
      offsetof(GenericServerOptions, server_type) },

    { "connection_timeout",
      conf_set_usec_slot,
      offsetof(GenericServerOptions, connection_timeout) },

    { "tick",
      conf_set_usec_slot,
      offsetof(GenericServerOptions, tick) },

    { "max_query_buffer_size",
      conf_set_mem_slot,
      offsetof(GenericServerOptions, max_query_buffer_size) },

    { "max_reply_list_size",
      conf_set_num_slot,
      offsetof(GenericServerOptions, max_reply_list_size) },

    null_command
};

GenericServer::GenericServer() : dispatcher(NULL) {
}

GenericServer::~GenericServer() {
    delete dispatcher;
    dispatcher = NULL;
}

int GenericServer::init_conf() {
    options = (struct GenericServerOptions){NULL,   // host
                                            0,      // port
                                            8,      // worker_num
                                            G_SERVER_TCP,
                                            60 * 1000 * 1000,   // connection_timeout
                                            1000,               // tick
                                            10 * 1024 * 1024,   // max_query_buffer_size
                                            1024,               // max_reply_list_size
                                            NULL,
                                            0,                  //ssl server flag
                                            };
    return SERVER_MODULE_OK;
}

int GenericServer::init_conf(const GenericServerOptions &o) {
    options = o;
    if (options.host == NULL) {
        options.host = "0.0.0.0";
    }
    return 0;
}

int GenericServer::load_conf(const char *filename) {
    if (load_conf_file(filename, server_cmd_table, &options) != CONFIG_OK) {
        log_fatal("failed to load config file for server module");
        return SERVER_MODULE_ERROR;
    } else {
        fprintf(stderr,
                "Server Options:\n"
                "host: %s\n"
                "port: %d\n"
                "worker_num: %d\n"
                "server_type: %d\n"
                "connection_timeout: %lld\n"
                "tick: %lld\n"
                "max_query_buffer_size: %lld\n"
                "max_reply_list_size: %d\n",
                options.host,
                options.port,
                options.worker_num,
                options.server_type,
                options.connection_timeout,
                options.tick,
                options.max_query_buffer_size,
                options.max_reply_list_size);
        fprintf(stderr, "\n");
        return SERVER_MODULE_OK;
    }
}


int GenericServer::validate_conf() {
    return SERVER_MODULE_OK;
}

int GenericServer::init() {
    dispatcher = new GenericDispatcher(options);
    if (dispatcher->init() != DISPATCHER_OK) {
        log_fatal("failed to create dispatcher");
        return SERVER_MODULE_ERROR;
    }
    return SERVER_MODULE_OK;
}
  
void GenericServer::run() {
    dispatcher->run();
}

void GenericServer::stop() {
    dispatcher->notify(GenericDispatcher::QUIT);
}
int64_t GenericServer::get_clients_count(std::string &clients_detail){
    return dispatcher->get_clients_count(clients_detail);
}

} // namespace zf


