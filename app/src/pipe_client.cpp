/**
 * WindowsAiMic Tray App - Pipe Client Implementation
 */

#include "pipe_client.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace WindowsAiMic {

PipeClient::PipeClient() = default;

PipeClient::~PipeClient() { disconnect(); }

bool PipeClient::connect() {
  if (connected_.load()) {
    return true;
  }

#ifdef _WIN32
  // Try to connect to the named pipe
  pipe_ = CreateFileA(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                      OPEN_EXISTING, 0, nullptr);

  if (pipe_ == INVALID_HANDLE_VALUE || pipe_ == nullptr) {
    DWORD error = GetLastError();
    if (error == ERROR_PIPE_BUSY) {
      // Wait and retry
      if (WaitNamedPipeA(PIPE_NAME, 2000)) {
        pipe_ = CreateFileA(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, 0, nullptr);
      }
    }

    if (pipe_ == INVALID_HANDLE_VALUE || pipe_ == nullptr) {
      pipe_ = nullptr;
      return false;
    }
  }

  // Set pipe to message mode
  DWORD mode = PIPE_READMODE_MESSAGE;
  HANDLE hPipe = static_cast<HANDLE>(pipe_);
  SetNamedPipeHandleState(hPipe, &mode, nullptr, nullptr);

  connected_ = true;

  // Start reader thread for meter updates
  readerThread_ = std::thread(&PipeClient::readerThread, this);

  return true;
#else
  return false;
#endif
}

void PipeClient::disconnect() {
  if (!connected_.load()) {
    return;
  }

  connected_ = false;

#ifdef _WIN32
  if (pipe_ != nullptr) {
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    CloseHandle(hPipe);
    pipe_ = nullptr;
  }
#endif

  if (readerThread_.joinable()) {
    readerThread_.join();
  }
}

bool PipeClient::sendCommand(const std::string &command) {
  if (!connected_.load()) {
    return false;
  }

#ifdef _WIN32
  DWORD bytesWritten;
  HANDLE hPipe = static_cast<HANDLE>(pipe_);
  BOOL success =
      WriteFile(hPipe, command.c_str(), static_cast<DWORD>(command.size()),
                &bytesWritten, nullptr);

  return success && bytesWritten == command.size();
#else
  return false;
#endif
}

void PipeClient::setMeterCallback(MeterCallback callback) {
  meterCallback_ = std::move(callback);
}

void PipeClient::readerThread() {
#ifdef _WIN32
  char buffer[4096];

  while (connected_.load()) {
    DWORD bytesRead;
    HANDLE hPipe = static_cast<HANDLE>(pipe_);
    BOOL success =
        ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);

    if (!success || bytesRead == 0) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        connected_ = false;
        break;
      }
      continue;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer, bytesRead);

    // Parse message
    if (message.substr(0, 7) == "METERS:") {
      // Parse meter values: "METERS:peak,rms,gr"
      std::string values = message.substr(7);
      float peak = 0, rms = 0, gr = 0;

      std::istringstream iss(values);
      char comma;
      iss >> peak >> comma >> rms >> comma >> gr;

      if (meterCallback_) {
        meterCallback_(peak, rms, gr);
      }
    }
  }
#endif
}

} // namespace WindowsAiMic
