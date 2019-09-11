/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file server.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/

#ifndef __SERVER_H_
#define __SERVER_H_

#include "server/common.h"

#include <string>

#include "server/thread.h"
#include "server/iconfig.h"
#include "util/config_file.h"

namespace zf {

enum {
    SERVER_OK = 0,
    SERVER_ERROR = 1
};

class GenericServerOptions;
class GenericDispatcher;
class GenericWorker;

typedef GenericWorker* (*worker_factory_func_t) (GenericServerOptions& o);

enum SERVER_TYPE {
    G_SERVER_TCP = 0,
    G_SERVER_UDP = 1,
    G_SERVER_PIPE = 2,
};

class GenericServerOptions : public Options {
public:
    // default constructor
    GenericServerOptions();

    char* host;
    uint16_t port;
    int worker_num;
    SERVER_TYPE server_type;
    uint64_t connection_timeout;
    uint64_t tick;
    uint64_t max_io_buffer_size;
    int max_reply_list_size;
    worker_factory_func_t worker_factory_func;
    int ssl_open;
};

class GenericServer : public Runnable, 
                      public IConfig 
{
public:
    GenericServer(const std::string& thread_name);
    virtual ~GenericServer(); 

    // Implementation of IConfig
    int init_conf() override;
    int init_conf(struct Options& o) override;
    int load_conf(const char* filename) override;
    int validate_conf() override;

    int init();
    virtual void run() override;
    void stop();

    int64_t get_clients_count(std::string& clients_detail);

protected:
    GenericServerOptions _options;
    GenericDispatcher* _dispatcher;
};

} // namespace zf

#endif // __SERVER_H_


