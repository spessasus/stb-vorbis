
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../wasm/vorbis_wrapper.c"

static uint8_t* read_file(const char* path, long* length) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = malloc((size_t) *length);

    if (data != NULL &&
        fread(data, 1, (size_t) *length, file) != (size_t) *length) {
        free(data);
        data = NULL;
    }

    fclose(file);
    return data;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        long length = 0;
        uint8_t* data = read_file(argv[i], &length);

        if (data == NULL) {
            fprintf(stderr, "cannot read %s\n", argv[i]);
            return 1;
        }

        VorbisResult* clean = vorbis_decode(data, (int32_t) length);

        if (clean == NULL) {
            fprintf(stderr, "%s failed to decode\n", argv[i]);
            return 1;
        }

        printf(
            "%s: %d ch, %d Hz, %d samples\n",
            argv[i],
            clean->channels,
            clean->sample_rate,
            clean->samples_per_channel
        );
        vorbis_free(clean);

        /* Truncations */
        for (long cut = 0; cut < length; cut += 17) {
            vorbis_free(vorbis_decode(data, (int32_t) cut));
        }

        /* Bit flips, deterministic so failures reproduce */
        uint8_t* copy = malloc((size_t) length);

        if (copy == NULL) {
            free(data);
            return 1;
        }

        for (unsigned seed = 1; seed <= 400; seed++) {
            memcpy(copy, data, (size_t) length);

            for (unsigned flip = 0; flip < 4; flip++) {
                const size_t index =
                    ((size_t) seed * 7919 + flip * 104729) % (size_t) length;
                copy[index] ^= (uint8_t) (1u << ((seed + flip) % 8));
            }

            vorbis_free(vorbis_decode(copy, (int32_t) length));
        }

        free(copy);
        free(data);
    }

    return 0;
}
