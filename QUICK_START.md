# WindowsAiMic - Quick Start Guide

## 🎯 One-Click Install (Fresh Windows PC)

**No build tools required!** Just download the pre-built package and run.

---

## Step 1: Get the Pre-Built Package

### Option A: Download from GitHub (Recommended)

1. Go to [Releases](../../releases) page
2. Download `WindowsAiMic-Release.zip`
3. Extract to a folder (e.g., `C:\WindowsAiMic`)

### Option B: Build Once, Use Anywhere

If you have a machine with build tools, run:
```powershell
.\scripts\quick-setup.ps1
```
This creates `dist\WindowsAiMic-Release.zip` - copy this ZIP to any fresh Windows PC.

---

## Step 2: Install (One Click!)

1. **Right-click `Install.bat`** → **Run as Administrator**
2. Click **Yes** when prompted
3. When asked about **VB-Cable**, click **Yes** to download (it's free!)
4. **Reboot** if prompted
5. Done! Look for the **tray icon** 🎤

That's it! No Visual Studio, no CMake, no build tools needed.

---

## About VB-Cable (Virtual Audio Device)

WindowsAiMic needs a "virtual audio cable" to route audio:
- **VB-Cable** is a FREE, pre-signed driver from [vb-audio.com](https://vb-audio.com/Cable/)
- The installer will offer to download it automatically
- If you already have VB-Cable, it will be detected and used

### How Audio Flows:
```
Your Mic → WindowsAiMic Engine → VB-Cable (Virtual Speaker)
                                        ↓
                                  (Internal Loop)
                                        ↓
Teams/Discord/Zoom ← VB-Cable (Virtual Mic) ← AI Enhanced Audio
```

---

## Step 3: Configure Your Apps

In **Teams, Discord, Zoom, OBS**, etc.:
1. Go to **Audio/Sound Settings**
2. Select **"CABLE Output (VB-Audio Virtual Cable)"** as your microphone
3. Your voice is now AI-enhanced! ✨

> 💡 **Tip**: The audio engine processes your mic and outputs to "CABLE Input".
> Apps then pick up the processed audio from "CABLE Output".

---

## 🎛️ Using WindowsAiMic

### Tray Icon Menu (Right-Click)
| Option | What it does |
|--------|--------------|
| **Settings** | Open configuration window |
| **Bypass** | Temporarily disable all processing |
| **Preset → Podcast** | Warm, broadcast-quality voice |
| **Preset → Meeting** | Natural, less processing |
| **Preset → Streaming** | Punchy, radio-style |
| **Exit** | Close the application |

### Audio Processing Chain
Your voice goes through:
```
Mic → AI Noise Removal → Gate → EQ → Compressor → Limiter → Virtual Mic
```

---

## 🔧 Troubleshooting

### "Virtual Mic Driver" not appearing?
- **Reboot your PC** (required after first install)
- Check Device Manager → Sound devices
- Ensure Test Mode is enabled (check bottom-right of screen)

### Test Mode won't enable?
1. Restart PC and enter **BIOS/UEFI**
2. Find **Secure Boot** and **disable** it
3. Boot back to Windows and run Install.bat again

### No sound or crackling?
- Make sure the right input mic is selected in WindowsAiMic settings
- Try a different USB port if using USB microphone
- Close other audio applications

### Want to uninstall?
Right-click `Uninstall.bat` → Run as Administrator

---

## 📁 What Gets Installed

| Component | Location |
|-----------|----------|
| Engine | `C:\Program Files\WindowsAiMic\` |
| Tray App | `C:\Program Files\WindowsAiMic\` |
| Config | `%APPDATA%\WindowsAiMic\config.json` |
| Driver | Windows Driver Store |
| Startup | Task Scheduler (auto-starts at login) |

---

## 🚀 For Developers: Building from Source

If you want to modify the code:

### Prerequisites (on a build machine)
- Visual Studio 2022 with C++ workload
- CMake 3.20+
- Git

### Build Commands
```powershell
# Full automated build
.\scripts\quick-setup.ps1

# Or manual build
.\scripts\build.ps1 -Configuration Release -Package
```

### Intel Core Ultra 7 165U Optimizations
This project is optimized for your CPU:
- **P-core scheduling** for audio thread
- **AVX2** SIMD instructions
- Optional **NPU acceleration** with OpenVINO

---

## ❓ FAQ

**Q: What's the "Test Mode" watermark on my screen?**
A: This appears because the driver isn't signed with an expensive certificate. It's completely safe - just a visual reminder that you're running test-signed drivers.

**Q: Is this safe to use?**
A: Yes! The code is open source. The test signing mode is used by developers and hobbyists worldwide.

**Q: Can I remove the watermark?**
A: Only by purchasing a code signing certificate ($200+/year) and signing the driver. For personal use, most people just ignore it.

**Q: Does this work with all apps?**
A: Any app that uses a standard Windows microphone will work - Teams, Zoom, Discord, OBS, games, etc.
