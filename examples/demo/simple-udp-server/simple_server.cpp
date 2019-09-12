/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file simple_server.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#include <stdio.h>

#include <zframework.h>

#include "simple_worker.h"

GenericServer* g_server = NULL;

GenericWorker* worker_factory(GenericServerOptions& o) {
    return new SimpleWorker(o);
}

int init_server() {
    // init log
    if (log_init("./log", "simple_server", STORE_LOG_DEBUG) != 0) {
        fprintf(stderr, "log init failed!");
        return -1;
    }

    // init server
    g_server = new GenericServer("simple_server");
    GenericServerOptions options;
    memset(&options, 0, sizeof(options));
    options.server_type = G_SERVER_UDP;
    options.port = 8888;
    options.worker_num = 1;
    options.connection_timeout = 30 * 1000 * 1000;  // 当连接空闲过久后，server会主动断开连接
    options.tick = 1000; // server的cronjob的定时周期
    options.max_io_buffer_size = 10 * 1024 * 1024;
    options.max_reply_list_size = 1024;
    options.worker_factory_func = worker_factory;
    g_server->init_conf(options);
    if (g_server->init() != 0) {
        return -1;
    }

    return 0;
}

void quit(int ret) {
    usleep(10*1000); /*waiting for log thread*/
    exit(ret);
}

int main() {
    int ret = 0;

    ret = init_server();
    if (ret) {
        log_warning("init server failed");
        quit(-1);
    }
    log_notice("init server finished");

    g_server->run(); // will block until someone call server->stop()

    quit(0);
}


