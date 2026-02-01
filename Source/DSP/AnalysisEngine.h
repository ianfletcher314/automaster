#pragma once

#include "SpectralAnalyzer.h"
#include "DynamicsAnalyzer.h"
#include "StereoAnalyzer.h"
#include "ReferenceProfile.h"
#include "LoudnessMeter.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

class AnalysisEngine
{
public:
    // Accumulated analysis data (Ozone-style workflow)
    struct AccumulatedAnalysis
    {
        std::array<float, 32> avgSpectrum = {};
        float avgLUFS = -60.0f;
        float peakLUFS = -60.0f;
        float avgWidth = 1.0f;
        float avgCorrelation = 1.0f;
        float avgCrestFactor = 12.0f;
        int sampleCount = 0;
        bool isValid = false;

        // Running sums for averaging
        std::array<double, 32> spectrumSum = {};
        double lufsSum = 0.0;
        double widthSum = 0.0;
        double correlationSum = 0.0;
        double crestSum = 0.0;
    };

    AnalysisEngine() = default;

    ~AnalysisEngine()
    {
        stopAnalysis();
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        spectralAnalyzer.prepare(sampleRate, samplesPerBlock);
        dynamicsAnalyzer.prepare(sampleRate, samplesPerBlock);
        stereoAnalyzer.prepare(sampleRate, samplesPerBlock);
        loudnessMeter.prepare(sampleRate, samplesPerBlock);

        reset();
    }

    void reset()
    {
        spectralAnalyzer.reset();
        dynamicsAnalyzer.reset();
        stereoAnalyzer.reset();
        loudnessMeter.reset();

        analysisValid.store(false);
        referenceMatchScore.store(0.0f);
    }

