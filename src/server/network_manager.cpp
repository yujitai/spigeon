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

namespace zf {

NetworkMgr::NetworkMgr(EventLoop* el)
    : _el(el)
{

}

NetworkMgr::~NetworkMgr() {

}

#if 0
int create_udp_server(int port, const char* ip) {
    int s;
    struct sockaddr_in sa;

    if ((s = create_socket(AF_INET, SOCK_DGRAM)) == NET_ERROR)
        return NET_ERROR;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t slen = sizeof(sa);
    if (ip && inet_aton(ip, &sa.sin_addr) == 0) {
        log_warning("invalid ip address");
        close(s);
        return NET_ERROR;
    }

    if (bind(s, (sockaddr*)&sa, slen) == -1) {
        log_warning("bind: %s", strerror(errno));
        close(s);
        return NET_ERROR;
    }

    return s;
}
#endif

SOCKET NetworkMgr::create_tcp_server(const std::string& ip, uint16_t port) {
    Socket* socket = new TCPSocket();
    SocketAddress* sa = new Ipv4Address(ip, port);
    log_debug("[server address] ip[%s] port[%d] addrlen[%d]", 
            sa->ip().c_str(), sa->port(), (socklen_t)(*sa));

    socket->create(AF_INET, SOCK_STREAM);
    socket->bind(sa);
    socket->listen(1024);

    int fd = socket->fd();
    if ((uint32_t)fd >= _sockets.size())
        _sockets.resize(fd * 2, NULL);
    _sockets[fd] = socket;

    return fd;
}

SOCKET NetworkMgr::tcp_accept(int fd, SocketAddress& sa) {
    Socket* socket = _sockets[fd];

    SOCKET s = socket->accept(sa);
    log_debug("accept: client addr: ip[%s] port[%d]", sa.ip().c_str(), sa.port());
    wrap_socket(s);

    return s; 
}

bool NetworkMgr::wrap_socket(SOCKET s) {
    Socket* socket = new TCPSocket(s);

    int on = 1;
    socket->set_option(Socket::OPT_NODELAY, on);
    socket->set_noblock();

    if ((uint32_t)s >= _sockets.size())
        _sockets.resize(s*2, NULL);
    _sockets[s] = socket;

    return true;
}   

std::vector<Socket*>& NetworkMgr::get_sockets() {
    return _sockets;
}

/*
int tcp_connect(const char* addr, uint16_t port) {
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


