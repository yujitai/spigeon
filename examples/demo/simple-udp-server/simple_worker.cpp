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
    UDPConnection* c = (UDPConnection*)conn;
    cout << "SimpleWorker::process_io_buffer -> " << c->_io_buffer << endl;
    //printf("recv msg: %s len: %d ip: %s port: %d\n", rcvbuf, r, inet_ntoa(ra.sin_addr), ntohs(ra.sin_port));

    char* buf = (char*)zmalloc(1024);
    memcpy(buf, "hello client", 1024);
    Slice reply(buf, 1024);
    // 注意：传给add_reply的数据必须是使用zmalloc分配的内存，
    // 并且传递后自己就不能再使用，因为worker会自动释放
    add_reply(c, reply);
    return WORKER_OK;
}


