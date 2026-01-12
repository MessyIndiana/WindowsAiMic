/**
 * WindowsAiMic - Expander Implementation
 */

#include "expander.h"
#include <algorithm>
#include <cmath>

namespace WindowsAiMic {

Expander::Expander() {
  setThreshold(-40.0f);
  setRatio(2.0f);
  setAttack(5.0f);
  setRelease(100.0f);
  setHysteresis(3.0f);
}

void Expander::setThreshold(float dbThreshold) {
  threshold_ = std::pow(10.0f, std::clamp(dbThreshold, -60.0f, 0.0f) / 20.0f);
}

void Expander::setRatio(float ratio) {
  ratio_ = std::clamp(ratio, 1.0f, 10.0f);
}

void Expander::setAttack(float ms) {
  float attackMs = std::clamp(ms, 0.1f, 100.0f);
  // Time constant for exponential decay to 1-1/e (~63%)
  attackCoeff_ = std::exp(-1.0f / (attackMs * sampleRate_ / 1000.0f));
}

void Expander::setRelease(float ms) {
  float releaseMs = std::clamp(ms, 10.0f, 1000.0f);
  releaseCoeff_ = std::exp(-1.0f / (releaseMs * sampleRate_ / 1000.0f));
}

void Expander::setHysteresis(float db) {
  // Hysteresis as a multiplier on threshold
  hysteresis_ = std::pow(10.0f, std::clamp(db, 0.0f, 10.0f) / 20.0f);
}

void Expander::reset() {
  envelope_ = 0.0f;
  gainReductionDb_ = 0.0f;
  gateOpen_ = false;
}

float Expander::computeGain(float envelope) {
  // Compute gain reduction based on envelope level relative to threshold
  if (envelope < 1e-10f) {
    return 0.001f; // -60 dB minimum
  }

  float envelopeDb = 20.0f * std::log10(envelope);
  float thresholdDb = 20.0f * std::log10(threshold_);

  // Expander: when below threshold, reduce gain
  if (envelopeDb < thresholdDb) {
    // Amount below threshold
    float belowDb = thresholdDb - envelopeDb;

    // Expansion: multiply the "below threshold" amount
    float expansionDb = belowDb * (ratio_ - 1.0f);

    gainReductionDb_ = -expansionDb;
    return std::pow(10.0f, -expansionDb / 20.0f);
  }

  gainReductionDb_ = 0.0f;
  return 1.0f;
}

void Expander::process(float *buffer, size_t frames) {
  if (!enabled_) {
    return;
  }

  for (size_t i = 0; i < frames; ++i) {
    float input = buffer[i];
    float level = std::abs(input);

    // Envelope follower with attack/release
    float coeff;
    if (level > envelope_) {
      coeff = attackCoeff_; // Fast attack
    } else {
      coeff = releaseCoeff_; // Slow release
    }
    envelope_ = coeff * envelope_ + (1.0f - coeff) * level;

    // Hysteresis logic
    float effectiveThreshold;
    if (gateOpen_) {
      // Once open, use lower threshold to close (hysteresis)
      effectiveThreshold = threshold_ / hysteresis_;
    } else {
      effectiveThreshold = threshold_;
    }

    // Update gate state
    if (envelope_ > threshold_) {
      gateOpen_ = true;
    } else if (envelope_ < effectiveThreshold) {
      gateOpen_ = false;
    }

    // Compute and apply gain
    float gain = computeGain(envelope_);
    buffer[i] = input * gain;
  }
}

} // namespace WindowsAiMic
