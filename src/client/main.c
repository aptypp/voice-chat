#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <pthread.h>
#include "cross_sockets/cross_sockets.h"
#include "cross_sockets/ip_address.h"

typedef struct {
    ma_device_config deviceConfig;
    ma_device device;
    ma_device_data_proc callback;
    size_t ring_buffer_size_in_bytes;
    ma_rb input_ring_buffer;
    ma_rb output_ring_buffer;
} sound_t;

void sound_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
    sound_t sound = *(sound_t*)device->pUserData;

    char* write_buffer_pointer;
    char* read_buffer_pointer;

    ma_rb_acquire_write(&sound.input_ring_buffer, &sound.ring_buffer_size_in_bytes, (void*)&write_buffer_pointer);
    ma_rb_acquire_read(&sound.output_ring_buffer, &sound.ring_buffer_size_in_bytes, (void*)&read_buffer_pointer);

    memcpy(write_buffer_pointer, input, frameCount * ma_get_bytes_per_frame(device->capture.format, device->capture.channels));

    ma_uint32 available_bytes = ma_rb_available_read(&sound.output_ring_buffer);
    memcpy(output, read_buffer_pointer, available_bytes);

    ma_rb_commit_write(&sound.input_ring_buffer, sound.ring_buffer_size_in_bytes);
    ma_rb_commit_read(&sound.output_ring_buffer, available_bytes);
}

void sound_initialize(sound_t* sound) {
    sound->deviceConfig = ma_device_config_init(ma_device_type_duplex);
    sound->deviceConfig.capture.pDeviceID = NULL;
    sound->deviceConfig.capture.format = ma_format_s16;
    sound->deviceConfig.capture.channels = 2;
    sound->deviceConfig.capture.shareMode = ma_share_mode_shared;
    sound->deviceConfig.playback.pDeviceID = NULL;
    sound->deviceConfig.playback.format = ma_format_s16;
    sound->deviceConfig.playback.channels = 2;
    sound->deviceConfig.dataCallback = sound->callback;
    sound->deviceConfig.pUserData = sound;

    sound->ring_buffer_size_in_bytes = 4096 * ma_get_bytes_per_frame(sound->device.capture.format, sound->device.capture.channels);

    ma_rb_init(
            sound->ring_buffer_size_in_bytes,
            nullptr,
            nullptr,
            &sound->input_ring_buffer);

    ma_rb_init(
            sound->ring_buffer_size_in_bytes,
            nullptr,
            nullptr,
            &sound->output_ring_buffer);

    ma_device_init(NULL, &sound->deviceConfig, &sound->device);
    ma_device_start(&sound->device);
}

void sound_cleanup(sound_t* sound) {
    ma_device_uninit(&sound->device);
}

uint64_t descriptor;

#define PACKET_SIZE_IN_BYTES 1024

void* receive_callback(void* args) {
    sound_t sound = *(sound_t*)args;

    uint32_t out_address;
    uint16_t out_port;

    while (true) {
        char* write_buffer_pointer;
        ma_rb_acquire_write(&sound.output_ring_buffer, &sound.ring_buffer_size_in_bytes, (void*)&write_buffer_pointer);

        cross_socket_receive_udp(descriptor, write_buffer_pointer, PACKET_SIZE_IN_BYTES, &out_address, &out_port);

        fprintf(stderr, "error: %s\n", get_error());
        fprintf(stderr, "receive data: %s\n", write_buffer_pointer);

        ma_rb_commit_write(&sound.output_ring_buffer, PACKET_SIZE_IN_BYTES);
    }

    return nullptr;
}

void* send_callback(void* args) {
    sound_t sound = *(sound_t*)args;

    uint32_t in_address = string_address_to_integer("127.0.0.1");
    uint16_t in_port = 12345;

    while (true) {
        ma_uint32 available_bytes = ma_rb_available_read(&sound.input_ring_buffer);

        if (available_bytes < PACKET_SIZE_IN_BYTES) {
            continue;
        }

        char* read_buffer_pointer;
        ma_rb_acquire_read(&sound.input_ring_buffer, &sound.ring_buffer_size_in_bytes, (void*)&read_buffer_pointer);

        cross_socket_send_udp(descriptor, read_buffer_pointer, PACKET_SIZE_IN_BYTES, in_address, in_port);

        fprintf(stderr, "error: %s\n", get_error());
        fprintf(stderr, "send data: %s\n", read_buffer_pointer);

        ma_rb_commit_read(&sound.input_ring_buffer, PACKET_SIZE_IN_BYTES);
    }

    return nullptr;
}


int main() {
    sound_t sound;
    sound_initialize(&sound);

    cross_socket_initialize();
    descriptor = cross_socket_open_udp();
    cross_socket_bind(descriptor, 12345);

    pthread_t receive_thread;
    pthread_create(&receive_thread, nullptr, receive_callback, (void*)&sound);

    pthread_t send_thread;
    pthread_create(&send_thread, nullptr, send_callback, (void*)&sound);

    getchar();

    cross_socket_cleanup();
    sound_cleanup(&sound);
    return 0;
}