#pragma once

#include "DSPUtils.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <atomic>

class Limiter
{
public:
    // Diagnostic data structure
    struct Diagnostics
    {
        float maxOutputLevel = 0.0f;         // Max output sample value (linear)
        float maxPreSoftClipLevel = 0.0f;    // Max level before soft clip
        int softClipEngagements = 0;         // Samples where soft clip was active
        int totalSamples = 0;                // Total samples processed
        float autoGainDB = 0.0f;             // Current auto-gain value
        float maxGainReductionDB = 0.0f;     // Max GR this period
        int samplesExceeding1 = 0;           // Samples > 1.0 (should be 0!)

        void reset()
        {
            maxOutputLevel = 0.0f;
            maxPreSoftClipLevel = 0.0f;
            softClipEngagements = 0;
            totalSamples = 0;
            samplesExceeding1 = 0;
            maxGainReductionDB = 0.0f;
        }
    };

    Limiter() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        // Lookahead buffer (5ms)
        lookaheadSamples = static_cast<int>(sampleRate * 0.005);
        lookaheadBufferL.resize(lookaheadSamples, 0.0f);
        lookaheadBufferR.resize(lookaheadSamples, 0.0f);
        gainBuffer.resize(lookaheadSamples, 1.0f);

        // 4x oversampling for true peak detection (ITU-R BS.1770 compliant)
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2,  // numChannels
            2,  // order (2^2 = 4x)
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true  // isMaxQuality
        );
        oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));

        updateCoefficients();
        reset();
    }

    void reset()
    {
        std::fill(lookaheadBufferL.begin(), lookaheadBufferL.end(), 0.0f);
        std::fill(lookaheadBufferR.begin(), lookaheadBufferR.end(), 0.0f);
        std::fill(gainBuffer.begin(), gainBuffer.end(), 1.0f);
        lookaheadIndex = 0;

        fastEnvelope = 0.0f;
        slowEnvelope = 0.0f;
        smoothedGain = 1.0f;
        gainReduction.store(0.0f);

        if (oversampling)
            oversampling->reset();
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypassed)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), 2);

        float ceilingLinear = DSPUtils::decibelsToLinear(ceiling);
        float maxGR = 0.0f;

        // Create audio block for oversampling
        juce::dsp::AudioBlock<float> inputBlock(buffer);

        // Upsample for true peak detection
        auto oversampledBlock = oversampling->processSamplesUp(inputBlock);

        // Find true peaks in oversampled domain
        const size_t osNumSamples = oversampledBlock.getNumSamples();
        const int osFactor = static_cast<int>(osNumSamples) / numSamples;

        // Temporary storage for per-sample true peaks
        std::vector<float> truePeaks(numSamples, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float maxPeak = 0.0f;
            for (int os = 0; os < osFactor; ++os)
            {
                int osIndex = i * osFactor + os;
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float absVal = std::abs(oversampledBlock.getSample(ch, osIndex));
                    maxPeak = std::max(maxPeak, absVal);
                }
            }
            truePeaks[i] = maxPeak;
        }

        // Downsample (we only needed the sidechain analysis)
        oversampling->processSamplesDown(inputBlock);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Read from lookahead buffer
            float delayedL = lookaheadBufferL[lookaheadIndex];
            float delayedR = numChannels > 1 ? lookaheadBufferR[lookaheadIndex] : delayedL;

            // Write current samples to lookahead buffer
            float inputL = buffer.getSample(0, sample);
            float inputR = numChannels > 1 ? buffer.getSample(1, sample) : inputL;

            lookaheadBufferL[lookaheadIndex] = inputL;
            if (numChannels > 1)
                lookaheadBufferR[lookaheadIndex] = inputR;

            // Use true peak from oversampled detection
            // IMPORTANT: Factor in auto-gain so limiter knows what the OUTPUT level will be
            float peak = truePeaks[sample];
            if (autoGainEnabled)
            {
                peak *= autoGainLinear;  // This is what the peak will be AFTER auto-gain
            }

            // Calculate required gain reduction (now accounts for auto-gain)
            float targetGain = 1.0f;
            if (peak > ceilingLinear)
            {
                targetGain = ceilingLinear / peak;
            }

            // Program-dependent release using dual envelopes
            // Fast envelope: catches transients, releases quickly
            if (targetGain < fastEnvelope)
                fastEnvelope = targetGain;  // Instant attack
            else
                fastEnvelope = fastReleaseCoeff * fastEnvelope + (1.0f - fastReleaseCoeff) * targetGain;

            // Slow envelope: handles sustained material, releases slowly
            if (targetGain < slowEnvelope)
                slowEnvelope = slowAttackCoeff * slowEnvelope + (1.0f - slowAttackCoeff) * targetGain;
            else
                slowEnvelope = slowReleaseCoeff * slowEnvelope + (1.0f - slowReleaseCoeff) * targetGain;

            // Use minimum of both envelopes (most restrictive)
            float envelope = std::min(fastEnvelope, slowEnvelope);

            // Store in gain buffer for lookahead smoothing
            gainBuffer[lookaheadIndex] = envelope;

            // Find minimum gain in lookahead window (anticipate peaks)
            float minGain = 1.0f;
            for (int j = 0; j < lookaheadSamples; ++j)
                minGain = std::min(minGain, gainBuffer[j]);

            // Smooth gain transitions to prevent clicks
            float gainSmoothCoeff = (minGain < smoothedGain) ? 0.9f : gainSmoothReleaseCoeff;
            smoothedGain = gainSmoothCoeff * smoothedGain + (1.0f - gainSmoothCoeff) * minGain;

            lookaheadIndex = (lookaheadIndex + 1) % lookaheadSamples;

            // Apply gain reduction to delayed signal
            delayedL *= smoothedGain;
            delayedR *= smoothedGain;

            // Apply output gain if targeting LUFS
            if (autoGainEnabled)
            {
                delayedL *= autoGainLinear;
                delayedR *= autoGainLinear;
            }

            // DIAGNOSTIC: Track pre-soft-clip levels
            float preSoftClipMax = std::max(std::abs(delayedL), std::abs(delayedR));
            diag.maxPreSoftClipLevel = std::max(diag.maxPreSoftClipLevel, preSoftClipMax);

            // Check if soft clip will engage (above knee)
            float knee = ceilingLinear * 0.95f;
            if (preSoftClipMax > knee)
            {
                diag.softClipEngagements++;
            }

            // SOFT CLIP safety (tanh-based) instead of hard clip
            // This catches any remaining peaks musically
            delayedL = softClipOutput(delayedL, ceilingLinear);
            delayedR = softClipOutput(delayedR, ceilingLinear);

            // DIAGNOSTIC: Track output levels
            float outputMax = std::max(std::abs(delayedL), std::abs(delayedR));
            diag.maxOutputLevel = std::max(diag.maxOutputLevel, outputMax);
            diag.totalSamples++;

            if (outputMax > 1.0f)
                diag.samplesExceeding1++;

            buffer.setSample(0, sample, delayedL);
            if (numChannels > 1)
                buffer.setSample(1, sample, delayedR);

            // Track gain reduction for metering
            float grDB = DSPUtils::linearToDecibels(smoothedGain);
            maxGR = std::max(maxGR, -grDB);
        }

        gainReduction.store(maxGR);
        diag.maxGainReductionDB = std::max(diag.maxGainReductionDB, maxGR);
        diag.autoGainDB = autoGainDB;

        // Periodically log diagnostics (every ~1 second at 48kHz with 512 block size)
        diagBlockCount++;
        if (diagBlockCount >= 94)  // ~1 second
        {
            logDiagnostics();
            diagBlockCount = 0;
        }
    }

    // Controls
    void setCeiling(float ceilingDB)
    {
        ceiling = juce::jlimit(-6.0f, 0.0f, ceilingDB);
    }

    void setRelease(float releaseMs)
    {
        releaseTime = juce::jlimit(10.0f, 1000.0f, releaseMs);
        updateCoefficients();
    }

    void setTargetLUFS(float targetDB)
    {
        targetLUFS = juce::jlimit(-24.0f, -6.0f, targetDB);
    }

    void setAutoGainEnabled(bool enabled)
    {
        autoGainEnabled = enabled;
    }

    void setAutoGainValue(float gainDB)
    {
        // Allow up to +18dB of auto-gain to accommodate:
        // - Quiet input tracks that need significant boost
        // - Headroom reduction compensation (up to +6dB)
        // - Target LUFS requirements
        // The limiter will catch any peaks that exceed ceiling
        autoGainDB = juce::jlimit(-12.0f, 18.0f, gainDB);
        autoGainLinear = DSPUtils::decibelsToLinear(autoGainDB);
    }

    void setTruePeakEnabled(bool enabled)
    {
        truePeakEnabled = enabled;
    }

    void setBypass(bool shouldBypass) { bypassed = shouldBypass; }
    bool isBypassed() const { return bypassed; }

    // Metering
    float getGainReduction() const { return gainReduction.load(); }

    // Getters
    float getCeiling() const { return ceiling; }
    float getRelease() const { return releaseTime; }
    float getTargetLUFS() const { return targetLUFS; }

    // Latency for host compensation (lookahead + oversampling filter)
    int getLatencySamples() const
    {
        int osLatency = oversampling ? static_cast<int>(oversampling->getLatencyInSamples()) : 0;
        return lookaheadSamples + osLatency;
    }

