#ifndef BYTE_ARRAY
#define BYTE_ARRAY

#include <unistd.h>

/* This data type is used to represent a string paired with it's length, being consumed by
terminal operations */
typedef struct ByteArray ByteArray;

ByteArray *ba_new(char *data, size_t size);
ByteArray *ba_from_descriptor(int descriptor, int size);

int ba_size(ByteArray *byte_array);

/* Note: This dinamically allocates memory to preserve immutability! */
char *ba_data(ByteArray *byte_array);

int ba_write(int descriptor, ByteArray *byte_array);

int ba_compare(ByteArray *byte_array1, ByteArray *byte_array2);
void ba_destroy(ByteArray *byte_array);

void ba_print(ByteArray *byte_array);

#endif