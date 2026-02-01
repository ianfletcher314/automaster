#include "PluginProcessor.h"
#include "PluginEditor.h"

AutomasterAudioProcessor::AutomasterAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers
    inputGainParam = apvts.getRawParameterValue("inputGain");
    outputGainParam = apvts.getRawParameterValue("outputGain");
    targetLUFSParam = apvts.getRawParameterValue("targetLUFS");
    autoMasterEnabledParam = apvts.getRawParameterValue("autoMasterEnabled");

    // EQ
    hpfFreqParam = apvts.getRawParameterValue("hpfFreq");
    hpfEnabledParam = apvts.getRawParameterValue("hpfEnabled");
    lpfFreqParam = apvts.getRawParameterValue("lpfFreq");
    lpfEnabledParam = apvts.getRawParameterValue("lpfEnabled");
    lowShelfFreqParam = apvts.getRawParameterValue("lowShelfFreq");
    lowShelfGainParam = apvts.getRawParameterValue("lowShelfGain");
    highShelfFreqParam = apvts.getRawParameterValue("highShelfFreq");
    highShelfGainParam = apvts.getRawParameterValue("highShelfGain");

    for (int i = 0; i < 4; ++i)
    {
        bandFreqParams[i] = apvts.getRawParameterValue("band" + juce::String(i + 1) + "Freq");
        bandGainParams[i] = apvts.getRawParameterValue("band" + juce::String(i + 1) + "Gain");
        bandQParams[i] = apvts.getRawParameterValue("band" + juce::String(i + 1) + "Q");
    }
    eqBypassParam = apvts.getRawParameterValue("eqBypass");

    // Compressor
    lowMidXoverParam = apvts.getRawParameterValue("lowMidXover");
    midHighXoverParam = apvts.getRawParameterValue("midHighXover");

    for (int i = 0; i < 3; ++i)
    {
        compThresholdParams[i] = apvts.getRawParameterValue("comp" + juce::String(i + 1) + "Threshold");
        compRatioParams[i] = apvts.getRawParameterValue("comp" + juce::String(i + 1) + "Ratio");
        compAttackParams[i] = apvts.getRawParameterValue("comp" + juce::String(i + 1) + "Attack");
        compReleaseParams[i] = apvts.getRawParameterValue("comp" + juce::String(i + 1) + "Release");
        compMakeupParams[i] = apvts.getRawParameterValue("comp" + juce::String(i + 1) + "Makeup");
    }
    compBypassParam = apvts.getRawParameterValue("compBypass");

    // Stereo
    globalWidthParam = apvts.getRawParameterValue("globalWidth");
    lowWidthParam = apvts.getRawParameterValue("lowWidth");
    midWidthParam = apvts.getRawParameterValue("midWidth");
    highWidthParam = apvts.getRawParameterValue("highWidth");
    monoBassFreqParam = apvts.getRawParameterValue("monoBassFreq");
    monoBassEnabledParam = apvts.getRawParameterValue("monoBassEnabled");
    stereoBypassParam = apvts.getRawParameterValue("stereoBypass");

    // Limiter
    ceilingParam = apvts.getRawParameterValue("ceiling");
    limiterReleaseParam = apvts.getRawParameterValue("limiterRelease");
    limiterBypassParam = apvts.getRawParameterValue("limiterBypass");

    // Load learning data
    learningSystem.loadFromFile(LearningSystem::getDefaultFilePath());
}

