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

namespace zf {

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


