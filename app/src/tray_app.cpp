/**
 * WindowsAiMic Tray App - Tray Application Implementation
 */

#include "tray_app.h"
#include "pipe_client.h"
#include "settings_window.h"

#include <iostream>

#ifdef _WIN32
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(                                                                   \
    linker,                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace WindowsAiMic {

#ifdef _WIN32

TrayApp::TrayApp(HINSTANCE hInstance) : hInstance_(hInstance) {}

TrayApp::~TrayApp() {
  removeTrayIcon();
  if (hwnd_) {
    DestroyWindow(hwnd_);
  }
  if (hContextMenu_) {
    DestroyMenu(hContextMenu_);
  }
}

bool TrayApp::initialize() {
  // Initialize common controls
  INITCOMMONCONTROLSEX icc = {};
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  // Register window class
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance_;
  wc.lpszClassName = L"WindowsAiMicTrayClass";
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  if (!RegisterClassEx(&wc)) {
    return false;
  }

  // Create hidden window for message handling
  hwnd_ = CreateWindowEx(0, L"WindowsAiMicTrayClass", L"WindowsAiMic",
                         WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr,
                         hInstance_, this);

  if (!hwnd_) {
    return false;
  }

  // Create context menu
  hContextMenu_ = CreatePopupMenu();
  AppendMenu(hContextMenu_, MF_STRING, ID_SETTINGS, L"&Settings...");
  AppendMenu(hContextMenu_, MF_SEPARATOR, 0, nullptr);
  AppendMenu(hContextMenu_, MF_STRING, ID_BYPASS, L"&Bypass Processing");
  AppendMenu(hContextMenu_, MF_SEPARATOR, 0, nullptr);

  HMENU hPresetMenu = CreatePopupMenu();
  AppendMenu(hPresetMenu, MF_STRING, ID_PRESET_PODCAST, L"&Podcast");
  AppendMenu(hPresetMenu, MF_STRING, ID_PRESET_MEETING, L"&Meeting");
  AppendMenu(hPresetMenu, MF_STRING, ID_PRESET_STREAMING, L"&Streaming");
  AppendMenu(hContextMenu_, MF_POPUP, (UINT_PTR)hPresetMenu, L"&Preset");

  AppendMenu(hContextMenu_, MF_SEPARATOR, 0, nullptr);
  AppendMenu(hContextMenu_, MF_STRING, ID_ABOUT, L"&About...");
  AppendMenu(hContextMenu_, MF_STRING, ID_EXIT, L"E&xit");

  // Check current preset
  CheckMenuRadioItem(hContextMenu_, ID_PRESET_PODCAST, ID_PRESET_STREAMING,
                     ID_PRESET_PODCAST, MF_BYCOMMAND);

  // Create tray icon
  if (!createTrayIcon()) {
    return false;
  }

  // Connect to engine via pipe
  pipeClient_ = std::make_unique<PipeClient>();
  if (pipeClient_->connect()) {
    updateTrayTooltip(L"WindowsAiMic - Connected");
  } else {
    updateTrayTooltip(L"WindowsAiMic - Engine not running");
  }

  // Create settings window (hidden initially)
  settingsWindow_ = std::make_unique<SettingsWindow>(hInstance_, hwnd_);

  running_ = true;
  return true;
}

int TrayApp::run() {
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return static_cast<int>(msg.wParam);
}

void TrayApp::quit() {
  running_ = false;
  PostQuitMessage(0);
}

bool TrayApp::createTrayIcon() {
  ZeroMemory(&nid_, sizeof(nid_));
  nid_.cbSize = sizeof(nid_);
  nid_.hWnd = hwnd_;
  nid_.uID = ID_TRAY_ICON;
  nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid_.uCallbackMessage = WM_TRAYICON;
  nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // TODO: Custom icon
  wcscpy_s(nid_.szTip, L"WindowsAiMic");

  return Shell_NotifyIcon(NIM_ADD, &nid_) == TRUE;
}

void TrayApp::removeTrayIcon() { Shell_NotifyIcon(NIM_DELETE, &nid_); }

void TrayApp::updateTrayTooltip(const std::wstring &status) {
  wcscpy_s(nid_.szTip, status.c_str());
  Shell_NotifyIcon(NIM_MODIFY, &nid_);
}

void TrayApp::showContextMenu() {
  POINT pt;
  GetCursorPos(&pt);

  SetForegroundWindow(hwnd_);
  TrackPopupMenu(hContextMenu_, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0,
                 hwnd_, nullptr);
  PostMessage(hwnd_, WM_NULL, 0, 0);
}

LRESULT CALLBACK TrayApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
  TrayApp *app = nullptr;

  if (uMsg == WM_NCCREATE) {
    CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
    app = static_cast<TrayApp *>(cs->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
  } else {
    app = reinterpret_cast<TrayApp *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  }

  if (app) {
    return app->handleMessage(uMsg, wParam, lParam);
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT TrayApp::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_TRAYICON:
    if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
      showContextMenu();
    } else if (lParam == WM_LBUTTONDBLCLK) {
      onSettings();
    }
    return 0;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case ID_SETTINGS:
      onSettings();
      break;
    case ID_BYPASS:
      onBypass();
      break;
    case ID_PRESET_PODCAST:
      onPreset("podcast");
      break;
    case ID_PRESET_MEETING:
      onPreset("meeting");
      break;
    case ID_PRESET_STREAMING:
      onPreset("streaming");
      break;
    case ID_ABOUT:
      onAbout();
      break;
    case ID_EXIT:
      onExit();
      break;
    }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hwnd_, uMsg, wParam, lParam);
}

