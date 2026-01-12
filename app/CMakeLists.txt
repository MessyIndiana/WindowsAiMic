# WindowsAiMic Tray App - CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(WindowsAiMicApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(APP_SOURCES
    src/main.cpp
    src/tray_app.cpp
    src/pipe_client.cpp
    src/settings_window.cpp
)

set(APP_HEADERS
    src/tray_app.h
    src/pipe_client.h
    src/settings_window.h
)

# Resources (Windows)
if(WIN32)
    list(APPEND APP_SOURCES
        resources/app.rc
    )
endif()

# Create executable (WIN32 = windows application, no console)
add_executable(WindowsAiMicApp WIN32 ${APP_SOURCES} ${APP_HEADERS})

target_include_directories(WindowsAiMicApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/engine/src  # For shared config types
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(WindowsAiMicApp PRIVATE
        comctl32
        shell32
        user32
        gdi32
        ole32
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(WindowsAiMicApp PRIVATE
        /W4
        /WX-
        /MP
    )
endif()

# Installation
install(TARGETS WindowsAiMicApp
    RUNTIME DESTINATION bin
)
