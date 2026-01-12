/**
 * WindowsAiMic - Compressor Header
 *
 * Dynamic range compressor for consistent voice levels.
 */

#pragma once

#include "dsp_processor_interface.h"
#include <cstddef>

namespace WindowsAiMic {

/**
 * Dynamic range compressor with soft knee
 *
 * Reduces dynamic range of audio by attenuating loud signals.
 * Essential for consistent voice levels in streaming/podcasting.
 */
class Compressor : public IDSPProcessor {
public:
  Compressor();

  // IDSPProcessor interface
  void process(float *buffer, size_t frames) override;
  void reset() override;
  void setEnabled(bool enabled) override { enabled_ = enabled; }
  bool isEnabled() const override { return enabled_; }

  /**
   * Set compression threshold
   * @param dbThreshold Threshold in dBFS (-40 to 0)
   */
  void setThreshold(float dbThreshold);

  /**
   * Set compression ratio
   * @param ratio Compression ratio (1:1 to 20:1)
   */
  void setRatio(float ratio);

  /**
   * Set knee width
   * @param db Knee width in dB (0 = hard, up to 12 = soft)
   */
  void setKnee(float db);

  /**
   * Set attack time
   * @param ms Attack time in milliseconds (0.1 to 100)
   */
  void setAttack(float ms);

  /**
   * Set release time
   * @param ms Release time in milliseconds (10 to 1000)
   */
  void setRelease(float ms);

  /**
   * Set makeup gain
   * @param db Makeup gain in dB (0 to 24)
   */
  void setMakeupGain(float db);

  /**
   * Get current gain reduction in dB
   */
  float getGainReduction() const { return gainReductionDb_; }

private:
  float computeGainDb(float inputDb);

  bool enabled_ = true;

  // Parameters
  float thresholdDb_ = -18.0f;
  float ratio_ = 4.0f;
  float kneeDb_ = 6.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float makeupGain_ = 1.0f;

  // State
  float envelope_ = 0.0f;
  float gainReductionDb_ = 0.0f;
  float smoothedGain_ = 1.0f;

  // Sample rate
  float sampleRate_ = 48000.0f;
};

} // namespace WindowsAiMic
