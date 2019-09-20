/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file ipv6_address.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __IPV6_ADDRESS_H_
#define  __IPV6_ADDRESS_H_

#include "server/socket_address.h"

namespace zf {

/**
 *  TODO:暂时不支持ipv6
 **/
class Ipv6Address : public SocketAddress {
public:

};

} // namespace zf

#endif  //__IPV6_ADDRESS_H_


