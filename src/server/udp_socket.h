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
    UDPSocket(Ipv4Address addr);
    UDPSocket(SOCKET fd);
    ~UDPSocket();

    // Socket implementation.
    SOCKET accept(SocketAddress& sa) override;
    int connect(SOCKET fd, SocketAddress& sa);
};

} // namespace zf 

#endif  //__UDP_SOCKET_H_