AutomasterAudioProcessor::~AutomasterAudioProcessor()
{
    // Save learning data
    if (learningSystem.hasUnsavedChanges())
    {
        juce::File saveFile = LearningSystem::getDefaultFilePath();
        saveFile.getParentDirectory().createDirectory();
        learningSystem.saveToFile(saveFile);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout AutomasterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Global
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "inputGain", "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "targetLUFS", "Target LUFS", -24.0f, -6.0f, -14.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "autoMasterEnabled", "Auto Master", false));

    // EQ
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hpfFreq", "HPF Frequency",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 30.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "hpfEnabled", "HPF Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lpfFreq", "LPF Frequency",
        juce::NormalisableRange<float>(5000.0f, 20000.0f, 1.0f, 0.5f), 18000.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "lpfEnabled", "LPF Enabled", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowShelfFreq", "Low Shelf Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowShelfGain", "Low Shelf Gain", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highShelfFreq", "High Shelf Freq",
        juce::NormalisableRange<float>(2000.0f, 16000.0f, 1.0f, 0.5f), 8000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highShelfGain", "High Shelf Gain", -12.0f, 12.0f, 0.0f));

    // Parametric bands
    const float defaultFreqs[] = { 200.0f, 800.0f, 2500.0f, 6000.0f };
    for (int i = 0; i < 4; ++i)
    {
        juce::String prefix = "band" + juce::String(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Freq", "Band " + juce::String(i + 1) + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), defaultFreqs[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Gain", "Band " + juce::String(i + 1) + " Gain", -12.0f, 12.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Q", "Band " + juce::String(i + 1) + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    }
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "eqBypass", "EQ Bypass", false));

    // Compressor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowMidXover", "Low-Mid Crossover",
        juce::NormalisableRange<float>(60.0f, 1000.0f, 1.0f, 0.5f), 200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "midHighXover", "Mid-High Crossover",
        juce::NormalisableRange<float>(1000.0f, 10000.0f, 1.0f, 0.5f), 3000.0f));

    const float defaultThresholds[] = { -20.0f, -18.0f, -16.0f };
    const float defaultRatios[] = { 3.0f, 4.0f, 4.0f };
    const float defaultAttacks[] = { 20.0f, 10.0f, 5.0f };
    const float defaultReleases[] = { 200.0f, 150.0f, 100.0f };

    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "comp" + juce::String(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Threshold", "Comp " + juce::String(i + 1) + " Threshold",
            -60.0f, 0.0f, defaultThresholds[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Ratio", "Comp " + juce::String(i + 1) + " Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), defaultRatios[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Attack", "Comp " + juce::String(i + 1) + " Attack",
            juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), defaultAttacks[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Release", "Comp " + juce::String(i + 1) + " Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.4f), defaultReleases[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "Makeup", "Comp " + juce::String(i + 1) + " Makeup",
            0.0f, 24.0f, 0.0f));
    }
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "compBypass", "Comp Bypass", false));

    // Stereo
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "globalWidth", "Global Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowWidth", "Low Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "midWidth", "Mid Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highWidth", "High Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "monoBassFreq", "Mono Bass Freq",
        juce::NormalisableRange<float>(60.0f, 300.0f, 1.0f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "monoBassEnabled", "Mono Bass", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "stereoBypass", "Stereo Bypass", false));

    // Limiter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Ceiling", -6.0f, 0.0f, -0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "limiterRelease", "Limiter Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.4f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "limiterBypass", "Limiter Bypass", false));

    return { params.begin(), params.end() };
}

void AutomasterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    masteringChain.prepare(sampleRate, samplesPerBlock);
    analysisEngine.prepare(sampleRate, samplesPerBlock);

    rulesEngine.setTargetLUFS(targetLUFSParam->load());
}

void AutomasterAudioProcessor::releaseResources()
{
    masteringChain.reset();
    analysisEngine.reset();
}

bool AutomasterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return true;
}

void AutomasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Update processing parameters from APVTS
    updateProcessingFromParameters();

    // Run analysis on input
    analysisEngine.process(buffer);

    // Apply mastering chain
    masteringChain.process(buffer);
}

void AutomasterAudioProcessor::updateProcessingFromParameters()
{
    // Global
    masteringChain.setInputGain(inputGainParam->load());
    masteringChain.setOutputGain(outputGainParam->load());

    // EQ
    auto& eq = masteringChain.getEQ();
    eq.setHPFFrequency(hpfFreqParam->load());
    eq.setHPFEnabled(hpfEnabledParam->load() > 0.5f);
    eq.setLPFFrequency(lpfFreqParam->load());
    eq.setLPFEnabled(lpfEnabledParam->load() > 0.5f);
    eq.setLowShelfFrequency(lowShelfFreqParam->load());
    eq.setLowShelfGain(lowShelfGainParam->load());
    eq.setHighShelfFrequency(highShelfFreqParam->load());
    eq.setHighShelfGain(highShelfGainParam->load());

    for (int i = 0; i < 4; ++i)
    {
        eq.setBandFrequency(i, bandFreqParams[i]->load());
        eq.setBandGain(i, bandGainParams[i]->load());
        eq.setBandQ(i, bandQParams[i]->load());
    }
    eq.setBypass(eqBypassParam->load() > 0.5f);

    // Compressor
    auto& comp = masteringChain.getCompressor();
    comp.setLowMidCrossover(lowMidXoverParam->load());
    comp.setMidHighCrossover(midHighXoverParam->load());

    for (int i = 0; i < 3; ++i)
    {
        comp.setBandThreshold(i, compThresholdParams[i]->load());
        comp.setBandRatio(i, compRatioParams[i]->load());
        comp.setBandAttack(i, compAttackParams[i]->load());
        comp.setBandRelease(i, compReleaseParams[i]->load());
        comp.setBandMakeup(i, compMakeupParams[i]->load());
    }
    comp.setBypass(compBypassParam->load() > 0.5f);

    // Stereo
    auto& stereo = masteringChain.getStereoImager();
    stereo.setGlobalWidth(globalWidthParam->load());
    stereo.setLowWidth(lowWidthParam->load());
    stereo.setMidWidth(midWidthParam->load());
    stereo.setHighWidth(highWidthParam->load());
    stereo.setMonoBassFrequency(monoBassFreqParam->load());
    stereo.setMonoBassEnabled(monoBassEnabledParam->load() > 0.5f);
    stereo.setBypass(stereoBypassParam->load() > 0.5f);

    // Limiter
    auto& limiter = masteringChain.getLimiter();
    limiter.setCeiling(ceilingParam->load());
    limiter.setRelease(limiterReleaseParam->load());
    limiter.setTargetLUFS(targetLUFSParam->load());
    limiter.setBypass(limiterBypassParam->load() > 0.5f);
}

