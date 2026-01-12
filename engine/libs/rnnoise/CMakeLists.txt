# RNNoise Library - CMakeLists.txt
# Builds RNNoise noise suppression library
# Note: We always use the stub for simplicity in CI builds

cmake_minimum_required(VERSION 3.10)
project(rnnoise C)

message(STATUS "Building RNNoise stub library (for full noise suppression, use pre-built binaries)")

# Create include directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create stub header
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/rnnoise.h
"/* RNNoise header */
#ifndef RNNOISE_H
#define RNNOISE_H

#ifdef __cplusplus
extern \"C\" {
#endif

typedef struct DenoiseState DenoiseState;

/**
 * Create a new RNNoise state
 * @param model Optional model (pass NULL for built-in)
 * @return New state or NULL on error
 */
DenoiseState *rnnoise_create(void *model);

/**
 * Destroy RNNoise state
 */
void rnnoise_destroy(DenoiseState *st);

/**
 * Process one frame of audio (480 samples at 48kHz)
 * @param st RNNoise state
 * @param out Output buffer (480 floats)
 * @param in Input buffer (480 floats)
 * @return Voice probability (0.0 to 1.0)
 */
float rnnoise_process_frame(DenoiseState *st, float *out, const float *in);

#ifdef __cplusplus
}
#endif

#endif /* RNNOISE_H */
")

# Create stub implementation that passes audio through
# In production, you'd link against the real RNNoise library
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/rnnoise_stub.c
"/* RNNoise stub implementation */
#include \"include/rnnoise.h\"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FRAME_SIZE 480

struct DenoiseState {
    float gain;
    float noise_floor;
};

DenoiseState *rnnoise_create(void *model) {
    (void)model;
    DenoiseState *st = (DenoiseState*)malloc(sizeof(DenoiseState));
    if (st) {
        st->gain = 1.0f;
        st->noise_floor = 0.01f;
    }
    return st;
}

void rnnoise_destroy(DenoiseState *st) {
    if (st) free(st);
}

float rnnoise_process_frame(DenoiseState *st, float *out, const float *in) {
    if (!st || !out || !in) return 0.0f;
    
    /* Simple noise gate for stub implementation */
    float energy = 0.0f;
    for (int i = 0; i < FRAME_SIZE; i++) {
        energy += in[i] * in[i];
    }
    energy = sqrtf(energy / FRAME_SIZE);
    
    /* Apply simple gain based on energy */
    float voice_prob = energy > st->noise_floor ? 1.0f : 0.0f;
    float gain = voice_prob > 0.5f ? 1.0f : 0.1f;
    
    for (int i = 0; i < FRAME_SIZE; i++) {
        out[i] = in[i] * gain;
    }
    
    return voice_prob;
}
")

add_library(rnnoise STATIC rnnoise_stub.c)

target_include_directories(rnnoise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(rnnoise PRIVATE /W0)
endif()
