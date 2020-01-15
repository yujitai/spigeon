/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file udp_socket.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/udp_socket.h"
#include "server/ipv4_address.h"

namespace zf {

UDPSocket::UDPSocket(const Ipv4Address& srv_addr) 
    : Socket(srv_addr)
{
}

UDPSocket::UDPSocket(SOCKET fd) 
    : Socket(fd)
{
}

UDPSocket::~UDPSocket()
{
}

SOCKET UDPSocket::accept(SocketAddress& sa) {
    char buf[1024] = {0};

    struct sockaddr* addr = (struct sockaddr*)sa;
    socklen_t addr_len = (socklen_t)sa;
    size_t r = ::recvfrom(_s, buf, 1024, 0, addr, &addr_len);
    if (r < 0) {
        log_warning("recvfrom error");
    }

    SOCKET fd = create_bind2(AF_INET, SOCK_DGRAM);

    connect(fd, sa);

    return fd;
}

int UDPSocket::connect(SOCKET fd, SocketAddress& sa) {
    if (::connect(fd, (struct sockaddr*)sa, (socklen_t)sa) == -1) {
        log_fatal("connect failed: %s", strerror(errno));
        return SOCKET_ERR;
    } 
    return SOCKET_OK;
}

} // namespace zf


