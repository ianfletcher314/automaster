#include "PluginEditor.h"

// =============================================================================
// DESIGN SYSTEM - Clean, organized sections
// =============================================================================
namespace {
    // Window
    constexpr int kWindowWidth = 720;
    constexpr int kWindowHeight = 320;

    // Spacing
    constexpr int kPadding = 8;
    constexpr int kGap = 6;              // Horizontal gap between knobs
    constexpr int kSectionGap = 12;      // Gap between sections
    constexpr int kRowGap = 6;           // Vertical gap between rows

    // Knobs
    constexpr int kKnobSize = 50;        // Knob diameter
    constexpr int kKnobLabelH = 18;      // Label height below knob
    constexpr int kKnobHeight = kKnobSize + kKnobLabelH;

    // Switches
    constexpr int kSwitchWidth = 32;
    constexpr int kSwitchHeight = 20;

    // Meters
    constexpr int kMeterWidth = 16;
    constexpr int kMeterSectionWidth = 50;

    // Headers
    constexpr int kHeaderHeight = 18;
    constexpr int kTabHeight = 26;
    constexpr int kSectionLabelH = 14;
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
    for (int i = 0; i < 4; ++i)
    {
        bandFreqKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandFreq[i]));
        bandGainKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandGain[i]));
        bandQKnobs[i] = ownedKnobs.add(new gin::Knob(p.bandQ[i]));
    }

    for (int i = 0; i < 3; ++i)
    {
        compThresholdKnobs[i] = ownedKnobs.add(new gin::Knob(p.compThreshold[i]));
        compRatioKnobs[i] = ownedKnobs.add(new gin::Knob(p.compRatio[i]));
        compAttackKnobs[i] = ownedKnobs.add(new gin::Knob(p.compAttack[i]));
        compReleaseKnobs[i] = ownedKnobs.add(new gin::Knob(p.compRelease[i]));
        compMakeupKnobs[i] = ownedKnobs.add(new gin::Knob(p.compMakeup[i]));
    }

    setupTopBar();
    setupReferenceSection();
    setupModeSection();
    setupModulePanels();
    setupMeterSection();

    showModulePanel(ProcessingChainView::Module::EQ);
    addKeyListener(this);  // Listen for keys from all child components
    setSize(kWindowWidth, kWindowHeight);
}

AutomasterAudioProcessorEditor::~AutomasterAudioProcessorEditor() {}

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
    titleLabel.setFont(juce::FontOptions(24.0f).withStyle("Bold"));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00D4AA));
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
                if (proc.loadReferenceFile(file))
                {
                    referenceWaveform.loadFile(file);
                    referenceWaveform.setProfile(&proc.getReferenceProfile());
                }
            }
        });
    };
    addAndMakeVisible(loadRefButton);
}

void AutomasterAudioProcessorEditor::setupModeSection()
{
    addAndMakeVisible(targetLUFSKnob);

    autoMasterButton.setButtonText("AUTO MASTER");
    autoMasterButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00D4AA));
    autoMasterButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    autoMasterButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    autoMasterButton.onClick = [this]() { proc.triggerAutoMaster(); };
    addAndMakeVisible(autoMasterButton);
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
}

void AutomasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1a1a1a));

    // Header bar
    g.setColour(juce::Colour(0xFF222222));
    g.fillRect(0, 0, getWidth(), 34);
    g.setColour(juce::Colour(0xFF00D4AA).withAlpha(0.3f));
    g.fillRect(0, 33, getWidth(), 1);

    auto bounds = getLocalBounds().reduced(kPadding);
    bounds.removeFromTop(34);

    // Module panel background
    auto moduleArea = bounds.withTrimmedRight(kMeterSectionWidth + kGap);
    moduleArea.removeFromTop(kTabHeight + 3);

    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(moduleArea.toFloat(), 3.0f);

    // Module header with colored accent
    auto headerArea = moduleArea.removeFromTop(kHeaderHeight);
    juce::Colour headerColor;
    juce::String moduleName;
    switch (currentModule) {
        case ProcessingChainView::Module::EQ:
            headerColor = AutomasterColors::eqColor;
            moduleName = "EQUALIZER";
            break;
        case ProcessingChainView::Module::Compressor:
            headerColor = AutomasterColors::compColor;
            moduleName = "COMPRESSOR";
            break;
        case ProcessingChainView::Module::Stereo:
            headerColor = AutomasterColors::stereoColor;
            moduleName = "STEREO";
            break;
        case ProcessingChainView::Module::Limiter:
            headerColor = AutomasterColors::limiterColor;
            moduleName = "LIMITER";
            break;
    }

    g.setColour(headerColor.withAlpha(0.1f));
    g.fillRoundedRectangle(headerArea.toFloat().withTrimmedBottom(-3), 3.0f);
    g.setColour(headerColor);
    g.fillRect(headerArea.getX(), headerArea.getBottom() - 1, headerArea.getWidth(), 1);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    g.drawText(moduleName, headerArea.reduced(4, 0), juce::Justification::centred);

    // Meter section background
    auto meterArea = bounds.withTrimmedLeft(bounds.getWidth() - kMeterSectionWidth);
    meterArea.removeFromTop(kTabHeight + 3);
    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(meterArea.toFloat(), 3.0f);

    // Draw module-specific section backgrounds
    auto contentArea = moduleArea.reduced(kPadding - 2, 2);
    switch (currentModule) {
        case ProcessingChainView::Module::EQ:
            paintEQSections(g, contentArea);
            break;
        case ProcessingChainView::Module::Compressor:
        case ProcessingChainView::Module::Stereo:
        case ProcessingChainView::Module::Limiter:
            // Other modules will get their own section painting later
            break;
    }
}

void AutomasterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header bar - 34px
    auto header = bounds.removeFromTop(34).reduced(kPadding, 4);
    titleLabel.setBounds(header.removeFromLeft(100));
    autoMasterButton.setBounds(header.removeFromLeft(78).reduced(1));
    header.removeFromLeft(kGap);
    targetLUFSKnob.setBounds(header.removeFromLeft(44));
    header.removeFromLeft(kGap);
    profileCombo.setBounds(header.removeFromLeft(80).reduced(0, 2));
    header.removeFromLeft(kGap);
    loadRefButton.setBounds(header.removeFromLeft(48).reduced(0, 2));

    settingsButton.setBounds(header.removeFromRight(44).reduced(0, 2));
    header.removeFromRight(kGap);
    auto abcdArea = header.removeFromRight(88);
    for (int i = 0; i < 4; ++i)
        abcdButtons[i].setBounds(abcdArea.removeFromLeft(21).reduced(1));

    bounds = bounds.reduced(kPadding, 0);
    bounds.removeFromBottom(kPadding);

    // Meter section on right
    auto meterSection = bounds.removeFromRight(kMeterSectionWidth);
    bounds.removeFromRight(kGap);

    // Tabs
    chainView.setBounds(bounds.removeFromTop(kTabHeight));
    bounds.removeFromTop(3);

    // Module panel content area (skip the header painted area)
    auto modulePanel = bounds.withTrimmedTop(kHeaderHeight);
    modulePanel = modulePanel.reduced(kPadding - 2, 2);

    layoutCurrentModule(modulePanel);

    // Meters layout
    meterSection.removeFromTop(kTabHeight + 3 + kHeaderHeight);
    meterSection = meterSection.reduced(2);

    int meterH = (meterSection.getHeight() - 22 - 20) / 2;

    inputLabel.setBounds(meterSection.removeFromTop(8));
    auto inArea = meterSection.removeFromTop(meterH);
    int meterX = (meterSection.getWidth() - kMeterWidth * 2 - 2) / 2;
    inputMeterL.setBounds(inArea.getX() + meterX, inArea.getY(), kMeterWidth, inArea.getHeight());
    inputMeterR.setBounds(inArea.getX() + meterX + kMeterWidth + 2, inArea.getY(), kMeterWidth, inArea.getHeight());

    outputLabel.setBounds(meterSection.removeFromTop(8));
    auto outArea = meterSection.removeFromTop(meterH);
    outputMeterL.setBounds(outArea.getX() + meterX, outArea.getY(), kMeterWidth, outArea.getHeight());
    outputMeterR.setBounds(outArea.getX() + meterX + kMeterWidth + 2, outArea.getY(), kMeterWidth, outArea.getHeight());

    lufsLabel.setBounds(meterSection.removeFromTop(6));
    lufsMeter.setBounds(meterSection.reduced(1, 0));
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
    // EQ Layout: [FILTERS] | [SHELVES] | [PARAMETRIC BANDS] | [BYPASS]

    // Section widths - slightly padded for section backgrounds
    const int filtersSectionW = kSwitchWidth + 4 + kKnobSize + 8;
    const int shelvesSectionW = kKnobSize * 2 + kGap + 8;
    const int bandsSectionW = kKnobSize * 4 + kGap * 3 + 8;

    // Reserve space for section labels
    area.removeFromTop(kSectionLabelH + 4);

    // Content height: 2 rows of knobs
    const int contentH = kKnobHeight * 2 + kRowGap;

    // Center content vertically
    int vPad = (area.getHeight() - contentH) / 2;
    if (vPad > 0) area.removeFromTop(vPad);

    // === FILTERS SECTION ===
    auto filtersArea = area.removeFromLeft(filtersSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    int switchY = (kKnobSize - kSwitchHeight) / 2;

    // HPF row
    auto hpfRow = filtersArea.removeFromTop(kKnobHeight);
    hpfEnableSwitch.setBounds(hpfRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    hpfRow.removeFromLeft(4);
    hpfFreqKnob.setBounds(hpfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    filtersArea.removeFromTop(kRowGap);

    // LPF row
    auto lpfRow = filtersArea.removeFromTop(kKnobHeight);
    lpfEnableSwitch.setBounds(lpfRow.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, switchY));
    lpfRow.removeFromLeft(4);
    lpfFreqKnob.setBounds(lpfRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));

    // === SHELVES SECTION ===
    auto shelvesArea = area.removeFromLeft(shelvesSectionW).reduced(4, 0);
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
    auto bandsArea = area.removeFromLeft(bandsSectionW).reduced(4, 0);
    area.removeFromLeft(kSectionGap);

    // Freq row (4 bands)
    auto freqRow = bandsArea.removeFromTop(kKnobHeight);
    for (int i = 0; i < 4; ++i)
    {
        bandFreqKnobs[i]->setBounds(freqRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        if (i < 3) freqRow.removeFromLeft(kGap);
    }

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

    // === BYPASS SWITCH ===
    int bypassY = kKnobHeight - kSwitchHeight / 2;
    eqBypassSwitch.setBounds(area.withWidth(kSwitchWidth).withHeight(kSwitchHeight).translated(6, bypassY));
}

void AutomasterAudioProcessorEditor::paintEQSections(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Section widths (must match layoutEQ exactly)
    const int filtersSectionW = kSwitchWidth + 4 + kKnobSize + 8;
    const int shelvesSectionW = kKnobSize * 2 + kGap + 8;
    const int bandsSectionW = kKnobSize * 4 + kGap * 3 + 8;
    const int contentH = kKnobHeight * 2 + kRowGap;

    // Work with a copy of the area for label positioning
    auto labelArea = area;
    auto labelRow = labelArea.removeFromTop(kSectionLabelH);

    // Section labels
    g.setColour(juce::Colour(0xFF99AABB));
    g.setFont(juce::FontOptions(9.0f).withStyle("Bold"));

    auto filtersLbl = labelRow.removeFromLeft(filtersSectionW);
    g.drawText("FILTERS", filtersLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto shelvesLbl = labelRow.removeFromLeft(shelvesSectionW);
    g.drawText("SHELVES", shelvesLbl, juce::Justification::centred);
    labelRow.removeFromLeft(kSectionGap);

    auto bandsLbl = labelRow.removeFromLeft(bandsSectionW);
    g.drawText("PARAMETRIC", bandsLbl, juce::Justification::centred);

    // Section backgrounds - positioned below labels, aligned with knobs
    auto bgStartY = area.getY() + kSectionLabelH + 2;
    int vPad = (area.getHeight() - kSectionLabelH - 4 - contentH) / 2;
    if (vPad > 2) bgStartY += vPad - 2;

    g.setColour(juce::Colour(0xFF262626));

    int x = area.getX();

    // Filters bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)filtersSectionW, (float)(contentH + 4), 4.0f);
    x += filtersSectionW + kSectionGap;

    // Shelves bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)shelvesSectionW, (float)(contentH + 4), 4.0f);
    x += shelvesSectionW + kSectionGap;

    // Bands bg
    g.fillRoundedRectangle((float)x, (float)bgStartY, (float)bandsSectionW, (float)(contentH + 4), 4.0f);
}

void AutomasterAudioProcessorEditor::layoutCompressor(juce::Rectangle<int> area)
{
    // Compressor: Crossovers on top, then 3 bands each with Thresh/Ratio/Attack/Release/Makeup + GR meter
    // Using SAME kKnobSize as EQ

    int totalRows = 3; // Crossover row + 2 rows of band controls
    int contentHeight = totalRows * kKnobHeight + kRowGap * 2;
    int vertPad = (area.getHeight() - contentHeight) / 2;
    if (vertPad > 0) area.removeFromTop(vertPad);

    // Crossover row
    auto xoverRow = area.removeFromTop(kKnobHeight);
    lowMidXoverKnob.setBounds(xoverRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    xoverRow.removeFromLeft(kGap);
    midHighXoverKnob.setBounds(xoverRow.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    compBypassSwitch.setBounds(xoverRow.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, (kKnobHeight - kSwitchHeight) / 2));

    area.removeFromTop(kRowGap);

    // Band controls - 2 rows, 3 bands
    int bandWidth = (area.getWidth() - kGap * 2) / 3;

    // Row with Threshold, Ratio, Attack for each band
    auto ctrlRow1 = area.removeFromTop(kKnobHeight);
    for (int i = 0; i < 3; ++i)
    {
        auto bandArea = ctrlRow1.removeFromLeft(bandWidth);
        if (i < 2) ctrlRow1.removeFromLeft(kGap);

        compThresholdKnobs[i]->setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        compRatioKnobs[i]->setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        compAttackKnobs[i]->setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    }

    area.removeFromTop(kRowGap);

    // Row with Release, Makeup, GR meter for each band
    auto ctrlRow2 = area.removeFromTop(kKnobHeight);
    for (int i = 0; i < 3; ++i)
    {
        auto bandArea = ctrlRow2.removeFromLeft(bandWidth);
        if (i < 2) ctrlRow2.removeFromLeft(kGap);

        compReleaseKnobs[i]->setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        compMakeupKnobs[i]->setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
        compGRMeters[i].setBounds(bandArea.removeFromLeft(kKnobSize).withHeight(kKnobHeight).reduced(8, 4));
    }
}

void AutomasterAudioProcessorEditor::layoutStereo(juce::Rectangle<int> area)
{
    // Stereo: 2 rows - Width controls, then MonoBass + Correlation
    // Using SAME kKnobSize as EQ

    int totalRows = 2;
    int contentHeight = totalRows * kKnobHeight + kRowGap;
    int vertPad = (area.getHeight() - contentHeight) / 2;
    if (vertPad > 0) area.removeFromTop(vertPad);

    // Row 1: Global + Low/Mid/High width
    auto row1 = area.removeFromTop(kKnobHeight);
    globalWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);
    lowWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    midWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    highWidthKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    stereoBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, (kKnobHeight - kSwitchHeight) / 2));

    area.removeFromTop(kRowGap);

    // Row 2: Mono bass + correlation
    auto row2 = area.removeFromTop(kKnobHeight);
    monoBassEnableSwitch.setBounds(row2.removeFromLeft(kSwitchWidth).withHeight(kSwitchHeight).translated(0, (kKnobHeight - kSwitchHeight) / 2));
    row2.removeFromLeft(4);
    monoBassFreqKnob.setBounds(row2.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row2.removeFromLeft(kGap * 2);
    correlationMeter.setBounds(row2.removeFromLeft(kKnobSize * 3).withHeight(kKnobHeight - 8).translated(0, 4));
}

void AutomasterAudioProcessorEditor::layoutLimiter(juce::Rectangle<int> area)
{
    // Limiter: 2 rows to match other panels visually
    // Using SAME kKnobSize as EQ

    int totalRows = 2;
    int contentHeight = totalRows * kKnobHeight + kRowGap;
    int vertPad = (area.getHeight() - contentHeight) / 2;
    if (vertPad > 0) area.removeFromTop(vertPad);

    // Row 1: Ceiling + Release + GR meter
    auto row1 = area.removeFromTop(kKnobHeight);
    ceilingKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap);
    limiterReleaseKnob.setBounds(row1.removeFromLeft(kKnobSize).withHeight(kKnobHeight));
    row1.removeFromLeft(kGap * 2);
    limiterGRMeter.setBounds(row1.removeFromLeft(kKnobSize * 4).withHeight(kKnobHeight - 8).translated(0, 4));
    limiterBypassSwitch.setBounds(row1.removeFromRight(kSwitchWidth).withHeight(kSwitchHeight).translated(0, (kKnobHeight - kSwitchHeight) / 2));

    // Row 2: Empty but maintains consistent 2-row height with other panels
    area.removeFromTop(kRowGap);
    area.removeFromTop(kKnobHeight);
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

    if (proc.hasReference())
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
