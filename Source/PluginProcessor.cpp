#include "PluginProcessor.h"
#include "PluginEditor.h"

static const char* percentTextFunction (const gin::Parameter&, float v)
{
    static char txt[32];
    snprintf(txt, 32, "%.0f%%", v * 100.0f);
    return txt;
}

AutomasterAudioProcessor::AutomasterAudioProcessor()
    : gin::Processor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true),
                     false,
                     gin::ProcessorOptions()
                     .withAdditionalCredits({"Ian Fletcher"})
                     .withoutUpdateChecker()
                     .withoutNewsChecker())
{

    // Global parameters
    inputGain = addExtParam("inputGain", "Input Gain", "In", "dB",
                            {-24.0f, 24.0f, 0.1f, 1.0f}, 0.0f,
                            gin::SmoothingType(0.02f));

    outputGain = addExtParam("outputGain", "Output Gain", "Out", "dB",
                             {-24.0f, 24.0f, 0.1f, 1.0f}, 0.0f,
                             gin::SmoothingType(0.02f));

    targetLUFS = addExtParam("targetLUFS", "Target LUFS", "Target", "LUFS",
                             {-24.0f, -6.0f, 0.5f, 1.0f}, -14.0f,
                             gin::SmoothingType(0.1f));

    autoMasterEnabled = addExtParam("autoMasterEnabled", "Auto Master", "Auto", "",
                                    {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                                    gin::SmoothingType(0.0f));

    // EQ Parameters
    hpfFreq = addExtParam("hpfFreq", "HPF Frequency", "HPF", "Hz",
                          {20.0f, 500.0f, 1.0f, 0.5f}, 30.0f,
                          gin::SmoothingType(0.05f));

    hpfEnabled = addExtParam("hpfEnabled", "HPF Enabled", "HPF On", "",
                             {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                             gin::SmoothingType(0.0f));

    lpfFreq = addExtParam("lpfFreq", "LPF Frequency", "LPF", "Hz",
                          {5000.0f, 20000.0f, 1.0f, 0.5f}, 18000.0f,
                          gin::SmoothingType(0.05f));

    lpfEnabled = addExtParam("lpfEnabled", "LPF Enabled", "LPF On", "",
                             {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                             gin::SmoothingType(0.0f));

    lowShelfFreq = addExtParam("lowShelfFreq", "Low Shelf Freq", "LoShelf", "Hz",
                               {20.0f, 500.0f, 1.0f, 0.5f}, 100.0f,
                               gin::SmoothingType(0.05f));

    lowShelfGain = addExtParam("lowShelfGain", "Low Shelf Gain", "LoShelf", "dB",
                               {-12.0f, 12.0f, 0.1f, 1.0f}, 0.0f,
                               gin::SmoothingType(0.02f));

    highShelfFreq = addExtParam("highShelfFreq", "High Shelf Freq", "HiShelf", "Hz",
                                {2000.0f, 16000.0f, 1.0f, 0.5f}, 8000.0f,
                                gin::SmoothingType(0.05f));

    highShelfGain = addExtParam("highShelfGain", "High Shelf Gain", "HiShelf", "dB",
                                {-12.0f, 12.0f, 0.1f, 1.0f}, 0.0f,
                                gin::SmoothingType(0.02f));

    // Parametric bands
    const float defaultFreqs[] = { 200.0f, 800.0f, 2500.0f, 6000.0f };
    for (int i = 0; i < 4; ++i)
    {
        juce::String prefix = "band" + juce::String(i + 1);
        juce::String name = "Band " + juce::String(i + 1);

        bandFreq[i] = addExtParam(prefix + "Freq", name + " Freq", "Freq", "Hz",
                                  {20.0f, 20000.0f, 1.0f, 0.3f}, defaultFreqs[i],
                                  gin::SmoothingType(0.05f));

        bandGain[i] = addExtParam(prefix + "Gain", name + " Gain", "Gain", "dB",
                                  {-12.0f, 12.0f, 0.1f, 1.0f}, 0.0f,
                                  gin::SmoothingType(0.02f));

        bandQ[i] = addExtParam(prefix + "Q", name + " Q", "Q", "",
                               {0.1f, 10.0f, 0.01f, 0.5f}, 1.0f,
                               gin::SmoothingType(0.05f));
    }

    eqBypass = addExtParam("eqBypass", "EQ Bypass", "EQ Byp", "",
                           {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                           gin::SmoothingType(0.0f));

    // Compressor parameters
    lowMidXover = addExtParam("lowMidXover", "Low-Mid Crossover", "Lo-Mid", "Hz",
                              {60.0f, 1000.0f, 1.0f, 0.5f}, 200.0f,
                              gin::SmoothingType(0.05f));

    midHighXover = addExtParam("midHighXover", "Mid-High Crossover", "Mid-Hi", "Hz",
                               {1000.0f, 10000.0f, 1.0f, 0.5f}, 3000.0f,
                               gin::SmoothingType(0.05f));

    const float defaultThresholds[] = { -20.0f, -18.0f, -16.0f };
    const float defaultRatios[] = { 3.0f, 4.0f, 4.0f };
    const float defaultAttacks[] = { 20.0f, 10.0f, 5.0f };
    const float defaultReleases[] = { 200.0f, 150.0f, 100.0f };
    const char* bandNames[] = { "Low", "Mid", "High" };

    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "comp" + juce::String(i + 1);
        juce::String name = juce::String(bandNames[i]) + " Comp";

        compThreshold[i] = addExtParam(prefix + "Threshold", name + " Threshold", "Thresh", "dB",
                                       {-60.0f, 0.0f, 0.1f, 1.0f}, defaultThresholds[i],
                                       gin::SmoothingType(0.02f));

        compRatio[i] = addExtParam(prefix + "Ratio", name + " Ratio", "Ratio", ":1",
                                   {1.0f, 20.0f, 0.1f, 0.5f}, defaultRatios[i],
                                   gin::SmoothingType(0.02f));

        compAttack[i] = addExtParam(prefix + "Attack", name + " Attack", "Atk", "ms",
                                    {0.1f, 100.0f, 0.1f, 0.4f}, defaultAttacks[i],
                                    gin::SmoothingType(0.05f));

        compRelease[i] = addExtParam(prefix + "Release", name + " Release", "Rel", "ms",
                                     {10.0f, 1000.0f, 1.0f, 0.4f}, defaultReleases[i],
                                     gin::SmoothingType(0.05f));

        compMakeup[i] = addExtParam(prefix + "Makeup", name + " Makeup", "Makeup", "dB",
                                    {0.0f, 24.0f, 0.1f, 1.0f}, 0.0f,
                                    gin::SmoothingType(0.02f));
    }

    compBypass = addExtParam("compBypass", "Comp Bypass", "Comp Byp", "",
                             {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                             gin::SmoothingType(0.0f));

    // Stereo parameters
    globalWidth = addExtParam("globalWidth", "Global Width", "Width", "",
                              {0.0f, 2.0f, 0.01f, 1.0f}, 1.0f,
                              gin::SmoothingType(0.02f));

    lowWidth = addExtParam("lowWidth", "Low Width", "Lo W", "",
                           {0.0f, 2.0f, 0.01f, 1.0f}, 1.0f,
                           gin::SmoothingType(0.02f));

    midWidth = addExtParam("midWidth", "Mid Width", "Mid W", "",
                           {0.0f, 2.0f, 0.01f, 1.0f}, 1.0f,
                           gin::SmoothingType(0.02f));

    highWidth = addExtParam("highWidth", "High Width", "Hi W", "",
                            {0.0f, 2.0f, 0.01f, 1.0f}, 1.0f,
                            gin::SmoothingType(0.02f));

    monoBassFreq = addExtParam("monoBassFreq", "Mono Bass Freq", "Mono", "Hz",
                               {60.0f, 300.0f, 1.0f, 1.0f}, 120.0f,
                               gin::SmoothingType(0.05f));

    monoBassEnabled = addExtParam("monoBassEnabled", "Mono Bass", "Mono On", "",
                                  {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                                  gin::SmoothingType(0.0f));

    stereoBypass = addExtParam("stereoBypass", "Stereo Bypass", "St Byp", "",
                               {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                               gin::SmoothingType(0.0f));

    // Limiter parameters
    ceiling = addExtParam("ceiling", "Ceiling", "Ceil", "dB",
                          {-6.0f, 0.0f, 0.1f, 1.0f}, -0.3f,
                          gin::SmoothingType(0.02f));

    limiterRelease = addExtParam("limiterRelease", "Limiter Release", "Rel", "ms",
                                 {10.0f, 1000.0f, 1.0f, 0.4f}, 100.0f,
                                 gin::SmoothingType(0.05f));

    limiterBypass = addExtParam("limiterBypass", "Limiter Bypass", "Lim Byp", "",
                                {0.0f, 1.0f, 1.0f, 1.0f}, 0.0f,
                                gin::SmoothingType(0.0f));

    // Initialize Gin
    init();

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

void AutomasterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    gin::Processor::prepareToPlay(sampleRate, samplesPerBlock);

    masteringChain.prepare(sampleRate, samplesPerBlock);
    analysisEngine.prepare(sampleRate, samplesPerBlock);
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

    // Update processing parameters from Gin parameters
    updateProcessingFromParameters();

    // Run analysis on input
    analysisEngine.process(buffer);

    // Apply mastering chain
    masteringChain.process(buffer);
}

void AutomasterAudioProcessor::updateProcessingFromParameters()
{
    // Global
    masteringChain.setInputGain(inputGain->getProcValue());
    masteringChain.setOutputGain(outputGain->getProcValue());

    // EQ
    auto& eq = masteringChain.getEQ();
    eq.setHPFFrequency(hpfFreq->getProcValue());
    eq.setHPFEnabled(hpfEnabled->isOn());
    eq.setLPFFrequency(lpfFreq->getProcValue());
    eq.setLPFEnabled(lpfEnabled->isOn());
    eq.setLowShelfFrequency(lowShelfFreq->getProcValue());
    eq.setLowShelfGain(lowShelfGain->getProcValue());
    eq.setHighShelfFrequency(highShelfFreq->getProcValue());
    eq.setHighShelfGain(highShelfGain->getProcValue());

    for (int i = 0; i < 4; ++i)
    {
        eq.setBandFrequency(i, bandFreq[i]->getProcValue());
        eq.setBandGain(i, bandGain[i]->getProcValue());
        eq.setBandQ(i, bandQ[i]->getProcValue());
    }
    eq.setBypass(eqBypass->isOn());

    // Compressor
    auto& comp = masteringChain.getCompressor();
    comp.setLowMidCrossover(lowMidXover->getProcValue());
    comp.setMidHighCrossover(midHighXover->getProcValue());

    for (int i = 0; i < 3; ++i)
    {
        comp.setBandThreshold(i, compThreshold[i]->getProcValue());
        comp.setBandRatio(i, compRatio[i]->getProcValue());
        comp.setBandAttack(i, compAttack[i]->getProcValue());
        comp.setBandRelease(i, compRelease[i]->getProcValue());
        comp.setBandMakeup(i, compMakeup[i]->getProcValue());
    }
    comp.setBypass(compBypass->isOn());

    // Stereo
    auto& stereo = masteringChain.getStereoImager();
    stereo.setGlobalWidth(globalWidth->getProcValue());
    stereo.setLowWidth(lowWidth->getProcValue());
    stereo.setMidWidth(midWidth->getProcValue());
    stereo.setHighWidth(highWidth->getProcValue());
    stereo.setMonoBassFrequency(monoBassFreq->getProcValue());
    stereo.setMonoBassEnabled(monoBassEnabled->isOn());
    stereo.setBypass(stereoBypass->isOn());

    // Limiter
    auto& limiter = masteringChain.getLimiter();
    limiter.setCeiling(ceiling->getProcValue());
    limiter.setRelease(limiterRelease->getProcValue());
    limiter.setTargetLUFS(targetLUFS->getProcValue());
    limiter.setBypass(limiterBypass->isOn());
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
    rulesEngine.setTargetLUFS(targetLUFS->getProcValue());

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
    // Apply EQ - using setUserValueNotifingHost for Gin parameters
    lowShelfGain->setUserValueNotifingHost(params.eq.lowShelfGain);
    highShelfGain->setUserValueNotifingHost(params.eq.highShelfGain);

    for (int i = 0; i < 4; ++i)
        bandGain[i]->setUserValueNotifingHost(params.eq.bandGain[i]);

    // Apply compressor
    for (int i = 0; i < 3; ++i)
    {
        compThreshold[i]->setUserValueNotifingHost(params.comp.threshold[i]);
        compRatio[i]->setUserValueNotifingHost(params.comp.ratio[i]);
    }

    // Apply stereo
    globalWidth->setUserValueNotifingHost(params.stereo.globalWidth);
    monoBassEnabled->setUserValueNotifingHost(params.stereo.monoBassEnabled ? 1.0f : 0.0f);

    // Apply limiter auto-gain through output gain
    float currentLUFS = analysisEngine.getShortTermLUFS();
    float target = targetLUFS->getProcValue();
    if (currentLUFS > -60.0f)
    {
        float autoGain = juce::jlimit(-12.0f, 12.0f, target - currentLUFS);
        masteringChain.getLimiter().setAutoGainValue(autoGain);
        masteringChain.getLimiter().setAutoGainEnabled(true);
    }
}

void AutomasterAudioProcessor::recordUserAdjustment()
{
    // Capture current parameter state as user's preferred settings
    userCurrentParams.eq.lowShelfGain = lowShelfGain->getProcValue();
    userCurrentParams.eq.highShelfGain = highShelfGain->getProcValue();
    for (int i = 0; i < 4; ++i)
        userCurrentParams.eq.bandGain[i] = bandGain[i]->getProcValue();

    for (int i = 0; i < 3; ++i)
    {
        userCurrentParams.comp.threshold[i] = compThreshold[i]->getProcValue();
        userCurrentParams.comp.ratio[i] = compRatio[i]->getProcValue();
    }

    userCurrentParams.stereo.globalWidth = globalWidth->getProcValue();
    userCurrentParams.limiter.autoGain = outputGain->getProcValue();
    userCurrentParams.limiter.ceiling = ceiling->getProcValue();

    // Record difference for learning
    learningSystem.recordUserAdjustment(lastGeneratedParams, userCurrentParams, rulesEngine.getGenre());
}

void AutomasterAudioProcessor::storeState(int slot)
{
    if (slot >= 0 && slot < 4)
    {
        juce::MemoryOutputStream stream(comparisonStates[slot].parameterState, false);
        state.writeToStream(stream);
        comparisonStates[slot].isValid = true;
    }
}

void AutomasterAudioProcessor::recallState(int slot)
{
    if (slot >= 0 && slot < 4 && comparisonStates[slot].isValid)
    {
        juce::MemoryInputStream stream(comparisonStates[slot].parameterState, false);
        auto recalled = juce::ValueTree::readFromStream(stream);
        if (recalled.isValid())
        {
            state.copyPropertiesAndChildrenFrom(recalled, nullptr);
            // Trigger state update
            stateUpdated();
        }
    }
}

juce::AudioProcessorEditor* AutomasterAudioProcessor::createEditor()
{
    return new AutomasterAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutomasterAudioProcessor();
}
