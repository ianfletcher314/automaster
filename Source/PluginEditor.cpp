#include "PluginEditor.h"

AutomasterAudioProcessorEditor::AutomasterAudioProcessorEditor(AutomasterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);

    setupTopBar();
    setupSpectrumSection();
    setupReferenceSection();
    setupModeSection();
    setupModuleTabs();
    setupEQControls();
    setupCompressorControls();
    setupStereoControls();
    setupLimiterControls();
    setupMeterSection();

    // Initial state
    showModuleControls(ProcessingChainView::Module::EQ);

    setSize(1200, 700);
    startTimerHz(30);
}

AutomasterAudioProcessorEditor::~AutomasterAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void AutomasterAudioProcessorEditor::setupTopBar()
{
    // Title
    titleLabel.setText("AUTOMASTER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, AutomasterColors::primary);
    addAndMakeVisible(titleLabel);

    // A/B/C/D buttons
    const char* labels[] = { "A", "B", "C", "D" };
    for (int i = 0; i < 4; ++i)
    {
        abcdButtons[i].setButtonText(labels[i]);
        abcdButtons[i].onClick = [this, i]()
        {
            if (abcdButtons[i].getToggleState())
                audioProcessor.recallState(i);
            else
                audioProcessor.storeState(i);
            abcdButtons[i].setToggleState(true, juce::dontSendNotification);
            for (int j = 0; j < 4; ++j)
                if (j != i) abcdButtons[j].setToggleState(false, juce::dontSendNotification);
        };
        abcdButtons[i].setClickingTogglesState(true);
        addAndMakeVisible(abcdButtons[i]);
    }
    abcdButtons[0].setToggleState(true, juce::dontSendNotification);

    // Settings button
    settingsButton.setButtonText("Settings");
    addAndMakeVisible(settingsButton);
}

void AutomasterAudioProcessorEditor::setupSpectrumSection()
{
    addAndMakeVisible(spectrumAnalyzer);
}

void AutomasterAudioProcessorEditor::setupReferenceSection()
{
    addAndMakeVisible(referenceWaveform);

    // Profile combo
    profileCombo.addItem("Select Profile...", 1);
    profileCombo.addSeparator();
    profileCombo.addItem("Pop", 10);
    profileCombo.addItem("Rock", 11);
    profileCombo.addItem("Electronic", 12);
    profileCombo.addItem("Hip-Hop", 13);
    profileCombo.addItem("Jazz", 14);
    profileCombo.addItem("Classical", 15);
    profileCombo.addItem("Metal", 16);
    profileCombo.addItem("R&B", 17);
    profileCombo.addItem("Country", 18);
    profileCombo.setSelectedId(1);
    profileCombo.onChange = [this]()
    {
        int id = profileCombo.getSelectedId();
        if (id >= 10)
        {
            auto genre = static_cast<ReferenceProfile::Genre>(id - 10 + 1);
            audioProcessor.getRulesEngine().setGenre(genre);
            audioProcessor.getRulesEngine().setMode(RulesEngine::Mode::Genre);
        }
    };
    addAndMakeVisible(profileCombo);

    // Load button
    loadRefButton.setButtonText("Load");
    loadRefButton.onClick = [this]()
    {
        auto chooser = std::make_unique<juce::FileChooser>(
            "Select Reference Track",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.aiff;*.mp3;*.flac");

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                if (audioProcessor.loadReferenceFile(file))
                {
                    referenceWaveform.loadFile(file);
                    referenceWaveform.setProfile(&audioProcessor.getReferenceProfile());
                    referenceModeButton.setToggleState(true, juce::sendNotification);
                }
            }
        });
    };
    addAndMakeVisible(loadRefButton);

    // Analyze button
    analyzeButton.setButtonText("Analyze");
    analyzeButton.onClick = [this]() { audioProcessor.triggerAutoMaster(); };
    addAndMakeVisible(analyzeButton);

    // Match indicator
    addAndMakeVisible(matchIndicator);
}

