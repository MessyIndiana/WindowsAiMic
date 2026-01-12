/**
 * WindowsAiMic - Limiter Implementation
 */

#include "limiter.h"
#include <algorithm>
#include <cmath>

namespace WindowsAiMic {

Limiter::Limiter() {
  setCeiling(-1.0f);
  setRelease(50.0f);
  setLookahead(5.0f);
}

void Limiter::setCeiling(float dbCeiling) {
  ceiling_ = std::pow(10.0f, std::clamp(dbCeiling, -6.0f, 0.0f) / 20.0f);
}

void Limiter::setRelease(float ms) {
  float releaseMs = std::clamp(ms, 10.0f, 500.0f);
  releaseCoeff_ = std::exp(-1.0f / (releaseMs * sampleRate_ / 1000.0f));
}

void Limiter::setLookahead(float ms) {
  float lookaheadMs = std::clamp(ms, 0.0f, 10.0f);
  size_t newLookahead =
      static_cast<size_t>(lookaheadMs * sampleRate_ / 1000.0f);

  if (newLookahead != lookaheadSamples_) {
    lookaheadSamples_ = newLookahead;
    lookaheadBuffer_.resize(lookaheadSamples_ + 1);
    std::fill(lookaheadBuffer_.begin(), lookaheadBuffer_.end(), 0.0f);
    bufferPos_ = 0;
  }
}

void Limiter::reset() {
  std::fill(lookaheadBuffer_.begin(), lookaheadBuffer_.end(), 0.0f);
  bufferPos_ = 0;
  gainReductionDb_ = 0.0f;
  smoothedGain_ = 1.0f;
}

void Limiter::process(float *buffer, size_t frames) {
  if (!enabled_) {
    return;
  }

  if (lookaheadSamples_ == 0) {
    // No lookahead - simple instantaneous limiting
    for (size_t i = 0; i < frames; ++i) {
      float input = buffer[i];
      float absInput = std::abs(input);

      // Calculate required gain reduction
      float targetGain = 1.0f;
      if (absInput > ceiling_) {
        targetGain = ceiling_ / absInput;
      }

      // Smooth gain changes (instant attack, slow release)
      if (targetGain < smoothedGain_) {
        smoothedGain_ = targetGain; // Instant attack
      } else {
        smoothedGain_ =
            releaseCoeff_ * smoothedGain_ + (1.0f - releaseCoeff_) * targetGain;
      }

      gainReductionDb_ = -20.0f * std::log10(std::max(smoothedGain_, 0.0001f));

      buffer[i] = input * smoothedGain_;
    }
  } else {
    // Lookahead limiting
    for (size_t i = 0; i < frames; ++i) {
      float input = buffer[i];

      // Store input in lookahead buffer
      float delayedSample = lookaheadBuffer_[bufferPos_];
      lookaheadBuffer_[bufferPos_] = input;

      // Find peak in lookahead window
      float peakLevel = 0.0f;
      for (size_t j = 0; j < lookaheadBuffer_.size(); ++j) {
        peakLevel = std::max(peakLevel, std::abs(lookaheadBuffer_[j]));
      }

      // Calculate required gain
      float targetGain = 1.0f;
      if (peakLevel > ceiling_) {
        targetGain = ceiling_ / peakLevel;
      }

      // Smooth gain (fast attack via lookahead, slow release)
      if (targetGain < smoothedGain_) {
        // Use lookahead attack time
        float attackCoeff =
            std::exp(-1.0f / static_cast<float>(lookaheadSamples_));
        smoothedGain_ =
            attackCoeff * smoothedGain_ + (1.0f - attackCoeff) * targetGain;
      } else {
        smoothedGain_ =
            releaseCoeff_ * smoothedGain_ + (1.0f - releaseCoeff_) * targetGain;
      }

      gainReductionDb_ = -20.0f * std::log10(std::max(smoothedGain_, 0.0001f));

      // Output delayed sample with gain applied
      buffer[i] = delayedSample * smoothedGain_;

      bufferPos_ = (bufferPos_ + 1) % lookaheadBuffer_.size();
    }
  }
}

} // namespace WindowsAiMic
