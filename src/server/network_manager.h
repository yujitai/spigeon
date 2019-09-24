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

#include "server/udp_socket.h"
#include "server/tcp_socket.h"
#include "server/ipv4_address.h"

namespace zf {

class UDPSocket;
class TCPSocket;
class EventLoop;

/**
 * NetworkManager
 * Sockets Manager.
 **/
class NetworkManager {
public:
    enum {
        NET_ERROR = -1,       // Generic net error
        NET_PEER_CLOSED = -2, // Peer closed
        NET_OK = 0
    };

    NetworkManager(EventLoop* el);
    ~NetworkManager();

    /** 
     * Create a concrete server.
     **/
    SOCKET create_server(uint8_t type, const std::string& ip, uint16_t port);

    /**
     * Accept a tcp socket.
     **/
    SOCKET tcp_accept(SOCKET s, SocketAddress& sa);
    
    /**
     * Wrap a udp/tcp socket.
     **/
    bool wrap_socket(SOCKET s);

    std::vector<Socket*>& get_sockets();

#if 0
    int tcp_connect(const char* host, int port);
    int sock_get_name(int fd, char* ip, uint16_t* port);
    int sock_peer_to_str(int fd, char* ip, uint16_t* port);
#endif

private:
    // owned by dispatcher.
    EventLoop* _el;

    // index is fd.
    std::vector<Socket*> _sockets;
};

} // namespace zf

#endif  //__NETWORK_MANAGER_H_


