/**
 * WindowsAiMic Tray App - Settings Window Header
 */

#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace WindowsAiMic {

/**
 * Settings/configuration window
 */
class SettingsWindow {
public:
#ifdef _WIN32
  SettingsWindow(HINSTANCE hInstance, HWND parentHwnd);
#else
  SettingsWindow();
#endif
  ~SettingsWindow();

  /**
   * Show the settings window
   */
  void show();

  /**
   * Hide the settings window
   */
  void hide();

  /**
   * Check if window is visible
   */
  bool isVisible() const;

private:
#ifdef _WIN32
  static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
  INT_PTR handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  void createControls();
  void loadSettings();
  void saveSettings();
  void updateDeviceList();

  HINSTANCE hInstance_ = nullptr;
  HWND parentHwnd_ = nullptr;
  HWND hwnd_ = nullptr;
#endif
};

} // namespace WindowsAiMic
