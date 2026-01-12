/**
 * WindowsAiMic - Audio Buffer Implementation
 */

#include "audio_buffer.h"
#include <algorithm>
#include <cstring>

namespace WindowsAiMic {

LockFreeRingBuffer::LockFreeRingBuffer(size_t capacity)
    : buffer_(capacity + 1) // +1 to distinguish full from empty
      ,
      capacity_(capacity) {}

size_t LockFreeRingBuffer::write(const float *data, size_t count) {
  const size_t currentWrite = writePos_.load(std::memory_order_relaxed);
  const size_t currentRead = readPos_.load(std::memory_order_acquire);

  // Calculate available space
  size_t available;
  if (currentWrite >= currentRead) {
    available = capacity_ - (currentWrite - currentRead);
  } else {
    available = currentRead - currentWrite - 1;
  }

  const size_t toWrite = std::min(count, available);
  if (toWrite == 0) {
    return 0;
  }

  // Write data
  const size_t bufferSize = buffer_.size();
  size_t firstPart = std::min(toWrite, bufferSize - currentWrite);
  std::memcpy(&buffer_[currentWrite], data, firstPart * sizeof(float));

  if (firstPart < toWrite) {
    std::memcpy(&buffer_[0], data + firstPart,
                (toWrite - firstPart) * sizeof(float));
  }

  // Update write position
  size_t newWrite = (currentWrite + toWrite) % bufferSize;
  writePos_.store(newWrite, std::memory_order_release);

  return toWrite;
}

size_t LockFreeRingBuffer::read(float *data, size_t count) {
  const size_t currentRead = readPos_.load(std::memory_order_relaxed);
  const size_t currentWrite = writePos_.load(std::memory_order_acquire);

  // Calculate available data
  size_t available;
  if (currentWrite >= currentRead) {
    available = currentWrite - currentRead;
  } else {
    available = buffer_.size() - currentRead + currentWrite;
  }

  const size_t toRead = std::min(count, available);
  if (toRead == 0) {
    return 0;
  }

  // Read data
  const size_t bufferSize = buffer_.size();
  size_t firstPart = std::min(toRead, bufferSize - currentRead);
  std::memcpy(data, &buffer_[currentRead], firstPart * sizeof(float));

  if (firstPart < toRead) {
    std::memcpy(data + firstPart, &buffer_[0],
                (toRead - firstPart) * sizeof(float));
  }

  // Update read position
  size_t newRead = (currentRead + toRead) % bufferSize;
  readPos_.store(newRead, std::memory_order_release);

  return toRead;
}

size_t LockFreeRingBuffer::availableRead() const {
  const size_t currentRead = readPos_.load(std::memory_order_acquire);
  const size_t currentWrite = writePos_.load(std::memory_order_acquire);

  if (currentWrite >= currentRead) {
    return currentWrite - currentRead;
  } else {
    return buffer_.size() - currentRead + currentWrite;
  }
}

size_t LockFreeRingBuffer::availableWrite() const {
  const size_t currentWrite = writePos_.load(std::memory_order_acquire);
  const size_t currentRead = readPos_.load(std::memory_order_acquire);

  if (currentWrite >= currentRead) {
    return capacity_ - (currentWrite - currentRead);
  } else {
    return currentRead - currentWrite - 1;
  }
}

void LockFreeRingBuffer::clear() {
  readPos_.store(0, std::memory_order_release);
  writePos_.store(0, std::memory_order_release);
}

} // namespace WindowsAiMic
