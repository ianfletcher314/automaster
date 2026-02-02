# Building AutoMaster: An AI-Powered Mastering Plugin

**How we built an intelligent mastering suite from scratch using JUCE and Claude**

---

## The Vision

AutoMaster started with a simple question: *What if mastering could be as easy as pressing a button, without sacrificing quality?*

Professional mastering requires years of experience to develop the ear for subtle EQ adjustments, compression ratios, and stereo imaging decisions. We set out to encode that knowledge into software that could analyze audio and make intelligent processing decisions—while keeping full manual control available for those who want it.

## The Stack

- **JUCE Framework** — Industry-standard C++ framework for audio plugins
- **gin Library** — Parameter management and UI components
- **Claude** — AI pair programming for architecture decisions and debugging
- **Xcode** — Build toolchain for AU/VST3 on macOS

## Key Challenges We Overcame

### 1. The Limiter Clipping Problem

Our first limiter implementation had a critical flaw: it was *causing* clipping rather than preventing it. The debugging journey took us deep into DSP theory:

- **Inter-sample peaks** — Standard peak detection misses peaks that occur between samples. We implemented 4x oversampling for true peak detection.
- **Hard vs. soft clipping** — Replaced harsh digital clipping with a tanh-based soft clipper that introduces subtle harmonic saturation instead of digital distortion.
- **Auto-gain anticipation** — The limiter was calculating gain reduction on the input signal, but auto-gain was applied afterward. We fixed the signal chain to anticipate the gain boost.

### 2. Preserving Dynamics

Early versions crushed the life out of tracks. A quiet verse would end up as loud as the chorus—the opposite of good mastering.

The fix: **gentler compression with higher thresholds**. Instead of compressing everything, we set thresholds that only catch the loudest peaks, preserving the natural dynamics between sections.

### 3. The "All Modules Visible" UI

Traditional plugin UIs show one section at a time. We wanted everything visible at once—EQ, compressor, stereo, limiter—like a hardware mastering chain.

This required careful layout math: calculating pixel-perfect positioning for knobs, meters, and bypass switches across four module panels, a spectrum analyzer, and I/O meters—all in a compact window.

### 4. Intelligent Analysis

The auto-master feature needed to make smart decisions:

- **Spectral analysis** — 32-band energy analysis to detect tonal imbalances
- **Dynamics analysis** — Crest factor measurement to determine how much compression is appropriate
- **Stereo analysis** — Correlation and width measurement to guide stereo imaging
- **Genre detection** — Automatic genre classification to apply appropriate processing profiles

## Architecture Highlights

```
Audio Input
    │
    ▼
┌─────────────────┐
│ Analysis Engine │ ← Spectral, Dynamics, Stereo, Loudness
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Rules Engine   │ ← Genre profiles, Reference matching
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Mastering Chain │
│  ├─ EQ          │
│  ├─ Compressor  │
│  ├─ Stereo      │
│  └─ Limiter     │
└────────┬────────┘
         │
         ▼
    Audio Output
```

## What's Next

AutoMaster is under active development. Current focus areas:

- **Waveform display** — Visual feedback of the audio being processed
- **A/B/C/D memory slots** — Store and compare different mastering approaches
- **UX refinements** — Continued polish on the interface

## Built With Claude

This project was developed using Claude as an AI pair programmer. From architecture decisions to debugging DSP math to writing this very article—Claude participated in every stage of development.

The collaboration proved especially valuable for:
- Debugging complex signal flow issues
- Iterating quickly on UI layouts with screenshot feedback
- Researching DSP techniques (oversampling, soft clipping, loudness metering)
- Code review and refactoring

---

**Fletcher Audio** · [GitHub](https://github.com/ianfletcher314/automaster)
