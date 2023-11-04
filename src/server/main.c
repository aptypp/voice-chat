//
// Created by artur on 28.10.23.
//
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "cross_sockets/cross_sockets.h"
#include "string_extension.h"
#include "server/properties.h"
#include "pthread.h"

typedef struct client_data
{
    uint32_t client_address;
    uint64_t client_tcp_socket;
    uint16_t client_udp_port;
} client_data_t;

typedef struct thread_args
{
    uint64_t current_client_tcp_socket;
    client_data_t* clients_data;
} thread_args_t;


void* listen_client(void* args)
{
    thread_args_t* thread_args = (thread_args_t*) args;

    uint64_t udp_socket = cross_socket_open_udp();


}

int main()
{
    ServerProperties server_properties;

    server_properties.port = 22422;
    server_properties.max_clients_count = 10;
    server_properties.sleep_time_seconds = 2;

    pthread_t threads[server_properties.max_clients_count];
    client_data_t clients_data[server_properties.max_clients_count];
    thread_args_t clients_args[server_properties.max_clients_count];

    uint32_t connected_clients_count = 0;

    cross_socket_initialize();

    uint64_t server_socket = cross_socket_open_tcp();

    cross_socket_bind(server_socket, server_properties.port);

    cross_socket_listen_tcp(server_socket, 10);

    while (true)
    {
        if (connected_clients_count > server_properties.max_clients_count)
        {
            sleep(server_properties.sleep_time_seconds);
            continue;
        }

        uint32_t client_ip_address;
        uint64_t client_tcp_socket = cross_socket_accept_tcp(server_socket, &client_ip_address);

        clients_data[connected_clients_count].client_address = client_ip_address;
        clients_data[connected_clients_count].client_tcp_socket = client_tcp_socket;

        clients_args[connected_clients_count].clients_data = clients_data;
        clients_args[connected_clients_count].current_client_tcp_socket = client_tcp_socket;

        pthread_create(threads + connected_clients_count, nullptr, listen_client, clients_args + connected_clients_count);

        connected_clients_count++;
    }

    cross_socket_cleanup();
}

