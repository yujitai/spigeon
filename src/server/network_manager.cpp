/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file network_manager.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/network_manager.h"

#include "server/server.h"

namespace zf {

NetworkManager::NetworkManager(EventLoop* el)
    : _el(el)
{

}

NetworkManager::~NetworkManager() {

}

Socket* NetworkManager::create_server(uint8_t type, 
                                      const std::string& ip, 
                                      uint16_t port) 
{
    Ipv4Address srv_addr(ip, port);
    Socket* s = nullptr;

    if (G_SERVER_TCP == type) {
        s = new TCPSocket(srv_addr);
        s->create_bind(AF_INET, SOCK_STREAM);
        s->listen(1024);
    } else if (G_SERVER_UDP == type) {
        s = new UDPSocket(srv_addr);
        s->create_bind(AF_INET, SOCK_DGRAM);
    }

    SOCKET fd = s->fd();
    if (fd >= _sockets.size())
        _sockets.resize(fd*2, NULL);
    _sockets[fd] = s;

    return s;
}

Socket* NetworkManager::generic_accept(SOCKET listen_fd, SocketAddress& sa) {
    Socket* s = _sockets[listen_fd];

    SOCKET fd = s->accept(sa);
    if (fd == INVALID_SOCKET) {
        log_fatal("[generic accept: invalid socket]");
        return nullptr;
    }

    log_debug("generic accept: client info: ip[%s] port[%d]", 
            sa.ip().c_str(), sa.port());

    return wrap_socket(s, fd);
}

Socket* NetworkManager::wrap_socket(Socket* listen_s, SOCKET fd) {
    int on = 1;
    Socket* s = nullptr;

    if (dynamic_cast<TCPSocket*>(listen_s)) {
        s = new TCPSocket(fd);
        s->set_option(Socket::OPT_NODELAY, on);
        s->set_noblock();
    } else if (dynamic_cast<UDPSocket*>(listen_s)) {
        s = new UDPSocket(fd);
        s->set_option(Socket::OPT_REUSEADDR, on);
        s->set_noblock();
    }

    if (fd >= _sockets.size())
        _sockets.resize(fd*2, NULL);
    _sockets[fd] = s;
    
    return s;
}   

std::vector<Socket*>& NetworkManager::sockets() {
    return _sockets;
}

#if 0
SOCKET NetworkManager::tcp_connect(const char* addr, uint16_t port) {
    int s = create_socket(AF_INET, SOCK_STREAM);
    if (s == NET_ERROR)
        return NET_ERROR;
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        he = gethostbyname(addr);
        if (he == NULL) {
            log_warning("can't resolve: %s", addr);
            close(s);
            return NET_ERROR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        log_warning("connect: %s", strerror(errno));
        close(s);
        return NET_ERROR;
    }

    return s;
}
#endif

} // namespace zf


