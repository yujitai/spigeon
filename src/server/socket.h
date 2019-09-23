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

namespace zf { 

/**
 * General interface for the socket implementations of various newworks.
 * The methods match those of normal UNIX sockets very closely.
 **/
class Socket {
public:
    enum {
        NET_ERROR = -1,       // Generic net error
        NET_PEER_CLOSED = -2, // Peer closed
        NET_OK = 0
    };

    virtual ~Socket() {}

    // Create a async socket
    virtual int create(int family, int type) = 0;
    virtual int bind(SocketAddress* sa) = 0;
    virtual int listen(int backlog);
    virtual int accept(SocketAddress& sa);
    virtual int connect(SocketAddress* sa);
    virtual int write(const char* buf, size_t len) = 0;
    virtual int read(char* buf, size_t len) = 0;
    virtual SocketAddress* get_local_address() const = 0; 
    virtual SocketAddress* get_remote_address() const = 0; 

    // Socket options config
    enum Option {
        OPT_RCVBUF,
        OPT_SNDBUF,
        OPT_NODELAY,
        OPT_REUSEADDR,
    };
    virtual int get_option(Option opt, int* value) = 0;
    virtual int set_option(Option opt, int value) = 0;
    virtual int set_noblock() = 0;

    virtual SOCKET fd() = 0;

protected:
    Socket() {}
};

}; // namespace zf

#endif  //__SOCKET_H_