void TrayApp::onSettings() {
  if (settingsWindow_) {
    settingsWindow_->show();
  }
}

void TrayApp::onBypass() {
  bypass_ = !bypass_;

  CheckMenuItem(hContextMenu_, ID_BYPASS, bypass_ ? MF_CHECKED : MF_UNCHECKED);

  if (pipeClient_ && pipeClient_->isConnected()) {
    pipeClient_->sendCommand("BYPASS:" + std::string(bypass_ ? "1" : "0"));
  }

  updateTrayTooltip(bypass_ ? L"WindowsAiMic - BYPASS"
                            : L"WindowsAiMic - Active");
}

void TrayApp::onPreset(const std::string &preset) {
  currentPreset_ = preset;

  int menuId = ID_PRESET_PODCAST;
  if (preset == "meeting")
    menuId = ID_PRESET_MEETING;
  else if (preset == "streaming")
    menuId = ID_PRESET_STREAMING;

  CheckMenuRadioItem(hContextMenu_, ID_PRESET_PODCAST, ID_PRESET_STREAMING,
                     menuId, MF_BYCOMMAND);

  if (pipeClient_ && pipeClient_->isConnected()) {
    pipeClient_->sendCommand("PRESET:" + preset);
  }
}

void TrayApp::onAbout() {
  MessageBox(hwnd_,
             L"WindowsAiMic v1.0.0\n\n"
             L"AI-Powered Virtual Microphone Enhancement\n\n"
             L"Features:\n"
             L"• RNNoise AI noise suppression\n"
             L"• Expander / Noise Gate\n"
             L"• Compressor with soft knee\n"
             L"• Brickwall limiter\n"
             L"• Multi-band EQ\n\n"
             L"© 2024",
             L"About WindowsAiMic", MB_ICONINFORMATION | MB_OK);
}

void TrayApp::onExit() { quit(); }

#else

TrayApp::TrayApp() = default;
TrayApp::~TrayApp() = default;
bool TrayApp::initialize() { return false; }
int TrayApp::run() { return 1; }
void TrayApp::quit() {}

#endif

} // namespace WindowsAiMic
