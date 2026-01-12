/**
 * WindowsAiMic - Biquad Filter Header
 *
 * Second-order IIR filter for EQ and various filtering applications.
 */

#pragma once

#include <cstddef>

namespace WindowsAiMic {

/**
 * Biquad filter types
 */
enum class BiquadType {
  LowPass,
  HighPass,
  BandPass,
  Notch,
  Peak,
  LowShelf,
  HighShelf
};

/**
 * Second-order IIR (biquad) filter
 *
 * Implements the standard Direct Form II transposed structure:
 * y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 */
class BiquadFilter {
public:
  BiquadFilter();

  /**
   * Set filter coefficients directly
   */
  void setCoefficients(float b0, float b1, float b2, float a1, float a2);

  /**
   * Configure as low-pass filter
   * @param sampleRate Sample rate in Hz
   * @param frequency Cutoff frequency in Hz
   * @param q Q factor (0.5 to 10, default 0.707 for Butterworth)
   */
  void setLowPass(float sampleRate, float frequency, float q = 0.707f);

  /**
   * Configure as high-pass filter
   */
  void setHighPass(float sampleRate, float frequency, float q = 0.707f);

  /**
   * Configure as band-pass filter
   */
  void setBandPass(float sampleRate, float frequency, float q);

  /**
   * Configure as notch (band-stop) filter
   */
  void setNotch(float sampleRate, float frequency, float q);

  /**
   * Configure as peaking EQ filter
   * @param gain Gain in dB
   */
  void setPeak(float sampleRate, float frequency, float q, float gainDb);

  /**
   * Configure as low shelf filter
   */
  void setLowShelf(float sampleRate, float frequency, float gainDb,
                   float q = 0.707f);

  /**
   * Configure as high shelf filter
   */
  void setHighShelf(float sampleRate, float frequency, float gainDb,
                    float q = 0.707f);

  /**
   * Process a single sample
   */
  float process(float input);

  /**
   * Process buffer in-place
   */
  void process(float *buffer, size_t frames);

  /**
   * Reset filter state
   */
  void reset();

private:
  // Coefficients
  float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
  float a1_ = 0.0f, a2_ = 0.0f;

  // State variables (Direct Form II transposed)
  float z1_ = 0.0f, z2_ = 0.0f;
};

} // namespace WindowsAiMic
