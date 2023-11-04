//
// Created by artur on 28.10.23.
//
#include <stdio.h>
#include "string_extension.h"
#include "cross_sockets/cross_sockets.h"
#include "cross_sockets/ip_address.h"
#include "soundio/soundio.h"
#include "logger/logger.h"
#include "client/soundio_microphone.h"


int main()
{
    int32_t errcode;

    soundio_args_t soundio_args;

    errcode = initialize_soundio(&soundio_args);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "soundio initialization failed, error code: %d", errcode);
        return -1;
    }

    while(true)
    {

    }

    cross_socket_initialize();

    uint64_t communication_socket = cross_socket_open_tcp();

    uint32_t server_ip_address = string_address_to_integer("127.0.0.1");

    cross_socket_connect_tcp(communication_socket, server_ip_address, 12345);

    String hello_message = string_new("Hello, my name is Client!");
    cross_socket_send_tcp(communication_socket, hello_message.buffer, hello_message.length);

    String response = string_new_capacity(50);

    cross_socket_receive_tcp(communication_socket, response.buffer, response.capacity);

    printf("%s%s", "Message from server: ", response.buffer);

    string_free(&response);

    cross_socket_cleanup();
    cleanup_soundio(&soundio_args);
}

