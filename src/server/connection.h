/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file connection.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __CONNECTION_H_
#define  __CONNECTION_H_

#include "server/common.h"

#include <openssl/ssl.h>

#include <vector>

namespace zf {

class IOWatcher;
class TimerWatcher;

/**
 * Base Connection
 */
class Connection {
public:
    Connection(int fd);
    virtual ~Connection();

    char _ip[20];            
    uint16_t _port;              
    int _fd;             

    // used to handle timeout.
    uint64_t _last_interaction;   

    void* _priv_data;
    void (*_priv_data_destructor)(void*);

    IOWatcher* _watcher;
    TimerWatcher* _timer;
};

} // namespace zf

#endif  //__CONNECTION_H_


