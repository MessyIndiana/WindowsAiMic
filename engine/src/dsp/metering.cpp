/**
 * WindowsAiMic - Metering Implementation
 */

#include "metering.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace WindowsAiMic {

Metering::Metering() : lufsBuffer_(LUFS_WINDOW_SAMPLES, 0.0f) {
  setPeakDecay(1500.0f); // 1.5 second decay
}

void Metering::setPeakDecay(float ms) {
  float decayMs = std::clamp(ms, 100.0f, 5000.0f);
  peakDecayCoeff_ = std::exp(-1.0f / (decayMs * sampleRate_ / 1000.0f));
}

void Metering::reset() {
  peak_ = 0.0f;
  peakDb_ = -96.0f;
  rms_ = 0.0f;
  rmsDb_ = -96.0f;
  rmsSum_ = 0.0f;
  rmsCount_ = 0;
  lufs_ = -70.0f;
  std::fill(lufsBuffer_.begin(), lufsBuffer_.end(), 0.0f);
  lufsPos_ = 0;
}

void Metering::process(const float *buffer, size_t frames) {
  float blockPeak = 0.0f;
  float blockSum = 0.0f;

  for (size_t i = 0; i < frames; ++i) {
    float sample = buffer[i];
    float absSample = std::abs(sample);

    // Update block peak
    blockPeak = std::max(blockPeak, absSample);

    // Update RMS sum (squared samples)
    float squared = sample * sample;
    blockSum += squared;

    // Update LUFS buffer
    lufsBuffer_[lufsPos_] = squared;
    lufsPos_ = (lufsPos_ + 1) % lufsBuffer_.size();
  }

  // Update peak with decay
  if (blockPeak > peak_) {
    peak_ = blockPeak;
  } else {
    // Apply decay
    peak_ *= std::pow(peakDecayCoeff_, static_cast<float>(frames));
  }
  peakDb_ = peak_ > 1e-10f ? 20.0f * std::log10(peak_) : -96.0f;

  // Update RMS (rolling average)
  rmsSum_ += blockSum;
  rmsCount_ += frames;

  if (rmsCount_ >= RMS_WINDOW_SAMPLES) {
    rms_ = std::sqrt(rmsSum_ / static_cast<float>(rmsCount_));
    rmsDb_ = rms_ > 1e-10f ? 20.0f * std::log10(rms_) : -96.0f;

    // Reset for next window (partial overlap)
    rmsSum_ = blockSum;
    rmsCount_ = frames;
  }

  // Update LUFS (simplified - just uses mean square over window)
  // Full ITU-R BS.1770 would include K-weighting filter
  float lufsSum = std::accumulate(lufsBuffer_.begin(), lufsBuffer_.end(), 0.0f);
  float meanSquare = lufsSum / static_cast<float>(lufsBuffer_.size());

  // LUFS = -0.691 + 10 * log10(meanSquare) for unweighted
  // Simplified version without K-weighting
  if (meanSquare > 1e-10f) {
    lufs_ = -0.691f + 10.0f * std::log10(meanSquare);
  } else {
    lufs_ = -70.0f;
  }
}

} // namespace WindowsAiMic
