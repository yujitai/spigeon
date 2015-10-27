#include "server/nshead_worker.h"
#include "util/nshead.h"
#include "util/zmalloc.h"

namespace store {

NsheadWorker::NsheadWorker(const GenericServerOptions &o)
    : GenericWorker(o) { }

NsheadWorker::~NsheadWorker() {
}

int NsheadWorker::process_query_buffer(Connection *c) {
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

int NsheadWorker::process_request(Connection *c,
                                const Slice &header, const Slice &body) {
    log_fatal("you should override process_request");

    // only for demo. echo reply
    char *buf = (char*)zmalloc(header.size() + body.size());
    memcpy(buf, header.data(), header.size());
    memcpy(buf + header.size(), body.data(), body.size());
    Slice reply(buf, header.size() + body.size());

    //sds reply_buf = sdsnewlen(header.data(), header.size());
    //reply_buf = sdscatlen(reply_buf, body.data(), body.size());
    //Slice reply(reply_buf, sdslen(reply_buf));

    add_reply(c, reply);

    return WORKER_OK;
}

}

