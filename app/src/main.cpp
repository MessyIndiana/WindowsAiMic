/**
 * WindowsAiMic Tray App - Main Entry Point
 */

#include "tray_app.h"
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <combaseapi.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;

  // Initialize COM
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    MessageBox(nullptr, L"Failed to initialize COM", L"Error", MB_ICONERROR);
    return 1;
  }

  // Create and run tray app
  WindowsAiMic::TrayApp app(hInstance);

  if (!app.initialize()) {
    MessageBox(nullptr, L"Failed to initialize application", L"Error",
               MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  int result = app.run();

  CoUninitialize();
  return result;
}

#else
// Non-Windows stub
int main() {
  std::cerr << "Tray application is Windows-only" << std::endl;
  return 1;
}
#endif
