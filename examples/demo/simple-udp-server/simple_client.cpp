#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <pthread.h>

int main(int ac, const char *av[])
{ 
    int send_socket = socket(AF_INET, SOCK_DGRAM, 0);
#if 1
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
	int ret = bind(send_socket, (struct sockaddr *) &local_addr, sizeof(local_addr)); 

	memset(&local_addr, 0, sizeof(local_addr));
	socklen_t len = sizeof(local_addr);
	if(-1 == getsockname(send_socket, (struct sockaddr*)&local_addr, &len)) {
		printf("audio getsockname failed\n");
		return 0;
	}
	int port = ntohs(local_addr.sin_port);
	printf("local port = %d\n", port);
#endif


    struct sockaddr_in send_address;
    socklen_t addrlen = sizeof(send_address);
    memset(&send_address, 0, addrlen);
    send_address.sin_family = AF_INET;
    send_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    send_address.sin_port = htons(8888);

    char buffer[2048];
    strcpy(buffer, "hello zframework");
	while(1) {
        size_t s = sendto(send_socket, buffer, 20, 0, (struct sockaddr *)&send_address, addrlen);
        printf("client send bytes = %d\n", s);
		usleep(2000 * 1000);
	}
    return 0;
}