bool AutomasterAudioProcessor::loadReferenceFile(const juce::File& file)
{
    ReferenceProfile newProfile;
    if (newProfile.loadFromFile(file))
    {
        newProfile.setName(file.getFileNameWithoutExtension().toStdString());
        currentReference = newProfile;
        analysisEngine.setReferenceProfile(currentReference);
        rulesEngine.setReferenceProfile(currentReference);
        rulesEngine.setMode(RulesEngine::Mode::Reference);
        return true;
    }
    return false;
}

void AutomasterAudioProcessor::clearReference()
{
    currentReference = ReferenceProfile();
    analysisEngine.clearReferenceProfile();
    rulesEngine.setMode(RulesEngine::Mode::Instant);
}

void AutomasterAudioProcessor::triggerAutoMaster()
{
    auto results = analysisEngine.getResults();
    rulesEngine.setTargetLUFS(targetLUFSParam->load());

    // Generate parameters
    auto params = rulesEngine.generateParameters(results);

    // Apply learning adjustments
    auto genre = rulesEngine.getGenre();
    params = learningSystem.applyLearning(params, genre, 0.5f);

    lastGeneratedParams = params;
    applyGeneratedParameters(params);
}

void AutomasterAudioProcessor::applyGeneratedParameters(const ParameterGenerator::GeneratedParameters& params)
{
    // Apply EQ
    if (auto* p = apvts.getParameter("lowShelfGain"))
        p->setValueNotifyingHost(p->convertTo0to1(params.eq.lowShelfGain));
    if (auto* p = apvts.getParameter("highShelfGain"))
        p->setValueNotifyingHost(p->convertTo0to1(params.eq.highShelfGain));

    for (int i = 0; i < 4; ++i)
    {
        if (auto* p = apvts.getParameter("band" + juce::String(i + 1) + "Gain"))
            p->setValueNotifyingHost(p->convertTo0to1(params.eq.bandGain[i]));
    }

    // Apply compressor
    for (int i = 0; i < 3; ++i)
    {
        if (auto* p = apvts.getParameter("comp" + juce::String(i + 1) + "Threshold"))
            p->setValueNotifyingHost(p->convertTo0to1(params.comp.threshold[i]));
        if (auto* p = apvts.getParameter("comp" + juce::String(i + 1) + "Ratio"))
            p->setValueNotifyingHost(p->convertTo0to1(params.comp.ratio[i]));
    }

    // Apply stereo
    if (auto* p = apvts.getParameter("globalWidth"))
        p->setValueNotifyingHost(p->convertTo0to1(params.stereo.globalWidth));
    if (auto* p = apvts.getParameter("monoBassEnabled"))
        p->setValueNotifyingHost(params.stereo.monoBassEnabled ? 1.0f : 0.0f);

    // Apply limiter auto-gain through output gain
    float currentLUFS = analysisEngine.getShortTermLUFS();
    float targetLUFS = targetLUFSParam->load();
    if (currentLUFS > -60.0f)
    {
        float autoGain = juce::jlimit(-12.0f, 12.0f, targetLUFS - currentLUFS);
        masteringChain.getLimiter().setAutoGainValue(autoGain);
        masteringChain.getLimiter().setAutoGainEnabled(true);
    }
}

void AutomasterAudioProcessor::recordUserAdjustment()
{
    // Capture current parameter state as user's preferred settings
    userCurrentParams.eq.lowShelfGain = lowShelfGainParam->load();
    userCurrentParams.eq.highShelfGain = highShelfGainParam->load();
    for (int i = 0; i < 4; ++i)
        userCurrentParams.eq.bandGain[i] = bandGainParams[i]->load();

    for (int i = 0; i < 3; ++i)
    {
        userCurrentParams.comp.threshold[i] = compThresholdParams[i]->load();
        userCurrentParams.comp.ratio[i] = compRatioParams[i]->load();
    }

    userCurrentParams.stereo.globalWidth = globalWidthParam->load();
    userCurrentParams.limiter.autoGain = outputGainParam->load();
    userCurrentParams.limiter.ceiling = ceilingParam->load();

    // Record difference for learning
    learningSystem.recordUserAdjustment(lastGeneratedParams, userCurrentParams, rulesEngine.getGenre());
}

void AutomasterAudioProcessor::storeState(int slot)
{
    if (slot >= 0 && slot < 4)
    {
        juce::MemoryOutputStream stream(comparisonStates[slot].parameterState, false);
        apvts.state.writeToStream(stream);
        comparisonStates[slot].isValid = true;
    }
}

void AutomasterAudioProcessor::recallState(int slot)
{
    if (slot >= 0 && slot < 4 && comparisonStates[slot].isValid)
    {
        juce::MemoryInputStream stream(comparisonStates[slot].parameterState, false);
        auto state = juce::ValueTree::readFromStream(stream);
        if (state.isValid())
            apvts.replaceState(state);
    }
}

void AutomasterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutomasterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* AutomasterAudioProcessor::createEditor()
{
    return new AutomasterAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutomasterAudioProcessor();
}
