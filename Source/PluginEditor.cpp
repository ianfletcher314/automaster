#include "PluginEditor.h"

// =============================================================================
// DESIGN SYSTEM - All modules visible at once
// =============================================================================
namespace {
    // Spacing
    constexpr int kPadding = 6;
    constexpr int kGap = 3;              // Horizontal gap between knobs
    constexpr int kSectionGap = 8;       // Gap between sections
    constexpr int kRowGap = 2;           // Vertical gap between rows
    constexpr int kModuleGap = 10;       // Gap between module rows

    // Knobs - compact but readable
    constexpr int kKnobSize = 44;        // Knob diameter
    constexpr int kKnobLabelH = 12;      // Label height below knob
    constexpr int kKnobHeight = kKnobSize + kKnobLabelH;

    // Switches
    constexpr int kSwitchWidth = 36;
    constexpr int kSwitchHeight = 36;  // Need room for toggle (18px) + label below

    // Meters
    constexpr int kMeterWidth = 10;
    constexpr int kMeterSectionWidth = 36;

    // Headers
    constexpr int kHeaderHeight = 50;
    constexpr int kModuleLabelH = 14;
    constexpr int kSectionLabelH = 12;
    constexpr int kTabHeight = 24;

    // Legacy section widths (for old layout functions, can remove later)
    constexpr int kFiltersSectionW = kSwitchWidth + kGap + kKnobSize + 8;
    constexpr int kShelvesSectionW = kKnobSize * 2 + kGap + 8;
    constexpr int kBandsSectionW = kKnobSize * 4 + kGap * 3 + kSwitchWidth + 12;
    constexpr int kXoverSectionW = kKnobSize + 8;
    constexpr int kCompBandSectionW = kKnobSize * 2 + kGap + 8;
    constexpr int kCompHighSectionW = kKnobSize * 2 + kGap + kSwitchWidth + 12;
    constexpr int kWidthSectionW = kKnobSize + 8;
    constexpr int kBandWidthSectionW = kKnobSize * 3 + kGap * 2 + 8;
    constexpr int kMonoBassSectionW = kKnobSize + 8;
    constexpr int kStereoMeterSectionW = kKnobSize * 2 + kSwitchWidth + 12;
    constexpr int kLimiterCtrlSectionW = kKnobSize * 2 + kGap + 8;
    constexpr int kLimiterMeterSectionW = kKnobSize * 3 + kSwitchWidth + 12;

    // Meter strip
    constexpr int kMeterStripHeight = 50;

    // Spectrum analyzer - BIG
    constexpr int kSpectrumHeight = 280;

    // Module rows - EQ/Comp need 2 rows of knobs, Stereo/Limiter only need 1
    constexpr int kModuleRow1H = 134;  // label(16) + padding(4) + row1(56) + gap(2) + row2(56) = 134
    constexpr int kModuleRow2H = 80;   // label(16) + padding(4) + row1(56) + extra(4) = 80

    // Window - sized to fit content
    constexpr int kWindowWidth = 950;
    constexpr int kWindowHeight = 610;
}

AutomasterAudioProcessorEditor::AutomasterAudioProcessorEditor(AutomasterAudioProcessor& p)
    : gin::ProcessorEditor(p),
      proc(p),
      targetLUFSKnob(p.targetLUFS),
      hpfFreqKnob(p.hpfFreq),
      lpfFreqKnob(p.lpfFreq),
      hpfEnableSwitch(p.hpfEnabled),
      lpfEnableSwitch(p.lpfEnabled),
      lowShelfFreqKnob(p.lowShelfFreq),
      lowShelfGainKnob(p.lowShelfGain),
      highShelfFreqKnob(p.highShelfFreq),
      highShelfGainKnob(p.highShelfGain),
      eqBypassSwitch(p.eqBypass),
      lowMidXoverKnob(p.lowMidXover),
      midHighXoverKnob(p.midHighXover),
      compBypassSwitch(p.compBypass),
      globalWidthKnob(p.globalWidth),
      lowWidthKnob(p.lowWidth),
      midWidthKnob(p.midWidth),
      highWidthKnob(p.highWidth),
      monoBassFreqKnob(p.monoBassFreq),
      monoBassEnableSwitch(p.monoBassEnabled),
      stereoBypassSwitch(p.stereoBypass),
      ceilingKnob(p.ceiling),
      limiterReleaseKnob(p.limiterRelease),
      limiterBypassSwitch(p.limiterBypass)
{
    // Create colored LookAndFeels for EQ knobs and apply them
    auto lowShelfLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::eqLowShelf));
    lowShelfFreqKnob.getSlider().setLookAndFeel(lowShelfLAF);
    lowShelfGainKnob.getSlider().setLookAndFeel(lowShelfLAF);

    auto highShelfLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::eqHighShelf));
    highShelfFreqKnob.getSlider().setLookAndFeel(highShelfLAF);
    highShelfGainKnob.getSlider().setLookAndFeel(highShelfLAF);

    auto hpfLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::eqHPF));
    hpfFreqKnob.getSlider().setLookAndFeel(hpfLAF);

    auto lpfLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::eqLPF));
    lpfFreqKnob.getSlider().setLookAndFeel(lpfLAF);

    for (int i = 0; i < 4; ++i)
    {
        bandFreqKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandFreq[i]));
        bandGainKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandGain[i]));
        bandQKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandQ[i]));

        // Create colored LookAndFeel for this band
        auto bandLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::getEQBandColor(i)));
        bandFreqKnobs[i]->getSlider().setLookAndFeel(bandLAF);
        bandGainKnobs[i]->getSlider().setLookAndFeel(bandLAF);
        bandQKnobs[i]->getSlider().setLookAndFeel(bandLAF);
    }

    // Compressor knobs (orange)
    auto compLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::compColor));
    lowMidXoverKnob.getSlider().setLookAndFeel(compLAF);
    midHighXoverKnob.getSlider().setLookAndFeel(compLAF);

    for (int i = 0; i < 3; ++i)
    {
        compThresholdKnobs[i] = ownedKnobs.add(new gin::Knob(p.compThreshold[i]));
        compRatioKnobs[i] = ownedKnobs.add(new gin::Knob(p.compRatio[i]));
        compAttackKnobs[i] = ownedKnobs.add(new gin::Knob(p.compAttack[i]));
        compReleaseKnobs[i] = ownedKnobs.add(new gin::Knob(p.compRelease[i]));
        compMakeupKnobs[i] = ownedKnobs.add(new gin::Knob(p.compMakeup[i]));

        compThresholdKnobs[i]->getSlider().setLookAndFeel(compLAF);
        compRatioKnobs[i]->getSlider().setLookAndFeel(compLAF);
        compAttackKnobs[i]->getSlider().setLookAndFeel(compLAF);
        compReleaseKnobs[i]->getSlider().setLookAndFeel(compLAF);
        compMakeupKnobs[i]->getSlider().setLookAndFeel(compLAF);
    }

    // Stereo knobs (purple)
    auto stereoLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::stereoColor));
    globalWidthKnob.getSlider().setLookAndFeel(stereoLAF);
    lowWidthKnob.getSlider().setLookAndFeel(stereoLAF);
    midWidthKnob.getSlider().setLookAndFeel(stereoLAF);
    highWidthKnob.getSlider().setLookAndFeel(stereoLAF);
    monoBassFreqKnob.getSlider().setLookAndFeel(stereoLAF);

    // Limiter knobs (red)
    auto limiterLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::limiterColor));
    ceilingKnob.getSlider().setLookAndFeel(limiterLAF);
    limiterReleaseKnob.getSlider().setLookAndFeel(limiterLAF);

    // Target LUFS knob (primary cyan)
    auto primaryLAF = coloredKnobLAFs.add(new ColoredKnobLookAndFeel(AutomasterColors::primary));
    targetLUFSKnob.getSlider().setLookAndFeel(primaryLAF);

    // Toggle switches - styled as pill toggles with section colors
    // Helper lambda to set LAF on switch and all its children, and hide the label
    auto setSwitchLAF = [](gin::Switch& sw, juce::LookAndFeel* laf) {
        sw.setLookAndFeel(laf);
        for (int i = 0; i < sw.getNumChildComponents(); ++i)
        {
            auto* child = sw.getChildComponent(i);
            child->setLookAndFeel(laf);
            // Hide labels (first child is the name label in gin::Switch)
            if (auto* label = dynamic_cast<juce::Label*>(child))
                label->setVisible(false);
        }
    };

    auto eqSwitchLAF = switchLAFs.add(new ToggleSwitchLookAndFeel(AutomasterColors::eqHPF));  // gray for filters
    setSwitchLAF(hpfEnableSwitch, eqSwitchLAF);
    setSwitchLAF(lpfEnableSwitch, eqSwitchLAF);

    auto eqBypassLAF = switchLAFs.add(new ToggleSwitchLookAndFeel(AutomasterColors::eqColor));
    setSwitchLAF(eqBypassSwitch, eqBypassLAF);

    auto compBypassLAF = switchLAFs.add(new ToggleSwitchLookAndFeel(AutomasterColors::compColor));
    setSwitchLAF(compBypassSwitch, compBypassLAF);

    auto stereoSwitchLAF = switchLAFs.add(new ToggleSwitchLookAndFeel(AutomasterColors::stereoColor));
    setSwitchLAF(monoBassEnableSwitch, stereoSwitchLAF);
    setSwitchLAF(stereoBypassSwitch, stereoSwitchLAF);

    auto limiterBypassLAF = switchLAFs.add(new ToggleSwitchLookAndFeel(AutomasterColors::limiterColor));
    setSwitchLAF(limiterBypassSwitch, limiterBypassLAF);

    setupTopBar();
    setupReferenceSection();
    setupModeSection();
    setupModulePanels();
    setupMeterSection();

    // Spectrum analyzer
    spectrumAnalyzer.setEQ(&proc.getMasteringChain().getEQ());
    spectrumAnalyzer.setSampleRate(proc.getSampleRate());
    addAndMakeVisible(spectrumAnalyzer);

    showModulePanel(ProcessingChainView::Module::EQ);
    addKeyListener(this);  // Listen for keys from all child components
    setSize(kWindowWidth, kWindowHeight);

    // Start meter update timer
    startTimerHz(30);
}

