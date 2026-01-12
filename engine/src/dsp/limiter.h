/**
 * WindowsAiMic - Limiter Header
 *
 * Brickwall limiter with lookahead for preventing clipping.
 */

#pragma once

#include "dsp_processor_interface.h"
#include <cstddef>
#include <vector>

namespace WindowsAiMic {

/**
 * Brickwall limiter with lookahead
 *
 * Prevents output from exceeding ceiling level.
 * Lookahead allows for transparent limiting without distortion.
 */
class Limiter : public IDSPProcessor {
public:
  Limiter();

  // IDSPProcessor interface
  void process(float *buffer, size_t frames) override;
  void reset() override;
  void setEnabled(bool enabled) override { enabled_ = enabled; }
  bool isEnabled() const override { return enabled_; }

  /**
   * Set output ceiling
   * @param dbCeiling Maximum output level in dBFS (-6 to 0)
   */
  void setCeiling(float dbCeiling);

  /**
   * Set release time
   * @param ms Release time in milliseconds (10 to 500)
   */
  void setRelease(float ms);

  /**
   * Set lookahead time
   * @param ms Lookahead in milliseconds (0 to 10)
   * Note: Adds latency equal to lookahead time
   */
  void setLookahead(float ms);

  /**
   * Get current gain reduction in dB
   */
  float getGainReduction() const { return gainReductionDb_; }

  /**
   * Get latency in samples (due to lookahead)
   */
  size_t getLatency() const { return lookaheadSamples_; }

private:
  bool enabled_ = true;

  // Parameters
  float ceiling_ = 0.891f; // -1 dBFS
  float releaseCoeff_ = 0.0f;
  size_t lookaheadSamples_ = 0;

  // Lookahead buffer
  std::vector<float> lookaheadBuffer_;
  size_t bufferPos_ = 0;

  // State
  float gainReductionDb_ = 0.0f;
  float smoothedGain_ = 1.0f;

  // Sample rate
  float sampleRate_ = 48000.0f;
};

} // namespace WindowsAiMic
