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

#include <netinet/tcp.h> // for TCP_NODELAY

namespace zf {

TCPSocket::TCPSocket() 
    : _s(-1) 
{
}

TCPSocket::TCPSocket(SOCKET s) 
    : _s(s) 
{
}

TCPSocket::~TCPSocket() {
}

int TCPSocket::create(int family, int type) {
    // create async noblocking socket.
    if ((_s = ::socket(family, type | SOCK_NONBLOCK, 0)) == -1) {
        log_warning("create socket: %s", strerror(errno));
        return SOCKET_ERROR;
    }
    log_debug("sockfd[%d]", _s);

    // set socket opt.
    int on = 1;
    set_option(Socket::OPT_REUSEADDR, on);
    set_option(Socket::OPT_NODELAY, on);
    set_noblock();

    return SOCKET_OK;
}

int TCPSocket::bind(SocketAddress& sa) {
    if (::bind(_s, (struct sockaddr*)sa, (socklen_t)sa) == -1) {
        log_warning("bind: %s", strerror(errno));
        close(_s);
        return SOCKET_ERROR;
    }

    return SOCKET_OK;
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

int TCPSocket::connect(SocketAddress* sa) {

}

int TCPSocket::write(const char* buf, size_t len) {
    int w = ::write(_s, buf, len);
    if (w == -1) {
        if (errno == EAGAIN)
            w = 0;
        else {
            log_debug("write: %s", strerror(errno));
            return SOCKET_ERROR;
        }
    }

    return w;
}

int TCPSocket::read(char* buf, size_t len) {
    int r = ::read(_s, buf, len);
    if (r == -1) {
        if (errno == EAGAIN)
            r = 0;
        else {
            log_debug("read: %s", strerror(errno));
            return SOCKET_ERROR;
        }
    } else if (r == 0) {
        log_debug("read: peer closed");
        return SOCKET_PEER_CLOSED;
    }
    
    return r;
}

bool TCPSocket::get_local_address(SocketAddress* const sa) const {
    struct sockaddr_storage addr = {0};
    socklen_t addr_len = sizeof(addr);
    sockaddr* saddr = reinterpret_cast<sockaddr*>(&addr);

    if(::getsockname(_s, saddr, &addr_len) == SOCKET_ERROR) {
        log_warning("[get remote address failed] fd[%d]", _s);
        return false;
    }

    if (addr.ss_family == AF_INET) {
        sockaddr_in* saddr = (sockaddr_in*)&addr;
        *((Ipv4Address*)sa) = Ipv4Address(
                inet_ntoa(saddr->sin_addr), 
                htons(saddr->sin_port)); 
    } 
    
    if (addr.ss_family == AF_INET6) {
    }

    return true;
}

bool TCPSocket::get_remote_address(SocketAddress* const sa) const {
    struct sockaddr_storage addr = {0};
    socklen_t addr_len = sizeof(addr);
    sockaddr* saddr = reinterpret_cast<sockaddr*>(&addr);

    if(::getpeername(_s, saddr, &addr_len) == SOCKET_ERROR) {
        log_warning("[get remote address failed] fd[%d]", _s);
        return false;
    }

    if (addr.ss_family == AF_INET) {
        sockaddr_in* saddr = (sockaddr_in*)&addr;
        *((Ipv4Address*)sa) = Ipv4Address(
                inet_ntoa(saddr->sin_addr), 
                htons(saddr->sin_port)); 
    } 
    
    if (addr.ss_family == AF_INET6) {
    }

    return true;
}

int TCPSocket::get_option(Option opt, int* value) {

}

int TCPSocket::set_option(Option opt, int value) {
   log_debug("[setsockopt] opt[%d]", opt);
   int slevel, sopt;
   if (translate_option(opt, &slevel, &sopt) == -1) {
       log_warning("translate socket option failed");
       return SOCKET_ERROR;
   }
   
   return ::setsockopt(_s, slevel, sopt, &value, sizeof(value));
}

int TCPSocket::translate_option(Option opt, int* slevel, int* sopt) {
    switch (opt) {
        case OPT_REUSEADDR:
            *slevel = SOL_SOCKET;
            *sopt = SO_REUSEADDR;
            break;
        case OPT_NODELAY:
            *slevel = IPPROTO_TCP;
            *sopt = TCP_NODELAY;
            break;
        case OPT_RCVBUF:
            break;
        case OPT_SNDBUF:
            break;
        default:
            break;
    }

    return SOCKET_OK;
}

int TCPSocket::set_noblock() {
    int flags;
    /** 
     * Set the socket nonblocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. 
     */
    if ((flags = fcntl(_s, F_GETFL)) == -1) {
        log_warning("fcntl(F_GETFL): %s", strerror(errno));
        return SOCKET_ERROR;
    }
    if (fcntl(_s, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_warning("fcntl(F_SETFL, O_NONBLOCK): %s", strerror(errno));
        return SOCKET_ERROR;
    }
    log_debug("[set noblocking] fd[%d]", _s);

    return SOCKET_OK;
}

SOCKET TCPSocket::fd() {
    return _s; 
}

} // namespace zf


