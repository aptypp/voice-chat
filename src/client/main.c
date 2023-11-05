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
#include <pthread.h>


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

    uint64_t socket = cross_socket_open_udp();

    cross_socket_bind(socket, 12345);

    pthread_t send;
    thread_args_t send_args;
    send_args.socket = socket;
    send_args.soundio_args = &soundio_args;
    pthread_create(&send, nullptr, send_thread_callback, (void*) &send_args);

    pthread_t receive;
    thread_args_t receive_args;
    receive_args.socket = socket;
    receive_args.soundio_args = &soundio_args;
    pthread_create(&receive, nullptr, receive_thread_callback, (void*) &receive_args);


    while(true)
    {

    }

    cross_socket_initialize();


    cross_socket_cleanup();
    cleanup_soundio(&soundio_args);
}

