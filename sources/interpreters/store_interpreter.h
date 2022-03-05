#ifndef ST_INT
#define ST_INT

#include "bs3-system.h"
#include "client_commands.h"
#include "server_commands.h"

int get_Size();

void list_Files(DIR *directory, char *bck, int sockfd);

int interpret_store_command(int command, int sockfd);
#endif 