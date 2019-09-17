/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file simple_client.cpp
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include <thread>

int fd = -1;

void send_to(void) {
    for(;;) {
        struct sockaddr_in sa;
        socklen_t salen = sizeof(sa);
        memset(&sa, 0, salen);
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");
        sa.sin_port = htons(8888);
        char sndbuf[1024];
        strcpy(sndbuf, "hello zframework");
        size_t s = sendto(fd, sndbuf, 1024, 0, (struct sockaddr *)&sa, salen);
        printf("send msg: %s len: %d\n", sndbuf, s);
        sleep(1);
    }
}

void recv_from(void) {
    for(;;) {
        char rcvbuf[1024];
        struct sockaddr_in ra;
        socklen_t ralen = sizeof(ra);
        size_t r = recvfrom(fd, rcvbuf, 1024, 0, (struct sockaddr*)&ra, &ralen);
        printf("recv msg: %s len: %d ip: %s port: %d\n", rcvbuf, r, inet_ntoa(ra.sin_addr), ntohs(ra.sin_port));
        usleep(10);
    }
}

int main(int ac, const char *av[])
{ 
    // create udp socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    // local address
    struct sockaddr_in la;
    memset(&la, 0, sizeof(la));
    la.sin_family = AF_INET;
    la.sin_addr.s_addr = INADDR_ANY;
    la.sin_port = htons(0);
	socklen_t lalen = sizeof(la);

    // bind
	if (bind(fd, (struct sockaddr*)&la, sizeof(la)) == -1) 
        perror("bind");

    // get local port
	if(-1 == getsockname(fd, (struct sockaddr*)&la, &lalen)) 
        perror("getsockname");
	printf("local port = %d\n", ntohs(la.sin_port));

    std::thread send_thread(send_to);
    std::thread recv_thread(recv_from);
    send_thread.join();
    recv_thread.join();

    return 0;
}


