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

#include "server/common.h"

#include "server/connection.h"

namespace zf {

/**
 * UDPConnection
 * 暂时不支持mcpack协议
 */
class UDPConnection : public Connection {
public:
    UDPConnection(int fd);
    virtual ~UDPConnection() override;

    
};

} // namespace zf

#endif  //__UDP_CONNECTION_H_