AutomasterAudioProcessorEditor::~AutomasterAudioProcessorEditor()
{
    // Clear custom LookAndFeels before components are destroyed
    autoMasterButton.setLookAndFeel(nullptr);

    // EQ section
    lowShelfFreqKnob.getSlider().setLookAndFeel(nullptr);
    lowShelfGainKnob.getSlider().setLookAndFeel(nullptr);
    highShelfFreqKnob.getSlider().setLookAndFeel(nullptr);
    highShelfGainKnob.getSlider().setLookAndFeel(nullptr);
    hpfFreqKnob.getSlider().setLookAndFeel(nullptr);
    lpfFreqKnob.getSlider().setLookAndFeel(nullptr);
    for (int i = 0; i < 4; ++i)
    {
        if (bandFreqKnobs[i]) bandFreqKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (bandGainKnobs[i]) bandGainKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (bandQKnobs[i]) bandQKnobs[i]->getSlider().setLookAndFeel(nullptr);
    }

    // Compressor section
    lowMidXoverKnob.getSlider().setLookAndFeel(nullptr);
    midHighXoverKnob.getSlider().setLookAndFeel(nullptr);
    for (int i = 0; i < 3; ++i)
    {
        if (compThresholdKnobs[i]) compThresholdKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (compRatioKnobs[i]) compRatioKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (compAttackKnobs[i]) compAttackKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (compReleaseKnobs[i]) compReleaseKnobs[i]->getSlider().setLookAndFeel(nullptr);
        if (compMakeupKnobs[i]) compMakeupKnobs[i]->getSlider().setLookAndFeel(nullptr);
    }

    // Stereo section
    globalWidthKnob.getSlider().setLookAndFeel(nullptr);
    lowWidthKnob.getSlider().setLookAndFeel(nullptr);
    midWidthKnob.getSlider().setLookAndFeel(nullptr);
    highWidthKnob.getSlider().setLookAndFeel(nullptr);
    monoBassFreqKnob.getSlider().setLookAndFeel(nullptr);

    // Limiter section
    ceilingKnob.getSlider().setLookAndFeel(nullptr);
    limiterReleaseKnob.getSlider().setLookAndFeel(nullptr);

    // Target
    targetLUFSKnob.getSlider().setLookAndFeel(nullptr);

    // Switches - clear LAF on switch and all children
    auto clearSwitchLAF = [](gin::Switch& sw) {
        sw.setLookAndFeel(nullptr);
        for (int i = 0; i < sw.getNumChildComponents(); ++i)
            sw.getChildComponent(i)->setLookAndFeel(nullptr);
    };
    clearSwitchLAF(hpfEnableSwitch);
    clearSwitchLAF(lpfEnableSwitch);
    clearSwitchLAF(eqBypassSwitch);
    clearSwitchLAF(compBypassSwitch);
    clearSwitchLAF(monoBassEnableSwitch);
    clearSwitchLAF(stereoBypassSwitch);
    clearSwitchLAF(limiterBypassSwitch);
}

bool AutomasterAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // 1/2/3/4 to switch tabs
    if (key.getTextCharacter() == '1') {
        chainView.selectModule(ProcessingChainView::Module::EQ);
        showModulePanel(ProcessingChainView::Module::EQ);
        resized();
        return true;
    }
    if (key.getTextCharacter() == '2') {
        chainView.selectModule(ProcessingChainView::Module::Compressor);
        showModulePanel(ProcessingChainView::Module::Compressor);
        resized();
        return true;
    }
    if (key.getTextCharacter() == '3') {
        chainView.selectModule(ProcessingChainView::Module::Stereo);
        showModulePanel(ProcessingChainView::Module::Stereo);
        resized();
        return true;
    }
    if (key.getTextCharacter() == '4') {
        chainView.selectModule(ProcessingChainView::Module::Limiter);
        showModulePanel(ProcessingChainView::Module::Limiter);
        resized();
        return true;
    }
    return false;
}

