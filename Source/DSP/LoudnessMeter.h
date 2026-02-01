#pragma once

#include "DSPUtils.h"
#include <deque>
#include <numeric>

class LoudnessMeter
{
public:
    LoudnessMeter() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        // K-weighting filter coefficients (ITU-R BS.1770-4)
        setupKWeightingFilters(sampleRate);

        // True peak detector
        truePeakDetectorL.prepare(sampleRate);
        truePeakDetectorR.prepare(sampleRate);

        reset();
    }

    void reset()
    {
        // Reset K-weighting filter states
        for (int ch = 0; ch < 2; ++ch)
        {
            kWeightState1[ch].reset();
            kWeightState2[ch].reset();
        }

        // Reset metering values
        momentaryLUFS.store(MINUS_INFINITY);
        shortTermLUFS.store(MINUS_INFINITY);
        integratedLUFS.store(MINUS_INFINITY);
        loudnessRange.store(0.0f);
        peakLevelL.store(MINUS_INFINITY);
        peakLevelR.store(MINUS_INFINITY);
        truePeakL.store(MINUS_INFINITY);
        truePeakR.store(MINUS_INFINITY);

        // Clear buffers
        momentaryBuffer.clear();
        shortTermBuffer.clear();
        integratedBlocks.clear();

        truePeakDetectorL.reset();
        truePeakDetectorR.reset();

        blockSampleCount = 0;
        currentBlockPower = 0.0f;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), 2);

        if (numChannels < 1)
            return;

        // Process peak levels
        float peakL = 0.0f, peakR = 0.0f;
        float truePeakLVal = 0.0f, truePeakRVal = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = numChannels > 1 ? buffer.getSample(1, sample) : left;

            // Sample peak
            peakL = std::max(peakL, std::abs(left));
            peakR = std::max(peakR, std::abs(right));

            // True peak with oversampling
            truePeakLVal = std::max(truePeakLVal, truePeakDetectorL.process(left));
            truePeakRVal = std::max(truePeakRVal, truePeakDetectorR.process(right));

            // K-weighted filtering for loudness
            float kWeightedL = processKWeighting(left, 0);
            float kWeightedR = processKWeighting(right, 1);

            // Accumulate power for current 100ms block
            currentBlockPower += kWeightedL * kWeightedL + kWeightedR * kWeightedR;
            blockSampleCount++;

            // Check if we've completed a 100ms block
            int samplesPerBlock = static_cast<int>(currentSampleRate * 0.1);
            if (blockSampleCount >= samplesPerBlock)
            {
                float meanSquare = currentBlockPower / (2.0f * blockSampleCount);
                float blockLoudness = -0.691f + 10.0f * std::log10(std::max(meanSquare, 1e-10f));

                // Add to windowed buffers
                momentaryBuffer.push_back(meanSquare);
                shortTermBuffer.push_back(meanSquare);

                // Keep 4 blocks for momentary (400ms)
                while (momentaryBuffer.size() > 4)
                    momentaryBuffer.pop_front();

                // Keep 30 blocks for short-term (3s)
                while (shortTermBuffer.size() > 30)
                    shortTermBuffer.pop_front();

                // Calculate momentary loudness
                if (!momentaryBuffer.empty())
                {
                    float momMean = std::accumulate(momentaryBuffer.begin(), momentaryBuffer.end(), 0.0f)
                                    / momentaryBuffer.size();
                    float momLUFS = -0.691f + 10.0f * std::log10(std::max(momMean, 1e-10f));
                    momentaryLUFS.store(momLUFS);
                }

                // Calculate short-term loudness
                if (!shortTermBuffer.empty())
                {
                    float stMean = std::accumulate(shortTermBuffer.begin(), shortTermBuffer.end(), 0.0f)
                                   / shortTermBuffer.size();
                    float stLUFS = -0.691f + 10.0f * std::log10(std::max(stMean, 1e-10f));
                    shortTermLUFS.store(stLUFS);
                }

                // Integrated loudness with gating
                if (blockLoudness > ABSOLUTE_GATE)
                {
                    integratedBlocks.push_back(meanSquare);
                    updateIntegratedLoudness();
                }

                // Reset block accumulators
                currentBlockPower = 0.0f;
                blockSampleCount = 0;
            }
        }

        // Store peak values
        peakLevelL.store(DSPUtils::linearToDecibels(peakL));
        peakLevelR.store(DSPUtils::linearToDecibels(peakR));
        truePeakL.store(DSPUtils::linearToDecibels(truePeakLVal));
        truePeakR.store(DSPUtils::linearToDecibels(truePeakRVal));
    }

    // Getters for metering values
    float getMomentaryLUFS() const { return momentaryLUFS.load(); }
    float getShortTermLUFS() const { return shortTermLUFS.load(); }
    float getIntegratedLUFS() const { return integratedLUFS.load(); }
    float getLoudnessRange() const { return loudnessRange.load(); }
    float getPeakLevelL() const { return peakLevelL.load(); }
    float getPeakLevelR() const { return peakLevelR.load(); }
    float getTruePeakL() const { return truePeakL.load(); }
    float getTruePeakR() const { return truePeakR.load(); }
    float getMaxTruePeak() const { return std::max(truePeakL.load(), truePeakR.load()); }

    void resetIntegratedLoudness()
    {
        integratedBlocks.clear();
        integratedLUFS.store(MINUS_INFINITY);
        loudnessRange.store(0.0f);
    }

