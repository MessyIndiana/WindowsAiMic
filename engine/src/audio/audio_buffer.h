/**
 * WindowsAiMic - Audio Buffer Header
 *
 * Lock-free ring buffer for audio processing.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace WindowsAiMic {

/**
 * Lock-free single-producer single-consumer ring buffer
 * Optimized for real-time audio processing
 */
class LockFreeRingBuffer {
public:
  explicit LockFreeRingBuffer(size_t capacity);

  /**
   * Write samples to buffer
   * @param data Source data
   * @param count Number of samples
   * @return Number of samples actually written
   */
  size_t write(const float *data, size_t count);

  /**
   * Read samples from buffer
   * @param data Destination buffer
   * @param count Number of samples to read
   * @return Number of samples actually read
   */
  size_t read(float *data, size_t count);

  /**
   * Get number of samples available to read
   */
  size_t availableRead() const;

  /**
   * Get number of samples that can be written
   */
  size_t availableWrite() const;

  /**
   * Clear the buffer
   */
  void clear();

  /**
   * Get buffer capacity
   */
  size_t capacity() const { return capacity_; }

private:
  std::vector<float> buffer_;
  size_t capacity_;
  std::atomic<size_t> writePos_{0};
  std::atomic<size_t> readPos_{0};
};

} // namespace WindowsAiMic