void AutomasterAudioProcessorEditor::setupTopBar()
{
    titleLabel.setText("AUTOMASTER", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(28.0f).withStyle("Bold"));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    const char* labels[] = { "A", "B", "C", "D" };
    for (int i = 0; i < 4; ++i)
    {
        abcdButtons[i].setButtonText(labels[i]);
        abcdButtons[i].onClick = [this, i]()
        {
            if (abcdButtons[i].getToggleState())
                proc.recallState(i);
            else
                proc.storeState(i);
            abcdButtons[i].setToggleState(true, juce::dontSendNotification);
            for (int j = 0; j < 4; ++j)
                if (j != i) abcdButtons[j].setToggleState(false, juce::dontSendNotification);
        };
        abcdButtons[i].setClickingTogglesState(true);
        addAndMakeVisible(abcdButtons[i]);
    }
    abcdButtons[0].setToggleState(true, juce::dontSendNotification);

    settingsButton.setButtonText("Settings");
    addAndMakeVisible(settingsButton);
}

void AutomasterAudioProcessorEditor::setupReferenceSection()
{
    profileCombo.addItem("Genre: Auto", 1);
    profileCombo.addItem("Pop", 10);
    profileCombo.addItem("Rock", 11);
    profileCombo.addItem("Electronic", 12);
    profileCombo.addItem("Hip-Hop", 13);
    profileCombo.addItem("Jazz", 14);
    profileCombo.addItem("Classical", 15);
    profileCombo.setSelectedId(1);
    profileCombo.onChange = [this]()
    {
        int id = profileCombo.getSelectedId();
        if (id >= 10)
        {
            auto genre = static_cast<ReferenceProfile::Genre>(id - 10 + 1);
            proc.getRulesEngine().setGenre(genre);
            proc.getRulesEngine().setMode(RulesEngine::Mode::Genre);
        }
    };
    addAndMakeVisible(profileCombo);

    loadRefButton.setButtonText("Load Ref");
    loadRefButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Reference Track",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.aiff;*.mp3;*.flac");

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                if (proc.loadReferenceFile(file))
                {
                    referenceWaveform.loadFile(file);
                    referenceWaveform.setProfile(&proc.getReferenceProfile());
                }
            }
        });
    };
    addAndMakeVisible(loadRefButton);

    // Match indicator shows how close we are to reference
    addAndMakeVisible(matchIndicator);
}

void AutomasterAudioProcessorEditor::setupModeSection()
{
    addAndMakeVisible(targetLUFSKnob);

    autoMasterButton.setButtonText("AUTO MASTER");
    rainbowButtonLAF = std::make_unique<RainbowButtonLookAndFeel>();
    autoMasterButton.setLookAndFeel(rainbowButtonLAF.get());
    autoMasterButton.onClick = [this]() { proc.triggerAutoMaster(); };
    addAndMakeVisible(autoMasterButton);

    // Analyze button (Ozone-style workflow) - subtle, secondary
    analyzeButton.setButtonText("ANALYZE");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2a2a2a));
    analyzeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFF6B35));  // Orange when active
    analyzeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF888888));  // Muted gray
    analyzeButton.setClickingTogglesState(true);
    analyzeButton.onClick = [this]()
    {
        if (analyzeButton.getToggleState())
        {
            proc.startAnalysis();
            analyzeButton.setButtonText("STOP");
        }
        else
        {
            proc.stopAnalysis();
            analyzeButton.setButtonText("ANALYZE");
        }
    };
    addAndMakeVisible(analyzeButton);

    // Progress bar for analysis
    analysisProgressBar = std::make_unique<juce::ProgressBar>(analysisProgress);
    analysisProgressBar->setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xFF1a1a1a));
    analysisProgressBar->setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xFFFF6B35));
    addAndMakeVisible(*analysisProgressBar);
}

void AutomasterAudioProcessorEditor::setupModulePanels()
{
    chainView.setChain(&proc.getMasteringChain());
    chainView.onModuleSelected = [this](ProcessingChainView::Module mod)
    {
        showModulePanel(mod);
    };
    addAndMakeVisible(chainView);

    // EQ
    addAndMakeVisible(hpfEnableSwitch);
    addAndMakeVisible(hpfFreqKnob);
    addAndMakeVisible(lpfEnableSwitch);
    addAndMakeVisible(lpfFreqKnob);
    addAndMakeVisible(lowShelfFreqKnob);
    addAndMakeVisible(lowShelfGainKnob);
    addAndMakeVisible(highShelfFreqKnob);
    addAndMakeVisible(highShelfGainKnob);
    addAndMakeVisible(eqBypassSwitch);
    for (int i = 0; i < 4; ++i)
    {
        addAndMakeVisible(bandFreqKnobs[i]);
        addAndMakeVisible(bandGainKnobs[i]);
        addAndMakeVisible(bandQKnobs[i]);
    }

    // Compressor
    addAndMakeVisible(lowMidXoverKnob);
    addAndMakeVisible(midHighXoverKnob);
    addAndMakeVisible(compBypassSwitch);
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible(compThresholdKnobs[i]);
        addAndMakeVisible(compRatioKnobs[i]);
        addAndMakeVisible(compAttackKnobs[i]);
        addAndMakeVisible(compReleaseKnobs[i]);
        addAndMakeVisible(compMakeupKnobs[i]);
        addAndMakeVisible(compGRMeters[i]);
    }

    // Stereo
    addAndMakeVisible(globalWidthKnob);
    addAndMakeVisible(lowWidthKnob);
    addAndMakeVisible(midWidthKnob);
    addAndMakeVisible(highWidthKnob);
    addAndMakeVisible(monoBassFreqKnob);
    addAndMakeVisible(monoBassEnableSwitch);
    addAndMakeVisible(stereoBypassSwitch);
    addAndMakeVisible(correlationMeter);

    // Limiter
    addAndMakeVisible(ceilingKnob);
    addAndMakeVisible(limiterReleaseKnob);
    addAndMakeVisible(limiterBypassSwitch);
    addAndMakeVisible(limiterGRMeter);
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
    inputLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    addAndMakeVisible(inputLabel);

    outputLabel.setText("OUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    addAndMakeVisible(outputLabel);

    lufsLabel.setText("LUFS", juce::dontSendNotification);
    lufsLabel.setJustificationType(juce::Justification::centred);
    lufsLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    addAndMakeVisible(lufsLabel);
}

