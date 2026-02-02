# Building AutoMaster
### An AI-Powered Mastering Plugin

---

## What Is AutoMaster?

AutoMaster is a complete mastering suite in a single plugin. Drop it on your master bus, hit analyze, and it intelligently applies professional mastering settings tailored to your track.

**Core modules:**
- **8-Band EQ** — HPF/LPF, low and high shelves, plus 4 parametric bands with per-band coloring
- **3-Band Multiband Compressor** — Independent compression for lows, mids, and highs with adjustable crossovers
- **Stereo Imager** — Global and per-band width control, plus mono bass for tight low end
- **True-Peak Limiter** — Transparent loudness maximization with 4x oversampled peak detection

**The auto-master workflow:**
1. Play your loudest section
2. Click **Analyze** to capture dynamics and spectral characteristics
3. Click **Auto Master** to generate optimal settings
4. Tweak to taste, or use as-is

Target any loudness standard—Spotify's -14 LUFS, Apple Music's -16 LUFS, or push it louder for club play.

## The Idea

Most mastering plugins fall into two camps: simple "one knob" tools that lack control, or complex suites that require deep expertise. We wanted something in between.

The goal was to build a plugin that could:
- **Analyze intelligently** — Understand what a track needs by measuring its spectral balance, dynamics, and stereo field
- **Suggest settings** — Generate a solid starting point based on analysis, not arbitrary presets
- **Stay transparent** — Show all controls at once so you can see exactly what's happening and tweak anything
- **Sound professional** — Match the quality of dedicated mastering hardware and premium plugins

Think of it as having a mastering engineer's first pass built into the plugin. You get an intelligent starting point, then refine from there.

## The Stack

- **JUCE Framework** — Industry-standard C++ framework for audio plugins
- **gin Library** — Parameter management and UI components
- **Xcode** — Build toolchain for AU/VST3 on macOS

---

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

---

## Architecture

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

- Waveform display
- A/B/C/D memory slots for comparing different masters
- Continued UX refinements

---

**Fletcher Audio** · [GitHub](https://github.com/ianfletcher314/automaster)
