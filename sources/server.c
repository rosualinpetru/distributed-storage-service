#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "libraries/byte_array.h"
#include "libraries/netio.h"
#include "libraries/log.h"
#include "interpreters/server_interpreter.h"

#define SERVERPORT (short)5678

pthread_mutex_t mutex;
int client_count = 0, store_count = 0;
store_machine stores[20];
ByteArray *credentials;

void print_banner()
{
    printf("\n");
    printf("███████╗ ███████╗ ██████╗  ██╗   ██╗ ███████╗ ██████╗ \n");
    printf("██╔════╝ ██╔════╝ ██╔══██╗ ██║   ██║ ██╔════╝ ██╔══██╗\n");
    printf("███████╗ █████╗   ██████╔╝ ██║   ██║ █████╗   ██████╔╝\n");
    printf("╚════██║ ██╔══╝   ██╔══██╗ ╚██╗ ██╔╝ ██╔══╝   ██╔══██╗\n");
    printf("███████║ ███████╗ ██║  ██║  ╚████╔╝  ███████╗ ██║  ██║\n");
    printf("╚══════╝ ╚══════╝ ╚═╝  ╚═╝   ╚═══╝   ╚══════╝ ╚═╝  ╚═╝\n\n");
}

void remove_client(int connection_d)
{
    close(connection_d);
    log_info("Ending connection with CLIENT%d!", connection_d);
    pthread_mutex_lock(&mutex);
    client_count--;
    log_info("Clients count: %d", client_count);
    pthread_mutex_unlock(&mutex);
}

void *client_thread(void *arg)
{
    int connection_d, auth_response, command, code, reattempts = 0;

    connection_d = *((int *)arg);
    free(arg);
    ByteArray *client_credentials;
    do
    {
        netio_read_ba(connection_d, &client_credentials);
        if (client_credentials == NULL)
        {
            log_error("Failed reading credentials fot CLIENT%d!", connection_d);
            close(connection_d);
            return NULL;
        }
        log_trace("Read credentials stream!");
        auth_response = REQUEST_DECLINED;
        if (ba_compare(credentials, client_credentials) == 0)
            auth_response = REQUEST_ACCEPTED;
        log_trace("Compared credentials streams! Result: %s", (auth_response == REQUEST_ACCEPTED) ? "accepted" : "declined");

        ba_destroy(client_credentials);
        if (netio_write_int(connection_d, auth_response) < 0)
        {
            log_fatal("Unable to response credentials validity!");
            close(connection_d);
            return NULL;
        }
        log_trace("Wrote response to client!");

    } while (auth_response == REQUEST_DECLINED);

    log_debug("Authenticated! Waiting for commands!");

    while (netio_read_int(connection_d, &command) > 0)
    {
        code = interpret_client_command(command, stores, store_count, connection_d, mutex);
        reattempts = 0;
        while (netio_write_int(connection_d, code) < 0)
        {
            reattempts++;
            if (reattempts == 5)
            {
                log_error("Error sending response!");
                remove_client(connection_d);
                return NULL;
            }
        }
        if (code == RSP_CS_CLOSE_CONN)
            break;
    }

    remove_client(connection_d);
    return NULL;
}

int manage_store_role(int connection_d)
{
    store_machine new_machine;

    ByteArray *byte_array;
    if (netio_read_ba(connection_d, &byte_array) < 0)
    {
        log_error("Error reading hostname!");
        close(connection_d);
        return REQUEST_ERROR;
    }

    new_machine.socket_d = connection_d;
    new_machine.name = byte_array;

    char *aux = ba_data(byte_array);
    log_info("Identified as STORE: %s!", aux);
    free(aux);

    for (int i = 0; i < store_count; i++)
    {
        if (ba_compare(byte_array, stores[i].name) == 0)
        {
            log_info("A store with this hostname already exists! Decline!");
            return REQUEST_DECLINED;
        }
    }
    pthread_mutex_lock(&mutex);
    stores[store_count++] = new_machine;
    log_info("Stores count: %d", store_count);
    pthread_mutex_unlock(&mutex);
    return REQUEST_ACCEPTED;
}

void manage_client_role(int connection_d)
{
    int *aux;
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1);
    pthread_mutex_init(&mutex, NULL);

    log_info("Identified as CLIENT%d!", connection_d);
    aux = (int *)calloc(1, sizeof(int));
    if (aux == NULL)
    {
        log_fatal("Error allocating client socket descriptor!");
        exit(5);
    }
    *aux = connection_d;
    if (pthread_create(&thread, &attr, client_thread, (void *)aux) != 0)
    {
        log_error("Failed open thread! Failed to serve the client! Connection closed!");
        close(connection_d);
        return;
    }
    pthread_mutex_lock(&mutex);
    client_count++;
    log_info("Clients count: %d", client_count);
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
    int role, role_response;
    int socket_d, connection_d;
    struct sockaddr_in local_address, remote_address;
    socklen_t remote_address_length;

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

    int credentials_file = open("data/credentials", O_RDONLY);
    if (credentials_file < 0)
    {
        log_fatal("Cannot open credentials file!");
        perror("Cannot open credentials file!");
        exit(2);
    }
    struct stat file_stat;
    if (fstat(credentials_file, &file_stat) < 0)
    {
        log_fatal("Error getting credentials file information!");
        perror("stat: Error getting credentials file information!");
        exit(3);
    }
    credentials = ba_from_descriptor(credentials_file, file_stat.st_size);
    if (credentials == NULL)
    {
        log_fatal("Error allocating memory for credentials!");
        perror("Error allocating memory for credentials!");
        exit(4);
    }

    socket_d = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_d == -1)
    {
        log_fatal("Unable to create socket!");
        perror("Unable to create socket!");
        exit(5);
    }
    log_debug("Socket created!");

    set_addr(&local_address, NULL, INADDR_ANY, SERVERPORT);
    if (bind(socket_d, (struct sockaddr *)&local_address, sizeof(local_address)) == -1)
    {
        log_fatal("Failed to bind local address - INADDR_ANY - to the socket!");
        perror("Failed to bind local address!");
        exit(6);
    }
    log_debug("Local address - INADDR_ANY - bound to socket!");

    if (listen(socket_d, 10) == -1)
    {
        log_fatal("Failed to make the server to listen for connections!");
        perror("Failed to make the server to listen for connections!");
        exit(7);
    }
    log_debug("Server listening for connections!");

    remote_address_length = sizeof(remote_address);
    while (1)
    {
        connection_d = accept(socket_d, (struct sockaddr *)&remote_address, &remote_address_length);
        if (connection_d < 0)
        {
            log_error("Failed to accept connection!");
            continue;
        }
        log_info("Connection accepted!");
        log_debug("Waiting for the remote to send role!");
        role_response = REQUEST_ACCEPTED;
        if (netio_read_int(connection_d, &role) < 0)
        {
            log_error("Failed to read role!");
            close(connection_d);
            continue;
        }
        switch (role)
        {
        case ROLE_STORE:
            role_response = manage_store_role(connection_d);
            break;
        case ROLE_CLIENT:
            manage_client_role(connection_d);
            break;
        default:
            log_error("Role unidentified! Close connection with %d!", connection_d);
            close(connection_d);
            connection_d = -1;
        }
        if (netio_write_int(connection_d, role_response) < 0)
            log_warn("Failed to response to the accepted connection!");
    }
    free(credentials);
    exit(0);
}