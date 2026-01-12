/**
 * WindowsAiMic - AI Processor Interface
 *
 * Common interface for AI-based audio enhancement processors.
 */

#pragma once

#include <cstddef>
#include <string>

namespace WindowsAiMic {

/**
 * Abstract interface for AI-based audio processors
 */
class IAIProcessor {
public:
  virtual ~IAIProcessor() = default;

  /**
   * Initialize the processor
   * @return true on success
   */
  virtual bool initialize() = 0;

  /**
   * Process audio buffer in-place
   * @param buffer Audio data (float32, mono, 48kHz expected)
   * @param frames Number of samples
   */
  virtual void process(float *buffer, size_t frames) = 0;

  /**
   * Reset processor state
   */
  virtual void reset() = 0;

  /**
   * Get processor name
   */
  virtual std::string getName() const = 0;

  /**
   * Check if processor is initialized
   */
  virtual bool isInitialized() const = 0;

  /**
   * Get expected sample rate (usually 48000)
   */
  virtual int getExpectedSampleRate() const = 0;

  /**
   * Get expected frame size (for RNNoise: 480)
   */
  virtual size_t getExpectedFrameSize() const = 0;
};

} // namespace WindowsAiMic
