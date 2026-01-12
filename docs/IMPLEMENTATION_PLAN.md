# WindowsAiMic - Implementation Plan

## Project Overview

A real-time audio processing application that creates a virtual microphone device with AI-powered enhancement (RNNoise/DeepFilterNet) plus professional DSP chain (expander, compressor, limiter, EQ).

**Audio Flow:**
```
Physical Mic → WASAPI Capture → Resampler (→48kHz) → AI Enhancement → 
Expander → Compressor → Limiter → EQ → WASAPI Render → Virtual Speaker → Virtual Mic
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Layer                                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Tray Application                       │   │
│  │  • Device selection UI                                    │   │
│  │  • Model selection (RNNoise/DeepFilterNet)               │   │
│  │  • DSP parameter controls                                 │   │
│  │  • Presets (Podcast/Meeting/Streaming)                   │   │
│  │  • Real-time meters (RMS/Peak/Gain Reduction)            │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ↓ IPC                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Audio Engine                           │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐     │   │
│  │  │ WASAPI  │→ │Resampler│→ │   AI    │→ │   DSP   │     │   │
│  │  │ Capture │  │ (to 48k)│  │ Enhance │  │  Chain  │     │   │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘     │   │
│  │       ↑                                       ↓          │   │
│  │  Physical                              ┌─────────┐       │   │
│  │  Microphone                            │ WASAPI  │       │   │
│  │                                        │ Render  │       │   │
│  │                                        └─────────┘       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ↓                                   │
└─────────────────────────────────────────────────────────────────┘
                               ↓
┌─────────────────────────────────────────────────────────────────┐
│                       Kernel Layer                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Virtual Audio Driver (WDM)                   │   │
│  │  • Virtual Speaker endpoint (render target)               │   │
│  │  • Virtual Mic endpoint (capture source)                  │   │
│  │  • Internal loopback (speaker → mic)                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ↓                                   │
│                    Windows Audio Stack                           │
│                    (Available to apps)                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Project Structure & Build System

### 1.1 Directory Structure
```
WindowsAiMic/
├── driver/                    # Virtual Audio Driver (WDM)
│   ├── src/
│   ├── inf/
│   └── CMakeLists.txt
├── engine/                    # Core Audio Processing Engine
│   ├── src/
│   │   ├── main.cpp
│   │   ├── audio/
│   │   │   ├── wasapi_capture.cpp/.h
│   │   │   ├── wasapi_render.cpp/.h
│   │   │   └── resampler.cpp/.h
│   │   ├── ai/
│   │   │   ├── rnnoise_processor.cpp/.h
│   │   │   └── deepfilter_processor.cpp/.h
│   │   ├── dsp/
│   │   │   ├── expander.cpp/.h
│   │   │   ├── compressor.cpp/.h
│   │   │   ├── limiter.cpp/.h
│   │   │   ├── equalizer.cpp/.h
│   │   │   └── metering.cpp/.h
│   │   ├── config/
│   │   │   └── config_manager.cpp/.h
│   │   └── ipc/
│   │       └── pipe_server.cpp/.h
│   ├── libs/
│   │   ├── rnnoise/           # RNNoise library
│   │   └── resampler/         # libsamplerate or similar
│   └── CMakeLists.txt
├── app/                       # Tray Application (WinUI/Qt)
│   ├── src/
│   └── CMakeLists.txt
├── installer/                 # WiX/NSIS installer
│   └── installer.wxs
├── config/
│   └── default_config.json
├── docs/
│   └── IMPLEMENTATION_PLAN.md
└── CMakeLists.txt             # Root CMake
```

### 1.2 Dependencies
- **RNNoise**: https://github.com/xiph/rnnoise (BSD-3)
- **libsamplerate**: http://www.mega-nerd.com/SRC/ (BSD-2)
- **nlohmann/json**: https://github.com/nlohmann/json (MIT)
- **Virtual Audio Driver**: https://github.com/VirtualDrivers/Virtual-Audio-Driver

---

## Phase 2: Virtual Audio Driver

### 2.1 Driver Selection
We'll use the open-source Virtual-Audio-Driver as our base:
- Already creates Virtual Speaker + Virtual Mic endpoints
- Supports 48kHz (required for RNNoise)
- Windows 10 1903+ and Windows 11 compatible

### 2.2 Driver Integration
- Clone or fork Virtual-Audio-Driver
- Build with WDK
- Test signing for development
- Plan for WHQL signing for distribution

---

## Phase 3: Audio Engine Core

### 3.1 WASAPI Capture Module
```cpp
// Event-driven shared mode capture from physical mic
class WasapiCapture {
public:
    bool Initialize(const std::wstring& deviceId);
    void Start();
    void Stop();
    void SetCallback(std::function<void(float*, size_t)> callback);
private:
    IMMDevice* device_;
    IAudioClient* audioClient_;
    IAudioCaptureClient* captureClient_;
    HANDLE audioEvent_;
    std::thread captureThread_;
};
```

### 3.2 WASAPI Render Module
```cpp
// Render to Virtual Speaker endpoint
class WasapiRender {
public:
    bool Initialize(const std::wstring& deviceId);
    void Start();
    void Stop();
    void Write(const float* data, size_t frames);
private:
    IMMDevice* device_;
    IAudioClient* audioClient_;
    IAudioRenderClient* renderClient_;
};
```

### 3.3 Resampler Module
```cpp
// Convert any sample rate to 48kHz for AI processing
class Resampler {
public:
    bool Initialize(int srcRate, int dstRate, int channels);
    std::vector<float> Process(const float* input, size_t frames);
private:
    SRC_STATE* srcState_;  // libsamplerate
    int srcRate_, dstRate_, channels_;
};
```

---

## Phase 4: AI Enhancement

### 4.1 RNNoise Processor
```cpp
// Lightweight CPU-friendly noise suppression
class RNNoiseProcessor {
public:
    bool Initialize();
    void Process(float* buffer, size_t frames);
    void SetAttenuation(float db);  // -0 to -60 dB
private:
    DenoiseState* state_;
    std::vector<float> frameBuffer_;  // 480 samples @ 48kHz
    size_t bufferPos_;
};
```

**Key constraints:**
- Expects 48kHz, mono, float32
- Processes in 480-sample frames (10ms)
- Need ring buffer for frame alignment

### 4.2 DeepFilterNet Processor (Optional High-Quality Mode)
```cpp
// Higher quality enhancement (still CPU-viable)
class DeepFilterProcessor {
public:
    bool Initialize(const std::string& modelPath);
    void Process(float* buffer, size_t frames);
    void SetEnhancementStrength(float strength);  // 0.0 - 1.0
private:
    // ONNX Runtime or custom inference
};
```

---

## Phase 5: DSP Chain

### 5.1 Noise Gate / Expander
```cpp
class Expander {
public:
    void SetThreshold(float dbThreshold);  // -60 to 0 dB
    void SetRatio(float ratio);            // 1:1 to 10:1
    void SetAttack(float ms);              // 0.1 to 100 ms
    void SetRelease(float ms);             // 10 to 1000 ms
    void SetHysteresis(float db);          // 0 to 10 dB
    void Process(float* buffer, size_t frames);
private:
    float envelope_;
    // RMS detector with attack/release
};
```

### 5.2 Compressor
```cpp
class Compressor {
public:
    void SetThreshold(float dbThreshold);  // -40 to 0 dB
    void SetRatio(float ratio);            // 1:1 to 20:1
    void SetKnee(float db);                // 0 to 12 dB (soft knee)
    void SetAttack(float ms);              // 0.1 to 100 ms
    void SetRelease(float ms);             // 10 to 1000 ms
    void SetMakeupGain(float db);          // 0 to 24 dB
    void Process(float* buffer, size_t frames);
    float GetGainReduction() const;        // For metering
private:
    float envelope_;
    float gainReduction_;
};
```

### 5.3 Limiter
```cpp
class Limiter {
public:
    void SetCeiling(float dbCeiling);      // -6 to 0 dB
    void SetRelease(float ms);             // 10 to 500 ms
    void SetLookahead(float ms);           // 0 to 10 ms
    void Process(float* buffer, size_t frames);
private:
    std::vector<float> lookaheadBuffer_;
    float envelope_;
};
```

### 5.4 Equalizer
```cpp
class Equalizer {
public:
    void SetHighPass(float freq, float q);     // 20-500 Hz
    void SetLowShelf(float freq, float gain);  // Bass boost/cut
    void SetPresence(float freq, float gain, float q);  // 2-6 kHz
    void SetHighShelf(float freq, float gain); // Air/brightness
    void SetDeEsser(float freq, float threshold); // 4-8 kHz
    void Process(float* buffer, size_t frames);
private:
    BiquadFilter highPass_, lowShelf_, presence_, highShelf_, deEsser_;
};

