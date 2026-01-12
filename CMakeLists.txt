# WindowsAiMic - Root CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(WindowsAiMic VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# Build options
option(BUILD_ENGINE "Build the audio processing engine" ON)
option(BUILD_APP "Build the tray application" ON)
option(BUILD_TESTS "Build unit tests" OFF)
option(USE_DEEPFILTER "Enable DeepFilterNet support" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find dependencies
find_package(Threads REQUIRED)

# Add subdirectories
if(BUILD_ENGINE)
    add_subdirectory(engine)
endif()

if(BUILD_APP)
    add_subdirectory(app)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(DIRECTORY config/ 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/WindowsAiMic
    FILES_MATCHING PATTERN "*.json"
)

# CPack configuration for installer
set(CPACK_PACKAGE_NAME "WindowsAiMic")
set(CPACK_PACKAGE_VENDOR "WindowsAiMic")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-powered virtual microphone with studio-quality processing")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_UPGRADE_GUID "YOUR-GUID-HERE")
endif()

include(CPack)
