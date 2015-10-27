/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file proxy_worker.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/08 13:49:54
 * @brief 
 **/

#include "proxy_worker.h"

using namespace store;

ProxyWorker::ProxyWorker(const GenericServerOptions &o)
    : GenericWorker(o), client(NULL)
{ 
    client = new store::HttpClient(el, http_client_callback, "http://10.36.47.55:8066", 0/*timeout_us*/, 100);
    int ret = client->init();
    if (ret) {
        log_warning("HttpClient init failed");
    }
    client->set_timeout(100*1000);
}

ProxyWorker::~ProxyWorker() {
    delete client;
}

int ProxyWorker::process_query_buffer(Connection *c) {
    if (c->current_state == STATE_IDLE) {
        c->reset(STATE_HEAD, sizeof(nshead_t));
    } else if (c->current_state == STATE_HEAD) {
        nshead_t *hdr = (nshead_t*)c->querybuf;
        // check magic number
        if (hdr->magic_num != NSHEAD_MAGICNUM)
            return WORKER_ERROR;

        /* we have a complete header in querybuf
           now we expect a body */
        c->expect_next(STATE_BODY, hdr->body_len);
    } else if (c->current_state == STATE_BODY) {
        nshead_t *hdr = (nshead_t*)c->querybuf;
        /* now we have a complete request in querybuf */
        Slice header(c->querybuf, sizeof(nshead_t));
        Slice body(c->querybuf + sizeof(nshead_t), hdr->body_len);

        int ret = process_request(c, header, body);
        if (ret != WORKER_OK)
            return ret;

        c->shift_processed(STATE_HEAD, sizeof(nshead_t));
    } else {
        log_fatal("unexpected state:%d", c->current_state);
    }
    return WORKER_OK;
}

backend_req_t *ProxyWorker::backend_req_new(Connection *conn) {
    backend_req_t *br = (backend_req_t*)zmalloc(sizeof(backend_req_t));
    br->worker = this;
    br->conn = conn;
    conn->priv_data = br;

    br_flag_map[br] = true;
    return br;
}

void ProxyWorker::before_remove_conn(Connection *conn) {
    // 连接发生错误，标记flag，并清空对应的关联
    backend_req_t *br = (backend_req_t*)conn->priv_data;
    if (!br) {
        return;
    }

    br_flag_map[br] = false;
    br->conn = NULL;
    conn->priv_data = NULL;
}

void ProxyWorker::backend_req_free(backend_req_t *br) {
    if (br->conn) {
        br->conn->priv_data = NULL;
        br->conn = NULL;
    }
    br_flag_map.erase(br);
    zfree(br);
}

int ProxyWorker::process_request(Connection *conn,
                                const Slice &header, const Slice &body) {
    (void)(header);
    (void)(body);

    backend_req_t *br = backend_req_new(conn);

    // call backend
    log_debug("sending http request");
    std::string path = "/";
    //int ret = client->post(path, body, br);
    int ret = client->get(path, br);
    if (ret) {
        reply(conn, "post error");
        backend_req_free(br);
    }

    return WORKER_OK;
}

void ProxyWorker::reply(Connection *conn, const char *msg) {
    const int msg_len = strlen(msg) + 1;  // including '\0'
    const int buf_len = sizeof(nshead_t) + msg_len;
    char *buf = (char*)zmalloc(buf_len);

    nshead_t *head = (nshead_t*)buf;
    memset(head, 0, sizeof(nshead_t));
    head->magic_num = NSHEAD_MAGICNUM;
    head->body_len = msg_len;

    char *msg_buf = buf + sizeof(nshead_t);
    snprintf(msg_buf, msg_len, "%s", msg);

    Slice reply(buf, buf_len);
    // 注意：传给add_reply的数据必须是使用zmalloc分配的内存，
    // 并且传递后自己就不能再使用，因为worker会自动释放
    add_reply(conn, reply);
}

void ProxyWorker::http_client_callback(const HttpClientResponse *resp, void *data) {
    backend_req_t *br = (backend_req_t*)data;
    ProxyWorker *worker = br->worker;

    backend_req_flag_map_iter_t iter = worker->br_flag_map.find(br);
    if (iter == worker->br_flag_map.end() || iter->second == false) {
        //log_trace("connection error before backend response");
        goto out;
    }
    Connection *conn = br->conn;

    if (!resp) {
        log_warning("http response error");
        goto out;
    }
    log_trace("http response. is_timeout:%d, status_code:%d",
        resp->is_timeout, resp->status_code);

    if (resp->is_timeout) {
        worker->reply(conn, "timeout");
    } else if (resp->status_code != 200) {
        log_warning("http response detail: %.*s",
            (int)resp->response.size(), resp->response.data());
        worker->reply(conn, "wrong status code");
    } else {
        worker->reply(conn, "OK");
    }

out:
    worker->backend_req_free(br);
}

