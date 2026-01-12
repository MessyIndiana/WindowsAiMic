/**
 * WindowsAiMic - Engine Header
 *
 * Main audio processing engine that orchestrates capture, AI enhancement,
 * DSP chain, and rendering to the virtual audio device.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "audio/audio_buffer.h"
#include "config/config_manager.h"

// Forward declarations
namespace WindowsAiMic {
class WasapiCapture;
class WasapiRender;
class Resampler;
class RNNoiseProcessor;
class Expander;
class Compressor;
class Limiter;
class Equalizer;
class Metering;
class PipeServer;
} // namespace WindowsAiMic

namespace WindowsAiMic {

/**
 * Main audio processing engine
 */
class Engine {
public:
  explicit Engine(ConfigManager &configManager);
  ~Engine();

  // Lifecycle
  bool initialize();
  void start();
  void stop();
  bool isRunning() const { return running_.load(); }

  // Device enumeration
  void listAudioDevices();
  std::vector<std::pair<std::string, std::wstring>> getInputDevices();
  std::vector<std::pair<std::string, std::wstring>> getOutputDevices();

  // Configuration
  void setInputDevice(const std::wstring &deviceId);
  void setOutputDevice(const std::wstring &deviceId);
  void setAIModel(const std::string &modelName);
  void applyPreset(const std::string &presetName);
  void setBypass(bool bypass);

  // DSP parameter setters
  void setExpanderParams(const ExpanderConfig &params);
  void setCompressorParams(const CompressorConfig &params);
  void setLimiterParams(const LimiterConfig &params);
  void setEqualizerParams(const EqualizerConfig &params);

  // Metering callbacks
  using MeterCallback =
      std::function<void(float peak, float rms, float gainReduction)>;
  void setMeterCallback(MeterCallback callback);

  // Status
  struct Status {
    bool capturing = false;
    bool rendering = false;
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    float gainReduction = 0.0f;
    float cpuUsage = 0.0f;
    uint32_t bufferUnderruns = 0;
  };
  Status getStatus() const;

private:
  // Processing thread
  void processingThread();
  void processAudioBlock(float *buffer, size_t frames);

  // Initialization helpers
  bool initializeCapture();
  bool initializeRender();
  bool initializeProcessors();
  bool initializeIPC();

  // Audio callback from WASAPI capture
  void onAudioCaptured(float *buffer, size_t frames, int sampleRate,
                       int channels);

  // Configuration
  ConfigManager &configManager_;

  // Audio I/O
  std::unique_ptr<WasapiCapture> capture_;
  std::unique_ptr<WasapiRender> render_;
  std::unique_ptr<Resampler> inputResampler_;
  std::unique_ptr<Resampler> outputResampler_;

  // Processing chain
  std::unique_ptr<RNNoiseProcessor> rnnoise_;
#ifdef USE_DEEPFILTER
  std::unique_ptr<DeepFilterProcessor> deepfilter_;
#endif
  std::unique_ptr<Expander> expander_;
  std::unique_ptr<Compressor> compressor_;
  std::unique_ptr<Limiter> limiter_;
  std::unique_ptr<Equalizer> equalizer_;
  std::unique_ptr<Metering> inputMetering_;
  std::unique_ptr<Metering> outputMetering_;

  // IPC
  std::unique_ptr<PipeServer> pipeServer_;

  // Buffers
  LockFreeRingBuffer inputBuffer_;
  LockFreeRingBuffer outputBuffer_;
  std::vector<float> processingBuffer_;

  // State
  std::atomic<bool> running_{false};
  std::atomic<bool> bypass_{false};
  std::thread processingThread_;

  // Synchronization
  std::mutex processingMutex_;
  std::condition_variable processingCv_;

  // Metering
  MeterCallback meterCallback_;
  std::mutex meterMutex_;

  // Status
  mutable std::mutex statusMutex_;
  Status status_;

  // Constants
  static constexpr int INTERNAL_SAMPLE_RATE = 48000;   // Required for RNNoise
  static constexpr int INTERNAL_CHANNELS = 1;          // Mono processing
  static constexpr size_t PROCESSING_BLOCK_SIZE = 480; // 10ms at 48kHz
  static constexpr size_t BUFFER_SIZE = PROCESSING_BLOCK_SIZE * 16;
};

} // namespace WindowsAiMic
