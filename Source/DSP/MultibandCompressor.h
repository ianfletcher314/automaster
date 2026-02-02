#pragma once

#include "DSPUtils.h"
#include <array>

class MultibandCompressor
{
public:
    static constexpr int NUM_BANDS = 3;

    MultibandCompressor() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        // Prepare crossovers
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].prepare(sampleRate);
            crossover2[ch].prepare(sampleRate);
        }

        // Prepare envelope followers
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                envelopeFollowers[band][ch].prepare(sampleRate);
                envelopeFollowers[band][ch].setAttackTime(bandAttack[band]);
                envelopeFollowers[band][ch].setReleaseTime(bandRelease[band]);
            }
        }

        updateCrossovers();
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].reset();
            crossover2[ch].reset();
        }

        for (int band = 0; band < NUM_BANDS; ++band)
        {
            for (int ch = 0; ch < 2; ++ch)
                envelopeFollowers[band][ch].reset();
            gainReduction[band].store(0.0f);
        }
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypassed)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), 2);

        // Process sample by sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            std::array<float, 2> input;
            for (int ch = 0; ch < numChannels; ++ch)
                input[ch] = buffer.getSample(ch, sample);

            // Split into bands for each channel
            std::array<std::array<float, NUM_BANDS>, 2> bands;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float low, midHigh;
                crossover1[ch].process(input[ch], low, midHigh);

                float mid, high;
                crossover2[ch].process(midHigh, mid, high);

                bands[ch][0] = low;
                bands[ch][1] = mid;
                bands[ch][2] = high;
            }

            // Compress each band
            std::array<float, NUM_BANDS> bandGR = { 0.0f, 0.0f, 0.0f };

            for (int band = 0; band < NUM_BANDS; ++band)
            {
                if (!bandEnabled[band])
                    continue;

                float thresholdLinear = DSPUtils::decibelsToLinear(bandThreshold[band]);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    // Get envelope
                    float envelope = envelopeFollowers[band][ch].process(bands[ch][band]);

                    // Calculate gain reduction
                    float gr = 0.0f;
                    if (envelope > thresholdLinear)
                    {
                        float envelopeDB = DSPUtils::linearToDecibels(envelope);
                        float threshDB = bandThreshold[band];
                        float excessDB = envelopeDB - threshDB;

                        // Apply compression ratio
                        float compressedExcess = excessDB / bandRatio[band];
                        gr = excessDB - compressedExcess;
                    }

                    // Apply gain reduction + makeup
                    float gainDB = -gr + bandMakeup[band];
                    float gainLinear = DSPUtils::decibelsToLinear(gainDB);
                    bands[ch][band] *= gainLinear;

                    // Track max GR for metering
                    bandGR[band] = std::max(bandGR[band], gr);
                }
            }

            // Store gain reduction for metering
            for (int band = 0; band < NUM_BANDS; ++band)
                gainReduction[band].store(bandGR[band]);

            // Sum bands back together
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float output = bands[ch][0] + bands[ch][1] + bands[ch][2];
                buffer.setSample(ch, sample, output);
            }
        }
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

    // Per-band controls
    void setBandThreshold(int band, float thresholdDB)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandThreshold[band] = juce::jlimit(-60.0f, 0.0f, thresholdDB);
    }

    void setBandRatio(int band, float ratio)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandRatio[band] = juce::jlimit(1.0f, 20.0f, ratio);
    }

    void setBandAttack(int band, float attackMs)
    {
        if (band >= 0 && band < NUM_BANDS)
        {
            bandAttack[band] = juce::jlimit(0.1f, 100.0f, attackMs);
            for (int ch = 0; ch < 2; ++ch)
                envelopeFollowers[band][ch].setAttackTime(bandAttack[band]);
        }
    }

    void setBandRelease(int band, float releaseMs)
    {
        if (band >= 0 && band < NUM_BANDS)
        {
            bandRelease[band] = juce::jlimit(10.0f, 1000.0f, releaseMs);
            for (int ch = 0; ch < 2; ++ch)
                envelopeFollowers[band][ch].setReleaseTime(bandRelease[band]);
        }
    }

    void setBandMakeup(int band, float makeupDB)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandMakeup[band] = juce::jlimit(0.0f, 24.0f, makeupDB);
    }

    void setBandEnabled(int band, bool enabled)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandEnabled[band] = enabled;
    }

    // Global controls
    void setBypass(bool shouldBypass) { bypassed = shouldBypass; }
    bool isBypassed() const { return bypassed; }

    // Metering
    float getGainReduction(int band) const
    {
        return (band >= 0 && band < NUM_BANDS) ? gainReduction[band].load() : 0.0f;
    }

    float getMaxGainReduction() const
    {
        float maxGR = 0.0f;
        for (int band = 0; band < NUM_BANDS; ++band)
            maxGR = std::max(maxGR, gainReduction[band].load());
        return maxGR;
    }

    // Getters
    float getLowMidCrossover() const { return lowMidCrossover; }
    float getMidHighCrossover() const { return midHighCrossover; }
    float getBandThreshold(int band) const { return band >= 0 && band < NUM_BANDS ? bandThreshold[band] : -20.0f; }
    float getBandRatio(int band) const { return band >= 0 && band < NUM_BANDS ? bandRatio[band] : 4.0f; }
    float getBandAttack(int band) const { return band >= 0 && band < NUM_BANDS ? bandAttack[band] : 10.0f; }
    float getBandRelease(int band) const { return band >= 0 && band < NUM_BANDS ? bandRelease[band] : 100.0f; }
    float getBandMakeup(int band) const { return band >= 0 && band < NUM_BANDS ? bandMakeup[band] : 0.0f; }

private:
    void updateCrossovers()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            crossover1[ch].setCrossoverFrequency(lowMidCrossover);
            crossover2[ch].setCrossoverFrequency(midHighCrossover);
        }
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool bypassed = false;

    // Crossover frequencies
    float lowMidCrossover = 200.0f;
    float midHighCrossover = 3000.0f;

    // Crossover filters (Linkwitz-Riley 4th order)
    DSPUtils::LinkwitzRileyCrossover crossover1[2];  // Low-Mid split
    DSPUtils::LinkwitzRileyCrossover crossover2[2];  // Mid-High split

    // Per-band compressor settings (gentler defaults to preserve macrodynamics)
    std::array<float, NUM_BANDS> bandThreshold = { -10.0f, -8.0f, -6.0f };
    std::array<float, NUM_BANDS> bandRatio = { 2.0f, 2.0f, 2.0f };
    std::array<float, NUM_BANDS> bandAttack = { 20.0f, 10.0f, 5.0f };
    std::array<float, NUM_BANDS> bandRelease = { 200.0f, 150.0f, 100.0f };
    std::array<float, NUM_BANDS> bandMakeup = { 0.0f, 0.0f, 0.0f };
    std::array<bool, NUM_BANDS> bandEnabled = { true, true, true };

    // Envelope followers per band per channel
    DSPUtils::EnvelopeFollower envelopeFollowers[NUM_BANDS][2];

    // Gain reduction metering
    std::array<std::atomic<float>, NUM_BANDS> gainReduction = { 0.0f, 0.0f, 0.0f };
};
