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

class EventLoop;

/**
 * NetworkManager
 * Sockets Manager
 **/
class NetworkManager {
public:
    NetworkManager(EventLoop* el);
    ~NetworkManager();

    // create a concrete server
    Socket* create_server(uint8_t type,  
            const std::string& ip, uint16_t port);

    // accept a new udp or tcp connection
    Socket* generic_accept(SOCKET listen_fd, SocketAddress& sa);

    // wrap a udp or tcp socket
    Socket* wrap_socket(Socket* listen_s, SOCKET s);

    std::vector<Socket*>& sockets();
private:
    // owned by dispatcher.
    EventLoop* _el;

    // index is fd.
    std::vector<Socket*> _sockets;
};

} // namespace zf

#endif  //__NETWORK_MANAGER_H_


