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

Connection::Connection(int fd)
    : _fd(fd), 
      _priv_data(NULL), 
      _priv_data_destructor(NULL),
      _watcher(NULL), 
      _timer(NULL)
{
}

Connection::~Connection() {
}

} // namespace zf


