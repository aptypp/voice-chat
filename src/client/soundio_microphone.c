//
// Created by artur on 04.11.23.
//

#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "client/soundio_microphone.h"
#include "soundio/soundio.h"
#include "logger/logger.h"
#include "stdint.h"
#include "math_extension.h"
#include "cross_sockets/cross_sockets.h"
#include "cross_sockets/ip_address.h"

int initialize_soundio(soundio_args_t* soundio_args)
{
    int32_t prioritized_sample_rates[] = {
            48000,
            44100,
            96000,
            24000,
            0,
    };

    enum SoundIoFormat prioritized_formats[] = {
            SoundIoFormatFloat32NE,
            SoundIoFormatFloat32FE,
            SoundIoFormatS32NE,
            SoundIoFormatS32FE,
            SoundIoFormatS24NE,
            SoundIoFormatS24FE,
            SoundIoFormatS16NE,
            SoundIoFormatS16FE,
            SoundIoFormatFloat64NE,
            SoundIoFormatFloat64FE,
            SoundIoFormatU32NE,
            SoundIoFormatU32FE,
            SoundIoFormatU24NE,
            SoundIoFormatU24FE,
            SoundIoFormatU16NE,
            SoundIoFormatU16FE,
            SoundIoFormatS8,
            SoundIoFormatU8,
            SoundIoFormatInvalid,
    };

    int32_t errcode = 0;

    soundio_args->microphone_latency = 0.2;

    soundio_args->soundio = soundio_create();

    if (soundio_args->soundio == nullptr)
    {
        LOG_ERROR(stderr, "create soundio failed");
        return -1;
    }

    errcode = soundio_connect(soundio_args->soundio);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "%s", soundio_strerror(errcode));
        return -2;
    }

    soundio_flush_events(soundio_args->soundio);

    int32_t default_out_device_index = soundio_default_output_device_index(soundio_args->soundio);

    if (default_out_device_index < 0)
    {
        LOG_ERROR(stderr, "no output device found");
        return -3;
    }

    int32_t default_in_device_index = soundio_default_input_device_index(soundio_args->soundio);

    if (default_in_device_index < 0)
    {
        LOG_ERROR(stderr, "no input device found");
        return -4;
    }

    soundio_args->out_device = soundio_get_output_device(soundio_args->soundio, default_out_device_index);

    if (soundio_args->out_device == nullptr)
    {
        LOG_ERROR(stderr, "could not get output device: out of memory");
        return -5;
    }

    soundio_args->in_device = soundio_get_input_device(soundio_args->soundio, default_in_device_index);

    if (soundio_args->in_device == nullptr)
    {
        LOG_ERROR(stderr, "could not get input device: out of memory");
        return -6;
    }

    soundio_args->channel_layout = soundio_best_matching_channel_layout(
            soundio_args->out_device->layouts, soundio_args->out_device->layout_count,
            soundio_args->in_device->layouts, soundio_args->in_device->layout_count);

    if (soundio_args->channel_layout == nullptr)
    {
        LOG_ERROR(stderr, "channel layouts not compatible");
        return -7;
    }

    int32_t* result_sample_rate;
    for (result_sample_rate = prioritized_sample_rates; *result_sample_rate; result_sample_rate += 1)
    {
        if (soundio_device_supports_sample_rate(soundio_args->in_device, *result_sample_rate) &&
            soundio_device_supports_sample_rate(soundio_args->out_device, *result_sample_rate))
        {
            break;
        }
    }

    if (*result_sample_rate == 0)
    {
        LOG_ERROR(stderr, "incompatible sample rates");
        return -8;
    }

    soundio_args->sample_rate = *result_sample_rate;

    enum SoundIoFormat* result_format;
    for (result_format = prioritized_formats; *result_format != SoundIoFormatInvalid; result_format += 1)
    {
        if (soundio_device_supports_format(soundio_args->in_device, *result_format) &&
            soundio_device_supports_format(soundio_args->out_device, *result_format))
        {
            break;
        }
    }

    if (*result_format == SoundIoFormatInvalid)
    {
        LOG_ERROR(stderr, "incompatible sample formats");
        return -9;
    }

    soundio_args->format = *result_format;

    soundio_args->instream = soundio_instream_create(soundio_args->in_device);

    if (soundio_args->instream == nullptr)
    {
        LOG_ERROR(stderr, "instream create error: out of memory");
        return -10;
    }

    soundio_args->instream->format = soundio_args->format;
    soundio_args->instream->sample_rate = soundio_args->sample_rate;
    soundio_args->instream->layout = *soundio_args->channel_layout;
    soundio_args->instream->software_latency = soundio_args->microphone_latency;
    soundio_args->instream->read_callback = read_callback;

    errcode = soundio_instream_open(soundio_args->instream);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "unable to open input stream: %s", soundio_strerror(errcode));
        return -11;
    }

    soundio_args->outstream = soundio_outstream_create(soundio_args->out_device);

    if (soundio_args->outstream == nullptr)
    {
        LOG_ERROR(stderr, "outstream create error: out of memory");
        return -12;
    }

    soundio_args->outstream->format = soundio_args->format;
    soundio_args->outstream->sample_rate = soundio_args->sample_rate;
    soundio_args->outstream->layout = *soundio_args->channel_layout;
    soundio_args->outstream->software_latency = soundio_args->microphone_latency;
    soundio_args->outstream->write_callback = write_callback;
    soundio_args->outstream->underflow_callback = underflow_callback;

    errcode = soundio_outstream_open(soundio_args->outstream);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "unable to open output stream: %s", soundio_strerror(errcode));
        return -13;
    }

    int32_t capacity = (int32_t) (soundio_args->microphone_latency * 2 * soundio_args->instream->sample_rate * soundio_args->instream->bytes_per_frame);

    soundio_args->in_buffer = soundio_ring_buffer_create(soundio_args->soundio, capacity);
    soundio_args->out_buffer = soundio_ring_buffer_create(soundio_args->soundio, capacity);

    soundio_args->instream->userdata = soundio_args->in_buffer;
    soundio_args->outstream->userdata = soundio_args->out_buffer;

    if (soundio_args->in_buffer == nullptr)
    {
        LOG_ERROR(stderr, "unable to create ring buffer: out of memory");
        return -14;
    }

    if (soundio_args->out_buffer == nullptr)
    {
        LOG_ERROR(stderr, "unable to create ring buffer: out of memory");
        return -14;
    }

    char* in_buffer_pointer = soundio_ring_buffer_write_ptr(soundio_args->in_buffer);

    int32_t fill_count = (int32_t) (soundio_args->microphone_latency * soundio_args->outstream->sample_rate * soundio_args->outstream->bytes_per_frame);

    memset(in_buffer_pointer, 0, fill_count);

    soundio_ring_buffer_advance_write_ptr(soundio_args->in_buffer, fill_count);

    errcode = soundio_instream_start(soundio_args->instream);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "unable to start input device: %s", soundio_strerror(errcode));
        return -15;
    }

    errcode = soundio_outstream_start(soundio_args->outstream);

    if (errcode != 0)
    {
        LOG_ERROR(stderr, "unable to start output device: %s", soundio_strerror(errcode));
        return -16;
    }

    return 0;
}

