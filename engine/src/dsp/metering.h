/**
 * WindowsAiMic - Metering Header
 *
 * Audio level metering with peak, RMS, and LUFS measurement.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace WindowsAiMic {

/**
 * Audio level meter
 *
 * Provides peak, RMS, and LUFS short-term measurements
 * for real-time level visualization.
 */
class Metering {
public:
  Metering();

  /**
   * Process audio buffer and update measurements
   * @param buffer Audio data
   * @param frames Number of samples
   */
  void process(const float *buffer, size_t frames);

  /**
   * Reset all measurements
   */
  void reset();

  /**
   * Get peak level in dBFS
   */
  float getPeak() const { return peakDb_; }

  /**
   * Get peak level (linear)
   */
  float getPeakLinear() const { return peak_; }

  /**
   * Get RMS level in dBFS
   */
  float getRMS() const { return rmsDb_; }

  /**
   * Get RMS level (linear)
   */
  float getRMSLinear() const { return rms_; }

  /**
   * Get LUFS short-term (3 second window)
   */
  float getLUFSShortTerm() const { return lufs_; }

  /**
   * Set sample rate (default 48000)
   */
  void setSampleRate(float sampleRate) { sampleRate_ = sampleRate; }

  /**
   * Set decay time for peak meter
   * @param ms Decay time in milliseconds
   */
  void setPeakDecay(float ms);

private:
  float sampleRate_ = 48000.0f;

  // Peak meter
  float peak_ = 0.0f;
  float peakDb_ = -96.0f;
  float peakDecayCoeff_ = 0.0f;

  // RMS meter (300ms window typical)
  float rms_ = 0.0f;
  float rmsDb_ = -96.0f;
  float rmsSum_ = 0.0f;
  size_t rmsCount_ = 0;
  static constexpr size_t RMS_WINDOW_SAMPLES = 48000 * 300 / 1000; // 300ms

  // LUFS meter (simplified - uses 3 second window)
  float lufs_ = -70.0f;
  std::vector<float> lufsBuffer_;
  size_t lufsPos_ = 0;
  static constexpr size_t LUFS_WINDOW_SAMPLES = 48000 * 3; // 3 seconds
};

} // namespace WindowsAiMic
