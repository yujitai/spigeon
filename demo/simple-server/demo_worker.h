/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file demo_worker.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/03/08 13:49:46
 * @brief 
 **/

#ifndef __DEMO_WORKER_H__
#define __DEMO_WORKER_H__

#include <store_framework.h>

class DemoWorker: public store::GenericWorker {
public:
    DemoWorker(const store::GenericServerOptions &options);
    ~DemoWorker();

    int process_request(store::Connection *c,
                                const store::Slice &header, const store::Slice &body);
protected:
    enum state_t {
        STATE_IDLE = 0,
        STATE_HEAD = 1,
        STATE_BODY = 2,
    };

    int process_query_buffer(store::Connection *c);
};

#endif //__DEMO_WORKER_H__

