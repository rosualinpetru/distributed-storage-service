#ifndef NETIO
#define NETIO

#include <unistd.h>
#include <netinet/in.h>
#include "byte_array.h"

int set_addr(struct sockaddr_in *addr, char *name, u_int32_t inaddr, short sin_port);

int netio_read_ba(int socket_descriptor, ByteArray **byte_array);
int netio_write_ba(int socket_descriptor, ByteArray *byte_array);

int netio_read_int(int socket_descriptor, int *data);
int netio_write_int(int socket_descriptor, int data);
int netio_relay(int source, int destination);

int netio_send_file(int socket_descriptor, int file_descriptor);
int netio_retrieve_file(int socket_descriptor, int file_descriptor);

#endif