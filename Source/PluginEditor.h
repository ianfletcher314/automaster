#pragma once

#include "PluginProcessor.h"
#include "UI/MeterComponents.h"
#include "UI/ReferenceWaveform.h"
#include "UI/ProcessingChainView.h"

class AutomasterAudioProcessorEditor : public gin::ProcessorEditor,
                                        public juce::FileDragAndDropTarget,
                                        private juce::KeyListener
{
public:
    AutomasterAudioProcessorEditor(AutomasterAudioProcessor&);
    ~AutomasterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:
    AutomasterAudioProcessor& proc;

    // Top bar
    juce::Label titleLabel;
    std::array<juce::TextButton, 4> abcdButtons;
    juce::TextButton settingsButton;

    // Reference section
    ReferenceWaveform referenceWaveform;
    juce::TextButton loadRefButton;
    MatchIndicator matchIndicator;
    juce::ComboBox profileCombo;

    // Mode section
    gin::Knob targetLUFSKnob;
    juce::TextButton autoMasterButton;

    // Processing chain view
    ProcessingChainView chainView;

    // EQ knobs
    gin::Knob hpfFreqKnob, lpfFreqKnob;
    gin::Switch hpfEnableSwitch, lpfEnableSwitch;
    gin::Knob lowShelfFreqKnob, lowShelfGainKnob;
    gin::Knob highShelfFreqKnob, highShelfGainKnob;
    std::array<gin::Knob*, 4> bandFreqKnobs;
    std::array<gin::Knob*, 4> bandGainKnobs;
    std::array<gin::Knob*, 4> bandQKnobs;
    gin::Switch eqBypassSwitch;

    // Compressor knobs
    gin::Knob lowMidXoverKnob, midHighXoverKnob;
    std::array<gin::Knob*, 3> compThresholdKnobs;
    std::array<gin::Knob*, 3> compRatioKnobs;
    std::array<gin::Knob*, 3> compAttackKnobs;
    std::array<gin::Knob*, 3> compReleaseKnobs;
    std::array<gin::Knob*, 3> compMakeupKnobs;
    std::array<GainReductionMeter, 3> compGRMeters;
    gin::Switch compBypassSwitch;

    // Stereo knobs
    gin::Knob globalWidthKnob;
    gin::Knob lowWidthKnob, midWidthKnob, highWidthKnob;
    gin::Knob monoBassFreqKnob;
    gin::Switch monoBassEnableSwitch;
    gin::Switch stereoBypassSwitch;
    CorrelationMeter correlationMeter;

    // Limiter knobs
    gin::Knob ceilingKnob;
    gin::Knob limiterReleaseKnob;
    gin::Switch limiterBypassSwitch;
    GainReductionMeter limiterGRMeter;

    // Meter section
    LevelMeter inputMeterL, inputMeterR;
    LevelMeter outputMeterL, outputMeterR;
    LUFSMeter lufsMeter;
    juce::Label inputLabel, outputLabel, lufsLabel;

    // Dynamic knob arrays (owned)
    juce::OwnedArray<gin::Knob> ownedKnobs;

    ProcessingChainView::Module currentModule = ProcessingChainView::Module::EQ;

    void setupTopBar();
    void setupReferenceSection();
    void setupModeSection();
    void setupModulePanels();
    void setupMeterSection();
    void showModulePanel(ProcessingChainView::Module module);
    void updateMeters();

    // Layout helpers - tabbed view (legacy)
    void layoutCurrentModule(juce::Rectangle<int> area);
    void layoutEQ(juce::Rectangle<int> area);
    void layoutCompressor(juce::Rectangle<int> area);
    void layoutStereo(juce::Rectangle<int> area);
    void layoutLimiter(juce::Rectangle<int> area);

    // Layout helpers - all-in-one view
    void layoutEQAll(juce::Rectangle<int> area);
    void layoutCompressorAll(juce::Rectangle<int> area);
    void layoutStereoAll(juce::Rectangle<int> area);
    void layoutLimiterAll(juce::Rectangle<int> area);
    void layoutMeters(juce::Rectangle<int> area);
    void layoutMeterStrip(juce::Rectangle<int> area);
    void layoutMetersVertical(juce::Rectangle<int> area);

    // Section painting helpers
    void paintEQSections(juce::Graphics& g, juce::Rectangle<int> area);
    void paintCompressorSections(juce::Graphics& g, juce::Rectangle<int> area);
    void paintStereoSections(juce::Graphics& g, juce::Rectangle<int> area);
    void paintLimiterSections(juce::Graphics& g, juce::Rectangle<int> area);
    void paintAllModules(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomasterAudioProcessorEditor)
};