class BiquadFilter {
public:
    void SetCoefficients(float b0, float b1, float b2, float a1, float a2);
    float Process(float input);
private:
    float b0_, b1_, b2_, a1_, a2_;
    float x1_, x2_, y1_, y2_;
};
```

### 5.5 Metering
```cpp
class Metering {
public:
    void Process(const float* buffer, size_t frames);
    float GetPeak() const;           // dBFS
    float GetRMS() const;            // dBFS
    float GetLUFSShortTerm() const;  // LUFS
private:
    float peak_, rms_;
    // ITU-R BS.1770 filters for LUFS
};
```

---

## Phase 6: Configuration & Presets

### 6.1 Config Schema (config.json)
```json
{
  "version": 1,
  "devices": {
    "inputDevice": "{GUID}",
    "outputDevice": "{Virtual-Speaker-GUID}"
  },
  "aiModel": "rnnoise",
  "aiSettings": {
    "rnnoise": {
      "attenuation": -30
    },
    "deepfilter": {
      "strength": 0.8
    }
  },
  "expander": {
    "enabled": true,
    "threshold": -40,
    "ratio": 2.0,
    "attack": 5,
    "release": 100,
    "hysteresis": 3
  },
  "compressor": {
    "enabled": true,
    "threshold": -18,
    "ratio": 4.0,
    "knee": 6,
    "attack": 10,
    "release": 100,
    "makeupGain": 6
  },
  "limiter": {
    "enabled": true,
    "ceiling": -1,
    "release": 50,
    "lookahead": 5
  },
  "equalizer": {
    "enabled": true,
    "highPass": { "freq": 80, "q": 0.7 },
    "lowShelf": { "freq": 200, "gain": -2 },
    "presence": { "freq": 3000, "gain": 3, "q": 1.0 },
    "highShelf": { "freq": 8000, "gain": 2 },
    "deEsser": { "freq": 6000, "threshold": -20 }
  },
  "activePreset": "podcast"
}
```

### 6.2 Presets
```cpp
struct Preset {
    std::string name;
    // All DSP parameters...
};

