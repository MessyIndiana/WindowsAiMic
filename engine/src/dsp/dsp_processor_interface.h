/**
 * WindowsAiMic - DSP Processor Interface
 *
 * Common interface for DSP processors in the audio chain.
 */

#pragma once

#include <cstddef>

namespace WindowsAiMic {

/**
 * Abstract interface for DSP processors
 */
class IDSPProcessor {
public:
  virtual ~IDSPProcessor() = default;

  /**
   * Process audio buffer in-place
   * @param buffer Audio data (float32)
   * @param frames Number of samples
   */
  virtual void process(float *buffer, size_t frames) = 0;

  /**
   * Reset processor state
   */
  virtual void reset() = 0;

  /**
   * Enable/disable processor
   */
  virtual void setEnabled(bool enabled) = 0;

  /**
   * Check if processor is enabled
   */
  virtual bool isEnabled() const = 0;
};

} // namespace WindowsAiMic
