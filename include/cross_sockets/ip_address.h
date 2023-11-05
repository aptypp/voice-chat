//
// Created by artur on 28.10.23.
//

#ifndef CROSS_SOCKETS_IP_ADDRESS_H
#define CROSS_SOCKETS_IP_ADDRESS_H

#include <string_extension.h>
#include <stdint.h>

uint32_t string_address_to_integer(const char* address);

char* integer_address_to_string(uint32_t address);

#endif //CROSS_SOCKETS_IP_ADDRESS_H
