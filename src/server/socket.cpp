/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file socket.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/socket.h"

namespace zf {

int Socket::listen(int backlog) {
    return -1;
}

SOCKET Socket::accept(SocketAddress* sa) {
    return -1;
}

int Socket::connect(SocketAddress* sa) {
    return -1;
}

} // namespace zf