    // Process audio for analysis (called from audio thread)
    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numChannels < 1)
            return;

        // Loudness metering (non-destructive copy)
        juce::AudioBuffer<float> meterCopy(buffer);
        loudnessMeter.process(meterCopy);

        // Get pointers for analysis
        const float* left = buffer.getReadPointer(0);
        const float* right = numChannels > 1 ? buffer.getReadPointer(1) : left;

        // Feed analyzers
        spectralAnalyzer.pushStereoSamples(left, right, numSamples);
        dynamicsAnalyzer.process(left, right, numSamples);
        stereoAnalyzer.process(left, right, numSamples);

        // Update reference match if we have a reference
        updateReferenceMatch();

        // Accumulate data if in accumulation mode
        if (isAccumulating.load())
            accumulateData();

        analysisValid.store(true);
    }

    // Set reference profile
    void setReferenceProfile(const ReferenceProfile& profile)
    {
        std::lock_guard<std::mutex> lock(referenceMutex);
        referenceProfile = profile;
        hasReference = profile.isProfileValid();
    }

    void clearReferenceProfile()
    {
        std::lock_guard<std::mutex> lock(referenceMutex);
        hasReference = false;
        referenceMatchScore.store(0.0f);
    }

    bool hasReferenceProfile() const { return hasReference; }

    // Get aggregated analysis results
    struct AnalysisResults
    {
        // Spectral
        DSPUtils::SpectralFeatures spectral;
        std::array<float, 32> bandEnergies;

        // Dynamics
        DynamicsAnalyzer::DynamicsFeatures dynamics;

        // Stereo
        StereoAnalyzer::StereoFeatures stereo;

        // Loudness
        float momentaryLUFS;
        float shortTermLUFS;
        float integratedLUFS;
        float truePeak;
        float loudnessRange;

        // Reference match
        float referenceMatchScore;
        bool hasReference;
    };

    AnalysisResults getResults() const
    {
        AnalysisResults results;

        results.spectral = spectralAnalyzer.getSpectralFeatures();
        results.bandEnergies = spectralAnalyzer.getBandEnergies();
        results.dynamics = dynamicsAnalyzer.getFeatures();
        results.stereo = stereoAnalyzer.getFeatures();

        results.momentaryLUFS = loudnessMeter.getMomentaryLUFS();
        results.shortTermLUFS = loudnessMeter.getShortTermLUFS();
        results.integratedLUFS = loudnessMeter.getIntegratedLUFS();
        results.truePeak = loudnessMeter.getMaxTruePeak();
        results.loudnessRange = loudnessMeter.getLoudnessRange();

        results.referenceMatchScore = referenceMatchScore.load();
        results.hasReference = hasReference;

        return results;
    }

    // Individual analyzer access
    const SpectralAnalyzer& getSpectralAnalyzer() const { return spectralAnalyzer; }
    const DynamicsAnalyzer& getDynamicsAnalyzer() const { return dynamicsAnalyzer; }
    const StereoAnalyzer& getStereoAnalyzer() const { return stereoAnalyzer; }
    const LoudnessMeter& getLoudnessMeter() const { return loudnessMeter; }

    // Quick access to common values
    float getShortTermLUFS() const { return loudnessMeter.getShortTermLUFS(); }
    float getTruePeak() const { return loudnessMeter.getMaxTruePeak(); }
    float getCorrelation() const { return stereoAnalyzer.getCorrelation(); }
    float getWidth() const { return stereoAnalyzer.getWidth(); }
    float getCrestFactor() const { return dynamicsAnalyzer.getAverageCrestFactor(); }
    float getSpectralCentroid() const { return spectralAnalyzer.getSpectralCentroid(); }
    float getReferenceMatchScore() const { return referenceMatchScore.load(); }

    bool isAnalysisValid() const { return analysisValid.load(); }

    void resetIntegratedLoudness()
    {
        loudnessMeter.resetIntegratedLoudness();
    }

    // =========================================================================
    // ACCUMULATION METHODS (Ozone-style workflow)
    // =========================================================================

    void startAccumulation()
    {
        std::lock_guard<std::mutex> lock(accumulationMutex);
        accumulatedAnalysis = AccumulatedAnalysis{};  // Reset
        accumulationStartTime = std::chrono::steady_clock::now();
        isAccumulating.store(true);
    }

    void stopAccumulation()
    {
        std::lock_guard<std::mutex> lock(accumulationMutex);
        isAccumulating.store(false);

        if (accumulatedAnalysis.sampleCount > 0)
        {
            // Finalize averages
            float count = static_cast<float>(accumulatedAnalysis.sampleCount);
            for (int i = 0; i < 32; ++i)
                accumulatedAnalysis.avgSpectrum[i] = static_cast<float>(accumulatedAnalysis.spectrumSum[i] / count);

            accumulatedAnalysis.avgLUFS = static_cast<float>(accumulatedAnalysis.lufsSum / count);
            accumulatedAnalysis.avgWidth = static_cast<float>(accumulatedAnalysis.widthSum / count);
            accumulatedAnalysis.avgCorrelation = static_cast<float>(accumulatedAnalysis.correlationSum / count);
            accumulatedAnalysis.avgCrestFactor = static_cast<float>(accumulatedAnalysis.crestSum / count);
            accumulatedAnalysis.isValid = true;
        }
    }

    bool isAccumulationActive() const { return isAccumulating.load(); }
    bool hasValidAccumulation() const { return accumulatedAnalysis.isValid; }

    float getAccumulationProgress() const
    {
        if (!isAccumulating.load())
            return accumulatedAnalysis.isValid ? 1.0f : 0.0f;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - accumulationStartTime).count();
        return juce::jlimit(0.0f, 1.0f, static_cast<float>(elapsed) / (accumulationDurationMs));
    }

    float getAccumulationTimeSeconds() const
    {
        if (!isAccumulating.load())
            return 0.0f;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - accumulationStartTime).count();
        return static_cast<float>(elapsed) / 1000.0f;
    }

    void setAccumulationDuration(float seconds)
    {
        accumulationDurationMs = static_cast<int>(seconds * 1000.0f);
    }

    float getAccumulationDuration() const
    {
        return static_cast<float>(accumulationDurationMs) / 1000.0f;
    }

    void resetAccumulation()
    {
        std::lock_guard<std::mutex> lock(accumulationMutex);
        isAccumulating.store(false);
        accumulatedAnalysis = AccumulatedAnalysis{};
    }

    // Get accumulated results converted to AnalysisResults format
    AnalysisResults getAccumulatedResults() const
    {
        std::lock_guard<std::mutex> lock(accumulationMutex);

        AnalysisResults results;

        if (accumulatedAnalysis.isValid)
        {
            results.bandEnergies = accumulatedAnalysis.avgSpectrum;
            results.shortTermLUFS = accumulatedAnalysis.avgLUFS;
            results.momentaryLUFS = accumulatedAnalysis.avgLUFS;
            results.integratedLUFS = accumulatedAnalysis.avgLUFS;
            results.truePeak = accumulatedAnalysis.peakLUFS + 3.0f;  // Rough estimate
            results.loudnessRange = 6.0f;  // Default

            results.stereo.width = accumulatedAnalysis.avgWidth;
            results.stereo.correlation = accumulatedAnalysis.avgCorrelation;

            // Fill crest factors array with the average value
            for (auto& cf : results.dynamics.crestFactors)
                cf = accumulatedAnalysis.avgCrestFactor;

            results.referenceMatchScore = referenceMatchScore.load();
            results.hasReference = hasReference;
        }
        else
        {
            // Return current real-time results as fallback
            return getResults();
        }

        return results;
    }

    const AccumulatedAnalysis& getAccumulatedAnalysis() const { return accumulatedAnalysis; }

