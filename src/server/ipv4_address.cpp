/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file ipv4_address.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/ipv4_address.h"

#include <string.h>

namespace zf {

Ipv4Address::Ipv4Address() 
    : _ip("0.0.0.0"), _port(0)
{
    memset(&_addr, 0, sizeof(_addr));
}

Ipv4Address::Ipv4Address(const std::string& ip, uint16_t port) 
    : _ip(ip), _port(port)
{
    memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

Ipv4Address::~Ipv4Address() {

}

Ipv4Address::operator struct sockaddr*() {
    return (struct sockaddr*)&_addr;
}

Ipv4Address::operator socklen_t() {
    return sizeof(struct sockaddr_in);
}

int Ipv4Address::family() {
    return _addr.sin_family;
}

const std::string Ipv4Address::ip() {
    // return _ip;
    return inet_ntoa(_addr.sin_addr);
}

uint16_t Ipv4Address::port() {
    // return _port;
    return ntohs(_addr.sin_port);
}

} // namespace zf


