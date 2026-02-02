#pragma once

#include "AnalysisEngine.h"
#include "ReferenceProfile.h"
#include "DSPUtils.h"
#include <array>

class ParameterGenerator
{
public:
    // Generated parameter sets
    struct EQParameters
    {
        bool hpfEnabled = false;
        float hpfFreq = 30.0f;

        bool lpfEnabled = false;
        float lpfFreq = 18000.0f;

        float lowShelfFreq = 100.0f;
        float lowShelfGain = 0.0f;

        float highShelfFreq = 8000.0f;
        float highShelfGain = 0.0f;

        std::array<float, 4> bandFreq = { 200.0f, 800.0f, 2500.0f, 6000.0f };
        std::array<float, 4> bandGain = { 0.0f, 0.0f, 0.0f, 0.0f };
        std::array<float, 4> bandQ = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct CompressorParameters
    {
        float lowMidCrossover = 200.0f;
        float midHighCrossover = 3000.0f;

        // Gentler defaults to preserve verse-to-chorus dynamics
        std::array<float, 3> threshold = { -10.0f, -8.0f, -6.0f };
        std::array<float, 3> ratio = { 2.0f, 2.0f, 2.0f };
        std::array<float, 3> attack = { 20.0f, 10.0f, 5.0f };
        std::array<float, 3> release = { 200.0f, 150.0f, 100.0f };
        std::array<float, 3> makeup = { 0.0f, 0.0f, 0.0f };
    };

    struct StereoParameters
    {
        float globalWidth = 1.0f;
        float lowWidth = 1.0f;
        float midWidth = 1.0f;
        float highWidth = 1.0f;
        bool monoBassEnabled = false;
        float monoBassFreq = 120.0f;
    };

    struct LimiterParameters
    {
        float ceiling = -0.3f;
        float release = 100.0f;
        float targetLUFS = -14.0f;
        float autoGain = 0.0f;
    };

    struct GeneratedParameters
    {
        EQParameters eq;
        CompressorParameters comp;
        StereoParameters stereo;
        LimiterParameters limiter;
        float confidence = 0.0f;  // 0-1 confidence in generated parameters
    };

    ParameterGenerator() = default;

    // Generate parameters from analysis results
    GeneratedParameters generateFromAnalysis(const AnalysisEngine::AnalysisResults& analysis,
                                             float targetLUFS = -14.0f)
    {
        GeneratedParameters params;

        // Generate EQ parameters from spectral analysis
        params.eq = generateEQParameters(analysis);

        // Generate compressor parameters from dynamics analysis
        params.comp = generateCompressorParameters(analysis);

        // Generate stereo parameters
        params.stereo = generateStereoParameters(analysis);

        // Generate limiter parameters
        params.limiter = generateLimiterParameters(analysis, targetLUFS);

        // Calculate confidence based on analysis validity
        params.confidence = calculateConfidence(analysis);

        return params;
    }

