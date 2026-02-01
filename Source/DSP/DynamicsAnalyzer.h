#pragma once

#include "DSPUtils.h"
#include <array>
#include <atomic>
#include <deque>

class DynamicsAnalyzer
{
public:
    static constexpr int NUM_BANDS = 3;
    static constexpr int TRANSIENT_WINDOW = 512;

    DynamicsAnalyzer() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        // Prepare crossovers
        crossover1.prepare(sampleRate);
        crossover2.prepare(sampleRate);
        crossover1.setCrossoverFrequency(200.0f);
        crossover2.setCrossoverFrequency(3000.0f);

        // Prepare envelope followers
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            peakFollower[band].prepare(sampleRate);
            peakFollower[band].setAttackTime(0.1f);   // Fast peak
            peakFollower[band].setReleaseTime(300.0f);

            rmsFollower[band].prepare(sampleRate);
            rmsFollower[band].setAttackTime(10.0f);
            rmsFollower[band].setReleaseTime(300.0f);
        }

        reset();
    }

    void reset()
    {
        crossover1.reset();
        crossover2.reset();

        for (int band = 0; band < NUM_BANDS; ++band)
        {
            peakFollower[band].reset();
            rmsFollower[band].reset();
            crestFactor[band].store(0.0f);
        }

        transientBuffer.clear();
        transientDensity.store(0.0f);
        dynamicRange.store(0.0f);

        peakHistory.clear();
        rmsHistory.clear();
    }

    void process(const float* left, const float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = (left[i] + right[i]) * 0.5f;

            // Split into bands
            float low, midHigh, mid, high;
            crossover1.process(mono, low, midHigh);
            crossover2.process(midHigh, mid, high);

            float bands[NUM_BANDS] = { low, mid, high };

            // Analyze each band
            for (int band = 0; band < NUM_BANDS; ++band)
            {
                float peak = peakFollower[band].process(bands[band]);
                float rms = rmsFollower[band].processRMS(bands[band] * bands[band]);

                // Calculate crest factor (peak/RMS ratio in dB)
                if (rms > 1e-10f)
                {
                    float cf = DSPUtils::linearToDecibels(peak) - DSPUtils::linearToDecibels(rms);
                    crestFactor[band].store(cf);
                }
            }

            // Transient detection
            float absValue = std::abs(mono);
            transientBuffer.push_back(absValue);
            if (transientBuffer.size() > TRANSIENT_WINDOW)
                transientBuffer.pop_front();

            // Count transients (samples that exceed short-term average significantly)
            if (transientBuffer.size() == TRANSIENT_WINDOW)
            {
                float avg = 0.0f;
                for (float v : transientBuffer)
                    avg += v;
                avg /= TRANSIENT_WINDOW;

                if (absValue > avg * 3.0f)  // Transient threshold: 3x average
                    transientCount++;
            }

            sampleCount++;
        }

        // Update transient density every second
        int samplesPerSecond = static_cast<int>(currentSampleRate);
        if (sampleCount >= samplesPerSecond)
        {
            // Transients per second, normalized to 0-100 range
            float density = std::min(100.0f, transientCount * 10.0f);
            transientDensity.store(density);

            transientCount = 0;
            sampleCount = 0;
        }

        // Track overall dynamics
        float overallPeak = peakFollower[1].process((left[numSamples - 1] + right[numSamples - 1]) * 0.5f);
        float overallRMS = rmsFollower[1].processRMS(((left[numSamples - 1] + right[numSamples - 1]) * 0.5f));

        peakHistory.push_back(DSPUtils::linearToDecibels(overallPeak));
        rmsHistory.push_back(DSPUtils::linearToDecibels(overallRMS));

        // Keep 10 seconds of history at ~10 updates/sec
        while (peakHistory.size() > 100)
            peakHistory.pop_front();
        while (rmsHistory.size() > 100)
            rmsHistory.pop_front();

        // Calculate dynamic range from history
        if (peakHistory.size() >= 10)
        {
            float maxPeak = -100.0f, minRMS = 0.0f;
            for (float p : peakHistory)
                maxPeak = std::max(maxPeak, p);
            for (float r : rmsHistory)
                minRMS = std::min(minRMS, r);

            dynamicRange.store(maxPeak - minRMS);
        }
    }

    // Getters for analysis results
    float getCrestFactor(int band) const
    {
        return (band >= 0 && band < NUM_BANDS) ? crestFactor[band].load() : 0.0f;
    }

    float getAverageCrestFactor() const
    {
        float sum = 0.0f;
        for (int band = 0; band < NUM_BANDS; ++band)
            sum += crestFactor[band].load();
        return sum / NUM_BANDS;
    }

    float getTransientDensity() const { return transientDensity.load(); }
    float getDynamicRange() const { return dynamicRange.load(); }

    // Get all dynamics features as a struct
    struct DynamicsFeatures
    {
        std::array<float, NUM_BANDS> crestFactors;
        float transientDensity;
        float dynamicRange;
    };

    DynamicsFeatures getFeatures() const
    {
        DynamicsFeatures features;
        for (int band = 0; band < NUM_BANDS; ++band)
            features.crestFactors[band] = crestFactor[band].load();
        features.transientDensity = transientDensity.load();
        features.dynamicRange = dynamicRange.load();
        return features;
    }

private:
    double currentSampleRate = 44100.0;

    // Crossover filters
    DSPUtils::LinkwitzRileyCrossover crossover1;
    DSPUtils::LinkwitzRileyCrossover crossover2;

    // Per-band envelope followers
    DSPUtils::EnvelopeFollower peakFollower[NUM_BANDS];
    DSPUtils::EnvelopeFollower rmsFollower[NUM_BANDS];

    // Atomic crest factor per band
    std::array<std::atomic<float>, NUM_BANDS> crestFactor = { 0.0f, 0.0f, 0.0f };

    // Transient detection
    std::deque<float> transientBuffer;
    int transientCount = 0;
    int sampleCount = 0;
    std::atomic<float> transientDensity { 0.0f };

    // Dynamic range tracking
    std::deque<float> peakHistory;
    std::deque<float> rmsHistory;
    std::atomic<float> dynamicRange { 0.0f };
};
