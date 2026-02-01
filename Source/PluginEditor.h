#pragma once

#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/MeterComponents.h"
#include "UI/SpectrumAnalyzerUI.h"
#include "UI/ReferenceWaveform.h"
#include "UI/ProcessingChainView.h"

class AutomasterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer,
                                        public juce::FileDragAndDropTarget
{
public:
    AutomasterAudioProcessorEditor(AutomasterAudioProcessor&);
    ~AutomasterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Drag and drop for reference files
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void setupTopBar();
    void setupSpectrumSection();
    void setupReferenceSection();
    void setupModeSection();
    void setupModuleTabs();
    void setupMeterSection();
    void setupEQControls();
    void setupCompressorControls();
    void setupStereoControls();
    void setupLimiterControls();

    void updateMeters();
    void showModuleControls(ProcessingChainView::Module module);

    AutomasterAudioProcessor& audioProcessor;
    AutomasterLookAndFeel lookAndFeel;

    // Top bar
    juce::Label titleLabel;
    std::array<juce::TextButton, 4> abcdButtons;
    juce::TextButton settingsButton;

    // Spectrum analyzer
    SpectrumAnalyzerUI spectrumAnalyzer;

    // Reference section
    ReferenceWaveform referenceWaveform;
    juce::TextButton loadRefButton;
    juce::TextButton analyzeButton;
    MatchIndicator matchIndicator;
    juce::ComboBox profileCombo;

    // Mode section
    juce::ToggleButton instantModeButton;
    juce::ToggleButton referenceModeButton;
    juce::Slider targetLUFSSlider;
    juce::Label targetLUFSLabel;
    juce::ComboBox genreCombo;
    juce::TextButton autoMasterButton;

    // Processing chain view
    ProcessingChainView chainView;

    // Module control panels (switched based on tab)
    juce::Component eqPanel;
    juce::Component compPanel;
    juce::Component stereoPanel;
    juce::Component limiterPanel;

    // EQ controls
    juce::Slider hpfFreqSlider, lpfFreqSlider;
    juce::ToggleButton hpfEnableButton, lpfEnableButton;
    juce::Slider lowShelfFreqSlider, lowShelfGainSlider;
    juce::Slider highShelfFreqSlider, highShelfGainSlider;
    std::array<juce::Slider, 4> bandFreqSliders;
    std::array<juce::Slider, 4> bandGainSliders;
    std::array<juce::Slider, 4> bandQSliders;
    juce::ToggleButton eqBypassButton;

    // Compressor controls
    juce::Slider lowMidXoverSlider, midHighXoverSlider;
    std::array<juce::Slider, 3> compThresholdSliders;
    std::array<juce::Slider, 3> compRatioSliders;
    std::array<juce::Slider, 3> compAttackSliders;
    std::array<juce::Slider, 3> compReleaseSliders;
    std::array<juce::Slider, 3> compMakeupSliders;
    std::array<GainReductionMeter, 3> compGRMeters;
    juce::ToggleButton compBypassButton;

    // Stereo controls
    juce::Slider globalWidthSlider;
    juce::Slider lowWidthSlider, midWidthSlider, highWidthSlider;
    juce::Slider monoBassFreqSlider;
    juce::ToggleButton monoBassEnableButton;
    juce::ToggleButton stereoBypassButton;
    CorrelationMeter correlationMeter;

    // Limiter controls
    juce::Slider ceilingSlider;
    juce::Slider limiterReleaseSlider;
    juce::ToggleButton limiterBypassButton;
    GainReductionMeter limiterGRMeter;

    // Meter section
    LevelMeter inputMeterL, inputMeterR;
    LevelMeter outputMeterL, outputMeterR;
    LUFSMeter lufsMeter;
    juce::Label inputLabel, outputLabel, lufsLabel, grLabel;

    // Parameter attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    ProcessingChainView::Module currentModule = ProcessingChainView::Module::EQ;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomasterAudioProcessorEditor)
};
