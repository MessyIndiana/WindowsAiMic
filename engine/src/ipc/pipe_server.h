/**
 * WindowsAiMic - Named Pipe Server Header
 *
 * IPC server for communication with the UI application.
 */

#pragma once

#include "../config/config_types.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace WindowsAiMic {

/**
 * Named pipe server for UI communication
 *
 * Allows the tray application to retrieve/update configuration
 * and receive real-time meter updates.
 */
class PipeServer {
public:
  PipeServer();
  ~PipeServer();

  /**
   * Start the pipe server
   * @return true on success
   */
  bool start();

  /**
   * Stop the pipe server
   */
  void stop();

  /**
   * Send meter update to connected client
   */
  void sendMeterUpdate(float peak, float rms, float gainReduction);

  /**
   * Set callback for config update requests
   */
  using ConfigUpdateCallback = std::function<void(const Config &)>;
  void setConfigUpdateCallback(ConfigUpdateCallback callback);

  /**
   * Check if client is connected
   */
  bool isClientConnected() const { return clientConnected_.load(); }

private:
  void serverThread();
  void handleClient();
  void processMessage(const std::string &message);

  static constexpr const char *PIPE_NAME = "\\\\.\\pipe\\WindowsAiMicPipe";

#ifdef _WIN32
  void *pipe_ = nullptr; // HANDLE
#endif

  std::atomic<bool> running_{false};
  std::atomic<bool> clientConnected_{false};
  std::thread serverThread_;

  ConfigUpdateCallback configCallback_;
};

} // namespace WindowsAiMic
