/**
 * WindowsAiMic - Resampler Header
 *
 * Sample rate conversion for audio processing.
 */

#pragma once

#include <memory>
#include <vector>

namespace WindowsAiMic {

/**
 * High-quality audio resampler
 * Uses polyphase FIR filter implementation
 */
class Resampler {
public:
  Resampler();
  ~Resampler();

  /**
   * Initialize resampler
   * @param srcRate Source sample rate
   * @param dstRate Destination sample rate
   * @param channels Number of channels (typically 1 for mono)
   * @return true on success
   */
  bool initialize(int srcRate, int dstRate, int channels);

  /**
   * Process audio data
   * @param input Input samples
   * @param frames Number of input frames
   * @return Resampled output
   */
  std::vector<float> process(const float *input, size_t frames);

  /**
   * Reset resampler state
   */
  void reset();

  /**
   * Get ratio (srcRate / dstRate)
   */
  double getRatio() const { return ratio_; }

private:
  // Simple linear interpolation resampler (can be upgraded to libsamplerate)
  double ratio_ = 1.0;
  int srcRate_ = 0;
  int dstRate_ = 0;
  int channels_ = 0;

  // State for fractional sample position
  double position_ = 0.0;
  float lastSample_ = 0.0f;

  // Polyphase filter coefficients (for high quality)
  std::vector<std::vector<float>> filterBank_;
  std::vector<float> history_;
  int historySize_ = 0;
  int historyPos_ = 0;
};

} // namespace WindowsAiMic
