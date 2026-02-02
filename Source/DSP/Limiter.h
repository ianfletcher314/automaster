#pragma once

#include "DSPUtils.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

class Limiter
{
public:
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

            // SOFT CLIP safety (tanh-based) instead of hard clip
            // This catches any remaining peaks musically
            delayedL = softClipOutput(delayedL, ceilingLinear);
            delayedR = softClipOutput(delayedR, ceilingLinear);

            buffer.setSample(0, sample, delayedL);
            if (numChannels > 1)
                buffer.setSample(1, sample, delayedR);

            // Track gain reduction for metering
            float grDB = DSPUtils::linearToDecibels(smoothedGain);
            maxGR = std::max(maxGR, -grDB);
        }

        gainReduction.store(maxGR);
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
        autoGainDB = juce::jlimit(-12.0f, 12.0f, gainDB);
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
    float softClipOutput(float input, float ceiling)
    {
        float normalized = input / ceiling;

        // If already below ceiling, pass through
        if (std::abs(normalized) <= 1.0f)
            return input;

        // Soft clip region: use tanh to smoothly approach ceiling
        float sign = normalized > 0.0f ? 1.0f : -1.0f;
        float absNorm = std::abs(normalized);

        // tanh soft clip with knee starting at 0.9
        float knee = 0.9f;
        if (absNorm > knee)
        {
            float excess = absNorm - knee;
            float softRegion = 1.0f - knee;
            float clipped = knee + softRegion * std::tanh(excess / softRegion);
            return sign * clipped * ceiling;
        }

        return input;
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
};
