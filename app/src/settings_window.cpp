/**
 * WindowsAiMic Tray App - Settings Window Implementation
 */

#include "settings_window.h"

#ifdef _WIN32
#include <CommCtrl.h>
#endif

namespace WindowsAiMic {

#ifdef _WIN32

SettingsWindow::SettingsWindow(HINSTANCE hInstance, HWND parentHwnd)
    : hInstance_(hInstance), parentHwnd_(parentHwnd) {}

SettingsWindow::~SettingsWindow() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
  }
}

void SettingsWindow::show() {
  if (!hwnd_) {
    // Create window if it doesn't exist
    // In a full implementation, this would create a proper dialog
    // with all the DSP controls

    // For now, show a simple message
    MessageBox(parentHwnd_,
               L"Settings Window\n\n"
               L"Full implementation would include:\n"
               L"• Input device selection\n"
               L"• Output device selection\n"
               L"• AI model selection (RNNoise/DeepFilter)\n"
               L"• Expander controls\n"
               L"• Compressor controls\n"
               L"• Limiter controls\n"
               L"• EQ controls\n"
               L"• Real-time meters\n"
               L"• Preset management",
               L"WindowsAiMic Settings", MB_ICONINFORMATION | MB_OK);
    return;
  }

  ShowWindow(hwnd_, SW_SHOW);
  SetForegroundWindow(hwnd_);
}

void SettingsWindow::hide() {
  if (hwnd_) {
    ShowWindow(hwnd_, SW_HIDE);
  }
}

bool SettingsWindow::isVisible() const {
  return hwnd_ && IsWindowVisible(hwnd_);
}

// Stubs for full implementation
void SettingsWindow::createControls() {}
void SettingsWindow::loadSettings() {}
void SettingsWindow::saveSettings() {}
void SettingsWindow::updateDeviceList() {}

INT_PTR CALLBACK SettingsWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                            LPARAM lParam) {
  SettingsWindow *window = nullptr;

  if (uMsg == WM_INITDIALOG) {
    window = reinterpret_cast<SettingsWindow *>(lParam);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    window->hwnd_ = hwnd;
  } else {
    window = reinterpret_cast<SettingsWindow *>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));
  }

  if (window) {
    return window->handleMessage(uMsg, wParam, lParam);
  }

  return FALSE;
}

INT_PTR SettingsWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  (void)lParam;

  switch (uMsg) {
  case WM_INITDIALOG:
    createControls();
    loadSettings();
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      saveSettings();
      hide();
      return TRUE;
    case IDCANCEL:
      hide();
      return TRUE;
    }
    break;

  case WM_CLOSE:
    hide();
    return TRUE;
  }

  return FALSE;
}

#else

SettingsWindow::SettingsWindow() = default;
SettingsWindow::~SettingsWindow() = default;
void SettingsWindow::show() {}
void SettingsWindow::hide() {}
bool SettingsWindow::isVisible() const { return false; }

#endif

} // namespace WindowsAiMic
