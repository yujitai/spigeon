#ifndef __NSHEAD_WORKER_H__
#define __NSHEAD_WORKER_H__

#include "server/worker.h"
#include "util/nshead.h"

namespace store {

class NsheadWorker: public GenericWorker {
public:
    NsheadWorker(const GenericServerOptions &options);
    ~NsheadWorker();

    virtual int process_request(Connection *c,
                                const Slice &header, const Slice &body);
protected:
    enum state_t {
        STATE_IDLE = 0,
        STATE_HEAD = 1,
        STATE_BODY = 2,
    };

    int process_query_buffer(Connection *c);
};

}

#endif //__NSHEAD_WORKER_H__

