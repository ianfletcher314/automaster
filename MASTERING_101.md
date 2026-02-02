# Mastering 101: A Beginner's Guide

This guide explains the basics of audio mastering and how Automaster works.

---

## What is Mastering?

Mastering is the final step before releasing music. It makes your track:
- **Louder** (competitive with other songs)
- **Polished** (EQ, compression, stereo enhancement)
- **Ready for streaming** (correct loudness and peak levels)

---

## Understanding Loudness (LUFS)

### What is LUFS?

LUFS = **Loudness Units relative to Full Scale**

It measures how loud your track *sounds* to human ears, not just the peak levels.

| Measurement | What it means |
|-------------|---------------|
| **Peak (dB)** | The loudest single sample (technical limit) |
| **LUFS** | Perceived loudness (how loud it *feels*) |

### Why LUFS Matters

Two tracks can have the same peak level but sound completely different in loudness:

```
Track A: Peak at 0dB, LUFS at -20 (quiet, lots of dynamics)
Track B: Peak at 0dB, LUFS at -8  (loud, compressed/limited)
```

Streaming platforms use LUFS to normalize everything to the same perceived loudness.

---

## Streaming Platform Targets

| Platform | Target LUFS | Peak Limit |
|----------|-------------|------------|
| **Spotify** | -14 LUFS | -1 dBTP |
| **Apple Music** | -16 LUFS | -1 dBTP |
| **YouTube** | -14 LUFS | -1 dBTP |
| **Amazon Music** | -14 LUFS | -2 dBTP |
| **Tidal** | -14 LUFS | -1 dBTP |

**Automaster defaults:** -14 LUFS with -0.3 dBTP ceiling (works for all platforms)

### What Happens If You're Too Loud?

If your track is louder than the platform's target:
- The platform turns it DOWN
- You don't sound louder than other songs
- You might have more distortion for no benefit

### What Happens If You're Too Quiet?

If your track is quieter than the platform's target:
- The platform turns it UP
- Your track sounds as loud as others
- This is actually fine! (No penalty)

---

## The LUFS Meter in Automaster

The LUFS meter shows a letter and a number:

### The Letter (Display Mode)

| Letter | Mode | What it measures |
|--------|------|------------------|
| **M** | Momentary | Instant loudness (400ms window) - jumps around a lot |
| **S** | Short-term | 3-second average - good for watching in real-time |
| **I** | Integrated | Whole song average - **this is what Spotify uses** |

### The Number (Difference from Target)

The number in the corner shows how far you are from your target:

| Reading | Meaning | Status |
|---------|---------|--------|
| **0** | Exactly at target (-14 LUFS) | Perfect |
| **+2** | 2dB louder than target (-12 LUFS) | Slightly hot |
| **-1** | 1dB quieter than target (-15 LUFS) | Still great |
| **-5** | 5dB quieter than target (-19 LUFS) | Quiet section (normal) |

### The Color

| Color | Meaning |
|-------|---------|
| **Green** | Within 1dB of target (good!) |
| **Yellow/Orange** | A bit off from target |
| **Other** | Further from target |

---

## Dynamic Range (It's OK to Have Quiet Parts!)

A song is NOT supposed to be loud all the time. Music has dynamics:

```
Intro (quiet)     ████░░░░░░░░░░░░░░  -20 LUFS
Verse             ████████░░░░░░░░░░  -16 LUFS
Chorus (loud)     ██████████████░░░░  -12 LUFS
Breakdown         ██████░░░░░░░░░░░░  -18 LUFS
Final Chorus      ████████████████░░  -10 LUFS
                  ────────────────────
Integrated (avg)  ████████████░░░░░░  -14 LUFS ← This is what matters!
```

**Seeing -10 or -12 on the meter during a quiet part is NORMAL and GOOD.**

What matters is the **Integrated** (whole song) average.

---

## What Automaster Does

When you click **Auto Master**, the plugin:

### 1. Analyzes Your Track
- Measures the current loudness (LUFS)
- Detects peaks

### 2. Applies Processing
- **EQ**: Balances frequencies (bass, mids, highs)
- **Compression**: Glues the mix together, controls dynamics
- **Stereo Enhancement**: Widens the stereo image, mono bass for club systems
- **Limiting**: Catches peaks, prevents clipping

### 3. Hits the Target
- Calculates how much gain is needed to reach -14 LUFS
- Applies that gain (up to +6dB max)
- Limiter prevents any peaks from exceeding the ceiling (-0.3dB)

---

## The Signal Flow

```
Your Mix
    ↓
[Auto Headroom] ← Reduces hot inputs to create processing room
    ↓
[EQ] ← Tonal balance
    ↓
[Compressor] ← Dynamics control
    ↓
[Stereo Imager] ← Width and mono bass
    ↓
[Limiter] ← Loudness + peak control
    ↓
Final Master (-14 LUFS, peaks < -0.3dB)
```

---

## Tips for Best Results

### Before Mastering

1. **Leave headroom**: Your mix should peak around -3dB to -6dB
2. **No limiting on the master bus**: Let Automaster do the limiting
3. **Fix problems in the mix**: Mastering can't fix a bad mix

### Using Automaster

1. Click **Auto Master** and let it analyze
2. Watch the LUFS meter - should hover around 0 (green)
3. Listen! Does it sound good? Clear? Punchy?
4. If it sounds distorted, your input might be too hot

### After Mastering

1. Bounce/export your track
2. Listen on different speakers (headphones, car, phone)
3. Compare to reference tracks in the same genre

---

## Troubleshooting

| Problem | Likely Cause | Solution |
|---------|--------------|----------|
| Sounds distorted | Input too hot | Lower your mix level before Automaster |
| Not loud enough | Target LUFS too conservative | Check target is set to -14 |
| Too loud/squashed | Too much gain | Reduce input level or target LUFS |
| LUFS meter shows -10 | Quiet part of song | Normal! Check integrated LUFS |

---

## Glossary

| Term | Definition |
|------|------------|
| **LUFS** | Loudness Units relative to Full Scale - perceived loudness measurement |
| **dBTP** | Decibels True Peak - measures inter-sample peaks |
| **Ceiling** | Maximum allowed peak level |
| **Gain Reduction (GR)** | How much the limiter is reducing peaks |
| **Integrated LUFS** | Average loudness over entire playback |
| **Short-term LUFS** | 3-second rolling average |
| **Momentary LUFS** | 400ms instant reading |
| **Headroom** | Space between your peaks and 0dB |
| **Limiting** | Preventing peaks from exceeding a threshold |
| **True Peak** | Actual peak including inter-sample peaks |

---

## Summary

1. **Target: -14 LUFS** for streaming platforms
2. **Ceiling: -0.3 dB** to prevent clipping
3. **Green = Good** on the LUFS meter
4. **Quiet parts are normal** - only integrated LUFS matters
5. **Leave headroom** in your mix for best results

Happy mastering!
