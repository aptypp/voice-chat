/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include <soundio/soundio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

static int usage(char *exe)
{
    fprintf(stderr, "Usage: %s [options]\n"
                    "Options:\n"
                    "  [--backend dummy|alsa|pulseaudio|jack|coreaudio|wasapi]\n"
                    "  [--device id]\n"
                    "  [--raw]\n"
                    "  [--name stream_name]\n"
                    "  [--latency seconds]\n"
                    "  [--sample-rate hz]\n", exe);
    return 1;
}

static void write_sample_s16ne(char *ptr, double sample)
{
    int16_t *buf = (int16_t *) ptr;
    double range = (double) INT16_MAX - (double) INT16_MIN;
    double val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_s32ne(char *ptr, double sample)
{
    int32_t *buf = (int32_t *) ptr;
    double range = (double) INT32_MAX - (double) INT32_MIN;
    double val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_float32ne(char *ptr, double sample)
{
    float *buf = (float *) ptr;
    *buf = sample;
}

static void write_sample_float64ne(char *ptr, double sample)
{
    double *buf = (double *) ptr;
    *buf = sample;
}

static void (*write_sample)(char *ptr, double sample);

static const double PI = 3.14159265358979323846264338328;
static double seconds_offset = 0.0;
static volatile bool want_pause = false;

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
    double float_sample_rate = outstream->sample_rate;
    double seconds_per_frame = 1.0 / float_sample_rate;
    struct SoundIoChannelArea *areas;
    int err;

    int frames_left = frame_count_max;

    for (;;)
    {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
        {
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;

        const struct SoundIoChannelLayout *layout = &outstream->layout;

        double pitch = 440.0;
        double radians_per_second = pitch * 2.0 * PI;
        for (int frame = 0; frame < frame_count; frame += 1)
        {
            double sample = sin((seconds_offset + frame * seconds_per_frame) * radians_per_second);
            for (int channel = 0; channel < layout->channel_count; channel += 1)
            {
                write_sample(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);

        if ((err = soundio_outstream_end_write(outstream)))
        {
            if (err == SoundIoErrorUnderflow)
                return;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    soundio_outstream_pause(outstream, want_pause);
}

static void underflow_callback(struct SoundIoOutStream *outstream)
{
    static int count = 0;
    fprintf(stderr, "underflow %d\n", count++);
}

int main(int argc, char **argv)
{
    char *exe = argv[0];
    enum SoundIoBackend backend = SoundIoBackendNone;
    char *device_id = NULL;
    bool raw = false;
    char *stream_name = NULL;
    double latency = 0.0;
    int sample_rate = 0;

    struct SoundIo *soundio = soundio_create();

    if (!soundio)
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    int err = soundio_connect(soundio);

    if (err)
    {
        fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
        return 1;
    }

    fprintf(stderr, "Backend: %s\n", soundio_backend_name(soundio->current_backend));

    soundio_flush_events(soundio);

    int selected_device_index = soundio_default_output_device_index(soundio);

    if (selected_device_index < 0)
    {
        fprintf(stderr, "Output device not found\n");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);

    if (!device)
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    if (device->probe_error)
    {
        fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(device->probe_error));
        return 1;
    }

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);

    if (!outstream)
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = stream_name;
    outstream->software_latency = latency;
    outstream->sample_rate = sample_rate;

    if (soundio_device_supports_format(device, SoundIoFormatFloat32NE))
    {
        outstream->format = SoundIoFormatFloat32NE;
        write_sample = write_sample_float32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE))
    {
        outstream->format = SoundIoFormatFloat64NE;
        write_sample = write_sample_float64ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS32NE))
    {
        outstream->format = SoundIoFormatS32NE;
        write_sample = write_sample_s32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS16NE))
    {
        outstream->format = SoundIoFormatS16NE;
        write_sample = write_sample_s16ne;
    } else
    {
        fprintf(stderr, "No suitable device format available.\n");
        return 1;
    }

    if ((err = soundio_outstream_open(outstream)))
    {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    fprintf(stderr, "Software latency: %f\n", outstream->software_latency);

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream)))
    {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return 1;
    }

    while (true)
    {
        soundio_flush_events(soundio);
        int c = getc(stdin);
        if (c == 'q')
        {
            break;
        } else
        {
            fprintf(stderr, "Unrecognized command: %c\n", c);
        }
    }


    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}

