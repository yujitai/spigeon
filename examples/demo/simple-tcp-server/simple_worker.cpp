/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file worker.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/


#include "simple_worker.h"

SimpleWorker::SimpleWorker(const GenericServerOptions& o)
    : zf::GenericWorker(o, "simple_worker") 
{ 

}

SimpleWorker::~SimpleWorker() {

}

int SimpleWorker::process_io_buffer(Connection* conn) {
    TCPConnection* c = (TCPConnection*)conn;

    if (c->_receive_state == STATE_IDLE) {
        c->reset(STATE_HEAD, sizeof(nshead_t));
    } else if (c->_receive_state == STATE_HEAD) {
        nshead_t* hdr = (nshead_t*)c->_io_buffer;
        // check magic number
        if (hdr->magic_num != NSHEAD_MAGICNUM)
            return WORKER_ERROR;

        /** 
         *  We have a complete header in io_buffer,
         *  now we expect a body. 
         */
        c->expect_next(STATE_BODY, hdr->body_len);
    } else if (c->_receive_state == STATE_BODY) {
        nshead_t *hdr = (nshead_t*)c->_io_buffer;

        /** 
         *  Now we have a complete request in io_buffer.
         */
        Slice header(c->_io_buffer, sizeof(nshead_t));
        Slice body(c->_io_buffer + sizeof(nshead_t), hdr->body_len);

        int ret = this->process_request(c, header, body);
        if (ret != WORKER_OK)
            return ret;

        c->shrink_processed(STATE_HEAD, sizeof(nshead_t));
    } else {
        log_fatal("unexpected state:%d", c->_receive_state);
    }

    return WORKER_OK;
}

int SimpleWorker::process_request(Connection* c,
        const Slice& header, 
        const Slice& body) 
{
    // echo reply
    char* buf = (char*)zmalloc(header.size() + body.size());
    memcpy(buf, header.data(), header.size());
    memcpy(buf + header.size(), body.data(), body.size());
    Slice reply(buf, header.size() + body.size());

    // 注意：传给add_reply的数据必须是使用zmalloc分配的内存，
    // 并且传递后自己就不能再使用，因为worker会自动释放
    add_reply(c, reply);

    return WORKER_OK;
}


