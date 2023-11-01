//
// Created by artur on 28.10.23.
//

#ifndef CROSS_SOCKETS_CROSS_SOCKETS_H
#define CROSS_SOCKETS_CROSS_SOCKETS_H

#include <stdint.h>

void cross_socket_initialize();

uint64_t cross_socket_open_tcp();

uint64_t cross_socket_open_udp();

void cross_socket_close(uint64_t descriptor);

int32_t cross_socket_bind(uint64_t descriptor, int32_t in_port);

int32_t cross_socket_listen_tcp(uint64_t descriptor, int32_t connections_queue_length);

int32_t cross_socket_connect_tcp(uint64_t descriptor, uint32_t in_address, int32_t in_port);

uint64_t cross_socket_accept_tcp(uint64_t descriptor, uint32_t* out_address);

int32_t cross_socket_receive_tcp(uint64_t descriptor, char* out_buffer, uint32_t in_buffer_length);

int32_t cross_socket_receive_udp(uint64_t descriptor, char* out_buffer, uint32_t in_buffer_length, uint32_t* out_address, int32_t* out_port);

void cross_socket_send_tcp(uint64_t descriptor, const char* in_buffer, uint32_t in_buffer_length);

void cross_socket_send_udp(uint64_t descriptor, const char* in_buffer, uint32_t in_buffer_length, uint32_t in_address, int32_t in_port);

const char* get_error();

void cross_socket_cleanup();

#endif //CROSS_SOCKETS_CROSS_SOCKETS_H
