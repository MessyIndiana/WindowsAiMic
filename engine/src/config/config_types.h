/**
 * WindowsAiMic - Configuration Types Header
 *
 * Type definitions for application configuration.
 */

#pragma once

#include <string>

namespace WindowsAiMic {

struct HighPassConfig {
  float freq = 80.0f;
  float q = 0.7f;
};

struct ShelfConfig {
  float freq = 200.0f;
  float gain = 0.0f;
};

struct PresenceConfig {
  float freq = 3000.0f;
  float gain = 0.0f;
  float q = 1.0f;
};

struct DeEsserConfig {
  float freq = 6000.0f;
  float threshold = -20.0f;
};

struct ExpanderConfig {
  bool enabled = true;
  float threshold = -40.0f; // dB
  float ratio = 2.0f;
  float attack = 5.0f;     // ms
  float release = 100.0f;  // ms
  float hysteresis = 3.0f; // dB
};

struct CompressorConfig {
  bool enabled = true;
  float threshold = -18.0f; // dB
  float ratio = 4.0f;
  float knee = 6.0f;       // dB
  float attack = 10.0f;    // ms
  float release = 100.0f;  // ms
  float makeupGain = 6.0f; // dB
};

struct LimiterConfig {
  bool enabled = true;
  float ceiling = -1.0f;  // dB
  float release = 50.0f;  // ms
  float lookahead = 5.0f; // ms
};

struct EqualizerConfig {
  bool enabled = true;
  HighPassConfig highPass;
  ShelfConfig lowShelf;
  PresenceConfig presence;
  ShelfConfig highShelf;
  DeEsserConfig deEsser;
  bool deEsserEnabled = false;
};

struct RNNoiseSettings {
  float attenuation = -30.0f; // dB
};

struct DeepFilterSettings {
  std::string modelPath = "";
  float strength = 0.8f;
};

struct AISettings {
  RNNoiseSettings rnnoise;
  DeepFilterSettings deepfilter;
};

struct DevicesConfig {
  std::wstring inputDevice = L"";  // Empty = default
  std::wstring outputDevice = L""; // Virtual Speaker device ID
};

struct Config {
  int version = 1;
  DevicesConfig devices;
  std::string aiModel = "rnnoise"; // "rnnoise" or "deepfilter"
  AISettings aiSettings;
  ExpanderConfig expander;
  CompressorConfig compressor;
  LimiterConfig limiter;
  EqualizerConfig equalizer;
  std::string activePreset = "podcast";
};

} // namespace WindowsAiMic
