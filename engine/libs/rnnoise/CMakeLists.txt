# RNNoise Library - CMakeLists.txt
# This is a stub for building with the actual RNNoise library
# Clone https://github.com/xiph/rnnoise into this directory

cmake_minimum_required(VERSION 3.10)
project(rnnoise C)

# Check if we have the actual RNNoise source
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/denoise.c")
    # Build actual RNNoise
    set(RNNOISE_SOURCES
        src/denoise.c
        src/celt_lpc.c
        src/kiss_fft.c
        src/pitch.c
        src/rnn.c
        src/rnn_data.c
        src/rnn_reader.c
    )
    
    add_library(rnnoise STATIC ${RNNOISE_SOURCES})
    
    target_include_directories(rnnoise PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    if(MSVC)
        target_compile_definitions(rnnoise PRIVATE
            _USE_MATH_DEFINES
            WIN32
        )
    endif()
else()
    # Create stub library for initial build
    message(WARNING "RNNoise source not found. Creating stub library.")
    message(WARNING "Clone https://github.com/xiph/rnnoise to engine/libs/rnnoise")
    
    # Create stub header if it doesn't exist
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/rnnoise.h
"/* RNNoise stub header - replace with actual library */
#ifndef RNNOISE_H
#define RNNOISE_H

#ifdef __cplusplus
extern \"C\" {
#endif

typedef struct DenoiseState DenoiseState;

DenoiseState *rnnoise_create(void *model);
void rnnoise_destroy(DenoiseState *st);
float rnnoise_process_frame(DenoiseState *st, float *out, const float *in);

#ifdef __cplusplus
}
#endif

#endif /* RNNOISE_H */
")
    
    # Create stub implementation
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/rnnoise_stub.c
"/* RNNoise stub implementation - replace with actual library */
#include \"include/rnnoise.h\"
#include <stdlib.h>
#include <string.h>

struct DenoiseState {
    int dummy;
};

DenoiseState *rnnoise_create(void *model) {
    (void)model;
    DenoiseState *st = (DenoiseState*)malloc(sizeof(DenoiseState));
    if (st) st->dummy = 0;
    return st;
}

void rnnoise_destroy(DenoiseState *st) {
    if (st) free(st);
}

float rnnoise_process_frame(DenoiseState *st, float *out, const float *in) {
    (void)st;
    /* Stub: just copy input to output */
    memcpy(out, in, 480 * sizeof(float));
    return 1.0f; /* Always return voice detected */
}
")
    
    add_library(rnnoise STATIC rnnoise_stub.c)
    
    target_include_directories(rnnoise PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()