private:
    void accumulateData()
    {
        if (!isAccumulating.load())
            return;

        std::lock_guard<std::mutex> lock(accumulationMutex);

        // Get current analysis values
        auto spectrum = spectralAnalyzer.getBandEnergies();
        float lufs = loudnessMeter.getShortTermLUFS();
        float width = stereoAnalyzer.getWidth();
        float correlation = stereoAnalyzer.getCorrelation();
        float crest = dynamicsAnalyzer.getAverageCrestFactor();

        // Skip invalid readings
        if (lufs < -70.0f)
            return;

        // Accumulate
        for (int i = 0; i < 32; ++i)
            accumulatedAnalysis.spectrumSum[i] += spectrum[i];

        accumulatedAnalysis.lufsSum += lufs;
        accumulatedAnalysis.widthSum += width;
        accumulatedAnalysis.correlationSum += correlation;
        accumulatedAnalysis.crestSum += crest;

        // Track peak
        if (lufs > accumulatedAnalysis.peakLUFS)
            accumulatedAnalysis.peakLUFS = lufs;

        accumulatedAnalysis.sampleCount++;

        // Auto-stop after duration
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - accumulationStartTime).count();
        if (elapsed >= accumulationDurationMs)
        {
            // Stop without lock (we already have it)
            isAccumulating.store(false);

            if (accumulatedAnalysis.sampleCount > 0)
            {
                float count = static_cast<float>(accumulatedAnalysis.sampleCount);
                for (int i = 0; i < 32; ++i)
                    accumulatedAnalysis.avgSpectrum[i] = static_cast<float>(accumulatedAnalysis.spectrumSum[i] / count);

                accumulatedAnalysis.avgLUFS = static_cast<float>(accumulatedAnalysis.lufsSum / count);
                accumulatedAnalysis.avgWidth = static_cast<float>(accumulatedAnalysis.widthSum / count);
                accumulatedAnalysis.avgCorrelation = static_cast<float>(accumulatedAnalysis.correlationSum / count);
                accumulatedAnalysis.avgCrestFactor = static_cast<float>(accumulatedAnalysis.crestSum / count);
                accumulatedAnalysis.isValid = true;
            }
        }
    }

private:
    void updateReferenceMatch()
    {
        if (!hasReference)
            return;

        std::lock_guard<std::mutex> lock(referenceMutex);

        auto bandEnergies = spectralAnalyzer.getBandEnergies();
        float currentLoudness = loudnessMeter.getShortTermLUFS();
        float currentWidth = stereoAnalyzer.getWidth();
        float currentCorrelation = stereoAnalyzer.getCorrelation();

        float score = referenceProfile.calculateMatchScore(
            bandEnergies, currentLoudness, currentWidth, currentCorrelation);

        referenceMatchScore.store(score);
    }

    void stopAnalysis()
    {
        // Cleanup if needed
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Analyzers
    SpectralAnalyzer spectralAnalyzer;
    DynamicsAnalyzer dynamicsAnalyzer;
    StereoAnalyzer stereoAnalyzer;
    LoudnessMeter loudnessMeter;

    // Reference profile
    mutable std::mutex referenceMutex;
    ReferenceProfile referenceProfile;
    bool hasReference = false;

    // State
    std::atomic<bool> analysisValid { false };
    std::atomic<float> referenceMatchScore { 0.0f };

    // Accumulation state (Ozone-style workflow)
    mutable std::mutex accumulationMutex;
    std::atomic<bool> isAccumulating { false };
    AccumulatedAnalysis accumulatedAnalysis;
    std::chrono::steady_clock::time_point accumulationStartTime;
    int accumulationDurationMs = 10000;  // Default 10 seconds
};
