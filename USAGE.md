# Automaster - Usage Guide

**AI-Powered Automatic Mastering Plugin**

Automaster is an intelligent mastering suite that analyzes your audio and suggests or automatically applies appropriate mastering settings. Featuring 8-band EQ, 3-band multiband compression, stereo imaging, and a true-peak limiter, it can deliver professional results with minimal effort or serve as a sophisticated manual mastering tool.

---

## Use Cases in Modern Rock Production

### Drum Bus Processing

Not the intended use - use Bus Glue and TapeWarm for drum bus processing.

### Guitar Bus / Individual Tracks

Not recommended for individual tracks - Automaster is designed for full mixes.

### Bass Guitar

Not recommended - use NeveStrip or dedicated bass processing.

### Vocals

Not recommended - use VoxProc for vocal processing.

### Mix Bus / Mastering

This is Automaster's specialty - intelligent final stage mastering.

**Automatic Mastering - Quick and Easy:**
1. Set **Target LUFS** to your desired loudness (-14 for streaming, -9 for CD)
2. Enable **Auto Master**
3. Let Automaster analyze and process
4. Review and tweak if needed
5. Use comparison slots to A/B different versions

**Semi-Automatic Mastering:**
1. Set Target LUFS
2. Enable Auto Master for initial settings
3. Disable Auto Master
4. Tweak EQ, compression, and stereo to taste
5. Manually adjust limiting

**Manual Mastering - Rock/Alternative:**

*EQ Section:*
- HPF: 30 Hz, Enabled
- LPF: 18 kHz, Disabled (keep full highs)
- Low Shelf: 80 Hz, +1.5 dB
- High Shelf: 10 kHz, +2 dB
- Band 1: 150 Hz, +0.5 dB
- Band 2: 350 Hz, -1 dB
- Band 3: 3 kHz, +1 dB
- Band 4: 6 kHz, -0.5 dB

*Multiband Compression:*
- Low-Mid Crossover: 200 Hz
- Mid-High Crossover: 3000 Hz
- Low Band: Threshold -18 dB, Ratio 3:1, Attack 20ms, Release 200ms
- Mid Band: Threshold -16 dB, Ratio 3:1, Attack 15ms, Release 150ms
- High Band: Threshold -14 dB, Ratio 2:1, Attack 8ms, Release 100ms

*Stereo:*
- Global Width: 1.0 (100%)
- Low Width: 0.8 (slightly narrow lows)
- Mid Width: 1.0
- High Width: 1.2 (widen highs)
- Mono Bass: Enabled, 120 Hz

*Limiter:*
- Ceiling: -0.3 dB (true peak safe)
- Release: 100 ms
- Target LUFS: -14 (streaming) or -9 (CD)

**Manual Mastering - Heavy Metal:**

*EQ Section:*
- HPF: 35 Hz, Enabled
- Low Shelf: 60 Hz, +2 dB
- Band 1: 120 Hz, +1 dB (kick punch)
- Band 2: 400 Hz, -1.5 dB (clear mud)
- Band 3: 4 kHz, +1.5 dB (aggression)
- High Shelf: 8 kHz, +1 dB

*Multiband Compression:*
- Low-Mid Crossover: 180 Hz
- Mid-High Crossover: 2500 Hz
- Low Band: Threshold -20 dB, Ratio 4:1, Attack 15ms, Release 150ms
- Mid Band: Threshold -18 dB, Ratio 4:1, Attack 10ms, Release 120ms
- High Band: Threshold -16 dB, Ratio 3:1, Attack 5ms, Release 80ms

*Stereo:*
- Global Width: 1.1
- Low Width: 0.7 (tight low end)
- Mid Width: 1.1
- High Width: 1.3
- Mono Bass: Enabled, 150 Hz

*Limiter:*
- Ceiling: -0.3 dB
- Release: 80 ms
- Target LUFS: -10 to -8 (loud master)

---

## Recommended Settings

### Target LUFS Guide

| Platform/Format | Target LUFS | Notes |
|----------------|-------------|-------|
| Spotify | -14 LUFS | Normalized playback |
| Apple Music | -16 LUFS | More conservative |
| YouTube | -14 LUFS | Normalized |
| CD/Vinyl | -9 to -12 LUFS | Louder masters OK |
| Broadcast | -23 to -24 LUFS | Very dynamic |
| Club/DJ | -6 to -9 LUFS | Maximum loudness |

