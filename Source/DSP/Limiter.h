#pragma once

#include "DSPUtils.h"
#include <vector>

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

        // True peak detector
        truePeakL.prepare(sampleRate);
        truePeakR.prepare(sampleRate);

        updateCoefficients();
        reset();
    }

    void reset()
    {
        std::fill(lookaheadBufferL.begin(), lookaheadBufferL.end(), 0.0f);
        std::fill(lookaheadBufferR.begin(), lookaheadBufferR.end(), 0.0f);
        lookaheadIndex = 0;

        envelope = 0.0f;
        gainReduction.store(0.0f);

        truePeakL.reset();
        truePeakR.reset();
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypassed)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), 2);

        float ceilingLinear = DSPUtils::decibelsToLinear(ceiling);
        float maxGR = 0.0f;

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

            lookaheadIndex = (lookaheadIndex + 1) % lookaheadSamples;

            // Detect peak level (use current input for lookahead detection)
            float peakL = truePeakEnabled ? truePeakL.process(inputL) : std::abs(inputL);
            float peakR = truePeakEnabled ? truePeakR.process(inputR) : std::abs(inputR);
            float peak = std::max(peakL, peakR);

            // Calculate required gain reduction
            float requiredGR = 0.0f;
            if (peak > ceilingLinear)
            {
                float peakDB = DSPUtils::linearToDecibels(peak);
                requiredGR = peakDB - ceiling;
            }

            // Apply envelope (instant attack, smooth release)
            if (requiredGR > envelope)
            {
                envelope = requiredGR;  // Instant attack
            }
            else
            {
                envelope = releaseCoeff * envelope;  // Smooth release
            }

            // Apply gain reduction to delayed signal
            float gainDB = -envelope;
            float gainLinear = DSPUtils::decibelsToLinear(gainDB);

            delayedL *= gainLinear;
            delayedR *= gainLinear;

            // Apply output gain if targeting LUFS
            if (autoGainEnabled)
            {
                delayedL *= autoGainLinear;
                delayedR *= autoGainLinear;
            }

            // Final safety clip at ceiling
            delayedL = std::max(-ceilingLinear, std::min(ceilingLinear, delayedL));
            delayedR = std::max(-ceilingLinear, std::min(ceilingLinear, delayedR));

            buffer.setSample(0, sample, delayedL);
            if (numChannels > 1)
                buffer.setSample(1, sample, delayedR);

            maxGR = std::max(maxGR, envelope);
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

    // Latency for host compensation
    int getLatencySamples() const { return lookaheadSamples; }

private:
    void updateCoefficients()
    {
        if (currentSampleRate > 0.0)
        {
            releaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * releaseTime / 1000.0f));
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

    // Processing state
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;

    // Lookahead buffer
    std::vector<float> lookaheadBufferL;
    std::vector<float> lookaheadBufferR;
    int lookaheadSamples = 0;
    int lookaheadIndex = 0;

    // True peak detection
    DSPUtils::TruePeakDetector truePeakL;
    DSPUtils::TruePeakDetector truePeakR;

    // Metering
    std::atomic<float> gainReduction { 0.0f };
};
