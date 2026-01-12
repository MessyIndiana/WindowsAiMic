# WindowsAiMic Engine - CMakeLists.txt
# Optimized for Intel Core Ultra processors
cmake_minimum_required(VERSION 3.20)
project(WindowsAiMicEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(ENABLE_AVX2 "Enable AVX2 optimizations (Intel Core 4th gen+)" ON)
option(ENABLE_AVX512 "Enable AVX-512 optimizations (Ice Lake+)" OFF)
option(ENABLE_OPENVINO "Enable OpenVINO for NPU acceleration" OFF)

# Source files
set(ENGINE_SOURCES
    src/main.cpp
    src/engine.cpp
    src/audio/wasapi_capture.cpp
    src/audio/wasapi_render.cpp
    src/audio/resampler.cpp
    src/audio/audio_buffer.cpp
    src/ai/rnnoise_processor.cpp
    src/ai/openvino_processor.cpp
    src/dsp/biquad_filter.cpp
    src/dsp/expander.cpp
    src/dsp/compressor.cpp
    src/dsp/limiter.cpp
    src/dsp/equalizer.cpp
    src/dsp/metering.cpp
    src/config/config_manager.cpp
    src/ipc/pipe_server.cpp
    src/platform/cpu_features.cpp
)

set(ENGINE_HEADERS
    src/engine.h
    src/audio/wasapi_capture.h
    src/audio/wasapi_render.h
    src/audio/resampler.h
    src/audio/audio_buffer.h
    src/ai/rnnoise_processor.h
    src/ai/ai_processor_interface.h
    src/ai/openvino_processor.h
    src/dsp/biquad_filter.h
    src/dsp/expander.h
    src/dsp/compressor.h
    src/dsp/limiter.h
    src/dsp/equalizer.h
    src/dsp/metering.h
    src/dsp/dsp_processor_interface.h
    src/config/config_manager.h
    src/config/config_types.h
    src/ipc/pipe_server.h
    src/platform/cpu_features.h
    src/platform/simd_dsp.h
    src/platform/thread_utils.h
)

# Add DeepFilterNet if enabled
if(USE_DEEPFILTER)
    list(APPEND ENGINE_SOURCES src/ai/deepfilter_processor.cpp)
    list(APPEND ENGINE_HEADERS src/ai/deepfilter_processor.h)
    add_definitions(-DUSE_DEEPFILTER)
endif()

# Create executable
add_executable(WindowsAiMicEngine ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Include directories
target_include_directories(WindowsAiMicEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/rnnoise/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/json/include
)

# Find threading library
find_package(Threads REQUIRED)

# RNNoise library (we'll build this from source)
add_subdirectory(libs/rnnoise)

# Link libraries
target_link_libraries(WindowsAiMicEngine PRIVATE
    rnnoise
    Threads::Threads
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(WindowsAiMicEngine PRIVATE
        ole32
        oleaut32
        uuid
        mmdevapi
        avrt
        ksuser
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(WindowsAiMicEngine PRIVATE
        /W4
        /WX-
        /MP
        /Zc:__cplusplus
        /permissive-
        /fp:fast          # Fast floating point (safe for audio)
        /Oi               # Intrinsic functions
        /Ot               # Favor fast code
        /GL               # Whole program optimization
    )
    
    # Enable AVX2 for Intel Core Ultra 7 165U
    if(ENABLE_AVX2)
        target_compile_options(WindowsAiMicEngine PRIVATE /arch:AVX2)
        target_compile_definitions(WindowsAiMicEngine PRIVATE __AVX2__)
    endif()
    
    # Optional AVX-512 (Meteor Lake supports it)
    if(ENABLE_AVX512)
        target_compile_options(WindowsAiMicEngine PRIVATE /arch:AVX512)
        target_compile_definitions(WindowsAiMicEngine PRIVATE __AVX512F__)
    endif()
    
    # Link-time optimization
    target_link_options(WindowsAiMicEngine PRIVATE /LTCG)
    
else()
    target_compile_options(WindowsAiMicEngine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3                     # Maximum optimization
        -ffast-math             # Fast floating point
        -funroll-loops          # Loop unrolling
    )
    
    if(ENABLE_AVX2)
        target_compile_options(WindowsAiMicEngine PRIVATE -mavx2 -mfma)
    endif()
    
    if(ENABLE_AVX512)
        target_compile_options(WindowsAiMicEngine PRIVATE -mavx512f -mavx512dq)
    endif()
endif()

# OpenVINO support for NPU
if(ENABLE_OPENVINO)
    find_package(OpenVINO QUIET)
    if(OpenVINO_FOUND)
        target_link_libraries(WindowsAiMicEngine PRIVATE openvino::runtime)
        target_compile_definitions(WindowsAiMicEngine PRIVATE HAS_OPENVINO)
        message(STATUS "OpenVINO found - NPU acceleration enabled")
    else()
        message(WARNING "OpenVINO not found - NPU acceleration disabled")
    endif()
endif()

# Installation
install(TARGETS WindowsAiMicEngine
    RUNTIME DESTINATION bin
)
