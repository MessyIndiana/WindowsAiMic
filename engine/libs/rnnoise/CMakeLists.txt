# RNNoise Library - CMakeLists.txt
# Builds RNNoise noise suppression library

cmake_minimum_required(VERSION 3.10)
project(rnnoise C)

# Find available RNNoise source files
set(RNNOISE_SOURCES "")

# Core files that should always exist if RNNoise is present
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/denoise.c")
    list(APPEND RNNOISE_SOURCES src/denoise.c)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/celt_lpc.c")
    list(APPEND RNNOISE_SOURCES src/celt_lpc.c)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/kiss_fft.c")
    list(APPEND RNNOISE_SOURCES src/kiss_fft.c)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pitch.c")
    list(APPEND RNNOISE_SOURCES src/pitch.c)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/rnn.c")
    list(APPEND RNNOISE_SOURCES src/rnn.c)
endif()

# rnn_data.c may or may not exist depending on how RNNoise was cloned
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/rnn_data.c")
    list(APPEND RNNOISE_SOURCES src/rnn_data.c)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/rnn_reader.c")
    list(APPEND RNNOISE_SOURCES src/rnn_reader.c)
endif()

# Check if we have enough files to build actual RNNoise
list(LENGTH RNNOISE_SOURCES NUM_SOURCES)
if(NUM_SOURCES GREATER_EQUAL 4)
    message(STATUS "Building RNNoise from source (${NUM_SOURCES} files found)")
    
    add_library(rnnoise STATIC ${RNNOISE_SOURCES})
    
    target_include_directories(rnnoise PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Suppress warnings in third-party code
    if(MSVC)
        target_compile_options(rnnoise PRIVATE /W0)
        target_compile_definitions(rnnoise PRIVATE
            _USE_MATH_DEFINES
            WIN32
            _CRT_SECURE_NO_WARNINGS
        )
    else()
        target_compile_options(rnnoise PRIVATE -w)
    endif()
else()
    # Create stub library
    message(STATUS "RNNoise source not found, creating stub (noise suppression disabled)")
    
    # Create include directory if it doesn't exist
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    # Create stub header
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/rnnoise.h
"/* RNNoise stub header */
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
"/* RNNoise stub - passes audio through unchanged */
#include \"include/rnnoise.h\"
#include <stdlib.h>
#include <string.h>

struct DenoiseState { int dummy; };

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
    memcpy(out, in, 480 * sizeof(float));
    return 1.0f;
}
")
    
    add_library(rnnoise STATIC rnnoise_stub.c)
    
    target_include_directories(rnnoise PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()
