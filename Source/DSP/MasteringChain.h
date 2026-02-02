#pragma once

#include "MasteringEQ.h"
#include "MultibandCompressor.h"
#include "StereoImager.h"
#include "Limiter.h"
#include "LoudnessMeter.h"
#include "DSPUtils.h"

class MasteringChain
{
public:
    MasteringChain() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        eq.prepare(sampleRate, samplesPerBlock);
        compressor.prepare(sampleRate, samplesPerBlock);
        stereoImager.prepare(sampleRate, samplesPerBlock);
        limiter.prepare(sampleRate, samplesPerBlock);

        inputMeter.prepare(sampleRate, samplesPerBlock);
        outputMeter.prepare(sampleRate, samplesPerBlock);

        inputGainSmoothed.reset(sampleRate);
        outputGainSmoothed.reset(sampleRate);
        headroomGainSmoothed.reset(sampleRate);

        // Peak follower coefficient: ~100ms attack/release for smooth tracking
        peakFollowerCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.1f));

        reset();
    }

    void reset()
    {
        eq.reset();
        compressor.reset();
        stereoImager.reset();
        limiter.reset();
        inputMeter.reset();
        outputMeter.reset();

        trackedPeakLevel = 0.0f;
        currentHeadroomGainDB = 0.0f;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // Apply input gain
        if (std::abs(inputGainDB) > 0.01f)
        {
            inputGainSmoothed.setTargetValue(DSPUtils::decibelsToLinear(inputGainDB));
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float gain = inputGainSmoothed.getNextValue();
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
            }
        }

        // === AUTOMATIC HEADROOM CREATION ===
        // Measure peak level and create headroom if input is too hot.
        // This allows EQ and compressor to work cleanly without clipping.
        // The limiter's auto-gain will compensate at the end.
        if (autoHeadroomEnabled && chainEnabled)
        {
            // Find peak in this buffer
            float blockPeak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    float absVal = std::abs(buffer.getSample(ch, sample));
                    blockPeak = std::max(blockPeak, absVal);
                }
            }

            // Smooth peak tracking (fast attack, slow release)
            if (blockPeak > trackedPeakLevel)
                trackedPeakLevel = blockPeak;  // Instant attack
            else
                trackedPeakLevel = peakFollowerCoeff * trackedPeakLevel + (1.0f - peakFollowerCoeff) * blockPeak;

            // Calculate required headroom reduction
            // Target: peaks at -6dB (0.5 linear) to give processing headroom
            const float targetPeakLinear = 0.5f;  // -6dB
            const float minReductionThreshold = 0.56f;  // Only reduce if peaks > -5dB

            if (trackedPeakLevel > minReductionThreshold)
            {
                // Calculate gain needed to bring peaks to target
                float requiredGain = targetPeakLinear / trackedPeakLevel;
                currentHeadroomGainDB = DSPUtils::linearToDecibels(requiredGain);
                // Clamp to reasonable range (don't reduce by more than 12dB)
                currentHeadroomGainDB = std::max(currentHeadroomGainDB, -12.0f);
            }
            else
            {
                // Input is quiet enough, no reduction needed
                currentHeadroomGainDB = 0.0f;
            }

            // Apply headroom reduction with smoothing
            if (std::abs(currentHeadroomGainDB) > 0.01f)
            {
                headroomGainSmoothed.setTargetValue(DSPUtils::decibelsToLinear(currentHeadroomGainDB));
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    float gain = headroomGainSmoothed.getNextValue();
                    for (int ch = 0; ch < numChannels; ++ch)
                        buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
                }
            }
        }

        // Measure input (after headroom adjustment - this affects LUFS calculation,
        // which the limiter's auto-gain will then compensate for)
        juce::AudioBuffer<float> inputCopy(buffer);
        inputMeter.process(inputCopy);

        // Processing chain: EQ -> Compressor -> Stereo -> Limiter
        if (chainEnabled)
        {
            eq.process(buffer);
            compressor.process(buffer);
            stereoImager.process(buffer);
            limiter.process(buffer);
        }

        // Apply output gain
        if (std::abs(outputGainDB) > 0.01f)
        {
            outputGainSmoothed.setTargetValue(DSPUtils::decibelsToLinear(outputGainDB));
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float gain = outputGainSmoothed.getNextValue();
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
            }
        }

        // Measure output
        outputMeter.process(buffer);
    }

    // Global controls
    void setInputGain(float gainDB)
    {
        inputGainDB = juce::jlimit(-24.0f, 24.0f, gainDB);
    }

    void setOutputGain(float gainDB)
    {
        outputGainDB = juce::jlimit(-24.0f, 24.0f, gainDB);
    }

    void setChainEnabled(bool enabled)
    {
        chainEnabled = enabled;
    }

    // Module access
    MasteringEQ& getEQ() { return eq; }
    MultibandCompressor& getCompressor() { return compressor; }
    StereoImager& getStereoImager() { return stereoImager; }
    Limiter& getLimiter() { return limiter; }

    const MasteringEQ& getEQ() const { return eq; }
    const MultibandCompressor& getCompressor() const { return compressor; }
    const StereoImager& getStereoImager() const { return stereoImager; }
    const Limiter& getLimiter() const { return limiter; }

    // Metering
    const LoudnessMeter& getInputMeter() const { return inputMeter; }
    const LoudnessMeter& getOutputMeter() const { return outputMeter; }

    float getInputLUFS() const { return inputMeter.getShortTermLUFS(); }
    float getOutputLUFS() const { return outputMeter.getShortTermLUFS(); }
    float getIntegratedLUFS() const { return outputMeter.getIntegratedLUFS(); }
    float getTruePeak() const { return outputMeter.getMaxTruePeak(); }
    float getGainReduction() const { return limiter.getGainReduction() + compressor.getMaxGainReduction(); }

    // Latency
    int getLatencySamples() const { return limiter.getLatencySamples(); }

    // Getters
    float getInputGain() const { return inputGainDB; }
    float getOutputGain() const { return outputGainDB; }
    bool isChainEnabled() const { return chainEnabled; }

    // Auto headroom control
    void setAutoHeadroomEnabled(bool enabled) { autoHeadroomEnabled = enabled; }
    bool isAutoHeadroomEnabled() const { return autoHeadroomEnabled; }
    float getHeadroomReduction() const { return -currentHeadroomGainDB; }  // Return as positive dB

private:
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Processing modules
    MasteringEQ eq;
    MultibandCompressor compressor;
    StereoImager stereoImager;
    Limiter limiter;

    // Metering
    LoudnessMeter inputMeter;
    LoudnessMeter outputMeter;

    // Gain controls
    float inputGainDB = 0.0f;
    float outputGainDB = 0.0f;
    DSPUtils::SmoothedValue inputGainSmoothed;
    DSPUtils::SmoothedValue outputGainSmoothed;

    // Automatic headroom creation
    bool autoHeadroomEnabled = true;  // Enabled by default
    float trackedPeakLevel = 0.0f;
    float currentHeadroomGainDB = 0.0f;
    float peakFollowerCoeff = 0.99f;
    DSPUtils::SmoothedValue headroomGainSmoothed;

    bool chainEnabled = true;
};
