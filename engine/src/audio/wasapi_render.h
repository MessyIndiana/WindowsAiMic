/**
 * WindowsAiMic - WASAPI Render Header
 *
 * WASAPI audio rendering to the virtual speaker device.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#endif

namespace WindowsAiMic {

/**
 * WASAPI audio render to output devices (Virtual Speaker)
 */
class WasapiRender {
public:
  WasapiRender();
  ~WasapiRender();

  // Non-copyable
  WasapiRender(const WasapiRender &) = delete;
  WasapiRender &operator=(const WasapiRender &) = delete;

  /**
   * Initialize render to a specific device
   * @param deviceId Device ID (should be the Virtual Speaker device)
   * @return true on success
   */
  bool initialize(const std::wstring &deviceId);

  /**
   * Start rendering audio
   */
  void start();

  /**
   * Stop rendering
   */
  void stop();

  /**
   * Check if ready to write
   */
  bool isReady() const { return initialized_.load(); }

  /**
   * Write audio data
   * @param buffer Audio data (float32)
   * @param frames Number of frames
   */
  void write(const float *buffer, size_t frames);

  /**
   * Get sample rate of the render device
   */
  int getSampleRate() const { return sampleRate_; }

  /**
   * Get number of channels
   */
  int getChannels() const { return channels_; }

  /**
   * Enumerate available render devices
   * @return Vector of (name, deviceId) pairs
   */
  std::vector<std::pair<std::string, std::wstring>> enumerateDevices();

private:
  void renderThread();
  bool initializeDevice(const std::wstring &deviceId);
  void cleanup();

#ifdef _WIN32
  IMMDeviceEnumerator *deviceEnumerator_ = nullptr;
  IMMDevice *device_ = nullptr;
  IAudioClient *audioClient_ = nullptr;
  IAudioRenderClient *renderClient_ = nullptr;
  HANDLE audioEvent_ = nullptr;
  WAVEFORMATEX *waveFormat_ = nullptr;
  UINT32 bufferFrameCount_ = 0;
#endif

  int sampleRate_ = 0;
  int channels_ = 0;
  int bitsPerSample_ = 0;

  std::atomic<bool> initialized_{false};
  std::atomic<bool> running_{false};
  std::thread renderThread_;

  // Ring buffer for audio data
  std::vector<float> ringBuffer_;
  size_t writePos_ = 0;
  size_t readPos_ = 0;
  std::mutex bufferMutex_;
  std::condition_variable bufferCv_;
};

} // namespace WindowsAiMic
