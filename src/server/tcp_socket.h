/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_socket.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __TCP_SOCKET_H_
#define  __TCP_SOCKET_H_

#include "server/socket.h"

namespace zf {

class TCPSocket : public Socket {
public:
    TCPSocket(const Ipv4Address& srv_addr);
    TCPSocket(SOCKET s);
    ~TCPSocket();

    // Socket implementation.
    int listen(int backlog) override;
    SOCKET accept(SocketAddress& sa) override;
    int connect(SocketAddress& sa) override;
};

} // namespace zf 

#endif  //__TCP_SOCKET_H_


