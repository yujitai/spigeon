/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file socket_address.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __SOCKET_ADDRESS_H_
#define  __SOCKET_ADDRESS_H_

#include "server/common_include.h"
#include "server/socket_include.h"

namespace zf {

/*
bool socket_address_from_sockaddr_storage(const sockaddr_storage& addr, 
        SocketAddress* out_addr) 
{
    if (!out_addr)
        return false;

    if (addr.ss_family == AF_INET) {
        const sockaddr_in* saddr = (const sockaddr_in*)&addr;
        *out
    }
}
*/

/**
 * This interface class and its subclasses is intended to be used as replacement for 
 * the UNIX SOCKET data type 'struct sockaddr_in' and 'struct sockaddr_in6' and records 
 * an IP address and port.
 **/
class SocketAddress {
public:
    virtual ~SocketAddress() {}

    // Get a pointer to the address struct
    virtual operator struct sockaddr*() = 0;
    // Get length of address struct
    virtual operator socklen_t() = 0;

    virtual int family() = 0;
    virtual const std::string& ip() = 0;
    virtual uint16_t port() = 0;
};

} // namespace zf

#endif  //__SOCKET_ADDRESS_H_


