/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file socket.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __SOCKET_H_
#define  __SOCKET_H_

#include "server/common_include.h"
#include "server/socket_include.h"
#include "server/socket_address.h"
#include "server/ipv4_address.h"

namespace zf { 

enum {
    SOCKET_OK = 0,
    SOCKET_ERR = -1,       
    SOCKET_PEER_CLOSED = -2
};

/**
 * Base socket of various networks which match those of normal 
 * UNIX sockets very closely.
 **/
class Socket {
public:
    // for server
    Socket(const Ipv4Address& srv_addr);

    // for client
    Socket(SOCKET fd);

    virtual ~Socket() {}

    // create a async socket and bind it to the specified address
    int create_bind(int family, int type);
    SOCKET create_bind2(int family, int type);

    virtual int listen(int backlog);
    virtual SOCKET accept(SocketAddress& sa);
    virtual int connect(SocketAddress& sa);

    int write(const char* buf, size_t len);
    int read(char* buf, size_t len);

    bool get_local_address(SocketAddress* sa) const; 
    bool get_remote_address(SocketAddress* sa) const; 

    // socket options config
    enum Option {
        OPT_RCVBUF,
        OPT_SNDBUF,
        OPT_NODELAY,
        OPT_REUSEADDR,
    };

    int set_noblock();
    int set_option(Option opt, int value);
    int set_option(SOCKET fd, Option opt, int value);
    int translate_option(Option opt, int* slevel, int* sopt);

    SOCKET fd() { return _s; }

protected:
    SOCKET _s;
    Ipv4Address _local_addr;
};

}; // namespace zf

#endif  //__SOCKET_H_


