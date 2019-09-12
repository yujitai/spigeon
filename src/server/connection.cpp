/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file connection.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/connection.h"

#include "util/zmalloc.h"

namespace zf {

Connection::Connection(int client_fd)
    : fd(client_fd), 
      current_state(0),
      reply_list_size(0), 
      bytes_processed(0), 
      bytes_expected(1), 
      cur_resp_pos(0),
      priv_data(NULL), 
      priv_data_destructor(NULL),
      watcher(NULL), 
      timer(NULL)
{
    io_buffer = sdsempty();
}

Connection::~Connection() {
    sdsfree(io_buffer);

    std::list<Slice>::iterator it;
    for (it = reply_list.begin(); 
            it != reply_list.end(); ++it)
    {
        zfree((void*)(*it).data());
    }
    reply_list.clear();
}

void Connection::reset(int initial_state, size_t initial_bytes_expected) {
    bytes_processed = 0;
    current_state   = initial_state;
    bytes_expected  = initial_bytes_expected;
}

void Connection::expect_next(int next_state, size_t next_bytes_expected) {
    bytes_processed += bytes_expected;
    current_state   = next_state;
    bytes_expected  = next_bytes_expected;
}

void Connection::shift_processed(int next_state, size_t next_bytes_expected) {
    // Shrink the io_buffer
    bytes_processed += bytes_expected;
    io_buffer = sdsrange(io_buffer, bytes_processed, -1);

    bytes_processed = 0;
    current_state   = next_state;
    bytes_expected  = next_bytes_expected;
}

} // namespace zf


