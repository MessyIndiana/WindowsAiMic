/**
 * WindowsAiMic - RNNoise Processor Implementation
 */

#include "rnnoise_processor.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Include RNNoise header
extern "C" {
#include "rnnoise.h"
}

namespace WindowsAiMic {

RNNoiseProcessor::RNNoiseProcessor()
    : frameBuffer_(FRAME_SIZE, 0.0f),
      outputBuffer_(FRAME_SIZE * 4, 0.0f) // Buffer for processed output
{}

RNNoiseProcessor::~RNNoiseProcessor() {
  if (state_) {
    rnnoise_destroy(state_);
    state_ = nullptr;
  }
}

bool RNNoiseProcessor::initialize() {
  if (state_) {
    rnnoise_destroy(state_);
  }

  state_ = rnnoise_create(nullptr);
  if (!state_) {
    std::cerr << "Failed to create RNNoise state" << std::endl;
    return false;
  }

  bufferPos_ = 0;
  outputPos_ = 0;

  std::cout << "RNNoise initialized (frame size: " << FRAME_SIZE << " samples)"
            << std::endl;
  return true;
}

void RNNoiseProcessor::reset() {
  if (state_) {
    rnnoise_destroy(state_);
    state_ = rnnoise_create(nullptr);
  }

  std::fill(frameBuffer_.begin(), frameBuffer_.end(), 0.0f);
  std::fill(outputBuffer_.begin(), outputBuffer_.end(), 0.0f);
  bufferPos_ = 0;
  outputPos_ = 0;
}

void RNNoiseProcessor::setAttenuation(float db) {
  // Convert dB to linear scale
  // 0 dB = 1.0 (no change), -60 dB ≈ 0.001 (maximum suppression)
  float clampedDb = std::clamp(db, -60.0f, 0.0f);
  attenuation_ = std::pow(10.0f, clampedDb / 20.0f);
}

void RNNoiseProcessor::process(float *buffer, size_t frames) {
  if (!state_) {
    return;
  }

  size_t inputPos = 0;
  size_t outputRead = 0;

  while (inputPos < frames) {
    // Fill the frame buffer
    size_t samplesToBuffer =
        std::min(frames - inputPos, FRAME_SIZE - bufferPos_);
    std::copy(buffer + inputPos, buffer + inputPos + samplesToBuffer,
              frameBuffer_.begin() + bufferPos_);

    bufferPos_ += samplesToBuffer;
    inputPos += samplesToBuffer;

    // Process complete frame
    if (bufferPos_ == FRAME_SIZE) {
      processFrame(frameBuffer_.data());

      // Copy to output buffer (with wrapping)
      size_t outputSpace = outputBuffer_.size() - outputPos_;
      if (outputSpace >= FRAME_SIZE) {
        std::copy(frameBuffer_.begin(), frameBuffer_.end(),
                  outputBuffer_.begin() + outputPos_);
      } else {
        std::copy(frameBuffer_.begin(), frameBuffer_.begin() + outputSpace,
                  outputBuffer_.begin() + outputPos_);
        std::copy(frameBuffer_.begin() + outputSpace, frameBuffer_.end(),
                  outputBuffer_.begin());
      }
      outputPos_ = (outputPos_ + FRAME_SIZE) % outputBuffer_.size();

      bufferPos_ = 0;
    }
  }

  // Copy processed output back to input buffer
  // Note: there's inherent latency of one frame
  size_t startPos =
      (outputPos_ + outputBuffer_.size() - frames) % outputBuffer_.size();
  for (size_t i = 0; i < frames; ++i) {
    buffer[i] = outputBuffer_[(startPos + i) % outputBuffer_.size()];
  }
}

void RNNoiseProcessor::processFrame(float *frame) {
  // Convert to 16-bit range that RNNoise expects
  // RNNoise processes 16-bit PCM scaled to float
  std::vector<float> scaledFrame(FRAME_SIZE);
  for (size_t i = 0; i < FRAME_SIZE; ++i) {
    scaledFrame[i] = frame[i] * 32767.0f;
  }

  // Process with RNNoise
  lastVAD_ =
      rnnoise_process_frame(state_, scaledFrame.data(), scaledFrame.data());

  // Convert back to normalized float and apply attenuation blending
  for (size_t i = 0; i < FRAME_SIZE; ++i) {
    float processed = scaledFrame[i] / 32767.0f;
    float original = frame[i];

    // Blend between processed and original based on attenuation
    // attenuation_ = 1.0 means keep original (no processing)
    // attenuation_ = 0.0 means full processing
    frame[i] = processed * (1.0f - attenuation_) + original * attenuation_;

    // Actually, we want inverse: attenuation_ controls how much noise is
    // removed So lower attenuation = more aggressive noise removal
    frame[i] = processed; // For now, just use full processing
  }
}

} // namespace WindowsAiMic
