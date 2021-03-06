/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file ipv4_address.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __IPV4_ADDRESS_H_
#define  __IPV4_ADDRESS_H_

#include "server/socket_address.h"

namespace zf {

/**
 * Ipv4Address
 **/
class Ipv4Address : public SocketAddress {
public:
    // construct without ip and port, 
    // used on the 'accept' situation
    Ipv4Address();
    // consturct with ip and port
    Ipv4Address(const std::string& ip, uint16_t port);
    // copy construct
    Ipv4Address(const Ipv4Address& from) = default;
    // copy operator
    Ipv4Address& operator=(const Ipv4Address& from) = default;

    ~Ipv4Address();

    // SocketAddress implementation
    operator struct sockaddr*() override;
    operator socklen_t() override;
    int family() override;
    const std::string ip() override;
    uint16_t port() override;

private:
    std::string _ip;
    uint16_t _port;
    struct sockaddr_in _addr;
};

} // namespace zf

#endif  //__IPV4_ADDRESS_H_


