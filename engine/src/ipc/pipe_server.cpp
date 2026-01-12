/**
 * WindowsAiMic - Named Pipe Server Implementation
 */

#include "pipe_server.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace WindowsAiMic {

PipeServer::PipeServer() = default;

PipeServer::~PipeServer() { stop(); }

bool PipeServer::start() {
  if (running_.load()) {
    return true;
  }

#ifdef _WIN32
  // Create named pipe
  pipe_ =
      CreateNamedPipeA(PIPE_NAME, PIPE_ACCESS_DUPLEX,
                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                       1,      // Max instances
                       4096,   // Output buffer size
                       4096,   // Input buffer size
                       0,      // Default timeout
                       nullptr // Default security
      );

  if (pipe_ == INVALID_HANDLE_VALUE || pipe_ == nullptr) {
    std::cerr << "Failed to create named pipe: " << GetLastError() << std::endl;
    pipe_ = nullptr;
    return false;
  }

  running_ = true;
  serverThread_ = std::thread(&PipeServer::serverThread, this);

  std::cout << "IPC pipe server started: " << PIPE_NAME << std::endl;
  return true;
#else
  std::cout << "Named pipes not supported on this platform" << std::endl;
  return false;
#endif
}

void PipeServer::stop() {
  if (!running_.load()) {
    return;
  }

  running_ = false;

#ifdef _WIN32
  // Close pipe to unblock WaitForSingleObject/ConnectNamedPipe
  if (pipe_ != nullptr) {
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    pipe_ = nullptr;
  }
#endif

  if (serverThread_.joinable()) {
    serverThread_.join();
  }

  std::cout << "IPC pipe server stopped" << std::endl;
}

void PipeServer::serverThread() {
#ifdef _WIN32
  HANDLE hPipe = static_cast<HANDLE>(pipe_);
  while (running_.load()) {
    // Wait for client connection
    BOOL connected = ConnectNamedPipe(hPipe, nullptr);

    if (!running_.load()) {
      break;
    }

    if (connected || GetLastError() == ERROR_PIPE_CONNECTED) {
      clientConnected_ = true;
      std::cout << "Client connected to pipe" << std::endl;

      handleClient();

      clientConnected_ = false;
      std::cout << "Client disconnected" << std::endl;

      DisconnectNamedPipe(hPipe);
    }
  }
#endif
}

void PipeServer::handleClient() {
#ifdef _WIN32
  char buffer[4096];
  DWORD bytesRead;

  while (running_.load() && clientConnected_.load()) {
    // Read message from client
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    BOOL success =
        ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);

    if (!success || bytesRead == 0) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        break; // Client disconnected
      }
      continue;
    }

    buffer[bytesRead] = '\0';
    processMessage(std::string(buffer, bytesRead));
  }
#endif
}

void PipeServer::processMessage(const std::string &message) {
  // Simple command parsing
  // Format: "COMMAND:DATA"

  size_t colonPos = message.find(':');
  std::string command =
      colonPos != std::string::npos ? message.substr(0, colonPos) : message;
  std::string data =
      colonPos != std::string::npos ? message.substr(colonPos + 1) : "";

  if (command == "PING") {
    // Health check
#ifdef _WIN32
    const char *response = "PONG";
    DWORD bytesWritten;
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    WriteFile(hPipe, response, 4, &bytesWritten, nullptr);
#endif
  } else if (command == "GET_STATUS") {
    // Return current status
#ifdef _WIN32
    std::string response = "STATUS:OK";
    DWORD bytesWritten;
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    WriteFile(hPipe, response.c_str(), static_cast<DWORD>(response.size()),
              &bytesWritten, nullptr);
#endif
  } else if (command == "CONFIG") {
    // Config update - would parse JSON and call callback
    // For now, just acknowledge
    if (configCallback_) {
      // In production, parse JSON from data
      // configCallback_(parsedConfig);
    }
  } else if (command == "PRESET") {
    // Apply preset
    if (configCallback_) {
      Config config;
      config.activePreset = data;
      configCallback_(config);
    }
  } else if (command == "BYPASS") {
    // Toggle bypass
    // Would be handled by engine
  }
}

void PipeServer::sendMeterUpdate(float peak, float rms, float gainReduction) {
  if (!clientConnected_.load()) {
    return;
  }

#ifdef _WIN32
  std::ostringstream ss;
  ss << "METERS:" << peak << "," << rms << "," << gainReduction;
  std::string message = ss.str();

  DWORD bytesWritten;
  HANDLE hPipe = static_cast<HANDLE>(pipe_);
  WriteFile(hPipe, message.c_str(), static_cast<DWORD>(message.size()),
            &bytesWritten, nullptr);
#endif
}

void PipeServer::setConfigUpdateCallback(ConfigUpdateCallback callback) {
  configCallback_ = std::move(callback);
}

} // namespace WindowsAiMic