const Preset PRESET_PODCAST = {
    .name = "Podcast",
    .expander = { .threshold = -45, .ratio = 2.5, ... },
    .compressor = { .threshold = -16, .ratio = 3.5, ... },
    // Warm, present voice with controlled dynamics
};

const Preset PRESET_MEETING = {
    .name = "Meeting",
    // Less aggressive, more natural
};

const Preset PRESET_STREAMING = {
    .name = "Streaming",
    // Punchy, broadcast-style
};
```

---

## Phase 7: Tray Application UI

### 7.1 Technology Choice
- **Option A**: WinUI 3 (modern Windows native)
- **Option B**: Qt (cross-platform, but heavier)
- **Recommendation**: WinUI 3 for native Windows feel

### 7.2 UI Components
- System tray icon with status indicator
- Popup window with:
  - Device dropdowns (input mic, output virtual device)
  - Model toggle (RNNoise / DeepFilterNet)
  - DSP chain controls (collapsible sections)
  - Real-time meters (peak, RMS, gain reduction)
  - Preset buttons
  - Bypass toggle

---

## Phase 8: IPC Between Engine & UI

### 8.1 Named Pipes
```cpp
// Engine side
class PipeServer {
public:
    void Start();
    void SendMeterUpdate(float peak, float rms, float gr);
    void OnConfigUpdate(std::function<void(const Config&)> callback);
private:
    HANDLE pipe_;
    std::thread listenerThread_;
};

// UI side
class PipeClient {
public:
    bool Connect();
    void SendConfig(const Config& config);
    void OnMeterUpdate(std::function<void(float, float, float)> callback);
};
```

---

## Phase 9: Autostart & Background Service

### 9.1 Task Scheduler Approach (Recommended)
```xml
<Task>
  <Triggers>
    <LogonTrigger>
      <Enabled>true</Enabled>
    </LogonTrigger>
  </Triggers>
  <Settings>
    <RestartOnFailure>
      <Interval>PT1M</Interval>
      <Count>3</Count>
    </RestartOnFailure>
    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
  </Settings>
  <Actions>
    <Exec>
      <Command>C:\Program Files\WindowsAiMic\engine.exe</Command>
      <Arguments>--background</Arguments>
    </Exec>
  </Actions>
</Task>
```

---

## Phase 10: Installer

### 10.1 WiX Installer Components
1. Install Virtual Audio Driver (admin, PnPUtil)
2. Install Audio Engine executable
3. Install Tray Application
4. Create scheduled task for autostart
5. Register with Add/Remove Programs
6. Create Start Menu shortcuts

---

## Implementation Order

1. **Week 1-2**: Project setup, build system, WASAPI capture/render
2. **Week 3**: RNNoise integration, resampler, basic audio pipeline
3. **Week 4**: DSP chain (expander, compressor, limiter, EQ)
4. **Week 5**: Configuration system, presets
5. **Week 6**: Tray UI, IPC
6. **Week 7**: Virtual driver integration, installer
7. **Week 8**: Testing, WHQL planning, polish

---

## Risk Mitigation

1. **Driver signing**: Start with test signing; budget time for WHQL
2. **Latency**: Target <20ms total; profile each stage
3. **CPU usage**: RNNoise is light; DeepFilterNet needs profiling
4. **Device enumeration**: Handle hot-plug gracefully
5. **Sample rate mismatches**: Always resample to 48kHz internally
