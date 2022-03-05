#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "byte_array.h"

struct ByteArray
{
    int size;
    char *data;
};

ByteArray *allocate_byte_array()
{
    ByteArray *byte_array = (ByteArray *)calloc(1, sizeof(byte_array));
    if (byte_array == NULL)
    {
        perror("Error allocating ByteArray!");
        return NULL;
    }
    return byte_array;
}

ByteArray *ba_new(char *data, size_t size)
{
    ByteArray *byte_array = allocate_byte_array();
    byte_array->data = (char *)calloc(size + 1, sizeof(char));
    if (byte_array->data == NULL)
    {
        perror("Error allocating ByteArray!");
        free(byte_array);
        return NULL;
    }
    byte_array->size = size;
    memcpy(byte_array->data, data, size);
    return byte_array;
}

ByteArray *ba_from_descriptor(int descriptor, int size)
{
    int read_step = 0;
    ByteArray *byte_array = allocate_byte_array();
    byte_array->data = (char *)calloc(size + 1, sizeof(char));
    if (byte_array->data == NULL)
    {
        perror("Error allocating ByteArray!");
        free(byte_array);
        return NULL;
    }
    byte_array->size = 0;
    while (size > 0)
    {
        if ((read_step = read(descriptor, byte_array->data + byte_array->size, size)) < 0)
        {
            perror("Error reading from socket!");
            ba_destroy(byte_array);
        }
        if (read_step == 0)
            break;

        byte_array->size += read_step;
        size -= read_step;
    }
    if (read_step < 0)
    {
        ba_destroy(byte_array);
        perror("Error reading data from file!");
        return NULL;
    }
    return byte_array;
}

int ba_write(int descriptor, ByteArray *byte_array)
{
    int write_amount = byte_array->size, write_step, already_written = 0;

    while (write_amount > 0)
    {
        if ((write_step = write(descriptor, byte_array->data + already_written, write_amount)) < 0)
        {
            perror("Failed to write data to file!");
            return -2;
        }
        write_amount -= write_step;
        already_written += write_step;
    }
    return already_written;
}

int ba_compare(ByteArray *byte_array1, ByteArray *byte_array2)
{
    return strcmp(byte_array1->data, byte_array2->data);
}

int ba_size(ByteArray *byte_array)
{
    return byte_array->size;
}

char *ba_data(ByteArray *byte_array)
{
    char *data = (char *)calloc(byte_array->size + 1, sizeof(char));
    if (byte_array->data == NULL)
    {
        perror("Error allocating ByteArray!");
        free(byte_array);
        return NULL;
    }
    memcpy(data, byte_array->data, byte_array->size);
    return data;
}

void ba_destroy(ByteArray *byte_array)
{
    if (byte_array->data != NULL)
        free(byte_array->data);
    if (byte_array != NULL)
        free(byte_array);
}

void ba_print(ByteArray *byte_array)
{
    printf("%s", byte_array->data);
}