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

#include <netinet/tcp.h> // for TCP_NODELAY

namespace zf {

Socket::Socket(const Ipv4Address& srv_addr)
    : _local_addr(srv_addr) 
{
}

Socket::Socket(SOCKET fd)
    : _s(fd)
{
}

int Socket::create_bind(int family, int type) {
    // Create async noblocking socket
    if ((_s = ::socket(family, type | SOCK_NONBLOCK, 0)) == -1) {
        log_warning("create socket: %s", strerror(errno));
        return SOCKET_ERR;
    }

    // For concurrent server module, the port should be reused
    int on = 1;
    set_option(OPT_REUSEADDR, on);

    // Bind
    SocketAddress& sa = _local_addr;
    if (::bind(_s, (struct sockaddr*)sa, (socklen_t)sa) == -1) {
        log_fatal("bind: %s", strerror(errno));
        close(_s);
        return SOCKET_ERR;
    }

    log_debug("[socket info] type[%d] fd[%d] ip[%s] port[%d]",
            type, _s, sa.ip().c_str(), sa.port());

    return SOCKET_OK;
}

SOCKET Socket::create_bind2(int family, int type) {
    SOCKET fd = ::socket(family, type | SOCK_NONBLOCK, 0);
    if (fd == INVALID_SOCKET) {
        log_warning("create socket: %s", strerror(errno));
        return SOCKET_ERR;
    }

    int on = 1;
    set_option(fd, OPT_REUSEADDR, on);

    SocketAddress& sa = _local_addr;
    if (::bind(fd, (struct sockaddr*)sa, (socklen_t)sa) == -1) {
        log_fatal("bind: %s", strerror(errno));
        close(fd);
        return SOCKET_ERR;
    }

    log_debug("[socket info] type[%d] fd[%d] ip[%s] port[%d]",
            type, fd, sa.ip().c_str(), sa.port());

    return fd;
}

int Socket::set_option(SOCKET fd, Option opt, int value) {
   int slevel, sopt;
   if (translate_option(opt, &slevel, &sopt) == -1) {
       log_warning("translate socket option failed");
       return SOCKET_ERR;
   }
   
   if (::setsockopt(fd, slevel, sopt, &value, sizeof(value)) == -1) {
        log_warning("setsockopt failed: %s, fd[%d] opt[%d]", 
                strerror(errno), fd, opt);
        return SOCKET_ERR;
   }

   log_debug("[setsockopt] opt[%d]", opt);
   return SOCKET_OK;
}

int Socket::set_option(Option opt, int value) {
   int slevel, sopt;
   if (translate_option(opt, &slevel, &sopt) == -1) {
       log_warning("translate socket option failed");
       return SOCKET_ERR;
   }
   
   if (::setsockopt(_s, slevel, sopt, &value, sizeof(value)) == -1) {
        log_warning("setsockopt failed: %s, fd[%d] opt[%d]", 
                strerror(errno), _s, opt);
        return SOCKET_ERR;
   }

   log_debug("[setsockopt] opt[%d]", opt);
   return SOCKET_OK;
}

int Socket::translate_option(Option opt, int* slevel, int* sopt) {
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

int Socket::set_noblock() {
    int flags;
    /** 
     * Set the socket nonblocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. 
     */
    if ((flags = fcntl(_s, F_GETFL)) == -1) {
        log_warning("fcntl(F_GETFL): %s", strerror(errno));
        return SOCKET_ERR;
    }
    if (fcntl(_s, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_warning("fcntl(F_SETFL, O_NONBLOCK): %s", strerror(errno));
        return SOCKET_ERR;
    }

    log_debug("[set noblocking] fd[%d]", _s);

    return SOCKET_OK;
}

bool Socket::get_local_address(SocketAddress* const sa) const {
    struct sockaddr_storage addr = {0};
    socklen_t addr_len = sizeof(addr);
    sockaddr* saddr = reinterpret_cast<sockaddr*>(&addr);

    if(::getsockname(_s, saddr, &addr_len) == SOCKET_ERR) {
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

bool Socket::get_remote_address(SocketAddress* const sa) const {
    struct sockaddr_storage addr = {0};
    socklen_t addr_len = sizeof(addr);
    sockaddr* saddr = reinterpret_cast<sockaddr*>(&addr);

    if(::getpeername(_s, saddr, &addr_len) == SOCKET_ERR) {
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

int Socket::listen(int backlog) {
    return SOCKET_ERR;
}

SOCKET Socket::accept(SocketAddress& sa) {
    return SOCKET_ERR;
}

int Socket::connect(SocketAddress& sa) {
    return SOCKET_ERR;
}

int Socket::write(const char* buf, size_t len) {
    int w = ::write(_s, buf, len);
    if (w == -1) {
        if (errno == EAGAIN)
            w = 0;
        else {
            log_debug("write: %s", strerror(errno));
            return SOCKET_ERR;
        }
    }

    return w;
}

int Socket::read(char* buf, size_t len) {
    int r = ::read(_s, buf, len);
    if (r == -1) {
        if (errno == EAGAIN)
            r = 0;
        else {
            log_debug("read: %s", strerror(errno));
            return SOCKET_ERR;
        }
    } else if (r == 0) {
        log_debug("read: peer closed");
        return SOCKET_PEER_CLOSED;
    }
    
    return r;
}

} // namespace zf


