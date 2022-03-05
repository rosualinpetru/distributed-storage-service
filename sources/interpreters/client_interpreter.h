#ifndef CL_INT
#define CL_INT

#include "bs3-system.h"
#include "client_commands.h"

int parse_command(char *arg);
int interpret_command(int command, int socket_d, char *args);
void interpret_reponse(int server_response);

#endif