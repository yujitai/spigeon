/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file http_client.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/09 12:15:29
 * @brief 
 **/

#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include <stdint.h>
#include <string>
#include <map>
#include "util/slice.h"

class EvHttpClient;
class ResponseInfo;

namespace store {

typedef std::map<std::string, std::string> header_map_t;

class HttpClientResponse {
public:
    bool is_timeout;
    short status_code;
    uint64_t cost_time_us;
    const header_map_t &headers;
    Slice response;

    HttpClientResponse(bool in_is_timeout, short in_status_code,
        uint64_t in_cost_time_us, const header_map_t &in_headers,
        const Slice &in_response)
        : is_timeout(in_is_timeout), status_code(in_status_code),
        cost_time_us(in_cost_time_us), headers(in_headers), response(in_response) 
    { }
};

typedef void (*HttpClientCallback) (const HttpClientResponse *resp, void *data);

class EventLoop;
class HttpClient {
public:
    /**
     * HttpClient非线程安全，并且一个HttpClient对应一个host地址
     *
     * 需要外部提供EventLoop
     * pool_size可以为0，表示不启用连接池
     */
    HttpClient(EventLoop *loop, HttpClientCallback cb, const std::string &host,
        int timeout_us = 0, int pool_size = 0);
    ~HttpClient();

    int init();

    void set_timeout(uint64_t timeout_us);

    int get(const std::string &path, void *data);
    int get(const std::string &path, const header_map_t &headers, void *data);

    int post(const std::string &path, const Slice &body, void *data);
    int post(const std::string &path, const header_map_t &headers,
        const Slice &body, void *data);

private:
    static void client_callback(ResponseInfo *resp, void *data, void *data2);

    EvHttpClient *_client;
    HttpClientCallback _client_cb;
};
}

#endif //__HTTP_CLIENT_H__

