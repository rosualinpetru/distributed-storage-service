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

#include "libraries/netio.h"
#include "libraries/log.h"
#include "interpreters/store_interpreter.h"

#define MAXBUF 1024
#define SERVER_PORT 5678

int server_response = REQUEST_DECLINED;

void print_banner()
{
  printf("\n");
  printf("███████╗ ████████╗  ██████╗  ██████╗  ███████╗\n");
  printf("██╔════╝ ╚══██╔══╝ ██╔═══██╗ ██╔══██╗ ██╔════╝\n");
  printf("███████╗    ██║    ██║   ██║ ██████╔╝ █████╗  \n");
  printf("╚════██║    ██║    ██║   ██║ ██╔══██╗ ██╔══╝  \n");
  printf("███████║    ██║    ╚██████╔╝ ██║  ██║ ███████╗\n");
  printf("╚══════╝    ╚═╝     ╚═════╝  ╚═╝  ╚═╝ ╚══════╝\n\n");
}

void server_response_timeout(int sig)
{
  if (server_response != REQUEST_ACCEPTED)
  {
    log_fatal("Server declined the request or it has timed out!\n");
    exit(11);
  }
}

void wait_role_server_response(int socket_d)
{
  alarm(10);
  if (netio_read_int(socket_d, &server_response) < 0)
    log_error("Failed to receive role response!");

  switch (server_response)
  {
  case REQUEST_ACCEPTED:
    alarm(0);
    log_debug("Server says: ACCEPTED");
    break;
  case REQUEST_DECLINED:
    log_fatal("Server says: DECLINED");
    printf("Server says: DECLINED");
    exit(10);
    break;
  default:
    log_fatal("Server didn't return a valid message!");
    printf("Server didn't return a valid message!");
    exit(9);
  }
}

int main(int argc, char *argv[])
{
  int socket_d;
  struct sockaddr_in local_addr, remote_addr;
  struct sigaction process_actions;
  int role = ROLE_STORE;
  char host_name[64];

  if (argc > 2)
  {
    log_fatal("Usage: %s <LOG_LEVEL>", argv[0]);
    printf("Usage: %s <LOG_LEVEL>", argv[0]);
    exit(1);
  }

  print_banner();

  if (argc == 1)
    log_set_level(LOG_TRACE);
  else
    log_set_level(atoi(argv[1]));

  memset(&process_actions, 0x00, sizeof(struct sigaction));
  process_actions.sa_handler = &server_response_timeout;

  if (sigaction(SIGALRM, &process_actions, NULL) < 0)
    log_error("Process ignores SIGALRM!");
  else
    log_trace("Setting SIGALARM action!");

  socket_d = socket(PF_INET, SOCK_STREAM, 0);
  if (socket_d == -1)
  {
    log_fatal("Unable to create socket!");
    perror("Unable to create socket!");
    exit(2);
  }
  log_debug("Socket created!");
  set_addr(&local_addr, NULL, INADDR_ANY, 0);
  if (bind(socket_d, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1)
  {
    log_fatal("Failed to bind local address - INADDR_ANY - to the socket!");
    perror("Failed to bind local address!");
    exit(3);
  }
  log_debug("Local address - INADDR_ANY - bound to socket!");
  if (set_addr(&remote_addr, "localhost", 0, SERVER_PORT) == -1)
  {
    log_fatal("Setting address error!");
    perror("Failed to bind local address!");
    exit(4);
  }
  log_debug("Address was set succesfully!");
  if (-1 == connect(socket_d, (struct sockaddr *)&remote_addr, sizeof(remote_addr)))
  {
    log_fatal("Failed to connect to the server!");
    perror("Failed to connect to the server!");
    exit(5);
  }
  log_debug("Connection to the server was successful!");

  if (netio_write_int(socket_d, role) < 0)
  {
    log_fatal("Failed to send role to server!");
    perror("Failed to send role to server!");
    exit(6);
  }
  log_debug("Role was sent succesfully!");

  memset(host_name, 0, sizeof(host_name));
  if (gethostname(host_name, 64) < 0)
  {
    log_fatal("Failed to get host name!");
    perror("Failed to get host name!");
    exit(7);
  }
  ByteArray *byte_array = ba_new(host_name, strlen(host_name));
  if (netio_write_ba(socket_d, byte_array) < 0)
  {
    log_fatal("Failed to send host name to server!");
    perror("Failed to send host name to server!");
    exit(8);
  }

  wait_role_server_response(socket_d);

  int command;

  while (netio_read_int(socket_d, &command) > 0)
  {
    interpret_store_command(command, socket_d);
  }

  return 0;
}