void AutomasterAudioProcessorEditor::setupModeSection()
{
    // Mode toggles
    instantModeButton.setButtonText("Instant");
    instantModeButton.setRadioGroupId(1);
    instantModeButton.setToggleState(true, juce::dontSendNotification);
    instantModeButton.onClick = [this]()
    {
        if (instantModeButton.getToggleState())
            audioProcessor.getRulesEngine().setMode(RulesEngine::Mode::Instant);
    };
    addAndMakeVisible(instantModeButton);

    referenceModeButton.setButtonText("Reference");
    referenceModeButton.setRadioGroupId(1);
    referenceModeButton.onClick = [this]()
    {
        if (referenceModeButton.getToggleState())
        {
            if (audioProcessor.hasReference())
                audioProcessor.getRulesEngine().setMode(RulesEngine::Mode::Reference);
            else
                audioProcessor.getRulesEngine().setMode(RulesEngine::Mode::Genre);
        }
    };
    addAndMakeVisible(referenceModeButton);

    // Target LUFS
    targetLUFSSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    targetLUFSSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    targetLUFSSlider.setRange(-24.0, -6.0, 0.5);
    targetLUFSSlider.setValue(-14.0);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "targetLUFS", targetLUFSSlider));
    addAndMakeVisible(targetLUFSSlider);

    targetLUFSLabel.setText("Target", juce::dontSendNotification);
    targetLUFSLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(targetLUFSLabel);

    // Genre combo
    genreCombo.addItem("Auto", 1);
    genreCombo.addItem("Pop", 2);
    genreCombo.addItem("Rock", 3);
    genreCombo.addItem("Electronic", 4);
    genreCombo.addItem("Hip-Hop", 5);
    genreCombo.addItem("Jazz", 6);
    genreCombo.addItem("Classical", 7);
    genreCombo.setSelectedId(1);
    genreCombo.onChange = [this]()
    {
        auto genre = static_cast<ReferenceProfile::Genre>(genreCombo.getSelectedId() - 1);
        audioProcessor.getRulesEngine().setGenre(genre);
    };
    addAndMakeVisible(genreCombo);

    // Auto Master button
    autoMasterButton.setButtonText("AUTO MASTER");
    autoMasterButton.setColour(juce::TextButton::buttonColourId, AutomasterColors::primary);
    autoMasterButton.onClick = [this]() { audioProcessor.triggerAutoMaster(); };
    addAndMakeVisible(autoMasterButton);
}

void AutomasterAudioProcessorEditor::setupModuleTabs()
{
    chainView.setChain(&audioProcessor.getMasteringChain());
    chainView.onModuleSelected = [this](ProcessingChainView::Module mod)
    {
        showModuleControls(mod);
    };
    addAndMakeVisible(chainView);

    addAndMakeVisible(eqPanel);
    addAndMakeVisible(compPanel);
    addAndMakeVisible(stereoPanel);
    addAndMakeVisible(limiterPanel);
}

void AutomasterAudioProcessorEditor::setupEQControls()
{
    auto setupSlider = [this](juce::Slider& slider, const juce::String& paramId,
                               juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag)
    {
        slider.setSliderStyle(style);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider));
        eqPanel.addAndMakeVisible(slider);
    };

    // HPF/LPF
    setupSlider(hpfFreqSlider, "hpfFreq");
    setupSlider(lpfFreqSlider, "lpfFreq");

    hpfEnableButton.setButtonText("HPF");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "hpfEnabled", hpfEnableButton));
    eqPanel.addAndMakeVisible(hpfEnableButton);

    lpfEnableButton.setButtonText("LPF");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "lpfEnabled", lpfEnableButton));
    eqPanel.addAndMakeVisible(lpfEnableButton);

    // Shelves
    setupSlider(lowShelfFreqSlider, "lowShelfFreq");
    setupSlider(lowShelfGainSlider, "lowShelfGain");
    setupSlider(highShelfFreqSlider, "highShelfFreq");
    setupSlider(highShelfGainSlider, "highShelfGain");

    // Parametric bands
    for (int i = 0; i < 4; ++i)
    {
        juce::String prefix = "band" + juce::String(i + 1);
        setupSlider(bandFreqSliders[i], prefix + "Freq");
        setupSlider(bandGainSliders[i], prefix + "Gain");
        setupSlider(bandQSliders[i], prefix + "Q");
    }

    // Bypass
    eqBypassButton.setButtonText("Bypass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "eqBypass", eqBypassButton));
    eqPanel.addAndMakeVisible(eqBypassButton);
}