    // Generate parameters to match a reference profile
    GeneratedParameters generateFromReference(const AnalysisEngine::AnalysisResults& analysis,
                                              const ReferenceProfile& reference,
                                              float blendAmount = 1.0f)
    {
        GeneratedParameters params;

        if (!reference.isProfileValid())
            return generateFromAnalysis(analysis);

        // Generate EQ to match reference spectral envelope
        params.eq = generateEQToMatchReference(analysis, reference);

        // Adjust compression based on reference dynamics
        params.comp = generateCompressorToMatchReference(analysis, reference);

        // Match stereo characteristics
        params.stereo = generateStereoToMatchReference(analysis, reference);

        // Limiter to match reference loudness
        params.limiter = generateLimiterToMatchReference(analysis, reference);

        // Blend with neutral settings based on blend amount
        if (blendAmount < 1.0f)
        {
            GeneratedParameters neutral = generateFromAnalysis(analysis);
            params = blendParameters(neutral, params, blendAmount);
        }

        params.confidence = reference.isProfileValid() ? 0.8f : 0.5f;

        return params;
    }

private:
    EQParameters generateEQParameters(const AnalysisEngine::AnalysisResults& analysis)
    {
        EQParameters eq;

        // Analyze spectral balance
        auto& bands = analysis.bandEnergies;
        float avgEnergy = 0.0f;
        for (float e : bands)
            avgEnergy += e;
        avgEnergy /= 32.0f;

        // Sub-bass check (bands 0-3, ~20-80Hz)
        float subEnergy = (bands[0] + bands[1] + bands[2] + bands[3]) / 4.0f;
        if (subEnergy < avgEnergy - 12.0f)
        {
            // Very little sub content, enable HPF
            eq.hpfEnabled = true;
            eq.hpfFreq = 30.0f;
        }

        // Low end (bands 4-8, ~80-200Hz)
        float lowEnergy = (bands[4] + bands[5] + bands[6] + bands[7] + bands[8]) / 5.0f;
        float lowDiff = lowEnergy - avgEnergy;
        eq.lowShelfGain = juce::jlimit(-6.0f, 6.0f, -lowDiff * 0.3f);

        // Low-mid (bands 9-13, ~200-600Hz)
        float lowMidEnergy = (bands[9] + bands[10] + bands[11] + bands[12] + bands[13]) / 5.0f;
        float lowMidDiff = lowMidEnergy - avgEnergy;
        eq.bandGain[0] = juce::jlimit(-6.0f, 6.0f, -lowMidDiff * 0.25f);  // Cut muddy frequencies

        // Mid (bands 14-18, ~600-2000Hz)
        float midEnergy = (bands[14] + bands[15] + bands[16] + bands[17] + bands[18]) / 5.0f;
        float midDiff = midEnergy - avgEnergy;
        eq.bandGain[1] = juce::jlimit(-4.0f, 4.0f, -midDiff * 0.2f);

        // Presence (bands 19-23, ~2-5kHz)
        float presenceEnergy = (bands[19] + bands[20] + bands[21] + bands[22] + bands[23]) / 5.0f;
        float presenceDiff = presenceEnergy - avgEnergy;
        eq.bandGain[2] = juce::jlimit(-4.0f, 6.0f, -presenceDiff * 0.25f);  // Often boost presence

        // Air (bands 24-28, ~5-12kHz)
        float airEnergy = (bands[24] + bands[25] + bands[26] + bands[27] + bands[28]) / 5.0f;
        float airDiff = airEnergy - avgEnergy;
        eq.highShelfGain = juce::jlimit(-4.0f, 6.0f, -airDiff * 0.3f);

        // Ultra-high (bands 29-31, >12kHz)
        float ultraHighEnergy = (bands[29] + bands[30] + bands[31]) / 3.0f;
        if (ultraHighEnergy > avgEnergy + 6.0f)
        {
            // Harsh highs, enable gentle LPF
            eq.lpfEnabled = true;
            eq.lpfFreq = 16000.0f;
        }

        return eq;
    }

    CompressorParameters generateCompressorParameters(const AnalysisEngine::AnalysisResults& analysis)
    {
        CompressorParameters comp;

        // Adjust based on crest factor (dynamics)
        float avgCrest = (analysis.dynamics.crestFactors[0] +
                          analysis.dynamics.crestFactors[1] +
                          analysis.dynamics.crestFactors[2]) / 3.0f;

        // High crest factor = more dynamic, compress more aggressively
        // Low crest factor = already compressed, be gentle
        float compressionAmount = juce::jmap(avgCrest, 6.0f, 18.0f, 0.3f, 1.0f);
        compressionAmount = juce::jlimit(0.3f, 1.0f, compressionAmount);

        // ===== GENTLER COMPRESSION TO PRESERVE MACRODYNAMICS =====
        // Previous thresholds were too low (-20 to -10 dB), causing
        // loud sections (chorus) to be compressed more than quiet sections (verse),
        // which inverted the dynamics. Now using higher thresholds so compression
        // only catches the loudest peaks, preserving verse-to-chorus dynamics.

        // Low band - high threshold, gentle ratio, slow attack for punch
        // Threshold: -12dB to -6dB (was -20dB to -10dB)
        // Ratio: 1.5:1 to 2.5:1 (was 2:1 to 4:1)
        comp.threshold[0] = -12.0f + (1.0f - compressionAmount) * 6.0f;
        comp.ratio[0] = 1.5f + compressionAmount * 1.0f;
        comp.attack[0] = 30.0f - compressionAmount * 10.0f;
        comp.release[0] = 200.0f;

        // Mid band - moderate settings
        // Threshold: -10dB to -6dB (was -18dB to -10dB)
        // Ratio: 1.5:1 to 2.5:1 (was 3:1 to 5:1)
        comp.threshold[1] = -10.0f + (1.0f - compressionAmount) * 4.0f;
        comp.ratio[1] = 1.5f + compressionAmount * 1.0f;
        comp.attack[1] = 15.0f - compressionAmount * 5.0f;
        comp.release[1] = 150.0f;

        // High band - catches harshness, still gentle
        // Threshold: -8dB to -4dB (was -16dB to -10dB)
        // Ratio: 1.5:1 to 2.5:1 (was 3:1 to 5:1)
        comp.threshold[2] = -8.0f + (1.0f - compressionAmount) * 4.0f;
        comp.ratio[2] = 1.5f + compressionAmount * 1.0f;
        comp.attack[2] = 5.0f;
        comp.release[2] = 100.0f;

        // Adjust for transient density
        if (analysis.dynamics.transientDensity > 50.0f)
        {
            // Lots of transients, slower attack
            comp.attack[0] += 10.0f;
            comp.attack[1] += 5.0f;
        }

        return comp;
    }

