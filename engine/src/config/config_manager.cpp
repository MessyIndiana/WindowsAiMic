/**
 * WindowsAiMic - Configuration Manager Implementation
 */

#include "config_manager.h"
#include <fstream>
#include <iostream>
#include <sstream>

// Simple JSON handling without external dependency
// In production, use nlohmann/json
namespace {
std::string trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\n\r");
  size_t end = str.find_last_not_of(" \t\n\r");
  if (start == std::string::npos)
    return "";
  return str.substr(start, end - start + 1);
}
} // namespace

namespace WindowsAiMic {

ConfigManager::ConfigManager() { loadDefaults(); }

void ConfigManager::loadDefaults() {
  std::lock_guard<std::mutex> lock(mutex_);

  config_ = Config{};

  // Default devices (empty = system default)
  config_.devices.inputDevice = L"";
  config_.devices.outputDevice = L"";

  // Default AI model
  config_.aiModel = "rnnoise";
  config_.aiSettings.rnnoise.attenuation = -30.0f;
  config_.aiSettings.deepfilter.strength = 0.8f;

  // Default expander (noise gate)
  config_.expander.enabled = true;
  config_.expander.threshold = -40.0f;
  config_.expander.ratio = 2.0f;
  config_.expander.attack = 5.0f;
  config_.expander.release = 100.0f;
  config_.expander.hysteresis = 3.0f;

  // Default compressor
  config_.compressor.enabled = true;
  config_.compressor.threshold = -18.0f;
  config_.compressor.ratio = 4.0f;
  config_.compressor.knee = 6.0f;
  config_.compressor.attack = 10.0f;
  config_.compressor.release = 100.0f;
  config_.compressor.makeupGain = 6.0f;

  // Default limiter
  config_.limiter.enabled = true;
  config_.limiter.ceiling = -1.0f;
  config_.limiter.release = 50.0f;
  config_.limiter.lookahead = 5.0f;

  // Default EQ
  config_.equalizer.enabled = true;
  config_.equalizer.highPass = {80.0f, 0.7f};
  config_.equalizer.lowShelf = {200.0f, 0.0f};
  config_.equalizer.presence = {3000.0f, 2.0f, 1.0f};
  config_.equalizer.highShelf = {8000.0f, 1.0f};
  config_.equalizer.deEsser = {6000.0f, -20.0f};
  config_.equalizer.deEsserEnabled = false;

  config_.activePreset = "podcast";
}

bool ConfigManager::load(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Could not open config file: " << path << std::endl;
    return false;
  }

  configPath_ = path;

  // Read entire file
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Very basic JSON parsing - in production use nlohmann/json
  // For now, just load defaults and return true to indicate file exists
  std::cout << "Config file found: " << path << std::endl;
  std::cout << "Using default configuration (full JSON parsing requires "
               "nlohmann/json)"
            << std::endl;

  loadDefaults();
  return true;
}

bool ConfigManager::save(const std::string &path) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ofstream file(path);
  if (!file.is_open()) {
    std::cerr << "Could not open config file for writing: " << path
              << std::endl;
    return false;
  }

  // Write JSON manually (in production use nlohmann/json)
  file << "{\n";
  file << "  \"version\": " << config_.version << ",\n";
  file << "  \"aiModel\": \"" << config_.aiModel << "\",\n";
  file << "  \"activePreset\": \"" << config_.activePreset << "\",\n";

  // AI Settings
  file << "  \"aiSettings\": {\n";
  file << "    \"rnnoise\": { \"attenuation\": "
       << config_.aiSettings.rnnoise.attenuation << " },\n";
  file << "    \"deepfilter\": { \"strength\": "
       << config_.aiSettings.deepfilter.strength << " }\n";
  file << "  },\n";

  // Expander
  file << "  \"expander\": {\n";
  file << "    \"enabled\": " << (config_.expander.enabled ? "true" : "false")
       << ",\n";
  file << "    \"threshold\": " << config_.expander.threshold << ",\n";
  file << "    \"ratio\": " << config_.expander.ratio << ",\n";
  file << "    \"attack\": " << config_.expander.attack << ",\n";
  file << "    \"release\": " << config_.expander.release << ",\n";
  file << "    \"hysteresis\": " << config_.expander.hysteresis << "\n";
  file << "  },\n";

  // Compressor
  file << "  \"compressor\": {\n";
  file << "    \"enabled\": " << (config_.compressor.enabled ? "true" : "false")
       << ",\n";
  file << "    \"threshold\": " << config_.compressor.threshold << ",\n";
  file << "    \"ratio\": " << config_.compressor.ratio << ",\n";
  file << "    \"knee\": " << config_.compressor.knee << ",\n";
  file << "    \"attack\": " << config_.compressor.attack << ",\n";
  file << "    \"release\": " << config_.compressor.release << ",\n";
  file << "    \"makeupGain\": " << config_.compressor.makeupGain << "\n";
  file << "  },\n";

  // Limiter
  file << "  \"limiter\": {\n";
  file << "    \"enabled\": " << (config_.limiter.enabled ? "true" : "false")
       << ",\n";
  file << "    \"ceiling\": " << config_.limiter.ceiling << ",\n";
  file << "    \"release\": " << config_.limiter.release << ",\n";
  file << "    \"lookahead\": " << config_.limiter.lookahead << "\n";
  file << "  },\n";

  // Equalizer
  file << "  \"equalizer\": {\n";
  file << "    \"enabled\": " << (config_.equalizer.enabled ? "true" : "false")
       << ",\n";
  file << "    \"highPass\": { \"freq\": " << config_.equalizer.highPass.freq
       << ", \"q\": " << config_.equalizer.highPass.q << " },\n";
  file << "    \"lowShelf\": { \"freq\": " << config_.equalizer.lowShelf.freq
       << ", \"gain\": " << config_.equalizer.lowShelf.gain << " },\n";
  file << "    \"presence\": { \"freq\": " << config_.equalizer.presence.freq
       << ", \"gain\": " << config_.equalizer.presence.gain
       << ", \"q\": " << config_.equalizer.presence.q << " },\n";
  file << "    \"highShelf\": { \"freq\": " << config_.equalizer.highShelf.freq
       << ", \"gain\": " << config_.equalizer.highShelf.gain << " },\n";
  file << "    \"deEsser\": { \"freq\": " << config_.equalizer.deEsser.freq
       << ", \"threshold\": " << config_.equalizer.deEsser.threshold << " },\n";
  file << "    \"deEsserEnabled\": "
       << (config_.equalizer.deEsserEnabled ? "true" : "false") << "\n";
  file << "  }\n";

  file << "}\n";

  return true;
}

Config ConfigManager::getConfig() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return config_;
}

void ConfigManager::applyConfig(const Config &config) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
  }

  if (changeCallback_) {
    changeCallback_(config);
  }

  // Auto-save
  if (!configPath_.empty()) {
    save(configPath_);
  }
}

void ConfigManager::setChangeCallback(ConfigChangeCallback callback) {
  changeCallback_ = std::move(callback);
}

} // namespace WindowsAiMic
