#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "cross_sockets/cross_sockets.h"
#include "cross_sockets/ip_address.h"

typedef struct {
    ma_device_config deviceConfig;
    ma_device device;
    size_t ring_buffer_size_in_bytes;
    ma_rb input_ring_buffer;
    ma_rb output_ring_buffer;
} sound_t;

void sound_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
    sound_t* sound = (sound_t*)device->pUserData;

    size_t write_size_in_bytes = frameCount * ma_get_bytes_per_frame(device->capture.format, device->capture.channels);
    char* write_buffer_pointer;
    ma_rb_acquire_write(&sound->input_ring_buffer, &write_size_in_bytes, (void*)&write_buffer_pointer);

    size_t read_size_in_bytes = ma_rb_available_read(&sound->output_ring_buffer);
    char* read_buffer_pointer;
    ma_rb_acquire_read(&sound->output_ring_buffer, &read_size_in_bytes, (void*)&read_buffer_pointer);

    memcpy(write_buffer_pointer, input, write_size_in_bytes);
    memcpy(output, read_buffer_pointer, read_size_in_bytes);


    ma_rb_commit_write(&sound->input_ring_buffer, write_size_in_bytes);
    ma_rb_commit_read(&sound->output_ring_buffer, read_size_in_bytes);

    fprintf(stderr, "framesCount: %d\n", frameCount);
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
    sound->deviceConfig.dataCallback = sound_callback;
    sound->deviceConfig.pUserData = sound;
    sound->deviceConfig.periodSizeInFrames = 448;
    sound->deviceConfig.sampleRate = 44100;

    ma_device_init(NULL, &sound->deviceConfig, &sound->device);

    sound->ring_buffer_size_in_bytes = sound->deviceConfig.periodSizeInFrames * ma_get_bytes_per_frame(sound->device.capture.format, sound->device.capture.channels);

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

    ma_device_start(&sound->device);
}

void sound_cleanup(sound_t* sound) {
    ma_device_uninit(&sound->device);
}

uint64_t descriptor;

#define PACKET_SIZE_IN_BYTES 448 * 4

void* receive_callback(void* args) {
    sound_t* sound = (sound_t*)args;

    uint32_t out_address;
    uint16_t out_port;

    while (true) {
        size_t write_buffer_size = PACKET_SIZE_IN_BYTES;

        char* write_buffer_pointer;
        ma_rb_acquire_write(&sound->output_ring_buffer, &write_buffer_size, (void*)&write_buffer_pointer);

        cross_socket_receive_udp(descriptor, write_buffer_pointer, PACKET_SIZE_IN_BYTES, &out_address, &out_port);
        ma_rb_commit_write(&sound->output_ring_buffer, PACKET_SIZE_IN_BYTES);
    }

    return nullptr;
}

void* send_callback(void* args) {
    sound_t* sound = (sound_t*)args;

    uint32_t in_address = string_address_to_integer("127.0.0.1");
    uint16_t in_port = 12345;

    while (true) {
        ma_uint32 available_bytes = ma_rb_available_read(&sound->input_ring_buffer);

        if (available_bytes < PACKET_SIZE_IN_BYTES) {
            continue;
        }

        char* read_buffer_pointer;
        size_t read_buffer_size = PACKET_SIZE_IN_BYTES;
        ma_rb_acquire_read(&sound->input_ring_buffer, &read_buffer_size, (void*)&read_buffer_pointer);

        cross_socket_send_udp(descriptor, read_buffer_pointer, PACKET_SIZE_IN_BYTES, in_address, in_port);
        ma_rb_commit_read(&sound->input_ring_buffer, PACKET_SIZE_IN_BYTES);
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