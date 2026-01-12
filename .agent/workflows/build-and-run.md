---
description: How to build and run the WindowsAiMic application
---

# Build and Run Workflow

This project is developed on macOS but runs on Windows. 

## 🚀 One-Click Install (Personal Use)

// turbo-all

### Transfer to Windows
Copy the entire `WindowsAiMic` folder to your Windows PC.

### Run Quick Setup
Open **PowerShell as Administrator** and run:

```powershell
cd C:\Path\To\WindowsAiMic
Set-ExecutionPolicy Bypass -Scope Process -Force
.\scripts\quick-setup.ps1
```

This handles EVERYTHING:
- ✅ Installs build tools (CMake, VS Build Tools)
- ✅ Downloads RNNoise and Virtual Audio Driver
- ✅ Builds the project
- ✅ Creates distributable package
- ✅ Runs the installer

### Reboot
If prompted to enable test signing, reboot your PC.

---

## 📦 Using the Pre-Built Package

After initial build, use the ZIP at `dist/WindowsAiMic-Release.zip`:

1. Copy ZIP to target PC
2. Extract
3. Double-click `Install.bat`
4. Reboot if prompted

---

## 🔧 Manual Build (Development)

### Prerequisites
- Windows 10/11 (1903+) with admin rights
- Visual Studio 2022 (C++ workload)
- Windows SDK + WDK
- CMake 3.20+
- Git

### Build Commands
```powershell
# Clone dependencies
git clone https://github.com/xiph/rnnoise.git engine/libs/rnnoise
git clone https://github.com/VirtualDrivers/Virtual-Audio-Driver.git driver

# Build
.\scripts\build.ps1 -Configuration Release -Package

# Install
.\scripts\install.ps1
```

---

## 📋 Post-Install Usage

1. **Tray Icon**: Look for microphone icon in system tray
2. **Right-click**: Access presets (Podcast/Meeting/Streaming)
3. **In Apps**: Select "Virtual Mic Driver" as your input device

---

## 🔄 Uninstall

```powershell
.\scripts\install.ps1 -Uninstall
```
Or double-click `Uninstall.bat` in the package folder.

---

## ⚠️ Notes

- Uses **test signing** (displays "Test Mode" watermark on desktop)
- Secure Boot may need to be disabled in BIOS
- Driver creates Virtual Speaker + Virtual Mic endpoints
- RNNoise requires 48kHz/mono/480-sample frames
