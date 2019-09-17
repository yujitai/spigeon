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
    SimpleWorker(const zf::GenericServerOptions &options);
    ~SimpleWorker();

protected:
    // user must impl this method
    int process_io_buffer(zf::Connection *c);
};

#endif  // __SIMPLE_WORKER_H_


