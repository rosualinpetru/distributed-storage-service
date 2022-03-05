#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include "../libraries/log.h"
#include "../libraries/netio.h"
#include "store_interpreter.h"
#include "client_commands.h"
#include "bs3-system.h"

int get_size()
{
  int sum = 0;
  char buffer[1024];
  struct dirent *dptr;
  struct stat my_stat;
  DIR *direktor = opendir("data");

  while (NULL != (dptr = readdir(direktor)))
  {
    if (!strcmp(dptr->d_name, "."))
      continue;

    if (!strcmp(dptr->d_name, ".."))
      continue;

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "data/%s", dptr->d_name);
    if (stat(buffer, &my_stat) < 0)
      return MAX_STORE_SIZE+2;
    sum = sum + my_stat.st_size;
  }
  closedir(direktor);
  return MAX_STORE_SIZE - sum;
}

int upload_file(int sockfd, char *file_name)
{
  char buffer[1024];
  sprintf(buffer, "data/%s", file_name);
  int file = open(buffer, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, S_IRWXU | S_IRWXG | S_IRWXO);
  if (file < 0)
  {
    if (netio_write_int(sockfd, REQUEST_STOP) < 0)
      return RSP_SS_SYSTEM_ERROR;

    return RSP_SS_SYSTEM_ERROR;
  }
  if (netio_write_int(sockfd, REQUEST_START) < 0)
    return RSP_SS_SYSTEM_ERROR;

  if (netio_retrieve_file(sockfd, file) < 0)
    return RSP_SS_SYSTEM_ERROR;
  close(file);

  return RSP_SS_SUCCES;
}

int interpret_store_command(int command, int sockfd)
{
  ByteArray *arg1;
  char *buf;

  switch (command)
  {
  case CMD_SS_EXIT:
    return RSP_SS_CLOSE_CONN;
  case CMD_SS_UPLOAD_FILE:
    if (netio_read_ba(sockfd, &arg1) < 0)
      return RSP_SS_SYSTEM_ERROR;
    buf = ba_data(arg1);
    upload_file(sockfd, buf);
    free(buf);
    ba_destroy(arg1);
    break;
  case CMD_SS_REQUEST_SIZE:
    if (netio_write_int(sockfd, get_size()) < 0)
      return RSP_SS_SYSTEM_ERROR;
    break;
  }

  return RSP_SS_SUCCES;
}