private:
    // Soft clip using tanh - musical saturation instead of harsh digital clip
    // This version has NO discontinuity - starts engaging at knee and smoothly approaches ceiling
    float softClipOutput(float input, float ceiling)
    {
        float absInput = std::abs(input);

        // Knee at 95% of ceiling - only engage for actual emergencies
        // (Was 80%, which caused audible saturation on peaks that were already within limits)
        float knee = ceiling * 0.95f;

        // If below knee, pass through unchanged (linear region)
        if (absInput <= knee)
            return input;

        float sign = input > 0.0f ? 1.0f : -1.0f;

        // Soft region is from knee to ceiling (and beyond)
        float softRegion = ceiling - knee;  // 0.2 * ceiling
        float excess = absInput - knee;

        // tanh maps 0->0 and infinity->1
        // So knee + softRegion * tanh(excess/softRegion) approaches knee + softRegion = ceiling
        float clipped = knee + softRegion * std::tanh(excess / softRegion);

        return sign * clipped;
    }

    void updateCoefficients()
    {
        if (currentSampleRate > 0.0)
        {
            // Fast release: 30ms (for transients)
            float fastReleaseMs = 30.0f;
            fastReleaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * fastReleaseMs / 1000.0f));

            // Slow attack: 5ms (gradual engagement for sustained)
            float slowAttackMs = 5.0f;
            slowAttackCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * slowAttackMs / 1000.0f));

            // Slow release: user-controlled (default 100ms, range 50-300ms for program-dependent)
            float slowReleaseMs = releaseTime;
            slowReleaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * slowReleaseMs / 1000.0f));

            // Gain smoothing release (same as lookahead time for smooth transitions)
            float smoothMs = (lookaheadSamples / static_cast<float>(currentSampleRate)) * 1000.0f;
            gainSmoothReleaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * smoothMs / 1000.0f));
        }
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool bypassed = false;

    // Limiter settings
    float ceiling = -0.3f;     // dBTP
    float releaseTime = 100.0f; // ms
    float targetLUFS = -14.0f;
    bool autoGainEnabled = false;
    float autoGainDB = 0.0f;
    float autoGainLinear = 1.0f;
    bool truePeakEnabled = true;

    // Processing state - program-dependent dual envelope
    float fastReleaseCoeff = 0.0f;
    float slowAttackCoeff = 0.0f;
    float slowReleaseCoeff = 0.0f;
    float gainSmoothReleaseCoeff = 0.0f;

    float fastEnvelope = 1.0f;
    float slowEnvelope = 1.0f;
    float smoothedGain = 1.0f;

    // Lookahead buffer
    std::vector<float> lookaheadBufferL;
    std::vector<float> lookaheadBufferR;
    std::vector<float> gainBuffer;
    int lookaheadSamples = 0;
    int lookaheadIndex = 0;

    // 4x oversampling for true peak detection
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Metering
    std::atomic<float> gainReduction { 0.0f };

    // Diagnostics
    Diagnostics diag;
    int diagBlockCount = 0;

    void logDiagnostics()
    {
        // Write to log file in user's home directory
        static std::string logPath = std::string(getenv("HOME")) + "/automaster_limiter_diag.log";
        std::ofstream log(logPath, std::ios::app);

        if (log.is_open())
        {
            float softClipPercent = diag.totalSamples > 0 ?
                (100.0f * diag.softClipEngagements / diag.totalSamples) : 0.0f;

            log << "=== Limiter Diagnostics (1 sec) ===" << std::endl;
            log << "Max output level: " << diag.maxOutputLevel
                << " (" << DSPUtils::linearToDecibels(diag.maxOutputLevel) << " dB)" << std::endl;
            log << "Max pre-softclip: " << diag.maxPreSoftClipLevel
                << " (" << DSPUtils::linearToDecibels(diag.maxPreSoftClipLevel) << " dB)" << std::endl;
            log << "Soft clip engaged: " << diag.softClipEngagements << " / "
                << diag.totalSamples << " samples (" << softClipPercent << "%)" << std::endl;
            log << "Samples > 1.0: " << diag.samplesExceeding1 << std::endl;
            log << "Auto-gain: " << diag.autoGainDB << " dB" << std::endl;
            log << "Max GR: " << diag.maxGainReductionDB << " dB" << std::endl;
            log << "Ceiling: " << ceiling << " dB (" << DSPUtils::decibelsToLinear(ceiling) << " linear)" << std::endl;
            log << std::endl;
            log.close();
        }

        diag.reset();
    }

public:
    // Access diagnostics for UI display
    const Diagnostics& getDiagnostics() const { return diag; }
    void resetDiagnostics() { diag.reset(); }
};
