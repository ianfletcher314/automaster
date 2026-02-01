#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/MasteringChain.h"
#include "DSP/AnalysisEngine.h"
#include "DSP/ParameterGenerator.h"
#include "DSP/ReferenceProfile.h"
#include "AI/RulesEngine.h"
#include "AI/LearningSystem.h"
#include "AI/FeatureExtractor.h"

class AutomasterAudioProcessor : public juce::AudioProcessor
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

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

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

    // Record user adjustment for learning
    void recordUserAdjustment();

    // A/B/C/D comparison states
    void storeState(int slot);  // 0-3 for A-D
    void recallState(int slot);

    // Get current generated parameters for UI display
    const ParameterGenerator::GeneratedParameters& getLastGeneratedParams() const
    {
        return lastGeneratedParams;
    }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
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

    // Parameter tree
    juce::AudioProcessorValueTreeState apvts;

    // Cached parameter pointers
    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* targetLUFSParam = nullptr;
    std::atomic<float>* autoMasterEnabledParam = nullptr;

    // EQ parameters
    std::atomic<float>* hpfFreqParam = nullptr;
    std::atomic<float>* hpfEnabledParam = nullptr;
    std::atomic<float>* lpfFreqParam = nullptr;
    std::atomic<float>* lpfEnabledParam = nullptr;
    std::atomic<float>* lowShelfFreqParam = nullptr;
    std::atomic<float>* lowShelfGainParam = nullptr;
    std::atomic<float>* highShelfFreqParam = nullptr;
    std::atomic<float>* highShelfGainParam = nullptr;
    std::array<std::atomic<float>*, 4> bandFreqParams = {};
    std::array<std::atomic<float>*, 4> bandGainParams = {};
    std::array<std::atomic<float>*, 4> bandQParams = {};
    std::atomic<float>* eqBypassParam = nullptr;

    // Compressor parameters
    std::atomic<float>* lowMidXoverParam = nullptr;
    std::atomic<float>* midHighXoverParam = nullptr;
    std::array<std::atomic<float>*, 3> compThresholdParams = {};
    std::array<std::atomic<float>*, 3> compRatioParams = {};
    std::array<std::atomic<float>*, 3> compAttackParams = {};
    std::array<std::atomic<float>*, 3> compReleaseParams = {};
    std::array<std::atomic<float>*, 3> compMakeupParams = {};
    std::atomic<float>* compBypassParam = nullptr;

    // Stereo parameters
    std::atomic<float>* globalWidthParam = nullptr;
    std::atomic<float>* lowWidthParam = nullptr;
    std::atomic<float>* midWidthParam = nullptr;
    std::atomic<float>* highWidthParam = nullptr;
    std::atomic<float>* monoBassFreqParam = nullptr;
    std::atomic<float>* monoBassEnabledParam = nullptr;
    std::atomic<float>* stereoBypassParam = nullptr;

    // Limiter parameters
    std::atomic<float>* ceilingParam = nullptr;
    std::atomic<float>* limiterReleaseParam = nullptr;
    std::atomic<float>* limiterBypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomasterAudioProcessor)
};
