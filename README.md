# Automaster

**AI-Powered Automatic Mastering Plugin**

A professional mastering suite that intelligently analyzes your audio and applies appropriate mastering settings. Features 8-band EQ, 3-band multiband compression, stereo imaging, and a true-peak limiter.

![Fletcher Audio](https://img.shields.io/badge/Fletcher-Audio-blue)
![macOS](https://img.shields.io/badge/macOS-AU%20%7C%20VST3-green)
![License](https://img.shields.io/badge/license-Proprietary-red)

## Features

- **Intelligent Auto-Mastering** - Analyzes your mix and suggests optimal settings
- **8-Band Parametric EQ** - Surgical control with HPF/LPF and shelving bands
- **3-Band Multiband Compression** - Independent compression per frequency band
- **Stereo Imaging** - Per-band width control with mono bass option
- **True-Peak Limiter** - Transparent limiting with ceiling control
- **Target LUFS** - Master to streaming standards (-14 LUFS) or louder formats
- **Comparison Slots** - A/B up to 4 different mastering versions
- **Learning System** - Adapts to your preferences over time

## Installation

### macOS

1. Download the latest release
2. Copy `Automaster.component` to `~/Library/Audio/Plug-Ins/Components/`
3. Copy `Automaster.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
4. Restart your DAW

## Quick Start

1. Insert Automaster on your master bus (last in chain)
2. Set **Target LUFS** to -14 (streaming) or -9 (CD)
3. Play your loudest section
4. Click **Auto Master** - Automaster analyzes and applies settings
5. Review and tweak to taste
6. Export!

## Target LUFS Guide

| Platform | Target LUFS |
|----------|-------------|
| Spotify | -14 LUFS |
| Apple Music | -16 LUFS |
| YouTube | -14 LUFS |
| CD/Vinyl | -9 to -12 LUFS |
| Club/DJ | -6 to -9 LUFS |

## Building from Source

### Requirements

- [JUCE Framework](https://juce.com/) (v7.0+)
- Xcode 14+ (macOS)
- C++17 compatible compiler

### Build Steps

```bash
# Clone the repository
git clone https://github.com/ianfletcher314/automaster.git
cd automaster

# Open in Projucer and save to generate Xcode project
# Or build directly:
cd Builds/MacOSX
xcodebuild -scheme "Automaster - AU" -configuration Release build
```

## Documentation

See [USAGE.md](USAGE.md) for detailed usage instructions and recommended settings for rock/metal production.

## Author

**Fletcher Audio**
- GitHub: [@ianfletcher314](https://github.com/ianfletcher314)

## License

Proprietary - All rights reserved.

---

*Part of the Fletcher Audio plugin suite: Bus Glue, TapeWarm, MasterBus, Automaster, VoxProc, Ducker, Dynoverb, StereoImager, NeveStrip, PDLBRD*