### Multiband Crossover Suggestions

**Rock/Pop:**
- Low-Mid: 180-250 Hz
- Mid-High: 2500-4000 Hz

**Metal:**
- Low-Mid: 150-200 Hz
- Mid-High: 2000-3000 Hz

**Acoustic/Folk:**
- Low-Mid: 200-300 Hz
- Mid-High: 3500-5000 Hz

### Stereo Width Guidelines

| Element | Suggested Width |
|---------|-----------------|
| Low frequencies | 0.7-0.9 (narrower for focus) |
| Mid frequencies | 1.0-1.2 (natural to slightly wide) |
| High frequencies | 1.1-1.4 (wider for air) |
| Mono Bass Freq | 100-150 Hz |

---

## Signal Flow Tips

### Where to Place Automaster

1. **Last in Chain**: Automaster should be the final plugin on your master bus
2. **After MasterBus**: If using MasterBus for EQ/compression, place Automaster after for limiting
3. **Dithering**: If exporting to lower bit depth, add dither after Automaster

### Using Reference Tracks

1. Load a professionally mastered reference track
2. Use Automaster's reference analysis feature
3. Auto Master will adjust settings to match the reference's tonal balance
4. Fine-tune to taste

### Using Comparison Slots

1. **Slot 1**: Auto-generated settings
2. **Slot 2**: Your manual tweaks
3. **Slot 3**: More conservative version
4. **Slot 4**: More aggressive version
5. A/B compare and choose the best

### Learning System

Automaster learns from your adjustments:
- When you modify auto-generated settings, it remembers
- Future auto masters will incorporate your preferences
- Genre-specific learning improves over time

---

## Combining with Other Plugins

### Professional Mastering Chain
1. **MasterBus** - surgical EQ and glue compression
2. **StereoImager** - precise stereo adjustments (optional)
3. **Automaster** - multiband compression, final EQ, limiting

### Quick Mastering Chain
1. **Bus Glue** - mix bus glue
2. **Automaster** - everything else

### Stem Mastering Workflow
1. Process individual stems with appropriate plugins
2. Sum stems
3. Light MasterBus EQ
4. Automaster for final limiting only (bypass EQ and compression)

---

## Quick Start Guide

**Master a rock mix in 60 seconds:**

1. Insert Automaster on your master bus (last in chain)
2. Set **Target LUFS** to -14 (streaming) or -9 (CD)
3. Play your loudest section
4. Click **Auto Master** button
5. Automaster analyzes and applies settings
6. Review the EQ curve and compression settings
7. If the low end is too hyped, reduce Low Shelf gain
8. If too harsh, reduce High Shelf or Band 3/4
9. Check **Mono Bass** is enabled (around 120 Hz)
10. Verify **Ceiling** is -0.3 dB or lower
11. A/B with bypass
12. Export!

**Manual master in 2 minutes:**

1. Insert Automaster, disable Auto Master
2. **EQ Setup:**
   - Enable HPF at 30 Hz
   - Low Shelf: 60 Hz, +1.5 dB
   - Band 2: 350 Hz, -1 dB
   - Band 3: 3 kHz, +1 dB
   - High Shelf: 10 kHz, +1.5 dB
3. **Multiband Compression:**
   - Crossovers: 200 Hz / 3000 Hz
   - All bands: Threshold -16 to -18 dB, Ratio 3:1
   - Attack: 20/15/8 ms (low/mid/high)
   - Release: 200/150/100 ms
4. **Stereo:**
   - Global Width: 1.0
   - Enable Mono Bass at 120 Hz
5. **Limiter:**
   - Ceiling: -0.3 dB
   - Target LUFS: -14 or desired loudness
6. Play through and adjust thresholds for 2-4 dB GR per band
7. Compare with reference and adjust EQ
8. Export and verify loudness with external meter

**Quick loudness check:**

1. Watch the LUFS meter during playback
2. Short-term LUFS should hover around target
3. Integrated LUFS (full song) should match target
4. True Peak should not exceed ceiling
5. If too quiet, lower thresholds; if too loud, raise them
