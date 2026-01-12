/**
 * WindowsAiMic - Equalizer Header
 *
 * Multi-band parametric equalizer for voice shaping.
 */

#pragma once

#include "biquad_filter.h"
#include "dsp_processor_interface.h"
#include <cstddef>

namespace WindowsAiMic {

/**
 * Voice-optimized equalizer
 *
 * Includes high-pass filter, low/high shelves, presence peak,
 * and optional de-esser for sibilance control.
 */
class Equalizer : public IDSPProcessor {
public:
  Equalizer();

  // IDSPProcessor interface
  void process(float *buffer, size_t frames) override;
  void reset() override;
  void setEnabled(bool enabled) override { enabled_ = enabled; }
  bool isEnabled() const override { return enabled_; }

  /**
   * Set high-pass filter (rumble removal)
   * @param freq Cutoff frequency in Hz (20-500)
   * @param q Q factor (0.5-2.0)
   */
  void setHighPass(float freq, float q);

  /**
   * Set low shelf (bass control)
   * @param freq Shelf frequency in Hz (80-300)
   * @param gain Gain in dB (-12 to +12)
   */
  void setLowShelf(float freq, float gain);

  /**
   * Set presence/mid peak (voice clarity)
   * @param freq Center frequency in Hz (2000-6000)
   * @param gain Gain in dB (-12 to +12)
   * @param q Q factor (0.5-4.0)
   */
  void setPresence(float freq, float gain, float q);

  /**
   * Set high shelf (air/brightness)
   * @param freq Shelf frequency in Hz (6000-16000)
   * @param gain Gain in dB (-12 to +12)
   */
  void setHighShelf(float freq, float gain);

  /**
   * Set de-esser (sibilance control)
   * @param freq Detection frequency in Hz (4000-10000)
   * @param threshold Threshold in dB (-40 to 0)
   */
  void setDeEsser(float freq, float threshold);

  /**
   * Enable/disable de-esser separately
   */
  void setDeEsserEnabled(bool enabled) { deEsserEnabled_ = enabled; }

private:
  bool enabled_ = true;
  bool deEsserEnabled_ = false;

  // Filters
  BiquadFilter highPass_;
  BiquadFilter lowShelf_;
  BiquadFilter presence_;
  BiquadFilter highShelf_;

  // De-esser
  BiquadFilter deEsserDetect_; // Band-pass for sibilance detection
  float deEsserThreshold_ = 0.1f;
  float deEsserEnvelope_ = 0.0f;

  // Sample rate
  float sampleRate_ = 48000.0f;
};

} // namespace WindowsAiMic
