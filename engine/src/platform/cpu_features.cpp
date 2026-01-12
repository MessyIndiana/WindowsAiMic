/**
 * WindowsAiMic - CPU Features Detection Implementation
 */

#include "cpu_features.h"
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#include <powerbase.h>
#pragma comment(lib, "powrprof.lib")
#else
#include <cpuid.h>
#endif

namespace WindowsAiMic {

CPUFeatures CPUFeatures::instance_;
bool CPUFeatures::initialized_ = false;

void CPUFeatures::initialize() {
  if (!initialized_) {
    instance_.detect();
    initialized_ = true;
  }
}

const CPUFeatures &CPUFeatures::get() {
  if (!initialized_) {
    initialize();
  }
  return instance_;
}

void CPUFeatures::detect() {
  // Get CPU info using CPUID
  int cpuInfo[4] = {0};

#ifdef _WIN32
  __cpuid(cpuInfo, 0);
#else
  __cpuid(0, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

  int maxId = cpuInfo[0];

  // Get vendor string
  char vendorStr[13] = {0};
  *reinterpret_cast<int *>(vendorStr) = cpuInfo[1];
  *reinterpret_cast<int *>(vendorStr + 4) = cpuInfo[3];
  *reinterpret_cast<int *>(vendorStr + 8) = cpuInfo[2];
  vendor_ = vendorStr;

  isIntel_ = (vendor_ == "GenuineIntel");

  // Get feature flags
  if (maxId >= 1) {
#ifdef _WIN32
    __cpuid(cpuInfo, 1);
#else
    __cpuid(1, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

    sse_ = (cpuInfo[3] & (1 << 25)) != 0;
    sse2_ = (cpuInfo[3] & (1 << 26)) != 0;
    sse3_ = (cpuInfo[2] & (1 << 0)) != 0;
    sse41_ = (cpuInfo[2] & (1 << 19)) != 0;
    sse42_ = (cpuInfo[2] & (1 << 20)) != 0;
    avx_ = (cpuInfo[2] & (1 << 28)) != 0;
    fma_ = (cpuInfo[2] & (1 << 12)) != 0;
  }

  // Check for AVX2 and AVX-512
  if (maxId >= 7) {
#ifdef _WIN32
    __cpuidex(cpuInfo, 7, 0);
#else
    __cpuid_count(7, 0, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

    avx2_ = (cpuInfo[1] & (1 << 5)) != 0;
    avx512_ = (cpuInfo[1] & (1 << 16)) != 0; // AVX-512F
  }

  // Get brand string
  char brandStr[49] = {0};
#ifdef _WIN32
  __cpuid(cpuInfo, 0x80000000);
#else
  __cpuid(0x80000000, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

  if (static_cast<unsigned>(cpuInfo[0]) >= 0x80000004) {
    for (int i = 0; i < 3; ++i) {
#ifdef _WIN32
      __cpuid(cpuInfo, 0x80000002 + i);
#else
      __cpuid(0x80000002 + i, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
      memcpy(brandStr + i * 16, cpuInfo, 16);
    }
    brand_ = brandStr;
  }

  // Get core counts
  logicalCores_ = std::thread::hardware_concurrency();

#ifdef _WIN32
  DWORD length = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

  if (length > 0) {
    std::vector<char> buffer(length);
    if (GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                buffer.data()),
            &length)) {

      physicalCores_ = 0;
      char *ptr = buffer.data();
      while (ptr < buffer.data() + length) {
        auto *info =
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
        if (info->Relationship == RelationProcessorCore) {
          physicalCores_++;
        }
        ptr += info->Size;
      }
    }
  }
#else
  physicalCores_ = logicalCores_ / 2; // Approximate
#endif

  // Detect Intel hybrid architecture
  if (isIntel_) {
    detectIntelHybrid();
  }

  // Detect NPU
  detectNPU();

  // Log detected features
  std::cout << "CPU: " << brand_ << std::endl;
  std::cout << "  Cores: " << physicalCores_ << " physical, " << logicalCores_
            << " logical" << std::endl;
  if (isHybrid_) {
    std::cout << "  Hybrid: " << pCores_ << " P-cores, " << eCores_
              << " E-cores" << std::endl;
  }
  std::cout << "  SIMD: ";
  if (avx512_)
    std::cout << "AVX-512 ";
  else if (avx2_)
    std::cout << "AVX2 ";
  else if (avx_)
    std::cout << "AVX ";
  else if (sse42_)
    std::cout << "SSE4.2 ";
  if (fma_)
    std::cout << "FMA ";
  std::cout << std::endl;

  if (hasNPU_) {
    std::cout << "  NPU: Intel Neural Processing Unit detected" << std::endl;
  }
}

void CPUFeatures::detectIntelHybrid() {
  // Intel Core Ultra processors (Meteor Lake) have hybrid architecture
  // Check for hybrid architecture support

  int cpuInfo[4] = {0};

#ifdef _WIN32
  // Check if CPU supports hybrid info leaf
  __cpuid(cpuInfo, 0);
  int maxId = cpuInfo[0];

  if (maxId >= 0x1A) {
    __cpuidex(cpuInfo, 0x1A, 0);

    // Core type is in bits 31:24
    // If we can query this, it's a hybrid CPU
    int coreType = (cpuInfo[0] >> 24) & 0xFF;

    if (coreType != 0) {
      isHybrid_ = true;

      // Count P-cores and E-cores
      // This requires iterating through all processors
      DWORD length = 0;
      GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

      if (length > 0) {
        std::vector<char> buffer(length);
        if (GetLogicalProcessorInformationEx(
                RelationProcessorCore,
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                    buffer.data()),
                &length)) {

          // For now, estimate based on typical Intel Core Ultra 7 config
          // The 165U has: 2 P-cores + 8 E-cores + 2 LP E-cores
          // We'll use thread counts as approximation
          pCores_ = 2;
          eCores_ = 8;
        }
      }
    }
  }
#endif

  // Fallback: Check brand string for "Core Ultra" which indicates Meteor Lake
  if (!isHybrid_ && brand_.find("Core") != std::string::npos &&
      brand_.find("Ultra") != std::string::npos) {
    isHybrid_ = true;

    // Intel Core Ultra 7 165U: 2P + 8E + 2LP-E = 12 cores, 14 threads
    if (brand_.find("165U") != std::string::npos) {
      pCores_ = 2;
      eCores_ = 10; // 8E + 2LP-E
    }
  }
}

void CPUFeatures::detectNPU() {
  // Intel Core Ultra processors include an NPU (Intel AI Boost)
  // Detection via brand string for now, as there's no standard CPUID for NPU

  if (isIntel_ && brand_.find("Ultra") != std::string::npos) {
    // All Intel Core Ultra processors have NPU
    hasNPU_ = true;
  }

#ifdef _WIN32
  // Additional check: Look for OpenVINO-compatible NPU device
  // This would require DirectML or OpenVINO API calls
  // For now, rely on brand string detection
#endif
}

int CPUFeatures::recommendedBufferSize() const {
  // Smaller buffer = lower latency, but needs faster CPU
  // Intel Core Ultra 7 can handle very small buffers

  if (hasNPU_ || (isHybrid_ && pCores_ >= 2)) {
    return 128; // ~2.7ms at 48kHz - very low latency
  } else if (avx2_) {
    return 256; // ~5.3ms at 48kHz
  } else if (avx_) {
    return 480; // 10ms - RNNoise native
  } else {
    return 512; // ~10.7ms - safe default
  }
}

int CPUFeatures::recommendedThreadCount() const {
  if (isHybrid_) {
    // Use P-cores for audio processing
    return pCores_ > 0 ? pCores_ : 2;
  } else {
    // Use half of logical cores
    return std::max(1, logicalCores_ / 2);
  }
}

bool CPUFeatures::shouldUseNPU() const {
  // Use NPU if available - offloads AI to dedicated hardware
  return hasNPU_;
}

bool CPUFeatures::shouldUseAVX512() const {
  // AVX-512 can cause frequency throttling on some CPUs
  // Intel Core Ultra (Meteor Lake) handles it well
  return avx512_ && isIntel_ && brand_.find("Ultra") != std::string::npos;
}

} // namespace WindowsAiMic
