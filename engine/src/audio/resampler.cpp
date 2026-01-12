/**
 * WindowsAiMic - Resampler Implementation
 */

#include "resampler.h"
#include <algorithm>
#include <cmath>

namespace WindowsAiMic {

// Windowed sinc function for high-quality resampling
static double sinc(double x) {
  if (std::abs(x) < 1e-10) {
    return 1.0;
  }
  const double pi = 3.14159265358979323846;
  return std::sin(pi * x) / (pi * x);
}

// Kaiser window
static double kaiser(double n, double N, double beta) {
  // Simplified Kaiser window approximation
  double alpha = (N - 1.0) / 2.0;
  double x = (n - alpha) / alpha;
  if (std::abs(x) > 1.0)
    return 0.0;

  // I0 approximation
  double sum = 1.0;
  double term = 1.0;
  double bx2_4 =
      (beta * std::sqrt(1.0 - x * x)) * (beta * std::sqrt(1.0 - x * x)) / 4.0;
  for (int k = 1; k < 20; ++k) {
    term *= bx2_4 / (k * k);
    sum += term;
    if (term < 1e-12)
      break;
  }

  // Normalize
  double sum0 = 1.0;
  double term0 = 1.0;
  double b2_4 = beta * beta / 4.0;
  for (int k = 1; k < 20; ++k) {
    term0 *= b2_4 / (k * k);
    sum0 += term0;
    if (term0 < 1e-12)
      break;
  }

  return sum / sum0;
}

Resampler::Resampler() = default;
Resampler::~Resampler() = default;

bool Resampler::initialize(int srcRate, int dstRate, int channels) {
  srcRate_ = srcRate;
  dstRate_ = dstRate;
  channels_ = channels;
  ratio_ = static_cast<double>(srcRate) / static_cast<double>(dstRate);

  position_ = 0.0;
  lastSample_ = 0.0f;

  // For simple use cases (1:1 ratio), skip filter setup
  if (srcRate == dstRate) {
    return true;
  }

  // Build polyphase filter bank for high-quality resampling
  const int filterLength = 64;
  const int numPhases = 256;
  const double beta = 6.0; // Kaiser window parameter

  filterBank_.resize(numPhases);
  for (int phase = 0; phase < numPhases; ++phase) {
    filterBank_[phase].resize(filterLength);
    double phaseOffset = static_cast<double>(phase) / numPhases;

    double sum = 0.0;
    for (int i = 0; i < filterLength; ++i) {
      double x = i - filterLength / 2.0 + phaseOffset;
      double cutoff = (srcRate < dstRate) ? 1.0 : (1.0 / ratio_);
      double h = cutoff * sinc(x * cutoff) * kaiser(i, filterLength, beta);
      filterBank_[phase][i] = static_cast<float>(h);
      sum += h;
    }

    // Normalize
    if (sum > 0.0001) {
      for (int i = 0; i < filterLength; ++i) {
        filterBank_[phase][i] /= static_cast<float>(sum);
      }
    }
  }

  // Initialize history buffer
  historySize_ = filterLength;
  history_.resize(historySize_ * channels_, 0.0f);
  historyPos_ = 0;

  return true;
}

std::vector<float> Resampler::process(const float *input, size_t frames) {
  if (srcRate_ == dstRate_) {
    // No resampling needed
    return std::vector<float>(input, input + frames * channels_);
  }

  // Use simple linear interpolation for basic implementation
  // (Can be upgraded to polyphase filter for higher quality)

  // Calculate output size
  size_t outputFrames = static_cast<size_t>(frames / ratio_ + 1);
  std::vector<float> output;
  output.reserve(outputFrames * channels_);

  size_t inputIdx = 0;

  while (position_ < frames - 1) {
    size_t idx0 = static_cast<size_t>(position_);
    size_t idx1 = idx0 + 1;
    double frac = position_ - idx0;

    for (int ch = 0; ch < channels_; ++ch) {
      float sample0 = input[idx0 * channels_ + ch];
      float sample1 = input[idx1 * channels_ + ch];
      float interpolated =
          static_cast<float>(sample0 * (1.0 - frac) + sample1 * frac);
      output.push_back(interpolated);
    }

    position_ += ratio_;
  }

  // Adjust position for next block
  position_ -= frames;
  if (position_ < 0)
    position_ = 0;

  // Remember last sample for next block
  if (frames > 0) {
    lastSample_ = input[(frames - 1) * channels_];
  }

  return output;
}

void Resampler::reset() {
  position_ = 0.0;
  lastSample_ = 0.0f;
  std::fill(history_.begin(), history_.end(), 0.0f);
  historyPos_ = 0;
}

} // namespace WindowsAiMic
