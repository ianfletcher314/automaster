# Ozone-Style Workflow Implementation Plan

## Current Workflow (What Happens Now)

### When You Click "Auto Master":
1. `triggerAutoMaster()` is called
2. It grabs `analysisEngine.getResults()` - the **current moment's** analysis
3. `rulesEngine.generateParameters(results)` creates settings based on that snapshot
4. Settings are applied immediately

### Problem:
- Analysis is **real-time only** - no accumulation over time
- Clicking Auto Master at different points = different results
- Not comparing your full track to the reference - just comparing the current instant

---

## Desired Workflow (Ozone-Style)

### Step 1: Load Reference Track
- User loads a professionally mastered song
- Plugin **scans the entire file** and stores a profile (spectral, dynamics, loudness, stereo)
- **We already have this!** `ReferenceProfile::loadFromFile()` does exactly this

### Step 2: Analyze Your Track
- User clicks "Start Analysis"
- User plays the **loudest section** (chorus) for 8-10 seconds
- Plugin **accumulates** analysis data over this period
- User clicks "Stop Analysis" (or it auto-stops after X seconds)
- **This is what we're missing**

### Step 3: Generate Master
- Plugin compares **accumulated analysis** vs **reference profile**
- Generates EQ/Comp/Stereo/Limiter settings to make your track sound more like reference
- **We have the comparison logic** in `ParameterGenerator::generateFromReference()`

---

## What We Already Have

### Reference Profile System (`ReferenceProfile.h`)
- Loads audio files and analyzes them
- Extracts: spectral envelope, loudness, crest factor, stereo width/correlation
- Genre presets built-in
- Match score calculation

### Analysis Engine (`AnalysisEngine.h`)
- SpectralAnalyzer - frequency content
- DynamicsAnalyzer - crest factor, peak, RMS per band
- StereoAnalyzer - width, correlation
- LoudnessMeter - LUFS (momentary, short-term, integrated)

### Rules Engine (`RulesEngine.h`)
- Three modes: Instant, Reference, Genre
- `generateFromReference()` - compares analysis to reference and generates parameters
- Safety limits on all parameters
- Genre-specific adjustments

### Parameter Generator (`ParameterGenerator.h`)
- Converts analysis differences into actual EQ/Comp/Stereo/Limiter values

---

## What We Need to Add

### 1. Accumulated Analysis Storage

```cpp
// Add to AnalysisEngine.h
struct AccumulatedAnalysis {
    std::array<float, 32> avgSpectrum = {};
    float avgLUFS = -60.0f;
    float peakLUFS = -60.0f;
    float avgWidth = 1.0f;
    float avgCorrelation = 1.0f;
    float avgCrestFactor = 12.0f;
    int sampleCount = 0;
    bool isValid = false;
};

AccumulatedAnalysis accumulatedAnalysis;
bool isAccumulating = false;
```

### 2. Accumulation Methods

```cpp
void startAccumulation() {
    accumulatedAnalysis = {};  // Reset
    isAccumulating = true;
}

void stopAccumulation() {
    isAccumulating = false;
    if (accumulatedAnalysis.sampleCount > 0) {
        // Finalize averages
        accumulatedAnalysis.isValid = true;
    }
}

// Modify process() to accumulate when enabled
void process(buffer) {
    // ... existing real-time analysis ...

    if (isAccumulating) {
        accumulateData(currentResults);
    }
}

void accumulateData(const AnalysisResults& current) {
    // Running average of spectral data
    // Track peak LUFS
    // Average width, correlation, crest
    accumulatedAnalysis.sampleCount++;
}
```

### 3. UI Changes

**Add to header bar or new "Assistant" panel:**
- "ANALYZE" button (toggles Start/Stop)
- Progress indicator showing accumulation time
- Status text: "Analyzing... Play your loudest section"
- Visual feedback (pulsing border, progress bar)

**Workflow states:**
1. **Idle** - "Load a reference or click Analyze"
2. **Analyzing** - "Playing... 5s / 10s" with stop button
3. **Ready** - "Analysis complete. Click Auto Master"

### 4. Modified Auto Master Logic

```cpp
void triggerAutoMaster() {
    AnalysisResults resultsToUse;

    if (accumulatedAnalysis.isValid) {
        // Use accumulated data (Ozone-style)
        resultsToUse = convertAccumulatedToResults(accumulatedAnalysis);
    } else {
        // Fall back to real-time (current behavior)
        resultsToUse = analysisEngine.getResults();
    }

    auto params = rulesEngine.generateParameters(resultsToUse);
    applyGeneratedParameters(params);
}
```

---

## Implementation Steps

### Phase 1: Core Accumulation (Backend)
1. Add `AccumulatedAnalysis` struct to `AnalysisEngine.h`
2. Add `startAccumulation()`, `stopAccumulation()`, `accumulateData()` methods
3. Modify `process()` to accumulate when flag is set
4. Add `getAccumulatedResults()` method
5. Test: verify accumulation produces stable averages

### Phase 2: Workflow Integration
1. Modify `triggerAutoMaster()` to prefer accumulated data
2. Add `isAccumulationValid()` check
3. Add auto-stop after configurable time (default 10s)
4. Test: verify Auto Master uses accumulated data when available

### Phase 3: UI Updates
1. Add "ANALYZE" toggle button to header
2. Add accumulation timer display
3. Add status text component
4. Add visual feedback (analyzing state indicator)
5. Wire up button to `startAccumulation()`/`stopAccumulation()`

### Phase 4: Polish
1. Add "Reset Analysis" option
2. Show analysis vs reference comparison visually
3. Add tooltips explaining the workflow
4. Consider auto-starting analysis when playback starts

---

## Simple vs Full Implementation

### Simple Version (Recommended First)
- Single "ANALYZE" button that toggles accumulation
- Fixed 10-second analysis window
- Auto Master uses accumulated data if available, else real-time
- No fancy visualizations

### Full Version (Later)
- Configurable analysis duration
- Visual comparison graph (your track vs reference)
- Section detection (verse/chorus like Ozone)
- Multiple reference track support
- A/B comparison with level matching

---

## Files to Modify

| File | Changes |
|------|---------|
| `AnalysisEngine.h` | Add AccumulatedAnalysis, accumulation methods |
| `PluginProcessor.cpp` | Wire up accumulation start/stop, modify triggerAutoMaster |
| `PluginProcessor.h` | Add accumulation control methods |
| `PluginEditor.cpp` | Add Analyze button, timer display, status text |
| `PluginEditor.h` | Add UI component members |

---

## Expected User Workflow After Implementation

1. **Load Reference** (optional) - Click "Load Ref", select a mastered track
2. **Start Analysis** - Click "ANALYZE" button
3. **Play Loudest Section** - Play chorus/drop for ~10 seconds
4. **Analysis Stops** - Button changes to show "Ready"
5. **Click Auto Master** - Plugin generates settings based on accumulated analysis vs reference
6. **Fine-tune** - Adjust individual parameters as needed

This matches the Ozone workflow while keeping our simpler UI.
