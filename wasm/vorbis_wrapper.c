#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_INTEGER_CONVERSION

#include "stb_vorbis.c"

/*
 * Result layout:
 *
 *   offset  0: int32_t channels
 *   offset  4: int32_t sample_rate
 *   offset  8: int32_t samples_per_channel
 *   offset 12: float pcm[]
 *
 * The returned pointer must be released with vorbis_free()
 */
typedef struct {
    int32_t channels;
    int32_t sample_rate;
    int32_t samples_per_channel;
    float pcm[];
} VorbisResult;

#define VORBIS_HEADER_SIZE ((size_t) offsetof(VorbisResult, pcm))
#define VORBIS_INITIAL_FRAMES ((size_t) 4096)
#define VORBIS_MAX_PREALLOC_BYTES ((size_t) 64 * 1024 * 1024)

/**
 * Computes the size of a result buffer without overflowing size_t.
 * @return 1 on success, 0 if the requested size is too large
 */
static int vorbis_result_size(
    const size_t frames,
    const int channels,
    size_t* const size
) {
    const size_t frame_bytes = (size_t) channels * sizeof(float);

    if (frame_bytes == 0) {
        return 0;
    }

    if (frames > (SIZE_MAX - VORBIS_HEADER_SIZE) / frame_bytes) {
        return 0;
    }

    *size = VORBIS_HEADER_SIZE + frames * frame_bytes;
    return 1;
}

/**
 * stb_vorbis reads the input synchronously for the duration of the call
 * @param data the binary vorbis data
 * @param data_length the length of the binary vorbis data
 *
 * Returns NULL on failure.
 */
VorbisResult* vorbis_decode(
    const uint8_t* data,
    const int32_t data_length
) {
    int error = 0;
    stb_vorbis* vorbis = stb_vorbis_open_memory(data, data_length, &error, NULL);

    if (vorbis == NULL) {
        return NULL;
    }

    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    const int channels = info.channels;
    const int sample_rate = (int) info.sample_rate;

    if (channels <= 0 || sample_rate <= 0) {
        stb_vorbis_close(vorbis);
        return NULL;
    }

    /*
     * See https://github.com/nothings/stb/pull/2023
     */
    if (channels > 2) {
        stb_vorbis_close(vorbis);
        return NULL;
    }

    const unsigned int expected_samples =
        stb_vorbis_stream_length_in_samples(vorbis);

    (void) stb_vorbis_get_error(vorbis);

    size_t capacity_frames = VORBIS_INITIAL_FRAMES;

    if (expected_samples > 0) {
        const size_t frame_bytes = (size_t) channels * sizeof(float);
        const size_t max_prealloc_frames =
            VORBIS_MAX_PREALLOC_BYTES / frame_bytes;
        size_t wanted_frames = (size_t) expected_samples;

        if (wanted_frames > max_prealloc_frames) {
            wanted_frames = max_prealloc_frames;
        }

        if (wanted_frames > capacity_frames) {
            capacity_frames = wanted_frames;
        }
    }

    size_t allocated_size = 0;

    if (!vorbis_result_size(capacity_frames, channels, &allocated_size)) {
        stb_vorbis_close(vorbis);
        return NULL;
    }

    VorbisResult* result = malloc(allocated_size);

    if (result == NULL) {
        stb_vorbis_close(vorbis);
        return NULL;
    }

    size_t sample_count = 0;

    for (;;) {
        const size_t available_floats =
            (capacity_frames - sample_count) * (size_t) channels;

        const int samples = stb_vorbis_get_samples_float_interleaved(
            vorbis,
            channels,
            result->pcm + sample_count * (size_t) channels,
            (int) available_floats
        );

        if (samples <= 0) {
            break;
        }

        sample_count += (size_t) samples;

        if (sample_count >= capacity_frames) {
            const size_t new_capacity = capacity_frames * 2;
            size_t new_size = 0;

            if (
                new_capacity <= capacity_frames ||
                !vorbis_result_size(new_capacity, channels, &new_size)
            ) {
                free(result);
                stb_vorbis_close(vorbis);
                return NULL;
            }

            VorbisResult* resized = realloc(result, new_size);

            if (resized == NULL) {
                free(result);
                stb_vorbis_close(vorbis);
                return NULL;
            }

            result = resized;
            capacity_frames = new_capacity;
            allocated_size = new_size;
        }
    }

    const int decode_error = stb_vorbis_get_error(vorbis);
    stb_vorbis_close(vorbis);

    if (decode_error != VORBIS__no_error) {
        free(result);
        return NULL;
    }

    if (sample_count == 0) {
        free(result);
        return NULL;
    }

    if (
        expected_samples != 0 &&
        sample_count < (size_t) expected_samples
    ) {
        free(result);
        return NULL;
    }

    size_t output_samples = sample_count;

    if (
        expected_samples != 0 &&
        (size_t) expected_samples < output_samples
    ) {
        output_samples = (size_t) expected_samples;
    }

    if (output_samples > (size_t) INT32_MAX) {
        free(result);
        return NULL;
    }

    size_t exact_size = 0;

    if (
        vorbis_result_size(output_samples, channels, &exact_size) &&
        exact_size < allocated_size
    ) {
        VorbisResult* shrunk = realloc(result, exact_size);

        if (shrunk != NULL) {
            result = shrunk;
        }
    }

    result->channels = channels;
    result->sample_rate = sample_rate;
    result->samples_per_channel = (int32_t) output_samples;

    return result;
}

/**
 * Frees the VorbisResult pointer
 * @param result the pointer to free
 */
void vorbis_free(VorbisResult* result) {
    free(result);
}
