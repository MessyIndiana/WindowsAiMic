/**
 * WindowsAiMic - Thread Utilities
 *
 * Thread management optimized for Intel hybrid architecture (P-core/E-core).
 */

#pragma once

#include <string>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace WindowsAiMic {

/**
 * Thread priority levels
 */
enum class ThreadPriority {
  Low,     // Background tasks
  Normal,  // Default
  High,    // Time-sensitive
  Realtime // Audio processing - use with care
};

/**
 * Core type preference for Intel hybrid CPUs
 */
enum class CorePreference {
  Any,         // Let scheduler decide
  Performance, // Prefer P-cores
  Efficiency   // Prefer E-cores
};

/**
 * Set thread priority
 */
inline bool setThreadPriority(std::thread &thread, ThreadPriority priority) {
#ifdef _WIN32
  int winPriority = THREAD_PRIORITY_NORMAL;

  switch (priority) {
  case ThreadPriority::Low:
    winPriority = THREAD_PRIORITY_BELOW_NORMAL;
    break;
  case ThreadPriority::Normal:
    winPriority = THREAD_PRIORITY_NORMAL;
    break;
  case ThreadPriority::High:
    winPriority = THREAD_PRIORITY_HIGHEST;
    break;
  case ThreadPriority::Realtime:
    winPriority = THREAD_PRIORITY_TIME_CRITICAL;
    break;
  }

  return SetThreadPriority(thread.native_handle(), winPriority) != 0;
#else
  (void)thread;
  (void)priority;
  return false;
#endif
}

/**
 * Set current thread priority
 */
inline bool setCurrentThreadPriority(ThreadPriority priority) {
#ifdef _WIN32
  int winPriority = THREAD_PRIORITY_NORMAL;

  switch (priority) {
  case ThreadPriority::Low:
    winPriority = THREAD_PRIORITY_BELOW_NORMAL;
    break;
  case ThreadPriority::Normal:
    winPriority = THREAD_PRIORITY_NORMAL;
    break;
  case ThreadPriority::High:
    winPriority = THREAD_PRIORITY_HIGHEST;
    break;
  case ThreadPriority::Realtime:
    winPriority = THREAD_PRIORITY_TIME_CRITICAL;
    break;
  }

  return SetThreadPriority(GetCurrentThread(), winPriority) != 0;
#else
  (void)priority;
  return false;
#endif
}

/**
 * Set thread core preference (Intel hybrid architecture)
 * Uses Windows 11 Thread Director hints
 */
inline bool setThreadCorePreference(CorePreference preference) {
#ifdef _WIN32
  // Windows 11 22H2+ supports SetThreadInformation with ThreadPowerThrottling
  // which influences the scheduler's P-core/E-core decisions

  THREAD_POWER_THROTTLING_STATE throttle = {};
  throttle.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;

  switch (preference) {
  case CorePreference::Performance:
    // Disable power throttling = prefer P-cores
    throttle.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    throttle.StateMask = 0; // Don't throttle
    break;

  case CorePreference::Efficiency:
    // Enable power throttling = prefer E-cores
    throttle.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    throttle.StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    break;

  case CorePreference::Any:
  default:
    // Let OS decide
    throttle.ControlMask = 0;
    throttle.StateMask = 0;
    break;
  }

  return SetThreadInformation(GetCurrentThread(), ThreadPowerThrottling,
                              &throttle, sizeof(throttle)) != 0;
#else
  (void)preference;
  return false;
#endif
}

/**
 * Set thread for multimedia/audio work
 * Registers with MMCSS (Multimedia Class Scheduler Service)
 */
inline void *
setThreadMultimediaMode(const std::string &taskName = "Pro Audio") {
#ifdef _WIN32
  DWORD taskIndex = 0;
  std::wstring wTaskName(taskName.begin(), taskName.end());
  HANDLE hTask = AvSetMmThreadCharacteristicsW(wTaskName.c_str(), &taskIndex);

  if (hTask) {
    // Optionally boost priority within the task
    AvSetMmThreadPriority(hTask, AVRT_PRIORITY_CRITICAL);
  }

  return hTask;
#else
  (void)taskName;
  return nullptr;
#endif
}

/**
 * Revert multimedia mode
 */
inline void revertMultimediaMode(void *taskHandle) {
#ifdef _WIN32
  if (taskHandle) {
    AvRevertMmThreadCharacteristics(static_cast<HANDLE>(taskHandle));
  }
#else
  (void)taskHandle;
#endif
}

/**
 * Set thread name (for debugging)
 */
inline void setThreadName(const std::string &name) {
#ifdef _WIN32
  std::wstring wname(name.begin(), name.end());
  SetThreadDescription(GetCurrentThread(), wname.c_str());
#else
  (void)name;
#endif
}

/**
 * RAII wrapper for multimedia thread mode
 */
class MultimediaThreadScope {
public:
  explicit MultimediaThreadScope(const std::string &taskName = "Pro Audio")
      : handle_(setThreadMultimediaMode(taskName)) {}

  ~MultimediaThreadScope() { revertMultimediaMode(handle_); }

  bool isActive() const { return handle_ != nullptr; }

private:
  void *handle_;
};

/**
 * RAII wrapper for P-core preference
 */
class PerformanceCoreScope {
public:
  PerformanceCoreScope() {
    setThreadCorePreference(CorePreference::Performance);
  }

  ~PerformanceCoreScope() { setThreadCorePreference(CorePreference::Any); }
};

} // namespace WindowsAiMic
