//
// Created by artur on 04.11.23.
//

#ifndef VOICE_CHAT_SOUNDIO_MICROPHONE_H
#define VOICE_CHAT_SOUNDIO_MICROPHONE_H

#include <stdint.h>
#include "soundio/soundio.h"

typedef struct soundio_args
{
    struct SoundIo* soundio;
    struct SoundIoDevice* out_device;
    struct SoundIoDevice* in_device;
    struct SoundIoInStream* instream;
    struct SoundIoOutStream* outstream;
    struct SoundIoRingBuffer* ring_buffer;
    const struct SoundIoChannelLayout* channel_layout;
    int32_t sample_rate;
    double microphone_latency;
    enum SoundIoFormat format;
} soundio_args_t;

typedef struct {
    uint64_t socket;
    soundio_args_t* soundio_args;
} thread_args_t;


int initialize_soundio(soundio_args_t* soundio_args);

void read_callback(struct SoundIoInStream* instream, int32_t frame_count_min, int32_t frame_count_max);

void write_callback(struct SoundIoOutStream* outstream, int32_t frame_count_min, int32_t frame_count_max);

void underflow_callback(struct SoundIoOutStream* outstream);

void cleanup_soundio(soundio_args_t* soundio_args);

void* send_thread(void* args);

#endif //VOICE_CHAT_SOUNDIO_MICROPHONE_H
