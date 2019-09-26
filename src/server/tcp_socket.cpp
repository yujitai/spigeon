/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_socket.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/tcp_socket.h"

namespace zf {

TCPSocket::TCPSocket(Ipv4Address srv_addr) 
    : Socket(srv_addr) 
{
}

TCPSocket::TCPSocket(SOCKET s) 
    : Socket(s) 
{
}

TCPSocket::~TCPSocket() {
}

int TCPSocket::listen(int backlog) {
    if (::listen(_s, 4095) == -1) {
        log_warning("listen: %s", strerror(errno));
        close(_s);
        return SOCKET_ERROR;
    }

    return SOCKET_OK;
}

SOCKET TCPSocket::accept(SocketAddress& sa) {
    while(1) {
        socklen_t salen = sizeof(struct sockaddr);
        SOCKET a_s = ::accept(_s, (struct sockaddr*)sa, &salen);
        if (a_s == -1) {
            if (errno == EINTR)
                continue;
            else {
                log_warning("accept: %s", strerror(errno));
                return SOCKET_ERROR;
            }
        }
        log_debug("accept: new_fd[%d]", a_s);
        return a_s;
    }
}

int TCPSocket::connect(SocketAddress& sa) {
    return SOCKET_ERROR;
}

} // namespace zf


