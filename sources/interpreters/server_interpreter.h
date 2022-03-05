#ifndef SV_INT
#define SV_INT

#include <pthread.h>

#include "bs3-system.h"
#include "client_commands.h"
#include "server_commands.h"

int interpret_client_command(int command, store_machine stores[], int stores_count_aux, int socket_d, pthread_mutex_t mutex);

#endif