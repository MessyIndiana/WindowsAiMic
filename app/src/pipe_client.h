/**
 * WindowsAiMic Tray App - Pipe Client Header
 */

#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace WindowsAiMic {

/**
 * Named pipe client for communicating with the audio engine
 */
class PipeClient {
public:
  PipeClient();
  ~PipeClient();

  /**
   * Connect to the engine's pipe server
   * @return true on success
   */
  bool connect();

  /**
   * Disconnect from the server
   */
  void disconnect();

  /**
   * Check if connected
   */
  bool isConnected() const { return connected_.load(); }

  /**
   * Send a command to the engine
   * @param command Command string
   * @return true if sent successfully
   */
  bool sendCommand(const std::string &command);

  /**
   * Callback for meter updates from engine
   */
  using MeterCallback =
      std::function<void(float peak, float rms, float gainReduction)>;
  void setMeterCallback(MeterCallback callback);

private:
  void readerThread();

  static constexpr const char *PIPE_NAME = "\\\\.\\pipe\\WindowsAiMicPipe";

#ifdef _WIN32
  void *pipe_ = nullptr; // HANDLE
#endif

  std::atomic<bool> connected_{false};
  std::thread readerThread_;
  MeterCallback meterCallback_;
};

} // namespace WindowsAiMic
