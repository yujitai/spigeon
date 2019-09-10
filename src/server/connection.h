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

#include <stddef.h>
#include <stdint.h>
#include <openssl/ssl.h>

#include <list>
#include <vector>

#include "util/sds.h"
#include "util/slice.h"

namespace zf {

class IOWatcher;
class TimerWatcher;

/**
 * @brief Connection
 *
 **/
class Connection {
public:
   Connection(int fd);
    ~Connection();

    void reset(int initial_state, size_t initial_bytes_expected);
    void expect_next(int next_state, size_t next_bytes_expected);
    void shift_processed(int next_state, size_t next_bytes_expected);

    int  fd;             
    char ip[20];            
    uint16_t port;              
    bool sslConnected;
    int reply_list_size;
    int current_state;
    // data length that the user has processed.
    size_t bytes_processed;
    // data length that the user wants to receive.  
    size_t bytes_expected;
    size_t cur_resp_pos;
    sds io_buffer;
    SSL* ssl;
    IOWatcher* watcher;
    TimerWatcher* timer;
    void* priv_data;
    void (*priv_data_destructor)(void*);
    // used to handle timeout.
    uint64_t last_interaction;   
    std::list<Slice> reply_list;
    std::vector<uint64_t> ping_times;
};

} // namespace zf

#endif  //__CONNECTION_H_


