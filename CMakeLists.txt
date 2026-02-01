cmake_minimum_required(VERSION 3.22)
project(Automaster VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(Automaster
    COMPANY_NAME "Ian Fletcher"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Iflt
    PLUGIN_CODE Auto
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Automaster"
)

target_sources(Automaster
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/AnalysisEngine.cpp
        Source/DSP/SpectralAnalyzer.cpp
        Source/DSP/DynamicsAnalyzer.cpp
        Source/DSP/StereoAnalyzer.cpp
        Source/DSP/ReferenceProfile.cpp
        Source/DSP/ParameterGenerator.cpp
        Source/DSP/MasteringChain.cpp
        Source/DSP/MasteringEQ.cpp
        Source/DSP/MultibandCompressor.cpp
        Source/DSP/StereoImager.cpp
        Source/DSP/Limiter.cpp
        Source/DSP/LoudnessMeter.cpp
        Source/AI/RulesEngine.cpp
        Source/AI/ONNXInference.cpp
        Source/AI/LearningSystem.cpp
        Source/AI/FeatureExtractor.cpp
        Source/UI/MeterComponents.cpp
        Source/UI/SpectrumAnalyzerUI.cpp
        Source/UI/ReferenceWaveform.cpp
        Source/UI/ProcessingChainView.cpp
)

target_include_directories(Automaster
    PRIVATE
        Source
        Source/DSP
        Source/AI
        Source/UI
)

target_compile_definitions(Automaster
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Automaster
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
