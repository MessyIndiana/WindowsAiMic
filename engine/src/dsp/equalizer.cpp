/**
 * WindowsAiMic - Equalizer Implementation
 */

#include "equalizer.h"
#include <algorithm>
#include <cmath>

namespace WindowsAiMic {

Equalizer::Equalizer() {
  // Initialize with sensible defaults for voice
  setHighPass(80.0f, 0.7f);
  setLowShelf(200.0f, 0.0f);
  setPresence(3000.0f, 0.0f, 1.0f);
  setHighShelf(8000.0f, 0.0f);
  setDeEsser(6000.0f, -20.0f);
}

void Equalizer::setHighPass(float freq, float q) {
  freq = std::clamp(freq, 20.0f, 500.0f);
  q = std::clamp(q, 0.5f, 2.0f);
  highPass_.setHighPass(sampleRate_, freq, q);
}

void Equalizer::setLowShelf(float freq, float gain) {
  freq = std::clamp(freq, 80.0f, 300.0f);
  gain = std::clamp(gain, -12.0f, 12.0f);
  lowShelf_.setLowShelf(sampleRate_, freq, gain);
}

void Equalizer::setPresence(float freq, float gain, float q) {
  freq = std::clamp(freq, 2000.0f, 6000.0f);
  gain = std::clamp(gain, -12.0f, 12.0f);
  q = std::clamp(q, 0.5f, 4.0f);
  presence_.setPeak(sampleRate_, freq, q, gain);
}

void Equalizer::setHighShelf(float freq, float gain) {
  freq = std::clamp(freq, 6000.0f, 16000.0f);
  gain = std::clamp(gain, -12.0f, 12.0f);
  highShelf_.setHighShelf(sampleRate_, freq, gain);
}

void Equalizer::setDeEsser(float freq, float threshold) {
  freq = std::clamp(freq, 4000.0f, 10000.0f);
  threshold = std::clamp(threshold, -40.0f, 0.0f);

  // Narrow band-pass for sibilance detection
  deEsserDetect_.setBandPass(sampleRate_, freq, 4.0f);
  deEsserThreshold_ = std::pow(10.0f, threshold / 20.0f);
}

void Equalizer::reset() {
  highPass_.reset();
  lowShelf_.reset();
  presence_.reset();
  highShelf_.reset();
  deEsserDetect_.reset();
  deEsserEnvelope_ = 0.0f;
}

void Equalizer::process(float *buffer, size_t frames) {
  if (!enabled_) {
    return;
  }

  // Process through EQ chain
  for (size_t i = 0; i < frames; ++i) {
    float sample = buffer[i];

    // High-pass (remove rumble)
    sample = highPass_.process(sample);

    // Low shelf (bass)
    sample = lowShelf_.process(sample);

    // Presence peak (clarity)
    sample = presence_.process(sample);

    // High shelf (air)
    sample = highShelf_.process(sample);

    // De-esser
    if (deEsserEnabled_) {
      // Detect sibilance level
      float sibilance = deEsserDetect_.process(sample);
      float sibilanceLevel = std::abs(sibilance);

      // Envelope follower for sibilance
      float coeff = sibilanceLevel > deEsserEnvelope_ ? 0.1f : 0.995f;
      deEsserEnvelope_ =
          coeff * deEsserEnvelope_ + (1.0f - coeff) * sibilanceLevel;

      // Apply gain reduction when sibilance exceeds threshold
      if (deEsserEnvelope_ > deEsserThreshold_) {
        float reduction = deEsserThreshold_ / deEsserEnvelope_;
        // Only reduce high frequencies (subtract the excess sibilance)
        sample = sample - sibilance * (1.0f - reduction);
      }
    }

    buffer[i] = sample;
  }
}

} // namespace WindowsAiMic
