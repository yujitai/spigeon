/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_connection.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __TCP_CONNECTION_H_
#define  __TCP_CONNECTION_H_

#include <list>

#include "server/connection.h"
#include "util/sds.h"
#include "util/slice.h"

namespace zf {

/**
 * TCPConnection
 */
class TCPConnection : public Connection {
public:
    TCPConnection(int fd);
    virtual ~TCPConnection() override;

    void reset(int initial_state, size_t initial_bytes_expected);
    void expect_next(int next_state, size_t next_bytes_expected);
    void shrink_processed(int next_state, size_t next_bytes_expected);

    // simple state machine, handle TCP stream "nian bao".
    int _receive_state;

    // data length that user wants to receive.
    size_t _bytes_expected;

    // total data length that user has processed.
    size_t _bytes_processed;

    // current write position.
    size_t _bytes_written;

    // simple dynamic string.
    sds _io_buffer;   

    int _reply_list_size;
    std::list<Slice> _reply_list;
};

} // namespace zf

#endif  //__TCP_CONNECTION_H_


