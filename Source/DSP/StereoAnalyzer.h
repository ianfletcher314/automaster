#pragma once

#include "DSPUtils.h"
#include <array>
#include <atomic>
#include <deque>
#include <mutex>

class StereoAnalyzer
{
public:
    static constexpr int NUM_BANDS = 3;
    static constexpr int CORRELATION_WINDOW = 2048;
    static constexpr int VECTORSCOPE_SIZE = 512;

    StereoAnalyzer() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        // Prepare crossovers for multiband analysis
        crossover1L.prepare(sampleRate);
        crossover1R.prepare(sampleRate);
        crossover2L.prepare(sampleRate);
        crossover2R.prepare(sampleRate);

        crossover1L.setCrossoverFrequency(200.0f);
        crossover1R.setCrossoverFrequency(200.0f);
        crossover2L.setCrossoverFrequency(3000.0f);
        crossover2R.setCrossoverFrequency(3000.0f);

        reset();
    }

    void reset()
    {
        crossover1L.reset();
        crossover1R.reset();
        crossover2L.reset();
        crossover2R.reset();

        correlationBufferL.clear();
        correlationBufferR.clear();

        globalCorrelation.store(1.0f);
        globalWidth.store(1.0f);
        balance.store(0.0f);

        for (int band = 0; band < NUM_BANDS; ++band)
        {
            bandCorrelation[band].store(1.0f);
            bandWidth[band].store(1.0f);
        }

        vectorscopeBuffer.fill({ 0.0f, 0.0f });
        vectorscopeIndex = 0;
    }

    void process(const float* left, const float* right, int numSamples)
    {
        float sumL = 0.0f, sumR = 0.0f;
        float sumL2 = 0.0f, sumR2 = 0.0f, sumLR = 0.0f;
        float sumMid2 = 0.0f, sumSide2 = 0.0f;

        // Per-band accumulators
        std::array<float, NUM_BANDS> bandSumL2 = {}, bandSumR2 = {}, bandSumLR = {};
        std::array<float, NUM_BANDS> bandSumMid2 = {}, bandSumSide2 = {};

        for (int i = 0; i < numSamples; ++i)
        {
            float L = left[i];
            float R = right[i];

            // Global statistics
            sumL += L;
            sumR += R;
            sumL2 += L * L;
            sumR2 += R * R;
            sumLR += L * R;

            float mid = (L + R) * 0.5f;
            float side = (L - R) * 0.5f;
            sumMid2 += mid * mid;
            sumSide2 += side * side;

            // Store for correlation calculation
            correlationBufferL.push_back(L);
            correlationBufferR.push_back(R);
            while (correlationBufferL.size() > CORRELATION_WINDOW)
            {
                correlationBufferL.pop_front();
                correlationBufferR.pop_front();
            }

            // Split into bands
            float lowL, midHighL, lowR, midHighR;
            crossover1L.process(L, lowL, midHighL);
            crossover1R.process(R, lowR, midHighR);

            float midL, highL, midR, highR;
            crossover2L.process(midHighL, midL, highL);
            crossover2R.process(midHighR, midR, highR);

            float bandsL[NUM_BANDS] = { lowL, midL, highL };
            float bandsR[NUM_BANDS] = { lowR, midR, highR };

            for (int band = 0; band < NUM_BANDS; ++band)
            {
                bandSumL2[band] += bandsL[band] * bandsL[band];
                bandSumR2[band] += bandsR[band] * bandsR[band];
                bandSumLR[band] += bandsL[band] * bandsR[band];

                float bMid = (bandsL[band] + bandsR[band]) * 0.5f;
                float bSide = (bandsL[band] - bandsR[band]) * 0.5f;
                bandSumMid2[band] += bMid * bMid;
                bandSumSide2[band] += bSide * bSide;
            }

            // Update vectorscope (downsampled)
            if ((i % 4) == 0)
            {
                std::lock_guard<std::mutex> lock(vectorscopeMutex);
                vectorscopeBuffer[vectorscopeIndex] = { L, R };
                vectorscopeIndex = (vectorscopeIndex + 1) % VECTORSCOPE_SIZE;
            }
        }

        // Calculate global correlation
        float denom = std::sqrt(sumL2 * sumR2);
        if (denom > 1e-10f)
            globalCorrelation.store(sumLR / denom);

        // Calculate global stereo width (side/mid ratio)
        if (sumMid2 > 1e-10f)
        {
            float width = std::sqrt(sumSide2 / sumMid2);
            globalWidth.store(std::min(2.0f, width));
        }

        // Calculate balance
        float totalEnergy = sumL2 + sumR2;
        if (totalEnergy > 1e-10f)
        {
            float bal = (sumR2 - sumL2) / totalEnergy;  // -1 = left, +1 = right
            balance.store(bal);
        }

        // Per-band calculations
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            float bandDenom = std::sqrt(bandSumL2[band] * bandSumR2[band]);
            if (bandDenom > 1e-10f)
                bandCorrelation[band].store(bandSumLR[band] / bandDenom);

            if (bandSumMid2[band] > 1e-10f)
            {
                float bWidth = std::sqrt(bandSumSide2[band] / bandSumMid2[band]);
                bandWidth[band].store(std::min(2.0f, bWidth));
            }
        }
    }

    // Getters
    float getCorrelation() const { return globalCorrelation.load(); }
    float getWidth() const { return globalWidth.load(); }
    float getBalance() const { return balance.load(); }

    float getBandCorrelation(int band) const
    {
        return (band >= 0 && band < NUM_BANDS) ? bandCorrelation[band].load() : 1.0f;
    }

    float getBandWidth(int band) const
    {
        return (band >= 0 && band < NUM_BANDS) ? bandWidth[band].load() : 1.0f;
    }

    // Get all stereo features
    struct StereoFeatures
    {
        float correlation;
        float width;
        float balance;
        std::array<float, NUM_BANDS> bandCorrelation;
        std::array<float, NUM_BANDS> bandWidth;
    };

    StereoFeatures getFeatures() const
    {
        StereoFeatures features;
        features.correlation = globalCorrelation.load();
        features.width = globalWidth.load();
        features.balance = balance.load();
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            features.bandCorrelation[band] = bandCorrelation[band].load();
            features.bandWidth[band] = bandWidth[band].load();
        }
        return features;
    }

    // Vectorscope data for UI
    std::array<std::pair<float, float>, VECTORSCOPE_SIZE> getVectorscopeBuffer() const
    {
        std::lock_guard<std::mutex> lock(vectorscopeMutex);
        return vectorscopeBuffer;
    }

private:
    double currentSampleRate = 44100.0;

    // Crossover filters for multiband analysis
    DSPUtils::LinkwitzRileyCrossover crossover1L, crossover1R;
    DSPUtils::LinkwitzRileyCrossover crossover2L, crossover2R;

    // Correlation buffers
    std::deque<float> correlationBufferL;
    std::deque<float> correlationBufferR;

    // Global measurements
    std::atomic<float> globalCorrelation { 1.0f };
    std::atomic<float> globalWidth { 1.0f };
    std::atomic<float> balance { 0.0f };

    // Per-band measurements
    std::array<std::atomic<float>, NUM_BANDS> bandCorrelation = { 1.0f, 1.0f, 1.0f };
    std::array<std::atomic<float>, NUM_BANDS> bandWidth = { 1.0f, 1.0f, 1.0f };

    // Vectorscope
    mutable std::mutex vectorscopeMutex;
    std::array<std::pair<float, float>, VECTORSCOPE_SIZE> vectorscopeBuffer;
    int vectorscopeIndex = 0;
};