void AutomasterAudioProcessorEditor::setupCompressorControls()
{
    auto setupSlider = [this](juce::Slider& slider, const juce::String& paramId)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider));
        compPanel.addAndMakeVisible(slider);
    };

    // Crossovers
    setupSlider(lowMidXoverSlider, "lowMidXover");
    setupSlider(midHighXoverSlider, "midHighXover");

    // Per-band controls
    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "comp" + juce::String(i + 1);
        setupSlider(compThresholdSliders[i], prefix + "Threshold");
        setupSlider(compRatioSliders[i], prefix + "Ratio");
        setupSlider(compAttackSliders[i], prefix + "Attack");
        setupSlider(compReleaseSliders[i], prefix + "Release");
        setupSlider(compMakeupSliders[i], prefix + "Makeup");

        compPanel.addAndMakeVisible(compGRMeters[i]);
    }

    // Bypass
    compBypassButton.setButtonText("Bypass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "compBypass", compBypassButton));
    compPanel.addAndMakeVisible(compBypassButton);
}

void AutomasterAudioProcessorEditor::setupStereoControls()
{
    auto setupSlider = [this](juce::Slider& slider, const juce::String& paramId)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider));
        stereoPanel.addAndMakeVisible(slider);
    };

    setupSlider(globalWidthSlider, "globalWidth");
    setupSlider(lowWidthSlider, "lowWidth");
    setupSlider(midWidthSlider, "midWidth");
    setupSlider(highWidthSlider, "highWidth");
    setupSlider(monoBassFreqSlider, "monoBassFreq");

    monoBassEnableButton.setButtonText("Mono Bass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "monoBassEnabled", monoBassEnableButton));
    stereoPanel.addAndMakeVisible(monoBassEnableButton);

    stereoBypassButton.setButtonText("Bypass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "stereoBypass", stereoBypassButton));
    stereoPanel.addAndMakeVisible(stereoBypassButton);

    stereoPanel.addAndMakeVisible(correlationMeter);
}

void AutomasterAudioProcessorEditor::setupLimiterControls()
{
    auto setupSlider = [this](juce::Slider& slider, const juce::String& paramId)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider));
        limiterPanel.addAndMakeVisible(slider);
    };

    setupSlider(ceilingSlider, "ceiling");
    setupSlider(limiterReleaseSlider, "limiterRelease");

    limiterBypassButton.setButtonText("Bypass");
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "limiterBypass", limiterBypassButton));
    limiterPanel.addAndMakeVisible(limiterBypassButton);

    limiterPanel.addAndMakeVisible(limiterGRMeter);
}

void AutomasterAudioProcessorEditor::setupMeterSection()
{
    inputMeterL.setRange(-60.0f, 0.0f);
    inputMeterR.setRange(-60.0f, 0.0f);
    outputMeterL.setRange(-60.0f, 0.0f);
    outputMeterR.setRange(-60.0f, 0.0f);

    addAndMakeVisible(inputMeterL);
    addAndMakeVisible(inputMeterR);
    addAndMakeVisible(outputMeterL);
    addAndMakeVisible(outputMeterR);
    addAndMakeVisible(lufsMeter);

    inputLabel.setText("IN", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputLabel);

    outputLabel.setText("OUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputLabel);

    lufsLabel.setText("LUFS", juce::dontSendNotification);
    lufsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lufsLabel);
}

