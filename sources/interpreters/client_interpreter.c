#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../libraries/log.h"
#include "../libraries/netio.h"
#include "client_interpreter.h"

int parse_command(char *arg)
{
    if (strcmp(arg, "exit") == 0)
        return CMD_CS_EXIT;

    if (strcmp(arg, "create") == 0)
        return CMD_CS_CREATE_BUCKET;

    if (strcmp(arg, "upload") == 0)
        return CMD_CS_UPLOAD_FILE;

    if (strcmp(arg, "list") == 0)
        return CMD_CS_LIST_BUCKET;

    return CMD_CS_UNDEFINED;
}

int bad_args_count()
{
    log_warn("Wrong number of arguments!");
    printf("Wrong number of arguments!\n");
    return RSP_CS_NOT_EXECUTED;
}

int unknown_command()
{
    log_warn("The command is not defined!");
    printf("The command is not defined!\n");
    return RSP_CS_NOT_EXECUTED;
}

int not_issued()
{
    log_error("Error writing command to the server!");
    printf("The command was not sent to the server due to some system errors!\n");
    return RSP_CS_NOT_EXECUTED;
}

int sending_args_failure()
{

    log_fatal("Error writing byte array to server!");
    printf("Error executing command!\n");
    return RSP_CS_SYSTEM_ERROR;
}

int local_file_error()
{
    log_error("Could not open file from local system!");
    printf("Could not open file from local system!\n");
    return RSP_CS_NOT_EXECUTED;
}

int sending_file_error()
{
    log_fatal("Could not send file to server!");
    printf("Could not send file to server!\n");
    return RSP_CS_SYSTEM_ERROR;
}

int parse_args(ByteArray *cmd_args[MAX_ARGS_COUNT], char *args_ptr)
{
    int args_count = 0;
    args_ptr = strtok(NULL, " ");
    while (args_ptr)
    {
        cmd_args[args_count++] = ba_new(args_ptr, strlen(args_ptr));
        args_ptr = strtok(NULL, " ");
    }
    return args_count;
}

int get_command_desired_args_count(int command)
{
    switch (command)
    {
    case CMD_CS_EXIT:
        return 0;
    case CMD_CS_CREATE_BUCKET:
        return 1;
        break;
    case CMD_CS_UPLOAD_FILE:
        return 3;
        break;
    case CMD_CS_LIST_BUCKET:
        return 1;
        break;
    default:
        return 0;
    }
}

int interpret_command(int command, int socket_d, char *args_ptr)
{

    ByteArray *cmd_args[MAX_ARGS_COUNT];

    int args_count, desired_args_count;

    int reattempts = 0, response;

    struct stat file_stat;

    if (command == CMD_CS_UNDEFINED)
        return unknown_command();

    desired_args_count = get_command_desired_args_count(command);

    args_count = parse_args(cmd_args, args_ptr);

    if (args_count != desired_args_count)
        return bad_args_count();

    int file;
    char *aux;
    switch (command)
    {
    case CMD_CS_EXIT:
        if (netio_write_int(socket_d, command) < 0)
            return not_issued();
        if (netio_write_int(socket_d, desired_args_count) < 0)
            return sending_args_failure();
        break;
    case CMD_CS_CREATE_BUCKET:
        if (netio_write_int(socket_d, command) < 0)
            return not_issued();
        if (netio_write_int(socket_d, desired_args_count) < 0)
            return sending_args_failure();
        for (int i = 0; i < desired_args_count; i++)
        {
            if (netio_write_ba(socket_d, cmd_args[i]) < 0)
                return sending_args_failure();
            ba_destroy(cmd_args[i]);
        }
        break;
    case CMD_CS_UPLOAD_FILE:
        log_debug("Started uploading file!");
        aux = ba_data(cmd_args[0]);
        file = open(aux, O_RDONLY);
        free(aux);
        if (file < 0)
            return local_file_error();

        if (netio_write_int(socket_d, command) < 0)
            return not_issued();

        if (netio_write_int(socket_d, desired_args_count - 1) < 0)
            return sending_args_failure();

        for (int i = 1; i < desired_args_count; i++)
        {
            if (netio_write_ba(socket_d, cmd_args[i]) < 0)
                return sending_args_failure();
            ba_destroy(cmd_args[i]);
        }

        if (fstat(file, &file_stat) < 0)
            return sending_file_error();

        if (netio_write_int(socket_d, file_stat.st_size) < 0)
            return sending_file_error();

        if (netio_read_int(socket_d, &response) < 0)
            return sending_file_error();

        if (response == REQUEST_START)
        {
            if (netio_send_file(socket_d, file) < 0)
                return sending_file_error();

            close(file);
        }
        else
        {
            close(file);
            if (response != REQUEST_STOP)
                return RSP_CS_SYSTEM_ERROR;
                }

        break;
    case CMD_CS_LIST_BUCKET:
        if (netio_write_int(socket_d, command) < 0)
            return not_issued();

        if (netio_write_int(socket_d, desired_args_count) < 0)
            return sending_args_failure();

        for (int i = 0; i < desired_args_count; i++)
        {
            if (netio_write_ba(socket_d, cmd_args[i]) < 0)
                return sending_args_failure();
            ba_destroy(cmd_args[i]);
        }
        break;
    }

    while (netio_read_int(socket_d, &response) < 0)
    {
        sleep(1);
        reattempts++;
        if (reattempts == 5)
            return RSP_CS_SYSTEM_ERROR;
    }

    return response;
}

void interpret_reponse(int server_response)
{
    switch (server_response)
    {
    case RSP_CS_CLOSE_CONN:
        printf("Exiting...\n");
        log_debug("Exit!");
        exit(0);
    case RSP_CS_SYSTEM_ERROR:
        printf("There was a system error!\n");
        exit(RSP_CS_SYSTEM_ERROR);
        break;
    case RSP_CS_BUCKET_NOT_EXISTS:
        printf("Bucket does not exist!\n");
        break;
    case RSP_CS_BUCKET_ALREADY_EXISTS:
        printf("Bucket already exists!\n");
        break;
    case RSP_CS_FILE_ALREADY_EXISTS:
        printf("File already exists!\n");
        break;
    }
}
