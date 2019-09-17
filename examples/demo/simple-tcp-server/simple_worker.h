/***************************************************************************
 *
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/



/**
 * @file simple_worker.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$
 * @brief
 *
 **/


#ifndef  __SIMPLE_WORKER_H_
#define  __SIMPLE_WORKER_H_

#include <zframework.h>

using namespace zf;

class SimpleWorker: public zf::GenericWorker {
public:
    SimpleWorker(const zf::GenericServerOptions& options);
    ~SimpleWorker();

    int process_request(zf::Connection* c,
            const zf::Slice& header, 
            const zf::Slice& body);
protected:
    // user provide the data process method.
    enum state_t {
        STATE_IDLE = 0,
        STATE_HEAD = 1,
        STATE_BODY = 2,
    };

    // user must impl the interface.
    int process_io_buffer(zf::Connection* c) override;
};

#endif  // __SIMPLE_WORKER_H_