void read_callback(struct SoundIoInStream* instream, int32_t frame_count_min, int32_t frame_count_max)
{
    struct SoundIoRingBuffer* ring_buffer = (struct SoundIoRingBuffer*) instream->userdata;

    struct SoundIoChannelArea* areas;
    int32_t errcode;
    char* write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
    int32_t free_bytes = soundio_ring_buffer_free_count(ring_buffer);
    int32_t free_count = free_bytes / instream->bytes_per_frame;

    if (frame_count_min > free_count)
    {
        LOG_ERROR(stderr, "CRITICAL: ring buffer overflow");
        return;
    }

    int write_frames = get_min_int(free_count, frame_count_max);
    int frames_left = write_frames;

    while (true)
    {
        int frame_count = frames_left;

        errcode = soundio_instream_begin_read(instream, &areas, &frame_count);

        if (errcode != 0)
        {
            LOG_ERROR(stderr, "CRITICAL: begin read error: %s", soundio_strerror(errcode));
            return;
        }

        if (frame_count == 0)
        {
            break;
        }

        if (areas == nullptr)
        {
            memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
            LOG_ERROR(stderr, "Dropped %d frames due to internal overflow\n", frame_count);
        } else
        {
            for (int32_t frame_index = 0; frame_index < frame_count; frame_index += 1)
            {
                for (int32_t channel_index = 0; channel_index < instream->layout.channel_count; channel_index += 1)
                {
                    memcpy(write_ptr, areas[channel_index].ptr, instream->bytes_per_sample);
                    areas[channel_index].ptr += areas[channel_index].step;
                    write_ptr += instream->bytes_per_sample;
                }
            }
        }

        errcode = soundio_instream_end_read(instream);

        if (errcode != 0)
        {
            LOG_ERROR(stderr, "CRITICAL: end read error: %s", soundio_strerror(errcode));
            return;
        }

        frames_left -= frame_count;

        if (frames_left <= 0)
        {
            break;
        }
    }

    int32_t advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);
}