    StereoParameters generateStereoParameters(const AnalysisEngine::AnalysisResults& analysis)
    {
        StereoParameters stereo;

        // Current width
        float currentWidth = analysis.stereo.width;
        float correlation = analysis.stereo.correlation;

        // If too narrow, widen slightly
        if (currentWidth < 0.8f)
            stereo.globalWidth = 1.1f;
        // If too wide or low correlation (phase issues), narrow
        else if (currentWidth > 1.5f || correlation < 0.3f)
            stereo.globalWidth = 0.9f;

        // Per-band width
        stereo.lowWidth = std::min(1.0f, analysis.stereo.bandWidth[0]);  // Mono-ish bass
        stereo.midWidth = analysis.stereo.bandWidth[1];
        stereo.highWidth = std::min(1.3f, analysis.stereo.bandWidth[2] * 1.1f);  // Slightly wider highs

        // Enable mono bass if low-end width is problematic
        if (analysis.stereo.bandWidth[0] > 0.5f || analysis.stereo.bandCorrelation[0] < 0.7f)
        {
            stereo.monoBassEnabled = true;
            stereo.monoBassFreq = 120.0f;
        }

        return stereo;
    }

    LimiterParameters generateLimiterParameters(const AnalysisEngine::AnalysisResults& analysis,
                                                 float targetLUFS)
    {
        LimiterParameters limiter;

        limiter.targetLUFS = targetLUFS;
        limiter.ceiling = -0.3f;  // Safe true-peak ceiling

        // Calculate auto gain to reach target
        float currentLUFS = analysis.shortTermLUFS;
        if (currentLUFS > -60.0f)  // Valid reading
        {
            float needed = targetLUFS - currentLUFS;
            limiter.autoGain = juce::jlimit(-12.0f, 12.0f, needed);
        }

        // Adjust release based on dynamics
        float crest = analysis.dynamics.crestFactors[1];
        if (crest > 12.0f)
            limiter.release = 150.0f;  // More dynamic, slower release
        else
            limiter.release = 80.0f;   // Already compressed, faster release

        return limiter;
    }

    EQParameters generateEQToMatchReference(const AnalysisEngine::AnalysisResults& analysis,
                                             const ReferenceProfile& reference)
    {
        EQParameters eq;
        auto& current = analysis.bandEnergies;
        auto& target = reference.getSpectralEnvelope();

        // Calculate difference curve
        std::array<float, 32> diff;
        for (int i = 0; i < 32; ++i)
            diff[i] = target[i] - current[i];

        // Map difference to EQ bands with safety limits
        // Low shelf (average of bands 4-8)
        float lowDiff = (diff[4] + diff[5] + diff[6] + diff[7] + diff[8]) / 5.0f;
        eq.lowShelfGain = juce::jlimit(-6.0f, 6.0f, lowDiff);

        // Band 1 - low-mid (bands 9-13)
        eq.bandGain[0] = juce::jlimit(-6.0f, 6.0f,
            (diff[9] + diff[10] + diff[11] + diff[12] + diff[13]) / 5.0f);

        // Band 2 - mid (bands 14-18)
        eq.bandGain[1] = juce::jlimit(-6.0f, 6.0f,
            (diff[14] + diff[15] + diff[16] + diff[17] + diff[18]) / 5.0f);

        // Band 3 - presence (bands 19-23)
        eq.bandGain[2] = juce::jlimit(-6.0f, 6.0f,
            (diff[19] + diff[20] + diff[21] + diff[22] + diff[23]) / 5.0f);

        // High shelf (bands 24-28)
        float highDiff = (diff[24] + diff[25] + diff[26] + diff[27] + diff[28]) / 5.0f;
        eq.highShelfGain = juce::jlimit(-6.0f, 6.0f, highDiff);

        return eq;
    }

