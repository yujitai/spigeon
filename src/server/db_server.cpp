#include "server/db_server.h"
#include "server/db_dispatcher.h"
#include "util/zmalloc.h"

namespace store {

// slaveof host port
static int set_replication(int argc, char **argv, command_t *cmd, void *conf) {
    UNUSED(cmd);
    if (argc < 2) return CONFIG_ERROR;    
  
    DBServerOptions *options = (DBServerOptions*)conf;
  
    options->master = 0;
    zfree(options->master_host);
    options->master_host = zstrdup(argv[0]);
    options->master_port = atoi(argv[1]);
    fprintf(stderr,"slaveof %s %d\n", options->master_host, options->master_port);
    return CONFIG_OK;
}

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

    { "slaveof",
      set_replication,
      0 },

    { "heartbeat",
      conf_set_usec_slot,
      offsetof(DBServerOptions, heartbeat) },

    null_command
};

DBServer::DBServer() {
}

DBServer::~DBServer() {
    zfree(options.host);
    zfree(options.master_host);
}
int DBServer::init_conf() {
    options = (struct DBServerOptions){ NULL,
                                        0,
                                        8,
                                        60 * 1000 * 1000,
                                        1000,
                                        10 * 1024 * 1024,
                                        1024,
                                        NULL,
                                        1,
                                        NULL,
                                        0,
                                        30 * 1000 * 1000 };
    return SERVER_MODULE_OK;
}
int DBServer::load_conf(const char *filename) {
    if (load_conf_file(filename, server_cmd_table, &options) != CONFIG_OK) {
        log_fatal("failed to load config file for server module");
        return SERVER_MODULE_ERROR;
    } else {
        fprintf(stderr,
                "Server Options:\n"
                "host: %s\n"
                "port: %d\n"
                "worker_num: %d\n"
                "connection_timeout: %lld\n"
                "tick: %lld\n"
                "max_query_buffer_size: %lld\n"
                "max_reply_list_size: %d\n"
                "heartbeat: %lld\n",
                options.host,
                options.port,
                options.worker_num,
                options.connection_timeout,
                options.tick,
                options.max_query_buffer_size,
                options.max_reply_list_size,
                options.heartbeat);
        if (!options.master) {
            fprintf(stderr,
                    "slaveof: %s %d\n",
                    options.master_host,
                    options.master_port);
        }
        fprintf(stderr, "\n");
        return SERVER_MODULE_OK;
    }
}

int DBServer::validate_conf() {
    return SERVER_MODULE_OK;
}

int DBServer::init(DB *db, Replication *rep) {
    dispatcher = new DBDispatcher(options, db, rep);
    if (dispatcher->init() != DISPATCHER_OK) {
        log_fatal("failed to create dispatcher");
        return SERVER_MODULE_ERROR;
    }
    return SERVER_MODULE_OK;
}
  
void DBServer::run() {
    dispatcher->run();
}

void DBServer::stop() {
    dispatcher->notify(DBDispatcher::QUIT);
}

}
