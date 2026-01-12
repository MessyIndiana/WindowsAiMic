/**
 * WindowsAiMic - OpenVINO AI Processor
 *
 * AI enhancement using Intel OpenVINO for NPU acceleration.
 * Leverages the Intel Core Ultra NPU for noise suppression.
 */

#pragma once

#include "ai_processor_interface.h"
#include <memory>
#include <string>
#include <vector>

namespace WindowsAiMic {

/**
 * OpenVINO-based AI processor for Intel NPU
 *
 * When available, runs noise suppression on the NPU instead of CPU,
 * dramatically reducing CPU usage and power consumption.
 */
class OpenVINOProcessor : public IAIProcessor {
public:
  OpenVINOProcessor();
  ~OpenVINOProcessor() override;

  // IAIProcessor interface
  bool initialize() override;
  void process(float *buffer, size_t frames) override;
  void reset() override;
  std::string getName() const override;
  bool isInitialized() const override { return initialized_; }
  int getExpectedSampleRate() const override { return 48000; }
  size_t getExpectedFrameSize() const override { return 480; }

  /**
   * Check if OpenVINO is available on this system
   */
  static bool isAvailable();

  /**
   * Check if NPU device is available
   */
  static bool hasNPU();

  /**
   * Get list of available devices
   */
  static std::vector<std::string> getAvailableDevices();

  /**
   * Set preferred device (AUTO, CPU, GPU, NPU)
   */
  void setDevice(const std::string &device);

  /**
   * Set model path (ONNX or OpenVINO IR format)
   */
  void setModelPath(const std::string &path);

  /**
   * Get current device being used
   */
  std::string getCurrentDevice() const { return currentDevice_; }

private:
  bool loadModel();
  void processFrameNPU(float *frame);
  void processFrameCPU(float *frame); // Fallback

  bool initialized_ = false;
  std::string modelPath_;
  std::string preferredDevice_ = "AUTO"; // AUTO will prefer NPU if available
  std::string currentDevice_;

  // Frame buffering (same as RNNoise - 480 samples)
  static constexpr size_t FRAME_SIZE = 480;
  std::vector<float> frameBuffer_;
  size_t bufferPos_ = 0;

  // OpenVINO internals (opaque to avoid header dependency)
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace WindowsAiMic
