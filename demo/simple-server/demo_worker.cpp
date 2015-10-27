/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file demo_worker.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/08 13:49:54
 * @brief 
 **/

#include "demo_worker.h"

using namespace store;

DemoWorker::DemoWorker(const GenericServerOptions &o)
    : GenericWorker(o) { }

DemoWorker::~DemoWorker() {
}

int DemoWorker::process_query_buffer(Connection *c) {
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

int DemoWorker::process_request(Connection *c,
                                const Slice &header, const Slice &body) {
    // only for demo. echo reply
    char *buf = (char*)zmalloc(header.size() + body.size());
    memcpy(buf, header.data(), header.size());
    memcpy(buf + header.size(), body.data(), body.size());
    Slice reply(buf, header.size() + body.size());

    // 注意：传给add_reply的数据必须是使用zmalloc分配的内存，
    // 并且传递后自己就不能再使用，因为worker会自动释放
    add_reply(c, reply);

    return WORKER_OK;
}

