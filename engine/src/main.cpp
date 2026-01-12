/**
 * WindowsAiMic - Main Entry Point
 * 
 * This is the entry point for the audio processing engine.
 * It initializes all components and manages the audio processing loop.
 */

#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#include <combaseapi.h>
#endif

#include "engine.h"
#include "config/config_manager.h"

namespace {
    std::atomic<bool> g_running{true};
    WindowsAiMic::Engine* g_engine = nullptr;
}

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received..." << std::endl;
        g_running = false;
        if (g_engine) {
            g_engine->stop();
        }
    }
}

void printBanner() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║                     WindowsAiMic                          ║
║         AI-Powered Virtual Microphone Enhancement         ║
║                      Version 1.0.0                        ║
╚═══════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "\nOptions:\n"
              << "  --help, -h          Show this help message\n"
              << "  --background, -b    Run in background mode (no console)\n"
              << "  --config <path>     Path to configuration file\n"
              << "  --list-devices      List available audio devices\n"
              << "  --version, -v       Show version information\n"
              << std::endl;
}

bool parseArguments(int argc, char* argv[], std::string& configPath, bool& background, bool& listDevices) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        }
        else if (arg == "--version" || arg == "-v") {
            std::cout << "WindowsAiMic version 1.0.0" << std::endl;
            return false;
        }
        else if (arg == "--background" || arg == "-b") {
            background = true;
        }
        else if (arg == "--list-devices") {
            listDevices = true;
        }
        else if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string configPath = "config.json";
    bool background = false;
    bool listDevices = false;
    
    if (!parseArguments(argc, argv, configPath, background, listDevices)) {
        return 0;
    }
    
    // Print banner unless in background mode
    if (!background) {
        printBanner();
    }
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
#ifdef _WIN32
    // Initialize COM for WASAPI
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: 0x" << std::hex << hr << std::endl;
        return 1;
    }
    
    // If running in background mode on Windows, hide console
    if (background) {
        FreeConsole();
    }
#endif
    
    try {
        // Load configuration
        WindowsAiMic::ConfigManager configManager;
        if (!configManager.load(configPath)) {
            std::cerr << "Failed to load configuration from: " << configPath << std::endl;
            std::cout << "Using default configuration..." << std::endl;
            configManager.loadDefaults();
        }
        
        // Create and initialize engine
        WindowsAiMic::Engine engine(configManager);
        g_engine = &engine;
        
        // List devices if requested
        if (listDevices) {
            engine.listAudioDevices();
#ifdef _WIN32
            CoUninitialize();
#endif
            return 0;
        }
        
        // Initialize engine
        if (!engine.initialize()) {
            std::cerr << "Failed to initialize audio engine" << std::endl;
#ifdef _WIN32
            CoUninitialize();
#endif
            return 1;
        }
        
        std::cout << "Audio engine initialized successfully" << std::endl;
        std::cout << "Processing audio... Press Ctrl+C to stop." << std::endl;
        
        // Start processing
        engine.start();
        
        // Main loop - wait for shutdown signal
        while (g_running && engine.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        std::cout << "Stopping audio engine..." << std::endl;
        engine.stop();
        
        g_engine = nullptr;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
#ifdef _WIN32
        CoUninitialize();
#endif
        return 1;
    }
    
#ifdef _WIN32
    CoUninitialize();
#endif
    
    std::cout << "WindowsAiMic shut down cleanly." << std::endl;
    return 0;
}
