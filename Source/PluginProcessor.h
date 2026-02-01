#pragma once

#include <gin_plugin/gin_plugin.h>
#include "DSP/MasteringChain.h"
#include "DSP/AnalysisEngine.h"
#include "DSP/ParameterGenerator.h"
#include "DSP/ReferenceProfile.h"
#include "AI/RulesEngine.h"
#include "AI/LearningSystem.h"
#include "AI/FeatureExtractor.h"

class AutomasterAudioProcessor : public gin::Processor
{
public:
    AutomasterAudioProcessor();
    ~AutomasterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Module access for UI
    MasteringChain& getMasteringChain() { return masteringChain; }
    AnalysisEngine& getAnalysisEngine() { return analysisEngine; }
    RulesEngine& getRulesEngine() { return rulesEngine; }
    LearningSystem& getLearningSystem() { return learningSystem; }

    // Reference profile management
    bool loadReferenceFile(const juce::File& file);
    void clearReference();
    const ReferenceProfile& getReferenceProfile() const { return currentReference; }
    bool hasReference() const { return currentReference.isProfileValid(); }

    // Auto-master trigger
    void triggerAutoMaster();
    void applyGeneratedParameters(const ParameterGenerator::GeneratedParameters& params);

    // Accumulation controls (Ozone-style workflow)
    void startAnalysis();
    void stopAnalysis();
    bool isAnalyzing() const { return analysisEngine.isAccumulationActive(); }
    bool hasValidAnalysis() const { return analysisEngine.hasValidAccumulation(); }
    float getAnalysisProgress() const { return analysisEngine.getAccumulationProgress(); }
    float getAnalysisTimeSeconds() const { return analysisEngine.getAccumulationTimeSeconds(); }
    void resetAnalysis() { analysisEngine.resetAccumulation(); }

    // Record user adjustment for learning
    void recordUserAdjustment();

    // A/B/C/D comparison states
    void storeState(int slot);
    void recallState(int slot);

    // Get current generated parameters for UI display
    const ParameterGenerator::GeneratedParameters& getLastGeneratedParams() const
    {
        return lastGeneratedParams;
    }

    // Parameter pointers for UI access
    // Global
    gin::Parameter::Ptr inputGain;
    gin::Parameter::Ptr outputGain;
    gin::Parameter::Ptr targetLUFS;
    gin::Parameter::Ptr autoMasterEnabled;

    // EQ
    gin::Parameter::Ptr hpfFreq;
    gin::Parameter::Ptr hpfEnabled;
    gin::Parameter::Ptr lpfFreq;
    gin::Parameter::Ptr lpfEnabled;
    gin::Parameter::Ptr lowShelfFreq;
    gin::Parameter::Ptr lowShelfGain;
    gin::Parameter::Ptr highShelfFreq;
    gin::Parameter::Ptr highShelfGain;
    std::array<gin::Parameter::Ptr, 4> bandFreq;
    std::array<gin::Parameter::Ptr, 4> bandGain;
    std::array<gin::Parameter::Ptr, 4> bandQ;
    gin::Parameter::Ptr eqBypass;

    // Compressor
    gin::Parameter::Ptr lowMidXover;
    gin::Parameter::Ptr midHighXover;
    std::array<gin::Parameter::Ptr, 3> compThreshold;
    std::array<gin::Parameter::Ptr, 3> compRatio;
    std::array<gin::Parameter::Ptr, 3> compAttack;
    std::array<gin::Parameter::Ptr, 3> compRelease;
    std::array<gin::Parameter::Ptr, 3> compMakeup;
    gin::Parameter::Ptr compBypass;

    // Stereo
    gin::Parameter::Ptr globalWidth;
    gin::Parameter::Ptr lowWidth;
    gin::Parameter::Ptr midWidth;
    gin::Parameter::Ptr highWidth;
    gin::Parameter::Ptr monoBassFreq;
    gin::Parameter::Ptr monoBassEnabled;
    gin::Parameter::Ptr stereoBypass;

    // Limiter
    gin::Parameter::Ptr ceiling;
    gin::Parameter::Ptr limiterRelease;
    gin::Parameter::Ptr limiterBypass;

private:
    void updateProcessingFromParameters();

    // Processing
    MasteringChain masteringChain;

    // Analysis
    AnalysisEngine analysisEngine;
    FeatureExtractor featureExtractor;

    // AI
    RulesEngine rulesEngine;
    LearningSystem learningSystem;

    // Reference
    ReferenceProfile currentReference;

    // State
    ParameterGenerator::GeneratedParameters lastGeneratedParams;
    ParameterGenerator::GeneratedParameters userCurrentParams;

    // A/B/C/D comparison
    struct SavedState
    {
        juce::MemoryBlock parameterState;
        bool isValid = false;
    };
    std::array<SavedState, 4> comparisonStates;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomasterAudioProcessor)
};
