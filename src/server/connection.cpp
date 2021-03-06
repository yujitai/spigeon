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

Connection::Connection(Socket* s)
    : _port(0),
      _priv_data(NULL), 
      _priv_data_destructor(NULL),
      _watcher(NULL), 
      _timer(NULL),
      _reply_list_size(0),
      _s(s)
{
    _io_buffer = sdsempty();
}

Connection::~Connection() {
    sdsfree(_io_buffer);
    delete _s;
}

int Connection::read(char* buf, size_t len) {
    _s->read(buf, len);
}

int Connection::write(const char* buf, size_t len) {
    _s->write(buf, len);
}

SOCKET Connection::fd() { 
    return _s->fd(); 
}

} // namespace zf