void AutomasterAudioProcessorEditor::showModuleControls(ProcessingChainView::Module module)
{
    currentModule = module;

    eqPanel.setVisible(module == ProcessingChainView::Module::EQ);
    compPanel.setVisible(module == ProcessingChainView::Module::Compressor);
    stereoPanel.setVisible(module == ProcessingChainView::Module::Stereo);
    limiterPanel.setVisible(module == ProcessingChainView::Module::Limiter);
}

void AutomasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(AutomasterColors::background);

    // Draw section dividers
    g.setColour(AutomasterColors::panelBgLight);
}

void AutomasterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Top bar
    auto topBar = bounds.removeFromTop(40);
    titleLabel.setBounds(topBar.removeFromLeft(200));

    auto abcdArea = topBar.removeFromRight(180);
    int btnWidth = 40;
    for (int i = 0; i < 4; ++i)
        abcdButtons[i].setBounds(abcdArea.removeFromLeft(btnWidth).reduced(2));

    settingsButton.setBounds(topBar.removeFromRight(80).reduced(2));

    bounds.removeFromTop(10);

    // Spectrum analyzer
    auto spectrumArea = bounds.removeFromTop(150);
    spectrumAnalyzer.setBounds(spectrumArea);

    bounds.removeFromTop(10);

    // Reference and Mode sections side by side
    auto refModeArea = bounds.removeFromTop(100);
    auto refArea = refModeArea.removeFromLeft(refModeArea.getWidth() / 2 - 5);
    auto modeArea = refModeArea;

    // Reference section
    auto refTop = refArea.removeFromTop(25);
    profileCombo.setBounds(refTop.removeFromLeft(150));
    loadRefButton.setBounds(refTop.removeFromLeft(60).reduced(2));
    analyzeButton.setBounds(refTop.removeFromLeft(60).reduced(2));

    referenceWaveform.setBounds(refArea.removeFromTop(50));
    matchIndicator.setBounds(refArea.reduced(5, 2));

    // Mode section
    auto modeTop = modeArea.removeFromTop(25);
    instantModeButton.setBounds(modeTop.removeFromLeft(80));
    referenceModeButton.setBounds(modeTop.removeFromLeft(100));

    auto modeMiddle = modeArea.removeFromTop(60);
    auto targetArea = modeMiddle.removeFromLeft(80);
    targetLUFSSlider.setBounds(targetArea.removeFromTop(50));
    targetLUFSLabel.setBounds(targetArea);

    genreCombo.setBounds(modeMiddle.removeFromLeft(120).reduced(5, 15));
    autoMasterButton.setBounds(modeMiddle.removeFromLeft(150).reduced(10, 15));

    bounds.removeFromTop(10);

    // Chain view
    chainView.setBounds(bounds.removeFromTop(80));

    bounds.removeFromTop(10);

    // Meter section on right
    auto meterArea = bounds.removeFromRight(100);
    inputLabel.setBounds(meterArea.removeFromTop(20));
    auto inputMeters = meterArea.removeFromTop(150);
    inputMeterL.setBounds(inputMeters.removeFromLeft(20));
    inputMeterR.setBounds(inputMeters.removeFromLeft(20));

    meterArea.removeFromTop(10);
    outputLabel.setBounds(meterArea.removeFromTop(20));
    auto outputMeters = meterArea.removeFromTop(150);
    outputMeterL.setBounds(outputMeters.removeFromLeft(20));
    outputMeterR.setBounds(outputMeters.removeFromLeft(20));

    meterArea.removeFromTop(10);
    lufsLabel.setBounds(meterArea.removeFromTop(20));
    lufsMeter.setBounds(meterArea.removeFromTop(60));

    // Module panels
    auto moduleArea = bounds.reduced(5);
    eqPanel.setBounds(moduleArea);
    compPanel.setBounds(moduleArea);
    stereoPanel.setBounds(moduleArea);
    limiterPanel.setBounds(moduleArea);

    // Layout EQ panel
    auto eqBounds = eqPanel.getLocalBounds().reduced(5);
    int knobSize = 60;
    int knobSpacing = 10;

    auto eqRow1 = eqBounds.removeFromTop(knobSize + 25);
    hpfEnableButton.setBounds(eqRow1.removeFromLeft(50).reduced(2, knobSize / 2));
    hpfFreqSlider.setBounds(eqRow1.removeFromLeft(knobSize + knobSpacing));
    lowShelfFreqSlider.setBounds(eqRow1.removeFromLeft(knobSize + knobSpacing));
    lowShelfGainSlider.setBounds(eqRow1.removeFromLeft(knobSize + knobSpacing));

    eqRow1.removeFromLeft(20);
    for (int i = 0; i < 4; ++i)
    {
        bandFreqSliders[i].setBounds(eqRow1.removeFromLeft(knobSize + knobSpacing));
        bandGainSliders[i].setBounds(eqRow1.removeFromLeft(knobSize + knobSpacing));
    }

    auto eqRow2 = eqBounds.removeFromTop(knobSize + 25);
    lpfEnableButton.setBounds(eqRow2.removeFromLeft(50).reduced(2, knobSize / 2));
    lpfFreqSlider.setBounds(eqRow2.removeFromLeft(knobSize + knobSpacing));
    highShelfFreqSlider.setBounds(eqRow2.removeFromLeft(knobSize + knobSpacing));
    highShelfGainSlider.setBounds(eqRow2.removeFromLeft(knobSize + knobSpacing));

    eqRow2.removeFromLeft(20);
    for (int i = 0; i < 4; ++i)
    {
        bandQSliders[i].setBounds(eqRow2.removeFromLeft(knobSize + knobSpacing));
        eqRow2.removeFromLeft(knobSize + knobSpacing);  // Skip space for gain above
    }

    eqBypassButton.setBounds(eqBounds.removeFromBottom(25).removeFromRight(80));

    // Layout Compressor panel
    auto compBounds = compPanel.getLocalBounds().reduced(5);
    auto compRow1 = compBounds.removeFromTop(knobSize + 25);
    lowMidXoverSlider.setBounds(compRow1.removeFromLeft(knobSize + knobSpacing));
    midHighXoverSlider.setBounds(compRow1.removeFromLeft(knobSize + knobSpacing));

    compRow1.removeFromLeft(20);
    for (int i = 0; i < 3; ++i)
    {
        compThresholdSliders[i].setBounds(compRow1.removeFromLeft(knobSize + knobSpacing));
        compRatioSliders[i].setBounds(compRow1.removeFromLeft(knobSize + knobSpacing));
        compAttackSliders[i].setBounds(compRow1.removeFromLeft(knobSize + knobSpacing));
        compRow1.removeFromLeft(10);
    }

    auto compRow2 = compBounds.removeFromTop(knobSize + 25);
    compRow2.removeFromLeft(2 * (knobSize + knobSpacing) + 20);
    for (int i = 0; i < 3; ++i)
    {
        compReleaseSliders[i].setBounds(compRow2.removeFromLeft(knobSize + knobSpacing));
        compMakeupSliders[i].setBounds(compRow2.removeFromLeft(knobSize + knobSpacing));
        compGRMeters[i].setBounds(compRow2.removeFromLeft(knobSize + knobSpacing).reduced(10, 20));
        compRow2.removeFromLeft(10);
    }

    compBypassButton.setBounds(compBounds.removeFromBottom(25).removeFromRight(80));

    // Layout Stereo panel
    auto stereoBounds = stereoPanel.getLocalBounds().reduced(5);
    auto stereoRow1 = stereoBounds.removeFromTop(knobSize + 25);
    globalWidthSlider.setBounds(stereoRow1.removeFromLeft(knobSize + knobSpacing));
    stereoRow1.removeFromLeft(20);
    lowWidthSlider.setBounds(stereoRow1.removeFromLeft(knobSize + knobSpacing));
    midWidthSlider.setBounds(stereoRow1.removeFromLeft(knobSize + knobSpacing));
    highWidthSlider.setBounds(stereoRow1.removeFromLeft(knobSize + knobSpacing));

    auto stereoRow2 = stereoBounds.removeFromTop(knobSize + 25);
    monoBassFreqSlider.setBounds(stereoRow2.removeFromLeft(knobSize + knobSpacing));
    monoBassEnableButton.setBounds(stereoRow2.removeFromLeft(100).reduced(5, knobSize / 2));
    correlationMeter.setBounds(stereoRow2.removeFromLeft(200).reduced(5, knobSize / 2));

    stereoBypassButton.setBounds(stereoBounds.removeFromBottom(25).removeFromRight(80));

    // Layout Limiter panel
    auto limiterBounds = limiterPanel.getLocalBounds().reduced(5);
    auto limiterRow1 = limiterBounds.removeFromTop(knobSize + 25);
    ceilingSlider.setBounds(limiterRow1.removeFromLeft(knobSize + knobSpacing));
    limiterReleaseSlider.setBounds(limiterRow1.removeFromLeft(knobSize + knobSpacing));
    limiterRow1.removeFromLeft(20);
    limiterGRMeter.setBounds(limiterRow1.removeFromLeft(200).reduced(5, 10));

    limiterBypassButton.setBounds(limiterBounds.removeFromBottom(25).removeFromRight(80));
}

