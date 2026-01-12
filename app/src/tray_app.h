/**
 * WindowsAiMic Tray App - Tray Application Header
 */

#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

#include <atomic>
#include <memory>
#include <string>

namespace WindowsAiMic {

class PipeClient;
class SettingsWindow;

/**
 * System tray application
 *
 * Creates a system tray icon and manages the settings UI.
 */
class TrayApp {
public:
#ifdef _WIN32
  explicit TrayApp(HINSTANCE hInstance);
#else
  TrayApp();
#endif
  ~TrayApp();

  /**
   * Initialize the application
   */
  bool initialize();

  /**
   * Run the message loop
   * @return Exit code
   */
  int run();

  /**
   * Request application exit
   */
  void quit();

private:
#ifdef _WIN32
  // Window procedure
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
  LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  // Tray icon management
  bool createTrayIcon();
  void removeTrayIcon();
  void showContextMenu();
  void updateTrayTooltip(const std::wstring &status);

  // Menu commands
  void onSettings();
  void onBypass();
  void onPreset(const std::string &preset);
  void onAbout();
  void onExit();

  HINSTANCE hInstance_ = nullptr;
  HWND hwnd_ = nullptr;
  NOTIFYICONDATA nid_ = {};
  HMENU hContextMenu_ = nullptr;
#endif

  std::unique_ptr<PipeClient> pipeClient_;
  std::unique_ptr<SettingsWindow> settingsWindow_;
  std::atomic<bool> running_{false};
  bool bypass_ = false;
  std::string currentPreset_ = "podcast";

  // Menu item IDs
  static constexpr int ID_TRAY_ICON = 1;
  static constexpr int ID_SETTINGS = 100;
  static constexpr int ID_BYPASS = 101;
  static constexpr int ID_PRESET_PODCAST = 102;
  static constexpr int ID_PRESET_MEETING = 103;
  static constexpr int ID_PRESET_STREAMING = 104;
  static constexpr int ID_ABOUT = 105;
  static constexpr int ID_EXIT = 106;

  static constexpr UINT WM_TRAYICON = WM_USER + 1;
};

} // namespace WindowsAiMic
