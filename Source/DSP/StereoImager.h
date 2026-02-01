#pragma once

#include "DSPUtils.h"
#include <array>

class StereoImager
{
public:
    StereoImager() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        // Prepare crossovers for multiband width
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].prepare(sampleRate);
            crossover2[ch].prepare(sampleRate);
        }

        // Mono bass filter
        for (int ch = 0; ch < 2; ++ch)
            monoBassFilter[ch].makeLowPass(sampleRate, monoBassFreq, 0.707f);

        updateCrossovers();
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].reset();
            crossover2[ch].reset();
            monoBassState[ch].reset();
        }

        correlationBuffer.fill(0.0f);
        correlationBufferL.fill(0.0f);
        correlationBufferR.fill(0.0f);
        correlationIndex = 0;
        correlation.store(1.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypassed)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numChannels < 2)
            return;  // Stereo processing requires 2 channels

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = buffer.getSample(1, sample);

            // Calculate correlation for metering
            updateCorrelation(left, right);

            // Convert to M/S
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;

            if (multibandEnabled)
            {
                // Split into bands
                float lowL, midHighL, lowR, midHighR;
                crossover1[0].process(left, lowL, midHighL);
                crossover1[1].process(right, lowR, midHighR);

                float midL, highL, midR, highR;
                crossover2[0].process(midHighL, midL, highL);
                crossover2[1].process(midHighR, midR, highR);

                // Process each band with its own width
                processWidthBand(lowL, lowR, lowWidth);
                processWidthBand(midL, midR, midWidth);
                processWidthBand(highL, highR, highWidth);

                // Recombine bands
                left = lowL + midL + highL;
                right = lowR + midR + highR;
            }
            else
            {
                // Global width processing
                side *= globalWidth;
                left = mid + side;
                right = mid - side;
            }

            // Mono bass if enabled
            if (monoBassEnabled)
            {
                // Extract bass content
                float bassL = monoBassState[0].process(left, monoBassCoeffs);
                float bassR = monoBassState[1].process(right, monoBassCoeffs);

                // Make bass mono
                float bassMono = (bassL + bassR) * 0.5f;

                // Remove original bass and add mono bass
                left = (left - bassL) + bassMono;
                right = (right - bassR) + bassMono;
            }

            buffer.setSample(0, sample, left);
            buffer.setSample(1, sample, right);
        }
    }

    // Width controls
    void setGlobalWidth(float width)
    {
        globalWidth = juce::jlimit(0.0f, 2.0f, width);
    }

    void setLowWidth(float width)
    {
        lowWidth = juce::jlimit(0.0f, 2.0f, width);
    }

    void setMidWidth(float width)
    {
        midWidth = juce::jlimit(0.0f, 2.0f, width);
    }

    void setHighWidth(float width)
    {
        highWidth = juce::jlimit(0.0f, 2.0f, width);
    }

    void setMultibandEnabled(bool enabled)
    {
        multibandEnabled = enabled;
    }

    // Crossover controls
    void setLowMidCrossover(float freqHz)
    {
        lowMidCrossover = juce::jlimit(60.0f, 1000.0f, freqHz);
        updateCrossovers();
    }

    void setMidHighCrossover(float freqHz)
    {
        midHighCrossover = juce::jlimit(1000.0f, 10000.0f, freqHz);
        updateCrossovers();
    }

    // Mono bass controls
    void setMonoBassFrequency(float freqHz)
    {
        monoBassFreq = juce::jlimit(60.0f, 300.0f, freqHz);
        for (int ch = 0; ch < 2; ++ch)
            monoBassCoeffs.makeLowPass(currentSampleRate, monoBassFreq, 0.707f);
    }

    void setMonoBassEnabled(bool enabled)
    {
        monoBassEnabled = enabled;
    }

    // Global controls
    void setBypass(bool shouldBypass) { bypassed = shouldBypass; }
    bool isBypassed() const { return bypassed; }

    // Metering
    float getCorrelation() const { return correlation.load(); }

    // Getters
    float getGlobalWidth() const { return globalWidth; }
    float getLowWidth() const { return lowWidth; }
    float getMidWidth() const { return midWidth; }
    float getHighWidth() const { return highWidth; }
    float getMonoBassFrequency() const { return monoBassFreq; }

private:
    void updateCrossovers()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].setCrossoverFrequency(lowMidCrossover);
            crossover2[ch].setCrossoverFrequency(midHighCrossover);
        }
    }

    void processWidthBand(float& left, float& right, float width)
    {
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        side *= width;
        left = mid + side;
        right = mid - side;
    }

    void updateCorrelation(float left, float right)
    {
        // Store samples in circular buffer
        correlationBufferL[correlationIndex] = left;
        correlationBufferR[correlationIndex] = right;
        correlationBuffer[correlationIndex] = left * right;
        correlationIndex = (correlationIndex + 1) % CORRELATION_BUFFER_SIZE;

        // Calculate correlation every CORRELATION_BUFFER_SIZE samples
        if (correlationIndex == 0)
        {
            float sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
            for (int i = 0; i < CORRELATION_BUFFER_SIZE; ++i)
            {
                sumLR += correlationBufferL[i] * correlationBufferR[i];
                sumL2 += correlationBufferL[i] * correlationBufferL[i];
                sumR2 += correlationBufferR[i] * correlationBufferR[i];
            }

            float denom = std::sqrt(sumL2 * sumR2);
            float corr = denom > 0.0f ? sumLR / denom : 1.0f;
            correlation.store(corr);
        }
    }

    static constexpr int CORRELATION_BUFFER_SIZE = 2048;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool bypassed = false;

    // Width settings
    float globalWidth = 1.0f;
    float lowWidth = 1.0f;
    float midWidth = 1.0f;
    float highWidth = 1.0f;
    bool multibandEnabled = false;

    // Crossover settings
    float lowMidCrossover = 200.0f;
    float midHighCrossover = 3000.0f;
    DSPUtils::LinkwitzRileyCrossover crossover1[2];
    DSPUtils::LinkwitzRileyCrossover crossover2[2];

    // Mono bass
    float monoBassFreq = 120.0f;
    bool monoBassEnabled = false;
    DSPUtils::BiquadCoeffs monoBassCoeffs;
    DSPUtils::BiquadState monoBassState[2];
    DSPUtils::BiquadCoeffs monoBassFilter[2];

    // Correlation metering
    std::array<float, CORRELATION_BUFFER_SIZE> correlationBuffer;
    std::array<float, CORRELATION_BUFFER_SIZE> correlationBufferL;
    std::array<float, CORRELATION_BUFFER_SIZE> correlationBufferR;
    int correlationIndex = 0;
    std::atomic<float> correlation { 1.0f };
};
