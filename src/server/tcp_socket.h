/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file tcp_socket.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __TCP_SOCKET_H_
#define  __TCP_SOCKET_H_

#include "server/socket.h"

namespace zf {

class TCPSocket : public Socket {
public:
    TCPSocket();
    ~TCPSocket();

    // Socket implementation.
    int create(int family, int type) override;
    int bind(SocketAddress* sa) override;
    int listen(int backlog) override;
    int accept(SocketAddress* sa) override;
    int connect(SocketAddress* sa) override;
    int write(const char* buf, size_t len) override;
    int read(char* buf, size_t len) override;
    SocketAddress* get_local_address() const override;
    SocketAddress* get_remote_address() const override;
    int get_option(Option opt, int* value) override;
    int set_option(Option opt, int value) override;

    int translate_option(Option opt, int* slevel, int* sopt);

    SOCKET fd() override;
private:
    SOCKET _s;
    SocketAddress* _local_addr;
};

} // namespace zf 

#endif  //__TCP_SOCKET_H_


