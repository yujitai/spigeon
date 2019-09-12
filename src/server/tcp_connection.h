/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_connection.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __TCP_CONNECTION_H_
#define  __TCP_CONNECTION_H_

namespace zf {

class TCPConnection : public Connection {
public:
    TCPConnection();
    virtual ~TCPConnection() override;
};

} // namespace zf

#endif  //__TCP_CONNECTION_H_


