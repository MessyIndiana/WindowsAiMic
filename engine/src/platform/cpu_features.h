/**
 * WindowsAiMic - CPU Features Detection
 *
 * Detects CPU capabilities for optimal code paths.
 * Optimized for Intel Core Ultra series processors.
 */

#pragma once

#include <string>
#include <vector>

namespace WindowsAiMic {

/**
 * CPU feature detection and optimization hints
 */
class CPUFeatures {
public:
  /**
   * Initialize CPU detection (call once at startup)
   */
  static void initialize();

  /**
   * Get singleton instance
   */
  static const CPUFeatures &get();

  // SIMD capabilities
  bool hasSSE() const { return sse_; }
  bool hasSSE2() const { return sse2_; }
  bool hasSSE3() const { return sse3_; }
  bool hasSSE41() const { return sse41_; }
  bool hasSSE42() const { return sse42_; }
  bool hasAVX() const { return avx_; }
  bool hasAVX2() const { return avx2_; }
  bool hasAVX512() const { return avx512_; }
  bool hasFMA() const { return fma_; }

  // Intel-specific
  bool isIntel() const { return isIntel_; }
  bool isHybrid() const { return isHybrid_; } // P-core + E-core
  bool hasNPU() const { return hasNPU_; }     // Neural Processing Unit

  // Core counts
  int physicalCores() const { return physicalCores_; }
  int logicalCores() const { return logicalCores_; }
  int performanceCores() const { return pCores_; }
  int efficiencyCores() const { return eCores_; }

  // CPU info
  std::string vendor() const { return vendor_; }
  std::string brand() const { return brand_; }

  // Recommended settings based on detected CPU
  int recommendedBufferSize() const;
  int recommendedThreadCount() const;
  bool shouldUseNPU() const;
  bool shouldUseAVX512() const;

private:
  CPUFeatures() = default;
  void detect();
  void detectIntelHybrid();
  void detectNPU();

  static CPUFeatures instance_;
  static bool initialized_;

  // SIMD flags
  bool sse_ = false;
  bool sse2_ = false;
  bool sse3_ = false;
  bool sse41_ = false;
  bool sse42_ = false;
  bool avx_ = false;
  bool avx2_ = false;
  bool avx512_ = false;
  bool fma_ = false;

  // Intel features
  bool isIntel_ = false;
  bool isHybrid_ = false;
  bool hasNPU_ = false;

  // Core topology
  int physicalCores_ = 1;
  int logicalCores_ = 1;
  int pCores_ = 0;
  int eCores_ = 0;

  // Strings
  std::string vendor_;
  std::string brand_;
};

} // namespace WindowsAiMic
