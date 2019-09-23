/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file network_manager.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __NETWORK_MANAGER_H_
#define  __NETWORK_MANAGER_H_

#include <vector>

#include "server/tcp_socket.h"
#include "server/ipv4_address.h"

namespace zf {

class UDPSocket;
class TCPSocket;
class EventLoop;

/**
 * NetworkMgr
 * Sockets Manager.
 **/
class NetworkMgr {
public:
    enum {
        NET_ERROR = -1,       // Generic net error
        NET_PEER_CLOSED = -2, // Peer closed
        NET_OK = 0
    };

    NetworkMgr(EventLoop* el);
    ~NetworkMgr();

    /**
     * Return the udp sockfd or NET_ERROR
     **/
    SOCKET create_udp_server(const std::string& ip, uint16_t port);

    /** 
     * Return the tcp listening fd or NET_ERROR 
     **/
    SOCKET create_tcp_server(const std::string& ip, uint16_t port);

    /**
     * Accept a tcp socket
     **/
    SOCKET tcp_accept(SOCKET s, SocketAddress& sa);
    
    /**
     * Wrap a udp/tcp socket
     **/
    bool wrap_socket(SOCKET s);

    std::vector<Socket*>& get_sockets();

#if 0
    int tcp_connect(const char* host, int port);
    int sock_setnonblock(int s);
    int sock_setnodelay(int s);
    int sock_read_data(int fd, char* data, size_t len);
    int sock_write_data(int fd, const char* data, size_t len);
    int sock_get_name(int fd, char* ip, uint16_t* port);
    int sock_peer_to_str(int fd, char* ip, uint16_t* port);
#endif

private:
    EventLoop* _el;

    // index is fd.
    std::vector<Socket*> _sockets;
};

} // namespace zf

#endif  //__NETWORK_MANAGER_H_


