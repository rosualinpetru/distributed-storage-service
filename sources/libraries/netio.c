#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include "netio.h"

#define EOS "!-!-!-EOS-!-!-!"
#define CHUNK_SIZE 4096

int set_addr(struct sockaddr_in *addr, char *name, u_int32_t inaddr, short sin_port)
{
    struct hostent *h;
    memset((void *)addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    if (name != NULL)
    {
        h = gethostbyname(name);
        if (h == NULL)
            return -1;
        addr->sin_addr.s_addr =
            *(u_int32_t *)h->h_addr_list[0];
    }
    else
        addr->sin_addr.s_addr = htonl(inaddr);
    addr->sin_port = htons(sin_port);
    return 0;
}

ByteArray *get_eos()
{
    char buffer[20];
    strcpy(buffer, EOS);
    ByteArray *byte_array;
    while ((byte_array = ba_new(buffer, strlen(buffer))) == NULL)
        ;
    return byte_array;
}

int netio_read_int(int socket_descriptor, int *data)
{
    int response;
    if ((response = read(socket_descriptor, data, 4)) < 0)
    {
        perror("Failed to read int from socket!");
        return -1;
    }

    return response;
}

int netio_write_int(int socket_descriptor, int data)
{
    int response;
    if ((response = write(socket_descriptor, &data, 4)) < 0)
    {
        perror("Failed to write int to socket!");
        return -1;
    }

    return response;
}

int netio_read_ba(int socket_descriptor, ByteArray **byte_array)
{
    int read_amount;

    if (netio_read_int(socket_descriptor, &read_amount) < 0)
    {
        perror("Failed to read amount of data that needs to be read from socket!");
        return -1;
    }
    *byte_array = ba_from_descriptor(socket_descriptor, read_amount);
    return read_amount;
}

int netio_write_ba(int socket_descriptor, ByteArray *byte_array)
{
    int write_amount = ba_size(byte_array);

    if (write(socket_descriptor, &write_amount, 4) < 0)
    {
        perror("Failed to write the amount of data that will be written to socket!");
        return -1;
    }

    return ba_write(socket_descriptor, byte_array);
}

int netio_retrieve_file(int socket_descriptor, int file_descriptor)
{
    ByteArray *byte_array;
    ByteArray *eos = get_eos();
    while (netio_read_ba(socket_descriptor, &byte_array) > 0)
    {
        if (ba_compare(eos, byte_array) == 0)
            break;
        ba_write(file_descriptor, byte_array);
        ba_destroy(byte_array);
    }
    ba_destroy(eos);
    return 0;
}

int netio_relay(int source, int destination)
{
    ByteArray *byte_array;
    ByteArray *eos = get_eos();
    while (netio_read_ba(source, &byte_array) > 0)
    {
        netio_write_ba(destination, byte_array);
        if (ba_compare(eos, byte_array) == 0)
        {
            ba_destroy(byte_array);
            break;
        }
        ba_destroy(byte_array);
    }
    ba_destroy(eos);
    return 0;
}

int netio_send_file(int socket_descriptor, int file_descriptor)
{
    ByteArray *byte_array;
    ByteArray *eos = get_eos();
    while ((byte_array = ba_from_descriptor(file_descriptor, CHUNK_SIZE)) != NULL)
    {
        if (ba_size(byte_array) == 0)
            break;
        netio_write_ba(socket_descriptor, byte_array);
        ba_destroy(byte_array);
    }
    netio_write_ba(socket_descriptor, eos);
    ba_destroy(eos);
    return 0;
}