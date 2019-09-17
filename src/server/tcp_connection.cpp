/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_connection.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/tcp_connection.h"

#include "util/zmalloc.h"

namespace zf {

TCPConnection::TCPConnection(int fd) 
    : Connection(fd),
      _receive_state(0),
      _bytes_processed(0), 
      _bytes_expected(1), 
      _bytes_written(0)
{
}

TCPConnection::~TCPConnection() {
    std::list<Slice>::iterator it;
    for (it = _reply_list.begin(); 
        it != _reply_list.end(); ++it) 
    {
        zfree((void*)(*it).data());
    }
    _reply_list.clear();
}

void TCPConnection::reset(int initial_state, size_t initial_bytes_expected) {
    _bytes_processed = 0;
    _receive_state   = initial_state;
    _bytes_expected  = initial_bytes_expected;
}

void TCPConnection::expect_next(int next_state, size_t next_bytes_expected) {
    _bytes_processed += _bytes_expected;
    _receive_state   = next_state;
    _bytes_expected  = next_bytes_expected;
}

void TCPConnection::shrink_processed(int next_state, size_t next_bytes_expected) {
    // shrink io buffer
    _bytes_processed += _bytes_expected;
    _io_buffer = sdsrange(_io_buffer, _bytes_processed, -1);

    _bytes_processed = 0;
    _receive_state   = next_state;
    _bytes_expected  = next_bytes_expected;
}

} // namespace zf


