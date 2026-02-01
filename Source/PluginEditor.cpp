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
    constexpr int kSwitchWidth = 24;
    constexpr int kSwitchHeight = 14;

    // Meters
    constexpr int kMeterWidth = 10;
    constexpr int kMeterSectionWidth = 36;

    // Headers
    constexpr int kHeaderHeight = 28;
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
    constexpr int kMeterStripHeight = 60;

    // Window - compact, fits content
    constexpr int kWindowWidth = 950;
    constexpr int kWindowHeight = 360;
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

    // Header bar
    auto header = bounds.removeFromTop(kHeaderHeight).reduced(kPadding, 3);
    titleLabel.setBounds(header.removeFromLeft(100));
    autoMasterButton.setBounds(header.removeFromLeft(90).reduced(1));
    header.removeFromLeft(kGap);
    targetLUFSKnob.setBounds(header.removeFromLeft(44));
    header.removeFromLeft(kGap);
    profileCombo.setBounds(header.removeFromLeft(90).reduced(0, 2));
    header.removeFromLeft(kGap);
    loadRefButton.setBounds(header.removeFromLeft(60).reduced(0, 2));

    settingsButton.setBounds(header.removeFromRight(50).reduced(0, 2));
    header.removeFromRight(kGap);
    auto abcdArea = header.removeFromRight(100);
    for (int i = 0; i < 4; ++i)
        abcdButtons[i].setBounds(abcdArea.removeFromLeft(24).reduced(1));

    bounds = bounds.reduced(kPadding, 0);
    bounds.removeFromBottom(kPadding);

    // Hide the tab view
    chainView.setVisible(false);

    // === IN/OUT METERS on right side (vertical) ===
    auto meterSide = bounds.removeFromRight(kMeterSectionWidth);
    bounds.removeFromRight(kSectionGap);

    layoutMetersVertical(meterSide);

    // === METER STRIP at top (LUFS, correlation, comp GR, BIG limiter GR) ===
    auto meterStrip = bounds.removeFromTop(kMeterStripHeight);
    bounds.removeFromTop(kSectionGap);

    layoutMeterStrip(meterStrip);

    // Split remaining area into 2 rows for modules
    int rowH = (bounds.getHeight() - kModuleGap) / 2;

    // === ROW 1: EQ and COMPRESSOR ===
    auto row1 = bounds.removeFromTop(rowH);
    bounds.removeFromTop(kModuleGap);

    int halfW = (row1.getWidth() - kSectionGap) / 2;
    auto eqArea = row1.removeFromLeft(halfW);
    row1.removeFromLeft(kSectionGap);
    auto compArea = row1;

    layoutEQAll(eqArea);
    layoutCompressorAll(compArea);

    // === ROW 2: STEREO and LIMITER ===
    auto row2 = bounds;

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

    // Meter strip at top
    auto meterStrip = bounds.removeFromTop(kMeterStripHeight);
    g.setColour(juce::Colour(0xFF1e1e1e));
    g.fillRoundedRectangle(meterStrip.toFloat(), 4.0f);

    bounds.removeFromTop(kSectionGap);

    int rowH = (bounds.getHeight() - kModuleGap) / 2;
    int halfW = (bounds.getWidth() - kSectionGap) / 2;

    // Row 1
    auto row1 = bounds.removeFromTop(rowH);
    auto eqArea = row1.removeFromLeft(halfW);
    row1.removeFromLeft(kSectionGap);
    auto compArea = row1;

    // Row 2
    bounds.removeFromTop(kModuleGap);
    auto row2 = bounds;
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
