/**
 * WindowsAiMic - Biquad Filter Implementation
 */

#include "biquad_filter.h"
#include <cmath>

namespace WindowsAiMic {

static constexpr float PI = 3.14159265358979323846f;

BiquadFilter::BiquadFilter() = default;

void BiquadFilter::setCoefficients(float b0, float b1, float b2, float a1,
                                   float a2) {
  b0_ = b0;
  b1_ = b1;
  b2_ = b2;
  a1_ = a1;
  a2_ = a2;
}

void BiquadFilter::setLowPass(float sampleRate, float frequency, float q) {
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);

  float a0 = 1.0f + alpha;

  b0_ = ((1.0f - cosOmega) / 2.0f) / a0;
  b1_ = (1.0f - cosOmega) / a0;
  b2_ = ((1.0f - cosOmega) / 2.0f) / a0;
  a1_ = (-2.0f * cosOmega) / a0;
  a2_ = (1.0f - alpha) / a0;
}

void BiquadFilter::setHighPass(float sampleRate, float frequency, float q) {
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);

  float a0 = 1.0f + alpha;

  b0_ = ((1.0f + cosOmega) / 2.0f) / a0;
  b1_ = (-(1.0f + cosOmega)) / a0;
  b2_ = ((1.0f + cosOmega) / 2.0f) / a0;
  a1_ = (-2.0f * cosOmega) / a0;
  a2_ = (1.0f - alpha) / a0;
}

void BiquadFilter::setBandPass(float sampleRate, float frequency, float q) {
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);

  float a0 = 1.0f + alpha;

  b0_ = alpha / a0;
  b1_ = 0.0f;
  b2_ = -alpha / a0;
  a1_ = (-2.0f * cosOmega) / a0;
  a2_ = (1.0f - alpha) / a0;
}

void BiquadFilter::setNotch(float sampleRate, float frequency, float q) {
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);

  float a0 = 1.0f + alpha;

  b0_ = 1.0f / a0;
  b1_ = (-2.0f * cosOmega) / a0;
  b2_ = 1.0f / a0;
  a1_ = (-2.0f * cosOmega) / a0;
  a2_ = (1.0f - alpha) / a0;
}

void BiquadFilter::setPeak(float sampleRate, float frequency, float q,
                           float gainDb) {
  float A = std::pow(10.0f, gainDb / 40.0f);
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);

  float a0 = 1.0f + alpha / A;

  b0_ = (1.0f + alpha * A) / a0;
  b1_ = (-2.0f * cosOmega) / a0;
  b2_ = (1.0f - alpha * A) / a0;
  a1_ = (-2.0f * cosOmega) / a0;
  a2_ = (1.0f - alpha / A) / a0;
}

void BiquadFilter::setLowShelf(float sampleRate, float frequency, float gainDb,
                               float q) {
  float A = std::pow(10.0f, gainDb / 40.0f);
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);
  float sqrtA = std::sqrt(A);

  float a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha;

  b0_ = (A * ((A + 1.0f) - (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha)) / a0;
  b1_ = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega)) / a0;
  b2_ = (A * ((A + 1.0f) - (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha)) / a0;
  a1_ = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega)) / a0;
  a2_ = ((A + 1.0f) + (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha) / a0;
}

void BiquadFilter::setHighShelf(float sampleRate, float frequency, float gainDb,
                                float q) {
  float A = std::pow(10.0f, gainDb / 40.0f);
  float omega = 2.0f * PI * frequency / sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * q);
  float sqrtA = std::sqrt(A);

  float a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha;

  b0_ = (A * ((A + 1.0f) + (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha)) / a0;
  b1_ = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega)) / a0;
  b2_ = (A * ((A + 1.0f) + (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha)) / a0;
  a1_ = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega)) / a0;
  a2_ = ((A + 1.0f) - (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha) / a0;
}

float BiquadFilter::process(float input) {
  float output = b0_ * input + z1_;
  z1_ = b1_ * input - a1_ * output + z2_;
  z2_ = b2_ * input - a2_ * output;
  return output;
}

void BiquadFilter::process(float *buffer, size_t frames) {
  for (size_t i = 0; i < frames; ++i) {
    buffer[i] = process(buffer[i]);
  }
}

void BiquadFilter::reset() {
  z1_ = 0.0f;
  z2_ = 0.0f;
}

} // namespace WindowsAiMic
