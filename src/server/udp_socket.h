/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file udp_socket.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __UDP_SOCKET_H_
#define  __UDP_SOCKET_H_

#include "server/socket.h"

namespace zf {

class UDPSocket : public Socket {
public:
    UDPSocket(int family, int type);
private:
    SOCKET _s;
};

} // namespace zf 

#endif  //__UDP_SOCKET_H_


