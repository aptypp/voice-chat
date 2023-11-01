//
// Created by artur on 28.10.23.
//
#include <stdio.h>
#include "cross_sockets/cross_sockets.h"
#include "string_extension.h"

int main()
{
    cross_socket_initialize();

    uint64_t server_socket = cross_socket_open_tcp();

    cross_socket_bind(server_socket, 12345);

    cross_socket_listen_tcp(server_socket, 10);

    uint64_t client_socket;

    while (true)
    {
        uint32_t ip_address;
        client_socket = cross_socket_accept_tcp(server_socket, &ip_address);
        printf("%s", "New connection!");

        String hello_message = string_new("You successfully connected :)");
        cross_socket_send_tcp(client_socket, hello_message.buffer, hello_message.length);
    }

    cross_socket_cleanup();
}