void AutomasterAudioProcessorEditor::showModulePanel(ProcessingChainView::Module module)
{
    currentModule = module;

    auto setEQVisible = [this](bool v) {
        hpfEnableSwitch.setVisible(v);
        hpfFreqKnob.setVisible(v);
        lpfEnableSwitch.setVisible(v);
        lpfFreqKnob.setVisible(v);
        lowShelfFreqKnob.setVisible(v);
        lowShelfGainKnob.setVisible(v);
        highShelfFreqKnob.setVisible(v);
        highShelfGainKnob.setVisible(v);
        eqBypassSwitch.setVisible(v);
        for (int i = 0; i < 4; ++i) {
            bandFreqKnobs[i]->setVisible(v);
            bandGainKnobs[i]->setVisible(v);
            bandQKnobs[i]->setVisible(v);
        }
    };

    auto setCompVisible = [this](bool v) {
        lowMidXoverKnob.setVisible(v);
        midHighXoverKnob.setVisible(v);
        compBypassSwitch.setVisible(v);
        for (int i = 0; i < 3; ++i) {
            compThresholdKnobs[i]->setVisible(v);
            compRatioKnobs[i]->setVisible(v);
            compAttackKnobs[i]->setVisible(v);
            compReleaseKnobs[i]->setVisible(v);
            compMakeupKnobs[i]->setVisible(v);
            compGRMeters[i].setVisible(v);
        }
    };

    auto setStereoVisible = [this](bool v) {
        globalWidthKnob.setVisible(v);
        lowWidthKnob.setVisible(v);
        midWidthKnob.setVisible(v);
        highWidthKnob.setVisible(v);
        monoBassFreqKnob.setVisible(v);
        monoBassEnableSwitch.setVisible(v);
        stereoBypassSwitch.setVisible(v);
        correlationMeter.setVisible(v);
    };

    auto setLimiterVisible = [this](bool v) {
        ceilingKnob.setVisible(v);
        limiterReleaseKnob.setVisible(v);
        limiterBypassSwitch.setVisible(v);
        limiterGRMeter.setVisible(v);
    };

    setEQVisible(module == ProcessingChainView::Module::EQ);
    setCompVisible(module == ProcessingChainView::Module::Compressor);
    setStereoVisible(module == ProcessingChainView::Module::Stereo);
    setLimiterVisible(module == ProcessingChainView::Module::Limiter);

    resized();
    repaint();  // Repaint to update section backgrounds and header
}

void AutomasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1a1a1a));

    // Header bar
    g.setColour(juce::Colour(0xFF222222));
    g.fillRect(0, 0, getWidth(), kHeaderHeight);
    g.setColour(juce::Colour(0xFF00D4AA).withAlpha(0.3f));
    g.fillRect(0, kHeaderHeight - 1, getWidth(), 1);

    // Paint all module backgrounds and labels
    paintAllModules(g);
}

void AutomasterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header bar - compact layout
    auto header = bounds.removeFromTop(kHeaderHeight).reduced(kPadding, 5);

    // Right side first (so we know how much space is left)
    settingsButton.setBounds(header.removeFromRight(45).reduced(0, 6));
    header.removeFromRight(4);
    auto abcdArea = header.removeFromRight(96);
    for (int i = 0; i < 4; ++i)
        abcdButtons[i].setBounds(abcdArea.removeFromLeft(24).reduced(1, 6));
    header.removeFromRight(8);

    // Left side
    titleLabel.setBounds(header.removeFromLeft(170));
    header.removeFromLeft(8);
    analyzeButton.setBounds(header.removeFromLeft(65).reduced(0, 8));
    header.removeFromLeft(4);
    analysisProgressBar->setBounds(header.removeFromLeft(45).reduced(0, 12));
    header.removeFromLeft(8);
    autoMasterButton.setBounds(header.removeFromLeft(115).reduced(0, 4));
    header.removeFromLeft(6);
    targetLUFSKnob.setBounds(header.removeFromLeft(42));
    header.removeFromLeft(4);
    profileCombo.setBounds(header.removeFromLeft(80).reduced(0, 8));
    header.removeFromLeft(4);
    loadRefButton.setBounds(header.removeFromLeft(55).reduced(0, 8));
    header.removeFromLeft(4);
    matchIndicator.setBounds(header.removeFromLeft(60).reduced(0, 8));
    matchIndicator.setVisible(proc.hasReference());

    bounds = bounds.reduced(kPadding, 0);
    bounds.removeFromBottom(kPadding);

    // Hide the tab view
    chainView.setVisible(false);

    // === IN/OUT METERS on right side (vertical) ===
    auto meterSide = bounds.removeFromRight(kMeterSectionWidth);
    bounds.removeFromRight(kSectionGap);

    layoutMetersVertical(meterSide);

    // === SPECTRUM ANALYZER at top (full width, shows EQ curve) ===
    auto spectrumArea = bounds.removeFromTop(kSpectrumHeight);
    spectrumAnalyzer.setBounds(spectrumArea);
    bounds.removeFromTop(kSectionGap);

    // === METER STRIP below spectrum (LUFS, correlation, comp GR, BIG limiter GR) ===
    auto meterStrip = bounds.removeFromTop(kMeterStripHeight);
    bounds.removeFromTop(kSectionGap);

    layoutMeterStrip(meterStrip);

    // Use the namespace constant for module row height

    // === ROW 1: EQ and COMPRESSOR (2 rows of knobs, needs more height) ===
    auto row1 = bounds.removeFromTop(kModuleRow1H);
    bounds.removeFromTop(kModuleGap);

    int halfW = (row1.getWidth() - kSectionGap) / 2;
    auto eqArea = row1.removeFromLeft(halfW);
    row1.removeFromLeft(kSectionGap);
    auto compArea = row1;

    layoutEQAll(eqArea);
    layoutCompressorAll(compArea);

    // === ROW 2: STEREO and LIMITER (1 row of knobs, compact) ===
    auto row2 = bounds.removeFromTop(kModuleRow2H);

    auto stereoArea = row2.removeFromLeft(halfW);
    row2.removeFromLeft(kSectionGap);
    auto limiterArea = row2;

    layoutStereoAll(stereoArea);
    layoutLimiterAll(limiterArea);
}

void AutomasterAudioProcessorEditor::layoutCurrentModule(juce::Rectangle<int> area)
{
    switch (currentModule)
    {
        case ProcessingChainView::Module::EQ:         layoutEQ(area); break;
        case ProcessingChainView::Module::Compressor: layoutCompressor(area); break;
        case ProcessingChainView::Module::Stereo:     layoutStereo(area); break;
        case ProcessingChainView::Module::Limiter:    layoutLimiter(area); break;
    }
}

// =============================================================================
// ALL PANELS USE THE SAME kKnobSize AND kKnobHeight
// =============================================================================

