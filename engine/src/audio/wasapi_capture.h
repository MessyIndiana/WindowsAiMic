/**
 * WindowsAiMic - WASAPI Capture Header
 *
 * Event-driven WASAPI audio capture from a physical microphone.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
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
 * WASAPI audio capture from input devices (microphones)
 * Uses event-driven shared mode for low latency
 */
class WasapiCapture {
public:
  WasapiCapture();
  ~WasapiCapture();

  // Non-copyable
  WasapiCapture(const WasapiCapture &) = delete;
  WasapiCapture &operator=(const WasapiCapture &) = delete;

  /**
   * Initialize capture from a specific device
   * @param deviceId Device ID (empty for default device)
   * @return true on success
   */
  bool initialize(const std::wstring &deviceId = L"");

  /**
   * Start capturing audio
   */
  void start();

  /**
   * Stop capturing
   */
  void stop();

  /**
   * Check if capturing
   */
  bool isCapturing() const { return capturing_.load(); }

  /**
   * Get sample rate of the capture device
   */
  int getSampleRate() const { return sampleRate_; }

  /**
   * Get number of channels
   */
  int getChannels() const { return channels_; }

  /**
   * Audio callback type
   * Parameters: buffer, frames, sampleRate, channels
   */
  using AudioCallback = std::function<void(float *, size_t, int, int)>;

  /**
   * Set callback for captured audio
   */
  void setCallback(AudioCallback callback);

  /**
   * Enumerate available capture devices
   * @return Vector of (name, deviceId) pairs
   */
  std::vector<std::pair<std::string, std::wstring>> enumerateDevices();

private:
  void captureThread();
  bool initializeDevice(const std::wstring &deviceId);
  void cleanup();

#ifdef _WIN32
  IMMDeviceEnumerator *deviceEnumerator_ = nullptr;
  IMMDevice *device_ = nullptr;
  IAudioClient *audioClient_ = nullptr;
  IAudioCaptureClient *captureClient_ = nullptr;
  HANDLE audioEvent_ = nullptr;
  WAVEFORMATEX *waveFormat_ = nullptr;
#endif

  int sampleRate_ = 0;
  int channels_ = 0;
  int bitsPerSample_ = 0;

  std::atomic<bool> capturing_{false};
  std::thread captureThread_;
  AudioCallback callback_;

  std::vector<float> conversionBuffer_;
};

} // namespace WindowsAiMic
