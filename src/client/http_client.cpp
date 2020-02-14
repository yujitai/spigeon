/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file http_client.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/09 12:15:40
 * @brief 
 **/

#include "http_client.h"

#include <evhttpclient.h>

#include "server/event.h"
#include "util/slice.h"
#include "util/log.h"

namespace zf {

HttpClient::HttpClient(EventLoop *el, HttpClientCallback cb, const std::string &host, int timeout_us, int pool_size)
    : _client(NULL), _client_cb(cb)
{
    _client = new EvHttpClient(el->loop, host, 0, this, pool_size);
    _client->setTimeout(((double)timeout_us) / 1000000);
}

HttpClient::~HttpClient() {
    delete _client;
}

int HttpClient::init() {
    int ret = _client->init();
    if (ret) {
        log_warning("HttpClient init failed. error:%s", _client->lastError());
        return -1;
    }
    return 0;
}

void HttpClient::set_timeout(uint64_t timeout_us) { 
    _client->setTimeout(((double)timeout_us) / 1000000);
}

int HttpClient::get(const std::string &path, const header_map_t &headers, void *data) {
    int ret = _client->makeGet(client_callback,
        path, headers, data);
    if (ret) {
        log_warning("HttpClient get failed. error:%s", _client->lastError());
        return -1;
    }
    return 0;
}

int HttpClient::get(const std::string &path, void *data) {
    return get(path, map<std::string, std::string>(), data);
}

int HttpClient::post(const std::string &path, const header_map_t &headers,
    const Slice &body, void *data)
{
    std::string body_str(body.data(), body.size());
    int ret = _client->makePost(client_callback,
        path, headers, body_str, data);
    if (ret) {
        log_warning("HttpClient post failed. error:%s", _client->lastError());
        return -1;
    }
    return 0;
}

int HttpClient::post(const std::string &path, const Slice &body, void *data) {
    return post(path, map<std::string, std::string>(), body, data);
}

int HttpClient::del(const std::string &path, const header_map_t &headers,
    const Slice &body, void *data)
{
    std::string body_str(body.data(), body.size());
    int ret = _client->makeDelete(client_callback,
        path, headers, body_str, data);
    if (ret) {
        log_warning("HttpClient del failed. error:%s", _client->lastError());
        return -1;
    }
    return 0;
}

int HttpClient::del(const std::string &path, const Slice &body, void *data) {
    return del(path, map<std::string, std::string>(), body, data);
}

void HttpClient::client_callback(ResponseInfo *ri, void *data, void *data2) {
    HttpClient *c = (HttpClient*)data2;
    if (ri) {
        uint64_t cost_time_us = uint64_t(ri->latency * 1000 * 1000);
        log_debug("HttpClient finished. cost:%lu us", cost_time_us);

        Slice resp(ri->response.data(), ri->response.size());
        HttpClientResponse r(ri->timeout, ri->code, cost_time_us,
            ri->headers, resp);
        c->_client_cb(&r, data);
    } else {
        log_warning("HttpClient failed. error:%s", c->_client->lastError());
        c->_client_cb(NULL, data);
    }
}

} // namespace zf


