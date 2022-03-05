
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "../libraries/log.h"
#include "../libraries/netio.h"
#include "server_interpreter.h"

int suggest_store_index(store_machine stores[], int stores_count, int file_size)
{
    int max = 0, size;
    int index = -1;
    char *aux;
    for (int i = 0; i < stores_count; i++)
    {
        aux = ba_data(stores[i].name);
        free(aux);
        if (netio_write_int(stores[i].socket_d, CMD_SS_REQUEST_SIZE) < 0)
            continue;

        if (netio_read_int(stores[i].socket_d, &size) < 0)
            continue;

        if (size < file_size)
            continue;

        if (size > max)
        {
            index = i;
            max = size;
        }
    }
    return index;
}

int create_bucket(char *bucket_name)
{
    char buffer[1024];
    sprintf(buffer, "data/buckets/%s", bucket_name);
    int file = open(buffer, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file < 0)
    {
        if (errno == EEXIST)
            return RSP_CS_BUCKET_ALREADY_EXISTS;
        else
            return RSP_CS_SYSTEM_ERROR;
    }
    close(file);
    return RSP_CS_SUCCES;
}

int exists_bucket(char *bucket_name)
{
    char buffer[1024];
    sprintf(buffer, "data/buckets/%s", bucket_name);
    int file = open(buffer, O_RDONLY);
    if (file < 0)
        return 0;
    close(file);
    return 1;
}

// Imi asum, nu am mai avut nervi de functii de sistem
int exists_file(char *bucket_name, char *file_name)
{
    char buffer[1024];
    sprintf(buffer, "data/buckets/%s", bucket_name);
    FILE *file = fopen(buffer, "r");
    while (!feof(file))
    {
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, 1024, file);
        if (strstr(buffer, file_name) != NULL)
            return 1;
    }
    fclose(file);
    return 0;
}

void add_file_record(char *bucket_name, char *file_name, store_machine store)
{
    char buffer[1024];
    sprintf(buffer, "data/buckets/%s", bucket_name);
    FILE *file = fopen(buffer, "a");
    char *aux = ba_data(store.name);
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s %s", file_name, aux);
    free(aux);
    fputs(buffer, file);
    fputc('\n', file);
    fclose(file);
}

int upload_file(int socket_d, char *bucket_name, char *file_name, store_machine stores[], int stores_count, int file_size, pthread_mutex_t mutex)
{
    char buffer[1024];
    int response;

    if (!exists_bucket(bucket_name))
    {
        if (netio_write_int(socket_d, REQUEST_STOP) < 0)
            return RSP_CS_SYSTEM_ERROR;
        return RSP_CS_BUCKET_NOT_EXISTS;
    }

    if (exists_file(bucket_name, file_name))
    {
        if (netio_write_int(socket_d, REQUEST_STOP) < 0)
            return RSP_CS_SYSTEM_ERROR;
        return RSP_CS_FILE_ALREADY_EXISTS;
    }

    pthread_mutex_lock(&mutex);
    int store_index = suggest_store_index(stores, stores_count, file_size);
    pthread_mutex_unlock(&mutex);

    if (store_index < 0)
        return RSP_CS_SYSTEM_ERROR;

    if (netio_write_int(stores[store_index].socket_d, CMD_SS_UPLOAD_FILE) < 0)
        return RSP_CS_SYSTEM_ERROR;

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s-%s", bucket_name, file_name);
    ByteArray *byte_array = ba_new(buffer, strlen(buffer));

    if (netio_write_ba(stores[store_index].socket_d, byte_array) < 0)
        return RSP_CS_SYSTEM_ERROR;

    ba_destroy(byte_array);

    if (netio_read_int(stores[store_index].socket_d, &response) < 0)
        return RSP_CS_SYSTEM_ERROR;

    if (response == REQUEST_START)
    {
        if (netio_write_int(socket_d, REQUEST_START) < 0)
            return RSP_CS_SYSTEM_ERROR;
        if (netio_relay(socket_d, stores[store_index].socket_d) < 0)
            return RSP_CS_SYSTEM_ERROR;
    }
    else
    {
        if (response != REQUEST_STOP)
            return RSP_CS_SYSTEM_ERROR;
    }
    add_file_record(bucket_name, file_name, stores[store_index]);
    return RSP_CS_SUCCES;
}

int interpret_client_command(int command, store_machine stores[], int stores_count, int socket_d, pthread_mutex_t mutex)
{
    int code, args_count, aux_int;
    ByteArray *cmd_args[MAX_ARGS_COUNT];

    if (netio_read_int(socket_d, &args_count) < 0)
        return RSP_CS_SYSTEM_ERROR;

    for (int i = 0; i < args_count; i++)
        if (netio_read_ba(socket_d, &cmd_args[i]) < 0)
            return RSP_CS_SYSTEM_ERROR;

    char *aux0, *aux1;
    switch (command)
    {
    case CMD_CS_EXIT:
        code = RSP_CS_CLOSE_CONN;
        break;
    case CMD_CS_CREATE_BUCKET:
        aux0 = ba_data(cmd_args[0]);
        code = create_bucket(aux0);
        free(aux0);
        break;
    case CMD_CS_UPLOAD_FILE:
        if (netio_read_int(socket_d, &aux_int) < 0)
            return RSP_CS_SYSTEM_ERROR;
        aux0 = ba_data(cmd_args[0]);
        aux1 = ba_data(cmd_args[1]);
        code = upload_file(socket_d, aux1, aux0, stores, stores_count, aux_int, mutex);
        free(aux0);
        free(aux1);
        break;
    default:
        break;
    }

    for (int i = 0; i < args_count; i++)
        ba_destroy(cmd_args[i]);

    return code;
}