void write_callback(struct SoundIoOutStream* outstream, int32_t frame_count_min, int32_t frame_count_max)
{
    struct SoundIoRingBuffer* ring_buffer = (struct SoundIoRingBuffer*) outstream->userdata;

    struct SoundIoChannelArea* areas;
    int32_t frames_left;
    int32_t frame_count;
    int32_t errcode;

    char* read_ptr = soundio_ring_buffer_read_ptr(ring_buffer);
    int32_t fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
    int32_t fill_count = fill_bytes / outstream->bytes_per_frame;

    if (frame_count_min > fill_count)
    {
        frames_left = frame_count_min;
        while (true)
        {
            frame_count = frames_left;
            if (frame_count <= 0)
            {
                return;
            }

            errcode = soundio_outstream_begin_write(outstream, &areas, &frame_count);

            if (errcode != 0)
            {
                LOG_ERROR(stderr, "CRITICAL: begin write error: %s", soundio_strerror(errcode));
                return;
            }

            if (frame_count <= 0)
            {
                return;
            }

            for (int frame_index = 0; frame_index < frame_count; frame_index++)
            {
                for (int channel_index = 0; channel_index < outstream->layout.channel_count; channel_index++)
                {
                    memset(areas[channel_index].ptr, 0, outstream->bytes_per_sample);
                    areas[channel_index].ptr += areas[channel_index].step;
                }
            }

            errcode = soundio_outstream_end_write(outstream);

            if (errcode != 0)
            {
                LOG_ERROR(stderr, "CRITICAL: end write error: %s", soundio_strerror(errcode));
                return;
            }

            frames_left -= frame_count;
        }
    }

    int32_t read_count = get_min_int(frame_count_max, fill_count);
    frames_left = read_count;

    while (frames_left > 0)
    {
        int32_t frame_count = frames_left;

        errcode = soundio_outstream_begin_write(outstream, &areas, &frame_count);

        if (errcode != 0)
        {
            LOG_ERROR(stderr, "CRITICAL: begin write error: %s", soundio_strerror(errcode));
            return;
        }

        if (frame_count <= 0)
        {
            break;
        }

        for (int frame_index = 0; frame_index < frame_count; frame_index++)
        {
            for (int channel_index = 0; channel_index < outstream->layout.channel_count; channel_index++)
            {
                memcpy(areas[channel_index].ptr, read_ptr, outstream->bytes_per_sample);
                areas[channel_index].ptr += areas[channel_index].step;
                read_ptr += outstream->bytes_per_sample;
            }
        }

        errcode = soundio_outstream_end_write(outstream);

        if (errcode != 0)
        {
            LOG_ERROR(stderr, "CRITICAL: end write error: %s", soundio_strerror(errcode));
            return;
        }

        frames_left -= frame_count;
    }

    soundio_ring_buffer_advance_read_ptr(ring_buffer, read_count * outstream->bytes_per_frame);
}

void underflow_callback(struct SoundIoOutStream* outstream)
{
    static int32_t count = 0;
    //LOG_ERROR(stderr, "underflow %d\n", ++count);
}

void cleanup_soundio(soundio_args_t* soundio_args)
{
    soundio_outstream_destroy(soundio_args->outstream);
    soundio_instream_destroy(soundio_args->instream);
    soundio_device_unref(soundio_args->in_device);
    soundio_device_unref(soundio_args->out_device);
    soundio_destroy(soundio_args->soundio);
}

void* send_thread_callback(void* args) {
    thread_args_t thread_args = *(thread_args_t*)args;

    uint32_t in_address = string_address_to_integer("127.0.0.1");
    uint16_t in_port = 12345;

    while (true) {
        soundio_flush_events(thread_args.soundio_args->soundio);

        int fill_bytes = soundio_ring_buffer_fill_count(thread_args.soundio_args->in_buffer);
        char* read_buffer_pointer = soundio_ring_buffer_read_ptr(thread_args.soundio_args->in_buffer);

        fprintf(stderr, "send - fill_bytes: %d\n", fill_bytes);
        fprintf(stderr, "send - ptr: %s\n", read_buffer_pointer);

        cross_socket_send_udp(thread_args.socket, (const char*)&fill_bytes, sizeof(int), in_address, in_port);

        for (int i = 0; i < fill_bytes; i += fill_bytes / 4) {
            cross_socket_send_udp(thread_args.socket, read_buffer_pointer + i, fill_bytes / 4, in_address, in_port);

            fprintf(stderr, "ERROR: %s\n", get_error());
        }


        soundio_ring_buffer_advance_read_ptr(thread_args.soundio_args->in_buffer, fill_bytes);
    }

    return nullptr;
}

void* receive_thread_callback(void* args) {
    thread_args_t thread_args = *(thread_args_t*)args;

    uint32_t out_address;
    uint16_t out_port;

    while (true) {
        soundio_flush_events(thread_args.soundio_args->soundio);
        int fill_bytes;

        cross_socket_receive_udp(thread_args.socket, (char*)&fill_bytes, sizeof(int), &out_address, &out_port);

        char* write_buffer_pointer = soundio_ring_buffer_read_ptr(thread_args.soundio_args->out_buffer);

        for (int i = 0; i < fill_bytes; i += fill_bytes / 4) {
            cross_socket_receive_udp(thread_args.socket, write_buffer_pointer + i, fill_bytes / 4, &out_address, &out_port);

            fprintf(stderr, "i: %d\n", i);
        }

        fprintf(stderr, "receive - out_address: %s\n", integer_address_to_string(out_address));
        fprintf(stderr, "receive - fill_bytes: %d\n", fill_bytes);
        fprintf(stderr, "receive - data: %s\n", write_buffer_pointer);

        soundio_ring_buffer_advance_write_ptr(thread_args.soundio_args->out_buffer, fill_bytes);
    }

    return nullptr;
}
