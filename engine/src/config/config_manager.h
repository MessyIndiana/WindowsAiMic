/**
 * WindowsAiMic - Configuration Manager Header
 *
 * Handles loading, saving, and managing application configuration.
 */

#pragma once

#include "config_types.h"
#include <functional>
#include <mutex>
#include <string>

namespace WindowsAiMic {

/**
 * Configuration manager
 *
 * Loads/saves configuration from JSON file and provides
 * thread-safe access to configuration values.
 */
class ConfigManager {
public:
  ConfigManager();

  /**
   * Load configuration from file
   * @param path Path to JSON config file
   * @return true on success
   */
  bool load(const std::string &path);

  /**
   * Save configuration to file
   * @param path Path to JSON config file
   * @return true on success
   */
  bool save(const std::string &path) const;

  /**
   * Load default configuration
   */
  void loadDefaults();

  /**
   * Get current configuration (thread-safe copy)
   */
  Config getConfig() const;

  /**
   * Apply new configuration
   * @param config New configuration to apply
   */
  void applyConfig(const Config &config);

  /**
   * Set callback for configuration changes
   */
  using ConfigChangeCallback = std::function<void(const Config &)>;
  void setChangeCallback(ConfigChangeCallback callback);

  /**
   * Get path to config file
   */
  std::string getConfigPath() const { return configPath_; }

private:
  Config config_;
  std::string configPath_;
  mutable std::mutex mutex_;
  ConfigChangeCallback changeCallback_;
};

} // namespace WindowsAiMic
