#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libraries/netio.h"
#include "libraries/log.h"
#include "libraries/byte_array.h"
#include "interpreters/client_interpreter.h"

#define SERVER_PORT 5678
#define MAX_BUF 1024

int server_response = REQUEST_DECLINED;

void print_banner()
{
    printf("\n");
    printf(" ██████╗ ██╗      ██╗ ███████╗ ███╗   ██╗ ████████╗\n");
    printf("██╔════╝ ██║      ██║ ██╔════╝ ████╗  ██║ ╚══██╔══╝\n");
    printf("██║      ██║      ██║ █████╗   ██╔██╗ ██║    ██║   \n");
    printf("██║      ██║      ██║ ██╔══╝   ██║╚██╗██║    ██║   \n");
    printf("╚██████╗ ███████╗ ██║ ███████╗ ██║ ╚████║    ██║   \n");
    printf(" ╚═════╝ ╚══════╝ ╚═╝ ╚══════╝ ╚═╝  ╚═══╝    ╚═╝   \n\n");
}

void server_response_timeout(int sig)
{
    if (server_response != REQUEST_ACCEPTED)
    {
        log_fatal("Server declined the request or it has timed out!\n");
        exit(9);
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
        log_error("Server says: DECLINED");
        printf("Server says: DECLINED");
        exit(7);
        break;
    default:
        log_error("Server didn't return a valid message!");
        printf("Server didn't return a valid message!");
        exit(8);
    }
}

void wait_auth_server_response(int socket_d)
{
    alarm(10);
    if (netio_read_int(socket_d, &server_response) < 0)
    {
        log_fatal("Failed to receive server log-in response!");
        exit(7);
    }
    switch (server_response)
    {
    case REQUEST_ACCEPTED:
        alarm(0);
        log_trace("Logged in succesfully!");
        printf("Logged in succesfully!\n");
        break;
    case REQUEST_DECLINED:
        alarm(0);
        log_trace("Bad credentials!");
        printf("Bad credentials!\n");
        break;
    default:
        log_fatal("Server didn't return a valid message!");
        printf("Server didn't return a valid message!");
        exit(8);
    }
}

int main(int argc, char *argv[])
{
    int socket_d;
    char buffer[MAX_BUF];
    struct sockaddr_in local_addr, remote_addr;
    struct sigaction process_actions;
    char access_key[20], secret_key[40];
    char *arg_ptr;
    int role = ROLE_CLIENT;

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

    if ((netio_write_int(socket_d, role)) < 0)
    {
        log_fatal("Failed to send role to server!");
        perror("Failed to send role to server!");
        exit(6);
    }
    log_debug("Role was sent succesfully!");

    wait_role_server_response(socket_d);

    server_response = REQUEST_DECLINED;
    ByteArray *byte_array;
    while (server_response == REQUEST_DECLINED)
    {
        memset(access_key, '\0', 20);
        memset(secret_key, '\0', 40);
        printf("Enter access key: ");
        scanf("%s", access_key);
        printf("Enter secret key: ");
        scanf("%s", secret_key);
        memset(buffer, '\0', MAX_BUF);
        sprintf(buffer, "%s\n%s", access_key, secret_key);
        byte_array = ba_new(buffer, strlen(buffer));
        if (byte_array == NULL)
        {
            log_error("Failed create byte array!");
            continue;
        }
        if ((netio_write_ba(socket_d, byte_array)) < 0)
        {
            log_error("Failed to send keys to server!");
            continue;
        }
        ba_destroy(byte_array);
        log_debug("Keys were sent succesfully!");
        wait_auth_server_response(socket_d);
    }
    // C printing bugs! :)
    printf("BS3 > ");
    getchar();
    while (1)
    {
        memset(buffer, '\0', MAX_BUF);
        fgets(buffer, MAX_BUF, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        arg_ptr = strtok(buffer, " ");
        server_response = interpret_command(parse_command(arg_ptr), socket_d, arg_ptr);
        log_debug("Server response: %d", server_response);
        interpret_reponse(server_response);
        printf("BS3 > ");
    }
    exit(0);
}
