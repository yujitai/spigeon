#ifndef _NETWORK_H_
#define _NETWORK_H_
#include <stddef.h>

namespace store {
enum {
  NET_ERROR = -1,
  NET_OK = 0
};

// returns the listening fd or NET_ERROR
int create_tcp_server(int port, const char *bindaddr);

// accept a tcp socket
int tcp_accept(int s, char *ip, int *port);

int tcp_connect(const char* host, int port);

int sock_setnonblock(int s);

int sock_setnodelay(int s);

int sock_read_data(int fd, char *data, size_t len);

int sock_write_data(int fd, const char *data, size_t len);

int sock_get_name(int fd, char *ip, int *port);

int sock_peer_to_str(int fd, char *ip, int *port);

}
#endif
