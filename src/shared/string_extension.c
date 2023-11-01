//
// Created by Yurii on 10/28/2023.
//

#include "string_extension.h"
#include <string.h>
#include <stdlib.h>
#include "allocation_extension.h"

#define STRING_CAPACITY_MULTIPLIER 1.5

String string_new(const char* string)
{
    String result;

    result.capacity = strlen(string);
    result.length = result.capacity;
    result.buffer = allocate(char, result.capacity);

    strcpy(result.buffer, string);

    return result;
}

String string_new_capacity(int capacity)
{
    String result;

    result.capacity = capacity;
    result.length = 0;
    result.buffer = allocate(char, capacity);

    return result;
}

String string_concatenate(const String* left, const String* right)
{
    String result;

    result.length = left->length + right->length;
    result.capacity = (uint64_t) ((double) result.length * STRING_CAPACITY_MULTIPLIER);

    result.buffer = allocate(char, result.capacity);

    strcpy(result.buffer, left->buffer);

    strcat(result.buffer, right->buffer);

    return result;
}

String string_copy(const String* string)
{
    String result;

    result.length = string->length;
    result.capacity = string->capacity;
    result.buffer = allocate(char, result.capacity);

    strcpy(result.buffer, string->buffer);

    return result;
}


void string_write(String* string, int start_index, const char* source)
{
    uint64_t source_len = strlen(source);

    uint64_t new_length = string->length + source_len;

    if (new_length > string->capacity)
    {
        string->buffer = reallocate(char, string->buffer, new_length);
        string->length = new_length;
        string->capacity = (int) ((double) new_length * STRING_CAPACITY_MULTIPLIER);
    } else
    {
        string->length = new_length;
    }

    strcpy(string->buffer + start_index, source);
}

void string_free(const String* string)
{
    free(string->buffer);
}

