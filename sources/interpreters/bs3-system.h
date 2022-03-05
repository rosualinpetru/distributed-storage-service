#ifndef BS3SYS
#define BS3SYS

#define ROLE_STORE 1
#define ROLE_CLIENT 2

#define REQUEST_ACCEPTED 10
#define REQUEST_DECLINED 11
#define REQUEST_ERROR 12
#define REQUEST_START 13
#define REQUEST_STOP 14

#define MAX_ARGS_COUNT 10
#define MAX_STORE_SIZE 1000000000

#include "../libraries/byte_array.h"

typedef struct
{
    ByteArray *name;
    int socket_d;
} store_machine;

#endif