void AutomasterAudioProcessorEditor::timerCallback()
{
    updateMeters();
}

void AutomasterAudioProcessorEditor::updateMeters()
{
    auto& chain = audioProcessor.getMasteringChain();
    auto& analysis = audioProcessor.getAnalysisEngine();

    // Input/Output meters
    auto& inputMeter = chain.getInputMeter();
    auto& outputMeter = chain.getOutputMeter();

    inputMeterL.setLevel(inputMeter.getPeakLevelL());
    inputMeterR.setLevel(inputMeter.getPeakLevelR());
    outputMeterL.setLevel(outputMeter.getPeakLevelL());
    outputMeterR.setLevel(outputMeter.getPeakLevelR());

    // LUFS
    lufsMeter.setLevels(
        outputMeter.getMomentaryLUFS(),
        outputMeter.getShortTermLUFS(),
        outputMeter.getIntegratedLUFS()
    );
    lufsMeter.setTarget(audioProcessor.getAPVTS().getRawParameterValue("targetLUFS")->load());

    // Compressor GR meters
    auto& comp = chain.getCompressor();
    for (int i = 0; i < 3; ++i)
        compGRMeters[i].setGainReduction(comp.getGainReduction(i));

    // Limiter GR
    limiterGRMeter.setGainReduction(chain.getLimiter().getGainReduction());

    // Stereo correlation
    correlationMeter.setCorrelation(analysis.getCorrelation());

    // Reference match
    if (audioProcessor.hasReference())
        matchIndicator.setMatch(analysis.getReferenceMatchScore());
}

bool AutomasterAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (file.endsWith(".wav") || file.endsWith(".aiff") ||
            file.endsWith(".mp3") || file.endsWith(".flac"))
            return true;
    }
    return false;
}

void AutomasterAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (const auto& filePath : files)
    {
        juce::File file(filePath);
        if (file.existsAsFile())
        {
            if (audioProcessor.loadReferenceFile(file))
            {
                referenceWaveform.loadFile(file);
                referenceWaveform.setProfile(&audioProcessor.getReferenceProfile());
                referenceModeButton.setToggleState(true, juce::sendNotification);
                break;
            }
        }
    }
}
