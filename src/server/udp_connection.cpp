/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file udp_connection.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/udp_connection.h"

namespace zf {

UDPConnection::UDPConnection(Socket* s)
    : Connection(s)
{
}

UDPConnection::~UDPConnection() {
}


} // namespace zf


