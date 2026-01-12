/**
 * WindowsAiMic - Expander Header
 *
 * Downward expander / noise gate for reducing low-level noise.
 */

#pragma once

#include "dsp_processor_interface.h"
#include <cstddef>

namespace WindowsAiMic {

/**
 * Downward expander with hysteresis
 *
 * Reduces gain when signal falls below threshold.
 * Used as first stage after AI enhancement to further reduce
 * low-level noise between speech.
 */
class Expander : public IDSPProcessor {
public:
  Expander();

  // IDSPProcessor interface
  void process(float *buffer, size_t frames) override;
  void reset() override;
  void setEnabled(bool enabled) override { enabled_ = enabled; }
  bool isEnabled() const override { return enabled_; }

  /**
   * Set threshold for expansion
   * @param dbThreshold Threshold in dBFS (-60 to 0)
   */
  void setThreshold(float dbThreshold);

  /**
   * Set expansion ratio
   * @param ratio Expansion ratio (1:1 to 10:1, typical 2:1 to 4:1)
   */
  void setRatio(float ratio);

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
   * Set hysteresis (prevents chattering at threshold)
   * @param db Hysteresis in dB (0 to 10)
   */
  void setHysteresis(float db);

  /**
   * Get current gain reduction in dB
   */
  float getGainReduction() const { return gainReductionDb_; }

private:
  float computeGain(float envelope);

  bool enabled_ = true;

  // Parameters (linear)
  float threshold_ = 0.01f;   // -40 dB
  float ratio_ = 2.0f;        // 2:1 expansion
  float attackCoeff_ = 0.0f;  // Coefficient for attack
  float releaseCoeff_ = 0.0f; // Coefficient for release
  float hysteresis_ = 1.5f;   // Hysteresis factor

  // State
  float envelope_ = 0.0f;
  float gainReductionDb_ = 0.0f;
  bool gateOpen_ = false;

  // Sample rate (set during first process or via initialize)
  float sampleRate_ = 48000.0f;
};

} // namespace WindowsAiMic
