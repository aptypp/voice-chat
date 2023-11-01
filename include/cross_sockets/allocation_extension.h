//
// Created by artur on 28.10.23.
//

#ifndef CROSS_SOCKETS_ALLOCATION_EXTENSION_H
#define CROSS_SOCKETS_ALLOCATION_EXTENSION_H

#include "malloc.h"

#define allocate(type, count) ((type *)malloc(sizeof(type) * count))

#define reallocate(type, pointer, count) (type*)realloc(pointer, count)

#endif //CROSS_SOCKETS_ALLOCATION_EXTENSION_H
