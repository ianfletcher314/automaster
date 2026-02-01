#pragma once

#include "SpectralAnalyzer.h"
#include "DynamicsAnalyzer.h"
#include "StereoAnalyzer.h"
#include "ReferenceProfile.h"
#include "LoudnessMeter.h"
#include <mutex>
#include <thread>
#include <atomic>

class AnalysisEngine
{
public:
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
};
