/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file udp_connection.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __UDP_CONNECTION_H_
#define  __UDP_CONNECTION_H_

#include "server/connection.h"

namespace zf {

class Socket;

/**
 * UDPConnection
 * 暂时不支持mcpack协议
 */
class UDPConnection : public Connection {
public:
    UDPConnection(Socket* s);
    virtual ~UDPConnection() override;
    //int read(char* buf, size_t len) override;
    //int write(const char* buf, size_t len) override;
};

} // namespace zf

#endif  //__UDP_CONNECTION_H_


