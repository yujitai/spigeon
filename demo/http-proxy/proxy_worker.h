/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file proxy_worker.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/08 13:49:46
 * @brief 
 **/

#ifndef __PROXY_WORKER_H__
#define __PROXY_WORKER_H__

#include <store_framework.h>
#include <map>

class ProxyWorker;
typedef struct _backend_req_t {
    ProxyWorker *worker;
    store::Connection *conn;
} backend_req_t;
typedef std::map<backend_req_t *, bool> backend_req_flag_map_t;
typedef std::map<backend_req_t *, bool>::iterator backend_req_flag_map_iter_t;

class ProxyWorker: public store::GenericWorker {
public:
    ProxyWorker(const store::GenericServerOptions &options);
    ~ProxyWorker();

    int process_request(store::Connection *c,
        const store::Slice &header, const store::Slice &body);

    void reply(store::Connection *c, const char *msg);
protected:
    int process_query_buffer(store::Connection *c);
    void before_remove_conn(store::Connection *c);
    void after_remove_conn(store::Connection *c) { UNUSED(c); }

private:
    enum state_t {
        STATE_IDLE = 0,
        STATE_HEAD = 1,
        STATE_BODY = 2,
    };

    backend_req_t *backend_req_new(store::Connection *c);
    void backend_req_free(backend_req_t *br);

    static void http_client_callback(const store::HttpClientResponse *resp, void *data);

    store::HttpClient *client;
    backend_req_flag_map_t br_flag_map;
};

#endif //__PROXY_WORKER_H__