    CompressorParameters generateCompressorToMatchReference(const AnalysisEngine::AnalysisResults& analysis,
                                                             const ReferenceProfile& reference)
    {
        CompressorParameters comp = generateCompressorParameters(analysis);

        // Adjust based on target crest factor
        float currentCrest = analysis.dynamics.crestFactors[1];
        float targetCrest = reference.getCrestFactor();

        if (currentCrest > targetCrest + 2.0f)
        {
            // Need more compression
            for (int i = 0; i < 3; ++i)
            {
                comp.threshold[i] -= 3.0f;
                comp.ratio[i] += 1.0f;
            }
        }
        else if (currentCrest < targetCrest - 2.0f)
        {
            // Need less compression
            for (int i = 0; i < 3; ++i)
            {
                comp.threshold[i] += 3.0f;
                comp.ratio[i] = std::max(1.5f, comp.ratio[i] - 1.0f);
            }
        }

        return comp;
    }

    StereoParameters generateStereoToMatchReference(const AnalysisEngine::AnalysisResults& analysis,
                                                     const ReferenceProfile& reference)
    {
        StereoParameters stereo = generateStereoParameters(analysis);

        float currentWidth = analysis.stereo.width;
        float targetWidth = reference.getStereoWidth();

        // Adjust global width to match target
        float widthRatio = targetWidth / std::max(0.1f, currentWidth);
        stereo.globalWidth = juce::jlimit(0.5f, 2.0f, widthRatio);

        return stereo;
    }

    LimiterParameters generateLimiterToMatchReference(const AnalysisEngine::AnalysisResults& analysis,
                                                       const ReferenceProfile& reference)
    {
        // Target the reference's loudness
        float targetLUFS = reference.getLoudnessRMS() + 4.0f;  // Rough conversion RMS to LUFS
        targetLUFS = juce::jlimit(-24.0f, -6.0f, targetLUFS);

        return generateLimiterParameters(analysis, targetLUFS);
    }

    GeneratedParameters blendParameters(const GeneratedParameters& a,
                                         const GeneratedParameters& b,
                                         float blend)
    {
        GeneratedParameters result;

        // Linear interpolation helper
        auto lerp = [](float v1, float v2, float t) { return v1 + t * (v2 - v1); };

        // Blend EQ
        result.eq.lowShelfGain = lerp(a.eq.lowShelfGain, b.eq.lowShelfGain, blend);
        result.eq.highShelfGain = lerp(a.eq.highShelfGain, b.eq.highShelfGain, blend);
        for (int i = 0; i < 4; ++i)
            result.eq.bandGain[i] = lerp(a.eq.bandGain[i], b.eq.bandGain[i], blend);

        // Blend compressor
        for (int i = 0; i < 3; ++i)
        {
            result.comp.threshold[i] = lerp(a.comp.threshold[i], b.comp.threshold[i], blend);
            result.comp.ratio[i] = lerp(a.comp.ratio[i], b.comp.ratio[i], blend);
        }

        // Blend stereo
        result.stereo.globalWidth = lerp(a.stereo.globalWidth, b.stereo.globalWidth, blend);

        // Blend limiter
        result.limiter.autoGain = lerp(a.limiter.autoGain, b.limiter.autoGain, blend);

        result.confidence = lerp(a.confidence, b.confidence, blend);

        return result;
    }

    float calculateConfidence(const AnalysisEngine::AnalysisResults& analysis)
    {
        float confidence = 0.5f;

        // Higher confidence with valid loudness readings
        if (analysis.shortTermLUFS > -60.0f)
            confidence += 0.2f;

        // Higher confidence with stable correlation
        if (analysis.stereo.correlation > 0.5f)
            confidence += 0.15f;

        // Lower confidence with extreme values
        if (std::abs(analysis.spectral.slope) > 6.0f)
            confidence -= 0.1f;

        return juce::jlimit(0.0f, 1.0f, confidence);
    }
};