void AutomasterAudioProcessorEditor::layoutEQ(juce::Rectangle<int> area)
{
    // EQ Layout: [FILTERS] | [SHELVES] | [PARAMETRIC] - all fixed widths

    // Reserve space for section labels
    area.removeFromTop(kSectionLabelH + 4);

    // Content height: 2 rows of knobs
    const int contentH = kKnobHeight * 2 + kRowGap;

    // Center content vertically
    int vPad = (area.getHeight() - contentH) / 2;
    if (vPad > 0) area.removeFromTop(vPad);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // === FILTERS SECTION ===
    auto filtersArea = area.removeFromLeft(kFiltersSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    // HPF row
    auto hpfRow = filtersArea.removeFromTop(kKnobHeight);
    hpfEnableSwitch.setBounds(hpfRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    hpfRow.removeFromLeft(kGap);
    hpfFreqKnob.setBounds(hpfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    filtersArea.removeFromTop(kRowGap);

    // LPF row
    auto lpfRow = filtersArea.removeFromTop(kKnobHeight);
    lpfEnableSwitch.setBounds(lpfRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    lpfRow.removeFromLeft(kGap);
    lpfFreqKnob.setBounds(lpfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // === SHELVES SECTION ===
    auto shelvesArea = area.removeFromLeft(kShelvesSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    // Low shelf row
    auto loShelfRow = shelvesArea.removeFromTop(kKnobHeight);
    lowShelfFreqKnob.setBounds(loShelfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    loShelfRow.removeFromLeft(kGap);
    lowShelfGainKnob.setBounds(loShelfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    shelvesArea.removeFromTop(kRowGap);

    // High shelf row
    auto hiShelfRow = shelvesArea.removeFromTop(kKnobHeight);
    highShelfFreqKnob.setBounds(hiShelfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    hiShelfRow.removeFromLeft(kGap);
    highShelfGainKnob.setBounds(hiShelfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // === PARAMETRIC BANDS SECTION ===
    auto bandsArea = area.removeFromLeft(kBandsSectionW).reduced(4, 0);

    // Freq row (4 bands) + bypass in corner
    auto freqRow = bandsArea.removeFromTop(kKnobHeight);
    for (int i = 0; i < 4; ++i)
    {
        bandFreqKnobs[i]->setBounds(freqRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        freqRow.removeFromLeft(kGap);
    }
    // Bypass switch after last knob
    eqBypassSwitch.setBounds(freqRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));

    bandsArea.removeFromTop(kRowGap);

    // Gain row (4 bands)
    auto gainRow = bandsArea.removeFromTop(kKnobHeight);
    for (int i = 0; i < 4; ++i)
    {
        bandGainKnobs[i]->setBounds(gainRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        if (i < 3) gainRow.removeFromLeft(kGap);
    }

    // Q knobs hidden in compact view
    for (int i = 0; i < 4; ++i)
        bandQKnobs[i]->setVisible(false);
}

void AutomasterAudioProcessorEditor::paintEQSections(juce::Graphics& g, juce::Rectangle<int> area)
{
    const int contentH = kKnobHeight * 2 + kRowGap;

    // Work with a copy of the area for label positioning
    auto labelArea = area;
    auto labelRow = labelArea.removeFromTop(kSectionLabelH);

    // Section labels
    g.setColour(juce::Colour(0xFF99AABB));
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

    auto filtersLbl = labelRow.removeFromLeft(kFiltersSectionW);
    g.drawText("FILTERS", filtersLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto shelvesLbl = labelRow.removeFromLeft(kShelvesSectionW);
    g.drawText("SHELVES", shelvesLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto bandsLbl = labelRow.removeFromLeft(kBandsSectionW);
    g.drawText("PARAMETRIC", bandsLbl, juce::Justification::centred);

    // Section backgrounds - positioned below labels, aligned with knobs
    auto bgStartY = area.getY() + kSectionLabelH + 2;
    int vPad = (area.getHeight() - kSectionLabelH - 4 - contentH) / 2;
    if (vPad > 2) bgStartY += vPad - 2;

    g.setColour(juce::Colour(0xFF262626));

    int x = area.getX();

    // Filters bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kFiltersSectionW, (float)(contentH + 4), 4.0f);
    x += kFiltersSectionW + kSectionGap;

    // Shelves bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kShelvesSectionW, (float)(contentH + 4), 4.0f);
    x += kShelvesSectionW + kSectionGap;

    // Bands bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kBandsSectionW, (float)(contentH + 4), 4.0f);
}

void AutomasterAudioProcessorEditor::layoutCompressor(juce::Rectangle<int> area)
{
    // Compressor: [XOVER] | [LOW] | [MID] | [HIGH] - all fixed widths

    const int contentH = kKnobHeight * 2 + kRowGap;

    // Reserve space for section labels
    area.removeFromTop(kSectionLabelH + 4);

    // Center content vertically
    int vPad = (area.getHeight() - contentH) / 2;
    if (vPad > 0) area.removeFromTop(vPad);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // === CROSSOVER SECTION ===
    auto xoverArea = area.removeFromLeft(kXoverSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    auto xoverRow1 = xoverArea.removeFromTop(kKnobHeight);
    lowMidXoverKnob.setBounds(xoverRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    xoverArea.removeFromTop(kRowGap);

    auto xoverRow2 = xoverArea.removeFromTop(kKnobHeight);
    midHighXoverKnob.setBounds(xoverRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // === LOW BAND SECTION ===
    auto lowArea = area.removeFromLeft(kCompBandSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    auto lowRow1 = lowArea.removeFromTop(kKnobHeight);
    compThresholdKnobs[0]->setBounds(lowRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    lowRow1.removeFromLeft(kGap);
    compRatioKnobs[0]->setBounds(lowRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    lowArea.removeFromTop(kRowGap);

    auto lowRow2 = lowArea.removeFromTop(kKnobHeight);
    compAttackKnobs[0]->setBounds(lowRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    lowRow2.removeFromLeft(kGap);
    compReleaseKnobs[0]->setBounds(lowRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    compMakeupKnobs[0]->setVisible(false);
    compGRMeters[0].setVisible(false);

    // === MID BAND SECTION ===
    auto midArea = area.removeFromLeft(kCompBandSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    auto midRow1 = midArea.removeFromTop(kKnobHeight);
    compThresholdKnobs[1]->setBounds(midRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    midRow1.removeFromLeft(kGap);
    compRatioKnobs[1]->setBounds(midRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    midArea.removeFromTop(kRowGap);

    auto midRow2 = midArea.removeFromTop(kKnobHeight);
    compAttackKnobs[1]->setBounds(midRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    midRow2.removeFromLeft(kGap);
    compReleaseKnobs[1]->setBounds(midRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    compMakeupKnobs[1]->setVisible(false);
    compGRMeters[1].setVisible(false);

    // === HIGH BAND SECTION ===
    auto highArea = area.removeFromLeft(kCompHighSectionW).reduced(4, 0);

    auto highRow1 = highArea.removeFromTop(kKnobHeight);
    compThresholdKnobs[2]->setBounds(highRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    highRow1.removeFromLeft(kGap);
    compRatioKnobs[2]->setBounds(highRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    highRow1.removeFromLeft(kGap);
    // Bypass after knobs
    compBypassSwitch.setBounds(highRow1.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));

    highArea.removeFromTop(kRowGap);

    auto highRow2 = highArea.removeFromTop(kKnobHeight);
    compAttackKnobs[2]->setBounds(highRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    highRow2.removeFromLeft(kGap);
    compReleaseKnobs[2]->setBounds(highRow2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    compMakeupKnobs[2]->setVisible(false);
    compGRMeters[2].setVisible(false);
}

void AutomasterAudioProcessorEditor::paintCompressorSections(juce::Graphics& g, juce::Rectangle<int> area)
{
    const int contentH = kKnobHeight * 2 + kRowGap;

    // Section labels
    auto labelArea = area;
    auto labelRow = labelArea.removeFromTop(kSectionLabelH);

    g.setColour(juce::Colour(0xFF99AABB));
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

    auto xoverLbl = labelRow.removeFromLeft(kXoverSectionW);
    g.drawText("XOVER", xoverLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto lowLbl = labelRow.removeFromLeft(kCompBandSectionW);
    g.drawText("LOW", lowLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto midLbl = labelRow.removeFromLeft(kCompBandSectionW);
    g.drawText("MID", midLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto highLbl = labelRow.removeFromLeft(kCompHighSectionW);
    g.drawText("HIGH", highLbl, juce::Justification::centred);

    // Section backgrounds
    auto bgStartY = area.getY() + kSectionLabelH + 2;
    int vPad = (area.getHeight() - kSectionLabelH - 4 - contentH) / 2;
    if (vPad > 2) bgStartY += vPad - 2;

    g.setColour(juce::Colour(0xFF262626));

    int x = area.getX();

    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kXoverSectionW, (float)(contentH + 4), 4.0f);
    x += kXoverSectionW + kSectionGap;

    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kCompBandSectionW, (float)(contentH + 4), 4.0f);
    x += kCompBandSectionW + kSectionGap;

    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kCompBandSectionW, (float)(contentH + 4), 4.0f);
    x += kCompBandSectionW + kSectionGap;

    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)kCompHighSectionW, (float)(contentH + 4), 4.0f);
}

void AutomasterAudioProcessorEditor::layoutStereo(juce::Rectangle<int> area)
{
    // Stereo: [CORRELATION label + METER] | [CONTROLS label + knobs row]

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // === CORRELATION label ===
    area.removeFromTop(kSectionLabelH + 2);

    // === METER SECTION - full width ===
    auto meterArea = area.removeFromTop(50).reduced(4, 4);
    correlationMeter.setBounds(meterArea);

    area.removeFromTop(kSectionGap);

    // === CONTROLS label ===
    area.removeFromTop(kSectionLabelH + 2);

    // === CONTROLS SECTION - all controls in a row ===
    auto ctrlArea = area.reduced(4, 4);
    auto ctrlRow = ctrlArea.removeFromTop(kKnobHeight);

    globalWidthKnob.setBounds(ctrlRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    ctrlRow.removeFromLeft(kGap);

    lowWidthKnob.setBounds(ctrlRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    ctrlRow.removeFromLeft(kGap);

    midWidthKnob.setBounds(ctrlRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    ctrlRow.removeFromLeft(kGap);

    highWidthKnob.setBounds(ctrlRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    ctrlRow.removeFromLeft(kGap * 2);  // Extra gap before mono section

    monoBassEnableSwitch.setBounds(ctrlRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    ctrlRow.removeFromLeft(kGap);

    monoBassFreqKnob.setBounds(ctrlRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // Bypass at far right
    stereoBypassSwitch.setBounds(ctrlRow.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
}

void AutomasterAudioProcessorEditor::paintStereoSections(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Two stacked sections: CORRELATION on top, CONTROLS on bottom

    g.setColour(juce::Colour(0xFF99AABB));
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

    // CORRELATION label
    auto corrLabel = area.removeFromTop(kSectionLabelH);
    g.drawText("CORRELATION", corrLabel, juce::Justification::centred);
    area.removeFromTop(2);

    g.setColour(juce::Colour(0xFF262626));

    // Meter section background - full width
    auto meterBg = area.removeFromTop(50);
    g.fillRoundedRectangle(meterBg.toFloat(), 4.0f);

    area.removeFromTop(kSectionGap);

    // CONTROLS label
    g.setColour(juce::Colour(0xFF99AABB));
    auto ctrlLabel = area.removeFromTop(kSectionLabelH);
    g.drawText("CONTROLS", ctrlLabel, juce::Justification::centred);
    area.removeFromTop(2);

    g.setColour(juce::Colour(0xFF262626));

    // Controls section background - full width
    auto ctrlBg = area.withHeight(kKnobHeight + 8);
    g.fillRoundedRectangle(ctrlBg.toFloat(), 4.0f);
}

void AutomasterAudioProcessorEditor::layoutLimiter(juce::Rectangle<int> area)
{
    // Limiter: [CONTROLS] | [GAIN REDUCTION - fills remaining space]

    // Reserve space for section labels
    area.removeFromTop(kSectionLabelH + 4);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // === CONTROLS SECTION ===
    auto controlsArea = area.removeFromLeft(kLimiterCtrlSectionW).reduced(4, 4);
    area.removeFromLeft(kSectionGap);

    // Knobs at top of controls section
    auto ctrlRow1 = controlsArea.removeFromTop(kKnobHeight);
    ceilingKnob.setBounds(ctrlRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    ctrlRow1.removeFromLeft(kGap);
    limiterReleaseKnob.setBounds(ctrlRow1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // === METER SECTION - fills ALL remaining space ===
    auto meterArea = area.reduced(4, 4);

    // Bypass in top-right
    auto meterRow1 = meterArea.removeFromTop(kSwitchHeight + 4);
    limiterBypassSwitch.setBounds(meterRow1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight));

    // GR meter spans remaining width and height
    meterArea.removeFromTop(4);
    limiterGRMeter.setBounds(meterArea);
}

void AutomasterAudioProcessorEditor::paintLimiterSections(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Section labels
    auto labelRow = area.removeFromTop(kSectionLabelH);

    g.setColour(juce::Colour(0xFF99AABB));
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

    auto ctrlLbl = labelRow.removeFromLeft(kLimiterCtrlSectionW);
    g.drawText("CONTROLS", ctrlLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    // GAIN REDUCTION label fills remaining space
    g.drawText("GAIN REDUCTION", labelRow, juce::Justification::centred);

    // Section backgrounds - both fill remaining height equally
    area.removeFromTop(2);  // Small gap after labels
    int bgH = area.getHeight();

    g.setColour(juce::Colour(0xFF262626));

    int x = area.getX();

    g.fillRoundedRectangle((float)x, (float)area.getY(), (float)kLimiterCtrlSectionW, (float)bgH, 4.0f);
    x += kLimiterCtrlSectionW + kSectionGap;

    // GAIN REDUCTION fills ALL remaining width
    int remainingW = area.getRight() - x;
    g.fillRoundedRectangle((float)x, (float)area.getY(), (float)remainingW, (float)bgH, 4.0f);
}

// =============================================================================
// ALL-IN-ONE LAYOUT FUNCTIONS
// =============================================================================

void AutomasterAudioProcessorEditor::layoutEQAll(juce::Rectangle<int> area)
{
    // Make all EQ controls visible
    hpfEnableSwitch.setVisible(true);
    hpfFreqKnob.setVisible(true);
    lpfEnableSwitch.setVisible(true);
    lpfFreqKnob.setVisible(true);
    lowShelfFreqKnob.setVisible(true);
    lowShelfGainKnob.setVisible(true);
    highShelfFreqKnob.setVisible(true);
    highShelfGainKnob.setVisible(true);
    eqBypassSwitch.setVisible(true);
    for (int i = 0; i < 4; ++i) {
        bandFreqKnobs[i]->setVisible(true);
        bandGainKnobs[i]->setVisible(true);
        bandQKnobs[i]->setVisible(false);  // Q knobs hidden for space
    }

    area.removeFromTop(kModuleLabelH + 2);  // Label space
    area = area.reduced(4, 2);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // Row 1: HPF, LPF, LowShelf, HighShelf
    auto row1 = area.removeFromTop(kKnobHeight);

    hpfEnableSwitch.setBounds(row1.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    row1.removeFromLeft(2);
    hpfFreqKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);

    lpfEnableSwitch.setBounds(row1.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    row1.removeFromLeft(2);
    lpfFreqKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);

    lowShelfFreqKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    lowShelfGainKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);

    highShelfFreqKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    highShelfGainKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    eqBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));

    area.removeFromTop(kRowGap);

    // Row 2: 4 parametric bands (freq + gain)
    auto row2 = area.removeFromTop(kKnobHeight);
    for (int i = 0; i < 4; ++i) {
        bandFreqKnobs[i]->setBounds(row2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        row2.removeFromLeft(kGap);
        bandGainKnobs[i]->setBounds(row2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        if (i < 3) row2.removeFromLeft(kGap * 2);
    }
}

void AutomasterAudioProcessorEditor::layoutCompressorAll(juce::Rectangle<int> area)
{
    // Make all compressor controls visible (meters are in the strip now)
    lowMidXoverKnob.setVisible(true);
    midHighXoverKnob.setVisible(true);
    compBypassSwitch.setVisible(true);
    for (int i = 0; i < 3; ++i) {
        compThresholdKnobs[i]->setVisible(true);
        compRatioKnobs[i]->setVisible(true);
        compAttackKnobs[i]->setVisible(true);
        compReleaseKnobs[i]->setVisible(true);
        compMakeupKnobs[i]->setVisible(false);
        // GR meters positioned in layoutMeterStrip
    }

    area.removeFromTop(kModuleLabelH + 2);
    area = area.reduced(4, 2);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // Row 1: Xover knobs + Threshold for 3 bands
    auto row1 = area.removeFromTop(kKnobHeight);

    lowMidXoverKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    midHighXoverKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);

    for (int i = 0; i < 3; ++i) {
        compThresholdKnobs[i]->setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        row1.removeFromLeft(kGap);
        compRatioKnobs[i]->setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        if (i < 2) row1.removeFromLeft(kGap * 2);
    }

    compBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));

    area.removeFromTop(kRowGap);

    // Row 2: Attack/Release for 3 bands
    auto row2 = area.removeFromTop(kKnobHeight);
    row2.removeFromLeft(kKnobSize * 2 + kGap + kGap * 2);  // Skip xover column space

    for (int i = 0; i < 3; ++i) {
        compAttackKnobs[i]->setBounds(row2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        row2.removeFromLeft(kGap);
        compReleaseKnobs[i]->setBounds(row2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        if (i < 2) row2.removeFromLeft(kGap * 2);
    }
}

void AutomasterAudioProcessorEditor::layoutStereoAll(juce::Rectangle<int> area)
{
    // Make all stereo controls visible (correlation meter is in the strip now)
    globalWidthKnob.setVisible(true);
    lowWidthKnob.setVisible(true);
    midWidthKnob.setVisible(true);
    highWidthKnob.setVisible(true);
    monoBassFreqKnob.setVisible(true);
    monoBassEnableSwitch.setVisible(true);
    stereoBypassSwitch.setVisible(true);
    // correlationMeter positioned in layoutMeterStrip

    area.removeFromTop(kModuleLabelH + 2);
    area = area.reduced(4, 2);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // All width knobs + mono bass + bypass in one row
    auto row1 = area.removeFromTop(kKnobHeight);

    globalWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);

    lowWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    midWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    highWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);

    monoBassEnableSwitch.setBounds(row1.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    row1.removeFromLeft(2);
    monoBassFreqKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    stereoBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
}

void AutomasterAudioProcessorEditor::layoutLimiterAll(juce::Rectangle<int> area)
{
    // Make limiter controls visible (GR meter is in the strip now - it's the big one!)
    ceilingKnob.setVisible(true);
    limiterReleaseKnob.setVisible(true);
    limiterBypassSwitch.setVisible(true);
    // limiterGRMeter positioned in layoutMeterStrip

    area.removeFromTop(kModuleLabelH + 2);
    area = area.reduced(4, 2);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // Ceiling, Release, Bypass in one row
    auto row1 = area.removeFromTop(kKnobHeight);

    ceilingKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    limiterReleaseKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    limiterBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
}

void AutomasterAudioProcessorEditor::layoutMeters(juce::Rectangle<int> area)
{
    // Old vertical meter layout - kept for compatibility but not used in all-in-one view
    area = area.reduced(2);

    int meterH = (area.getHeight() - 50) / 3;

    inputLabel.setBounds(area.removeFromTop(12));
    auto inArea = area.removeFromTop(meterH);
    int meterX = (area.getWidth() - kMeterWidth * 2 - 2) / 2;
    inputMeterL.setBounds(inArea.getX() + meterX, inArea.getY(), kMeterWidth, inArea.getHeight());
    inputMeterR.setBounds(inArea.getX() + meterX + kMeterWidth + 2, inArea.getY(), kMeterWidth, inArea.getHeight());

    area.removeFromTop(4);

    outputLabel.setBounds(area.removeFromTop(12));
    auto outArea = area.removeFromTop(meterH);
    outputMeterL.setBounds(outArea.getX() + meterX, outArea.getY(), kMeterWidth, outArea.getHeight());
    outputMeterR.setBounds(outArea.getX() + meterX + kMeterWidth + 2, outArea.getY(), kMeterWidth, outArea.getHeight());

    area.removeFromTop(4);

    lufsLabel.setBounds(area.removeFromTop(12));
    lufsMeter.setBounds(area.reduced(2, 0));
}

void AutomasterAudioProcessorEditor::layoutMeterStrip(juce::Rectangle<int> area)
{
    // Meter strip layout: [LUFS] [Correlation] [Comp GR x3] [=== BIG LIMITER GR ===]

    area = area.reduced(4, 2);

    // LUFS meter - bigger so text is readable
    lufsMeter.setVisible(true);
    lufsMeter.setBounds(area.removeFromLeft(100));
    area.removeFromLeft(kSectionGap);

    // Correlation meter - wider for L/R display
    correlationMeter.setVisible(true);
    correlationMeter.setBounds(area.removeFromLeft(140).reduced(0, 4));
    area.removeFromLeft(kSectionGap);

    // 3 Compressor GR meters
    for (int i = 0; i < 3; ++i) {
        compGRMeters[i].setVisible(true);
        compGRMeters[i].setBounds(area.removeFromLeft(80).reduced(0, 8));
        area.removeFromLeft(4);
    }
    area.removeFromLeft(kSectionGap);

    // LIMITER GR meter takes ALL remaining space (big!)
    limiterGRMeter.setVisible(true);
    limiterGRMeter.setBounds(area);
}

void AutomasterAudioProcessorEditor::layoutMetersVertical(juce::Rectangle<int> area)
{
    // Vertical IN/OUT meters on the right side
    area = area.reduced(2, 4);

    inputMeterL.setVisible(true);
    inputMeterR.setVisible(true);
    outputMeterL.setVisible(true);
    outputMeterR.setVisible(true);
    inputLabel.setVisible(true);
    outputLabel.setVisible(true);
    lufsLabel.setVisible(false);  // LUFS is in the strip now

    int meterH = (area.getHeight() - 24) / 2;
    int meterX = (area.getWidth() - kMeterWidth * 2 - 2) / 2;

    inputLabel.setBounds(area.removeFromTop(10));
    auto inArea = area.removeFromTop(meterH);
    inputMeterL.setBounds(inArea.getX() + meterX, inArea.getY(), kMeterWidth, inArea.getHeight());
    inputMeterR.setBounds(inArea.getX() + meterX + kMeterWidth + 2, inArea.getY(), kMeterWidth, inArea.getHeight());

    area.removeFromTop(4);

    outputLabel.setBounds(area.removeFromTop(10));
    auto outArea = area;
    outputMeterL.setBounds(outArea.getX() + meterX, outArea.getY(), kMeterWidth, outArea.getHeight());
    outputMeterR.setBounds(outArea.getX() + meterX + kMeterWidth + 2, outArea.getY(), kMeterWidth, outArea.getHeight());
}

void AutomasterAudioProcessorEditor::paintAllModules(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kHeaderHeight);
    bounds = bounds.reduced(kPadding, 0);
    bounds.removeFromBottom(kPadding);

    // Vertical meter section on right
    auto meterSide = bounds.removeFromRight(kMeterSectionWidth);
    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(meterSide.toFloat(), 4.0f);

    bounds.removeFromRight(kSectionGap);

    // Spectrum analyzer area (painted by component itself)
    bounds.removeFromTop(kSpectrumHeight);
    bounds.removeFromTop(kSectionGap);

    // Meter strip
    auto meterStrip = bounds.removeFromTop(kMeterStripHeight);
    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(meterStrip.toFloat(), 4.0f);

    bounds.removeFromTop(kSectionGap);

    int halfW = (bounds.getWidth() - kSectionGap) / 2;

    // Row 1 - EQ/Comp need more height for 2 rows of knobs
    auto row1 = bounds.removeFromTop(kModuleRow1H);
    auto eqArea = row1.removeFromLeft(halfW);
    row1.removeFromLeft(kSectionGap);
    auto compArea = row1;

    // Row 2 - Stereo/Limiter are compact (1 row of knobs)
    bounds.removeFromTop(kModuleGap);
    auto row2 = bounds.removeFromTop(kModuleRow2H);
    auto stereoArea = row2.removeFromLeft(halfW);
    row2.removeFromLeft(kSectionGap);
    auto limiterArea = row2;

    // Draw module backgrounds
    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(eqArea.toFloat(), 4.0f);
    g.fillRoundedRectangle(compArea.toFloat(), 4.0f);
    g.fillRoundedRectangle(stereoArea.toFloat(), 4.0f);
    g.fillRoundedRectangle(limiterArea.toFloat(), 4.0f);

    // Draw module labels with colored accents
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    auto drawModuleLabel = [&](juce::Rectangle<int> area, const juce::String& name, juce::Colour color) {
        auto labelArea = area.removeFromTop(kModuleLabelH);
        g.setColour(color.withAlpha(0.15f));
        g.fillRoundedRectangle(labelArea.toFloat(), 4.0f);
        g.setColour(color);
        g.fillRect(labelArea.getX(), labelArea.getBottom() - 2, labelArea.getWidth(), 2);
        g.setColour(juce::Colours::white);
        g.drawText(name, labelArea, juce::Justification::centred);
    };

    drawModuleLabel(eqArea, "EQ", AutomasterColors::eqColor);
    drawModuleLabel(compArea, "COMPRESSOR", AutomasterColors::compColor);
    drawModuleLabel(stereoArea, "STEREO", AutomasterColors::stereoColor);
    drawModuleLabel(limiterArea, "LIMITER", AutomasterColors::limiterColor);

    // Draw colored band indicators in EQ section
    paintEQBandIndicators(g, eqArea);
}

void AutomasterAudioProcessorEditor::paintEQBandIndicators(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Skip module label area
    area.removeFromTop(kModuleLabelH + 2);
    area = area.reduced(4, 2);

    // Skip row 1 (HPF, LPF, shelves)
    area.removeFromTop(kKnobHeight);
    area.removeFromTop(kRowGap);

    // Row 2 is where the parametric bands are
    auto row2 = area.removeFromTop(kKnobHeight);

    g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));

    // Draw colored dot and label above each band pair
    for (int i = 0; i < 4; ++i)
    {
        // Each band takes: freq knob + gap + gain knob + (gap*2 if not last)
        auto bandArea = row2.removeFromLeft(kKnobSize);  // freq
        row2.removeFromLeft(kGap);
        auto gainArea = row2.removeFromLeft(kKnobSize);  // gain
        if (i < 3) row2.removeFromLeft(kGap * 2);

    }
}

void AutomasterAudioProcessorEditor::updateMeters()
{
    auto& chain = proc.getMasteringChain();
    auto& analysis = proc.getAnalysisEngine();

    auto& inputMeter = chain.getInputMeter();
    auto& outputMeter = chain.getOutputMeter();

    inputMeterL.setLevel(inputMeter.getPeakLevelL());
    inputMeterR.setLevel(inputMeter.getPeakLevelR());
    outputMeterL.setLevel(outputMeter.getPeakLevelL());
    outputMeterR.setLevel(outputMeter.getPeakLevelR());

    lufsMeter.setLevels(
        outputMeter.getMomentaryLUFS(),
        outputMeter.getShortTermLUFS(),
        outputMeter.getIntegratedLUFS()
    );
    lufsMeter.setTarget(proc.targetLUFS->getProcValue());

    auto& comp = chain.getCompressor();
    for (int i = 0; i < 3; ++i)
        compGRMeters[i].setGainReduction(comp.getGainReduction(i));

    limiterGRMeter.setGainReduction(chain.getLimiter().getGainReduction());
    correlationMeter.setCorrelation(analysis.getCorrelation());

    bool hasRef = proc.hasReference();
    matchIndicator.setVisible(hasRef);
    if (hasRef)
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

void AutomasterAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& filePath : files)
    {
        juce::File file(filePath);
        if (file.existsAsFile())
        {
            if (proc.loadReferenceFile(file))
            {
                referenceWaveform.loadFile(file);
                referenceWaveform.setProfile(&proc.getReferenceProfile());
                break;
            }
        }
    }
}

void AutomasterAudioProcessorEditor::timerCallback()
{
    updateMeters();
    spectrumAnalyzer.repaint();  // Update EQ curve display

    // Update analysis state (Ozone-style workflow)
    bool isAnalyzing = proc.isAnalyzing();
    bool hasAnalysis = proc.hasValidAnalysis();

    // Update progress bar
    analysisProgress = proc.getAnalysisProgress();

    // Update button state
    if (isAnalyzing)
    {
        analyzeButton.setToggleState(true, juce::dontSendNotification);
        float seconds = proc.getAnalysisTimeSeconds();
        analyzeButton.setButtonText(juce::String::formatted("%.1fs", seconds));
    }
    else if (hasAnalysis)
    {
        analyzeButton.setToggleState(false, juce::dontSendNotification);
        analyzeButton.setButtonText("READY");
        analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00FF88));  // Green when ready
    }
    else
    {
        analyzeButton.setToggleState(false, juce::dontSendNotification);
        analyzeButton.setButtonText("ANALYZE");
        analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00D4AA));  // Cyan default
    }

    // Update Auto Master button to show if using accumulated data
    if (hasAnalysis)
    {
        autoMasterButton.setButtonText("AUTO MASTER*");  // Asterisk indicates accumulated data
    }
    else
    {
        autoMasterButton.setButtonText("AUTO MASTER");
    }
}
