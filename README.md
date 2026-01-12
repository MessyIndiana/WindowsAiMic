# WindowsAiMic

**AI-Powered Virtual Microphone Enhancement for Windows**

Transform your regular microphone into a broadcast-quality audio source with AI-powered noise suppression and professional DSP processing.

![WindowsAiMic Architecture](docs/architecture.png)

## Features

### AI Enhancement
- **RNNoise** - Lightweight neural network noise suppression (CPU-friendly)
- **DeepFilterNet** (optional) - High-quality full-band speech enhancement

### Professional DSP Chain
- **Expander/Noise Gate** - Eliminate background noise during pauses
- **Compressor** - Consistent voice levels with soft knee and makeup gain
- **Brickwall Limiter** - Prevent clipping with optional lookahead
- **Multi-band EQ** - Shape your voice with high-pass, shelves, and presence
- **De-esser** - Tame harsh sibilance

### Additional Features
- **Real-time Metering** - Peak, RMS, and gain reduction display
- **Presets** - Podcast, Meeting, and Streaming optimized settings
- **System Tray** - Minimal footprint with quick access
- **Low Latency** - <20ms total processing delay
- **Virtual Audio Device** - Appears as a standard Windows microphone

## Requirements

- Windows 10 (1903+) or Windows 11
- x64 processor (ARM64 experimental)
- 4GB RAM minimum
- Any USB or built-in microphone

## Installation

### End Users
1. Download the latest installer from [Releases](https://github.com/your-repo/releases)
2. Run the installer as Administrator
3. Reboot when prompted (required for driver installation)
4. Select "Virtual Mic Driver" as your input in applications

### Developers

See **Building from Source** below.

## Quick Start

1. Launch WindowsAiMic from the Start Menu
2. Right-click the tray icon → Select your input microphone
3. Choose a preset (Podcast/Meeting/Streaming)
4. In your application (Teams/Discord/OBS), select "Virtual Mic Driver" as input

## Audio Flow

```
Physical Microphone
        ↓
  WASAPI Capture
        ↓
  Resampler (→48kHz)
        ↓
┌───────────────────┐
│   AI Enhancement  │
│ (RNNoise/DeepFilter) │
└───────────────────┘
        ↓
    Expander
        ↓
       EQ
        ↓
   Compressor
        ↓
    Limiter
        ↓
  WASAPI Render
        ↓
 Virtual Speaker
        ↓
   Virtual Mic
        ↓
  Applications
```

## Building from Source

### Prerequisites

| Requirement | Version |
|-------------|---------|
| Windows 10/11 | 1903+ |
| Visual Studio | 2022 |
| Windows SDK | 10.0.19041.0+ |
| Windows Driver Kit (WDK) | Matching SDK |
| CMake | 3.20+ |
| Git | Latest |

### Build Steps

```powershell
# Clone repository
git clone https://github.com/your-repo/WindowsAiMic.git
cd WindowsAiMic

# Clone RNNoise library
git clone https://github.com/xiph/rnnoise.git engine/libs/rnnoise

# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Output will be in build/bin/
```

### Building the Driver

The virtual audio driver requires the Windows Driver Kit:

```powershell
cd driver
msbuild VirtualAudioDriver.sln /p:Configuration=Release /p:Platform=x64
```

**Note:** Driver needs to be test-signed for development, or WHQL-signed for distribution.

## Configuration

Settings are stored in `%APPDATA%\WindowsAiMic\config.json`:

```json
{
  "aiModel": "rnnoise",
  "expander": {
    "enabled": true,
    "threshold": -40,
    "ratio": 2.0
  },
  "compressor": {
    "enabled": true,
    "threshold": -18,
    "ratio": 4.0,
    "makeupGain": 6
  }
}
```

See [Configuration Reference](docs/configuration.md) for all options.

## Presets

| Preset | Description |
|--------|-------------|
| **Podcast** | Warm, present voice with controlled dynamics |
| **Meeting** | Natural, less aggressive processing |
| **Streaming** | Punchy, broadcast-style sound |

## Troubleshooting

### Virtual Mic not appearing
- Ensure driver is installed (check Device Manager → Sound devices)
- Try reinstalling with administrator privileges
- Check if test signing is enabled for unsigned drivers

### Audio crackling/dropouts
- Increase buffer size in Settings
- Close other audio applications
- Update audio drivers

### High CPU usage
- Switch from DeepFilterNet to RNNoise
- Disable unused DSP stages

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      User Layer                              │
│   ┌────────────────┐        ┌────────────────────────────┐  │
│   │  Tray App (UI) │←─IPC──→│      Audio Engine          │  │
│   └────────────────┘        └────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                                        │
                                        ↓
┌─────────────────────────────────────────────────────────────┐
│                     Kernel Layer                             │
│              Virtual Audio Driver (WDM)                      │
│   ┌─────────────────┐    ┌─────────────────┐                │
│   │ Virtual Speaker │───→│   Virtual Mic   │                │
│   │   (Render)      │    │   (Capture)     │                │
│   └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

## Third-Party Libraries

- [RNNoise](https://github.com/xiph/rnnoise) - BSD-3 License
- [Virtual-Audio-Driver](https://github.com/VirtualDrivers/Virtual-Audio-Driver) - MIT License

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## Acknowledgments

- Jean-Marc Valin and the Xiph.Org Foundation for RNNoise
- The Virtual-Audio-Driver project for the kernel driver base
- The audio programming community for DSP algorithm references
