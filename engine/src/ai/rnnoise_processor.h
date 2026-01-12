/**
 * WindowsAiMic - RNNoise Processor Header
 *
 * RNNoise-based noise suppression using a recurrent neural network.
 * Lightweight, CPU-friendly, designed for real-time processing.
 */

#pragma once

#include "ai_processor_interface.h"
#include <memory>
#include <vector>

// Forward declare RNNoise types
struct DenoiseState;

namespace WindowsAiMic {

/**
 * RNNoise noise suppression processor
 *
 * Processes audio in 480-sample frames (10ms at 48kHz).
 * Automatically handles frame buffering for non-aligned inputs.
 */
class RNNoiseProcessor : public IAIProcessor {
public:
  RNNoiseProcessor();
  ~RNNoiseProcessor() override;

  // Non-copyable
  RNNoiseProcessor(const RNNoiseProcessor &) = delete;
  RNNoiseProcessor &operator=(const RNNoiseProcessor &) = delete;

  // IAIProcessor interface
  bool initialize() override;
  void process(float *buffer, size_t frames) override;
  void reset() override;
  std::string getName() const override { return "RNNoise"; }
  bool isInitialized() const override { return state_ != nullptr; }
  int getExpectedSampleRate() const override { return 48000; }
  size_t getExpectedFrameSize() const override { return FRAME_SIZE; }

  /**
   * Set noise attenuation level
   * @param db Attenuation in dB (0 to -60, where -60 is maximum noise
   * suppression)
   */
  void setAttenuation(float db);

  /**
   * Get current VAD (Voice Activity Detection) probability
   * @return Probability of speech (0.0 to 1.0)
   */
  float getVADProbability() const { return lastVAD_; }

private:
  void processFrame(float *frame);

  // RNNoise expects exactly 480 samples at 48kHz (10ms)
  static constexpr size_t FRAME_SIZE = 480;

  DenoiseState *state_ = nullptr;

  // Frame buffering for non-aligned inputs
  std::vector<float> frameBuffer_;
  size_t bufferPos_ = 0;

  // Output buffer for processed frames
  std::vector<float> outputBuffer_;
  size_t outputPos_ = 0;

  // Parameters
  float attenuation_ = 1.0f; // 0.0 = full suppression, 1.0 = no change to noise
  float lastVAD_ = 0.0f;
};

} // namespace WindowsAiMic
