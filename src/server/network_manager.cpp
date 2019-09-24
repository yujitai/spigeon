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

SOCKET NetworkManager::create_server(uint8_t type, const std::string& ip, uint16_t port) {
    // TODO:addr storage.
    Ipv4Address addr(ip, port);
    Socket* socket = nullptr;

    if (G_SERVER_TCP == type) {
        socket = new TCPSocket();
        socket->create(AF_INET, SOCK_STREAM);
        socket->bind(addr);
        socket->listen(1024);
    } 
    
    if (G_SERVER_UDP == type) {
        //socket = new UDPSocket();
        //socket->create(AF_INET, SOCK_DGRAM);
        //socket->bind(addr);
    }

    log_debug("[server address] type[%d] ip[%s] port[%d] addrlen[%d]", 
            type, addr.ip().c_str(), addr.port(), (socklen_t)addr);

    SOCKET fd = socket->fd();
    if (fd >= _sockets.size())
        _sockets.resize(fd * 2, NULL);
    _sockets[fd] = socket;

    return fd;
}

SOCKET NetworkManager::tcp_accept(int fd, SocketAddress& sa) {
    Socket* socket = _sockets[fd];

    SOCKET s = socket->accept(sa);
    log_debug("accept: client addr: ip[%s] port[%d]", sa.ip().c_str(), sa.port());
    wrap_socket(s);

    return s; 
}

bool NetworkManager::wrap_socket(SOCKET s) {
    Socket* socket = new TCPSocket(s);

    int on = 1;
    socket->set_option(Socket::OPT_NODELAY, on);
    socket->set_noblock();

    if ((uint32_t)s >= _sockets.size())
        _sockets.resize(s*2, NULL);
    _sockets[s] = socket;

    return true;
}   

std::vector<Socket*>& NetworkManager::get_sockets() {
    return _sockets;
}

/*
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

int sock_get_name(int fd, char* ip, uint16_t* port) {
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    if (getsockname(fd, (struct sockaddr*)&sa, &salen) == -1) {
        *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return NET_ERROR;
    }
    if (ip) strcpy(ip, inet_ntoa(sa.sin_addr));
    if (port) *port = ntohs(sa.sin_port);

    return NET_OK;
}

int sock_peer_to_str(int fd, char* ip, uint16_t* port) {
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd,(struct sockaddr*)&sa,&salen) == -1) {
        *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return NET_ERROR;
    }
    if (ip) strcpy(ip,inet_ntoa(sa.sin_addr));
    if (port) *port = ntohs(sa.sin_port);

    return NET_OK;
}
*/

} // namespace zf


