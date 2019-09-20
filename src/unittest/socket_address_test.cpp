/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file socket_address_test.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include "server/common_include.h"
#include "server/socket_include.h"
#include "server/ipv4_address.h"
#include "server/ipv6_address.h"

using namespace zf;

int main() {
    SocketAddress* sa = new Ipv4Address("192.168.32.44", 8888);
    cout << "ip=" << sa->ip() << " port=" << sa->port() << endl;
    cout << "struct sockaddr_in len=" << (socklen_t)(*sa) << endl;
    cout << "struct sockaddr*=" << (struct sockaddr*)(*sa) << endl;

    delete sa;
    return 0;
}