private:
    static constexpr float MINUS_INFINITY = -100.0f;
    static constexpr float ABSOLUTE_GATE = -70.0f;  // LUFS
    static constexpr float RELATIVE_GATE = -10.0f;  // dB below ungated loudness

    void setupKWeightingFilters(double sampleRate)
    {
        // Stage 1: High shelf (pre-filter)
        // +4dB shelving at high frequencies
        float f0 = 1681.974450955533f;
        float G = 3.999843853973347f;
        float Q = 0.7071752369554196f;

        float K = std::tan(DSPUtils::PI * f0 / static_cast<float>(sampleRate));
        float Vh = std::pow(10.0f, G / 20.0f);
        float Vb = std::pow(Vh, 0.4996667741545416f);

        float a0 = 1.0f + K / Q + K * K;
        kWeight1Coeffs.b0 = (Vh + Vb * K / Q + K * K) / a0;
        kWeight1Coeffs.b1 = 2.0f * (K * K - Vh) / a0;
        kWeight1Coeffs.b2 = (Vh - Vb * K / Q + K * K) / a0;
        kWeight1Coeffs.a1 = 2.0f * (K * K - 1.0f) / a0;
        kWeight1Coeffs.a2 = (1.0f - K / Q + K * K) / a0;

        // Stage 2: High-pass (RLB weighting)
        float f1 = 38.13547087602444f;
        float Q1 = 0.5003270373238773f;

        float K1 = std::tan(DSPUtils::PI * f1 / static_cast<float>(sampleRate));
        float a0_2 = 1.0f + K1 / Q1 + K1 * K1;

        kWeight2Coeffs.b0 = 1.0f / a0_2;
        kWeight2Coeffs.b1 = -2.0f / a0_2;
        kWeight2Coeffs.b2 = 1.0f / a0_2;
        kWeight2Coeffs.a1 = 2.0f * (K1 * K1 - 1.0f) / a0_2;
        kWeight2Coeffs.a2 = (1.0f - K1 / Q1 + K1 * K1) / a0_2;
    }

    float processKWeighting(float input, int channel)
    {
        float stage1 = kWeightState1[channel].process(input, kWeight1Coeffs);
        return kWeightState2[channel].process(stage1, kWeight2Coeffs);
    }

    void updateIntegratedLoudness()
    {
        if (integratedBlocks.empty())
            return;

        // First pass: ungated loudness
        float sum = std::accumulate(integratedBlocks.begin(), integratedBlocks.end(), 0.0f);
        float ungatedLoudness = -0.691f + 10.0f * std::log10(sum / integratedBlocks.size());

        // Second pass: apply relative gate
        float relativeThreshold = ungatedLoudness + RELATIVE_GATE;
        float relativeThresholdLinear = std::pow(10.0f, (relativeThreshold + 0.691f) / 10.0f);

        float gatedSum = 0.0f;
        int gatedCount = 0;

        std::vector<float> gatedBlocks;

        for (float block : integratedBlocks)
        {
            if (block >= relativeThresholdLinear)
            {
                gatedSum += block;
                gatedCount++;
                gatedBlocks.push_back(-0.691f + 10.0f * std::log10(block));
            }
        }

        if (gatedCount > 0)
        {
            float intLUFS = -0.691f + 10.0f * std::log10(gatedSum / gatedCount);
            integratedLUFS.store(intLUFS);

            // Calculate loudness range (LRA)
            if (gatedBlocks.size() >= 2)
            {
                std::sort(gatedBlocks.begin(), gatedBlocks.end());
                int low10Index = static_cast<int>(gatedBlocks.size() * 0.1f);
                int high95Index = static_cast<int>(gatedBlocks.size() * 0.95f);
                high95Index = std::min(high95Index, static_cast<int>(gatedBlocks.size()) - 1);
                loudnessRange.store(gatedBlocks[high95Index] - gatedBlocks[low10Index]);
            }
        }
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // K-weighting filters
    DSPUtils::BiquadCoeffs kWeight1Coeffs;
    DSPUtils::BiquadCoeffs kWeight2Coeffs;
    DSPUtils::BiquadState kWeightState1[2];
    DSPUtils::BiquadState kWeightState2[2];

    // True peak detection
    DSPUtils::TruePeakDetector truePeakDetectorL;
    DSPUtils::TruePeakDetector truePeakDetectorR;

    // Loudness measurement buffers
    std::deque<float> momentaryBuffer;
    std::deque<float> shortTermBuffer;
    std::vector<float> integratedBlocks;

    // Block accumulation
    float currentBlockPower = 0.0f;
    int blockSampleCount = 0;

    // Atomic metering outputs
    std::atomic<float> momentaryLUFS { MINUS_INFINITY };
    std::atomic<float> shortTermLUFS { MINUS_INFINITY };
    std::atomic<float> integratedLUFS { MINUS_INFINITY };
    std::atomic<float> loudnessRange { 0.0f };
    std::atomic<float> peakLevelL { MINUS_INFINITY };
    std::atomic<float> peakLevelR { MINUS_INFINITY };
    std::atomic<float> truePeakL { MINUS_INFINITY };
    std::atomic<float> truePeakR { MINUS_INFINITY };
};
