/**
 * WindowsAiMic - Compressor Implementation
 */

#include "compressor.h"
#include <algorithm>
#include <cmath>

namespace WindowsAiMic {

Compressor::Compressor() {
  setThreshold(-18.0f);
  setRatio(4.0f);
  setKnee(6.0f);
  setAttack(10.0f);
  setRelease(100.0f);
  setMakeupGain(6.0f);
}

void Compressor::setThreshold(float dbThreshold) {
  thresholdDb_ = std::clamp(dbThreshold, -40.0f, 0.0f);
}

void Compressor::setRatio(float ratio) {
  ratio_ = std::clamp(ratio, 1.0f, 20.0f);
}

void Compressor::setKnee(float db) { kneeDb_ = std::clamp(db, 0.0f, 12.0f); }

void Compressor::setAttack(float ms) {
  float attackMs = std::clamp(ms, 0.1f, 100.0f);
  attackCoeff_ = std::exp(-1.0f / (attackMs * sampleRate_ / 1000.0f));
}

void Compressor::setRelease(float ms) {
  float releaseMs = std::clamp(ms, 10.0f, 1000.0f);
  releaseCoeff_ = std::exp(-1.0f / (releaseMs * sampleRate_ / 1000.0f));
}

void Compressor::setMakeupGain(float db) {
  makeupGain_ = std::pow(10.0f, std::clamp(db, 0.0f, 24.0f) / 20.0f);
}

void Compressor::reset() {
  envelope_ = 0.0f;
  gainReductionDb_ = 0.0f;
  smoothedGain_ = 1.0f;
}

float Compressor::computeGainDb(float inputDb) {
  // Soft knee implementation
  float kneeStart = thresholdDb_ - kneeDb_ / 2.0f;
  float kneeEnd = thresholdDb_ + kneeDb_ / 2.0f;

  float outputDb;

  if (inputDb < kneeStart) {
    // Below knee - no compression
    outputDb = inputDb;
  } else if (inputDb > kneeEnd) {
    // Above knee - full compression
    outputDb = thresholdDb_ + (inputDb - thresholdDb_) / ratio_;
  } else {
    // In knee region - smooth transition
    float x = inputDb - kneeStart;
    float kneeWidth = kneeDb_;
    // Quadratic interpolation for smooth knee
    float slope = (1.0f / ratio_ - 1.0f) / (2.0f * kneeWidth);
    outputDb = inputDb + slope * x * x;
  }

  return outputDb - inputDb; // Return gain reduction in dB
}

void Compressor::process(float *buffer, size_t frames) {
  if (!enabled_) {
    return;
  }

  for (size_t i = 0; i < frames; ++i) {
    float input = buffer[i];
    float level = std::abs(input);

    // Avoid log of zero
    if (level < 1e-10f) {
      buffer[i] = input * smoothedGain_ * makeupGain_;
      continue;
    }

    // Envelope follower (peak detector with attack/release)
    float coeff;
    if (level > envelope_) {
      coeff = attackCoeff_;
    } else {
      coeff = releaseCoeff_;
    }
    envelope_ = coeff * envelope_ + (1.0f - coeff) * level;

    // Compute gain reduction
    float envelopeDb = 20.0f * std::log10(envelope_);
    float gainDb = computeGainDb(envelopeDb);
    gainReductionDb_ = -gainDb; // Store as positive value

    // Convert to linear gain
    float gain = std::pow(10.0f, gainDb / 20.0f);

    // Smooth gain changes to avoid zipper noise
    float smoothCoeff = 0.99f;
    smoothedGain_ = smoothCoeff * smoothedGain_ + (1.0f - smoothCoeff) * gain;

    // Apply compression and makeup gain
    buffer[i] = input * smoothedGain_ * makeupGain_;
  }
}

} // namespace WindowsAiMic
