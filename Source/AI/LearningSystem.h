#pragma once

#include "../DSP/ParameterGenerator.h"
#include "../DSP/ReferenceProfile.h"
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <mutex>

class LearningSystem
{
public:
    // Stores user preference adjustments
    struct ParameterBias
    {
        // EQ biases
        float lowShelfBias = 0.0f;
        float highShelfBias = 0.0f;
        std::array<float, 4> bandGainBias = { 0.0f, 0.0f, 0.0f, 0.0f };

        // Compressor biases
        std::array<float, 3> thresholdBias = { 0.0f, 0.0f, 0.0f };
        std::array<float, 3> ratioBias = { 0.0f, 0.0f, 0.0f };

        // Stereo bias
        float widthBias = 0.0f;

        // Limiter biases
        float autoGainBias = 0.0f;
        float ceilingBias = 0.0f;

        int sampleCount = 0;  // Number of samples used to calculate bias
    };

    LearningSystem() = default;

    // Record difference between AI suggestion and user's final settings
    void recordUserAdjustment(
        const ParameterGenerator::GeneratedParameters& suggested,
        const ParameterGenerator::GeneratedParameters& userFinal,
        ReferenceProfile::Genre genre = ReferenceProfile::Genre::Auto)
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        // Calculate differences
        ParameterBias diff;

        // EQ differences
        diff.lowShelfBias = userFinal.eq.lowShelfGain - suggested.eq.lowShelfGain;
        diff.highShelfBias = userFinal.eq.highShelfGain - suggested.eq.highShelfGain;
        for (int i = 0; i < 4; ++i)
            diff.bandGainBias[i] = userFinal.eq.bandGain[i] - suggested.eq.bandGain[i];

        // Compressor differences
        for (int i = 0; i < 3; ++i)
        {
            diff.thresholdBias[i] = userFinal.comp.threshold[i] - suggested.comp.threshold[i];
            diff.ratioBias[i] = userFinal.comp.ratio[i] - suggested.comp.ratio[i];
        }

        // Stereo differences
        diff.widthBias = userFinal.stereo.globalWidth - suggested.stereo.globalWidth;

        // Limiter differences
        diff.autoGainBias = userFinal.limiter.autoGain - suggested.limiter.autoGain;
        diff.ceilingBias = userFinal.limiter.ceiling - suggested.limiter.ceiling;

        diff.sampleCount = 1;

        // Accumulate into global bias
        accumulateBias(globalBias, diff);

        // Accumulate into genre-specific bias
        if (genre != ReferenceProfile::Genre::Auto)
            accumulateBias(genreBiases[static_cast<int>(genre)], diff);

        // Mark as dirty for saving
        isDirty = true;
    }

    // Apply learned biases to generated parameters
    ParameterGenerator::GeneratedParameters applyLearning(
        const ParameterGenerator::GeneratedParameters& params,
        ReferenceProfile::Genre genre = ReferenceProfile::Genre::Auto,
        float learningStrength = 0.5f)
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (globalBias.sampleCount == 0)
            return params;

        ParameterGenerator::GeneratedParameters adjusted = params;

        // Get appropriate bias
        const ParameterBias& bias = (genre != ReferenceProfile::Genre::Auto &&
                                      genreBiases.count(static_cast<int>(genre)) > 0 &&
                                      genreBiases[static_cast<int>(genre)].sampleCount > 5)
            ? genreBiases[static_cast<int>(genre)]
            : globalBias;

        // Apply biases with learning strength
        adjusted.eq.lowShelfGain += bias.lowShelfBias * learningStrength;
        adjusted.eq.highShelfGain += bias.highShelfBias * learningStrength;

        for (int i = 0; i < 4; ++i)
            adjusted.eq.bandGain[i] += bias.bandGainBias[i] * learningStrength;

        for (int i = 0; i < 3; ++i)
        {
            adjusted.comp.threshold[i] += bias.thresholdBias[i] * learningStrength;
            adjusted.comp.ratio[i] += bias.ratioBias[i] * learningStrength;
        }

        adjusted.stereo.globalWidth += bias.widthBias * learningStrength;
        adjusted.limiter.autoGain += bias.autoGainBias * learningStrength;
        adjusted.limiter.ceiling += bias.ceilingBias * learningStrength;

        return adjusted;
    }

    // Save learning data to JSON
    bool saveToFile(const juce::File& file)
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        juce::DynamicObject::Ptr root = new juce::DynamicObject();

        // Save global bias
        root->setProperty("global", biasToJson(globalBias));

        // Save genre biases
        juce::DynamicObject::Ptr genreObj = new juce::DynamicObject();
        for (const auto& [genreIdx, bias] : genreBiases)
        {
            genreObj->setProperty(juce::String(genreIdx), biasToJson(bias));
        }
        root->setProperty("genres", genreObj.get());

        // Write to file
        juce::String json = juce::JSON::toString(root.get(), true);
        bool success = file.replaceWithText(json);

        if (success)
            isDirty = false;

        return success;
    }

    // Load learning data from JSON
    bool loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;

        std::lock_guard<std::mutex> lock(dataMutex);

        juce::String json = file.loadFileAsString();
        juce::var parsed = juce::JSON::parse(json);

        if (!parsed.isObject())
            return false;

        auto* root = parsed.getDynamicObject();

        // Load global bias
        if (root->hasProperty("global"))
            globalBias = jsonToBias(root->getProperty("global"));

        // Load genre biases
        if (root->hasProperty("genres"))
        {
            auto* genreObj = root->getProperty("genres").getDynamicObject();
            if (genreObj)
            {
                for (const auto& prop : genreObj->getProperties())
                {
                    int genreIdx = prop.name.toString().getIntValue();
                    genreBiases[genreIdx] = jsonToBias(prop.value);
                }
            }
        }

        isDirty = false;
        return true;
    }

    // Get default file path for learning data
    static juce::File getDefaultFilePath()
    {
        juce::File appData = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory);
        return appData.getChildFile("Automaster").getChildFile("learning.json");
    }

    // Clear all learned data
    void reset()
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        globalBias = ParameterBias();
        genreBiases.clear();
        isDirty = true;
    }

    // Check if there's unsaved data
    bool hasUnsavedChanges() const { return isDirty; }

    // Get sample count for UI display
    int getTotalSampleCount() const
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        return globalBias.sampleCount;
    }

    // Get bias summary for UI
    std::string getBiasSummary() const
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (globalBias.sampleCount == 0)
            return "No learning data yet.";

        std::string summary = "Learned from " + std::to_string(globalBias.sampleCount) + " sessions.\n";

        // Summarize significant biases
        if (std::abs(globalBias.lowShelfBias) > 0.5f)
            summary += "Low freq preference: " + std::to_string(globalBias.lowShelfBias) + " dB\n";

        if (std::abs(globalBias.highShelfBias) > 0.5f)
            summary += "High freq preference: " + std::to_string(globalBias.highShelfBias) + " dB\n";

        if (std::abs(globalBias.autoGainBias) > 1.0f)
            summary += "Loudness preference: " + std::to_string(globalBias.autoGainBias) + " dB\n";

        return summary;
    }

private:
    void accumulateBias(ParameterBias& accumulated, const ParameterBias& newSample)
    {
        // Exponential moving average for stability
        float weight = 1.0f / (accumulated.sampleCount + 1);
        float oldWeight = 1.0f - weight;

        accumulated.lowShelfBias = oldWeight * accumulated.lowShelfBias + weight * newSample.lowShelfBias;
        accumulated.highShelfBias = oldWeight * accumulated.highShelfBias + weight * newSample.highShelfBias;

        for (int i = 0; i < 4; ++i)
            accumulated.bandGainBias[i] = oldWeight * accumulated.bandGainBias[i] + weight * newSample.bandGainBias[i];

        for (int i = 0; i < 3; ++i)
        {
            accumulated.thresholdBias[i] = oldWeight * accumulated.thresholdBias[i] + weight * newSample.thresholdBias[i];
            accumulated.ratioBias[i] = oldWeight * accumulated.ratioBias[i] + weight * newSample.ratioBias[i];
        }

        accumulated.widthBias = oldWeight * accumulated.widthBias + weight * newSample.widthBias;
        accumulated.autoGainBias = oldWeight * accumulated.autoGainBias + weight * newSample.autoGainBias;
        accumulated.ceilingBias = oldWeight * accumulated.ceilingBias + weight * newSample.ceilingBias;

        accumulated.sampleCount++;
    }

    juce::var biasToJson(const ParameterBias& bias)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();

        obj->setProperty("lowShelf", bias.lowShelfBias);
        obj->setProperty("highShelf", bias.highShelfBias);

        juce::Array<juce::var> bandGains;
        for (float g : bias.bandGainBias)
            bandGains.add(g);
        obj->setProperty("bandGains", bandGains);

        juce::Array<juce::var> thresholds, ratios;
        for (int i = 0; i < 3; ++i)
        {
            thresholds.add(bias.thresholdBias[i]);
            ratios.add(bias.ratioBias[i]);
        }
        obj->setProperty("thresholds", thresholds);
        obj->setProperty("ratios", ratios);

        obj->setProperty("width", bias.widthBias);
        obj->setProperty("autoGain", bias.autoGainBias);
        obj->setProperty("ceiling", bias.ceilingBias);
        obj->setProperty("sampleCount", bias.sampleCount);

        return juce::var(obj.get());
    }

    ParameterBias jsonToBias(const juce::var& json)
    {
        ParameterBias bias;

        if (!json.isObject())
            return bias;

        auto* obj = json.getDynamicObject();

        bias.lowShelfBias = obj->getProperty("lowShelf");
        bias.highShelfBias = obj->getProperty("highShelf");

        auto* bandGains = obj->getProperty("bandGains").getArray();
        if (bandGains)
        {
            for (int i = 0; i < std::min(4, bandGains->size()); ++i)
                bias.bandGainBias[i] = (*bandGains)[i];
        }

        auto* thresholds = obj->getProperty("thresholds").getArray();
        auto* ratios = obj->getProperty("ratios").getArray();
        if (thresholds && ratios)
        {
            for (int i = 0; i < std::min(3, thresholds->size()); ++i)
            {
                bias.thresholdBias[i] = (*thresholds)[i];
                bias.ratioBias[i] = (*ratios)[i];
            }
        }

        bias.widthBias = obj->getProperty("width");
        bias.autoGainBias = obj->getProperty("autoGain");
        bias.ceilingBias = obj->getProperty("ceiling");
        bias.sampleCount = obj->getProperty("sampleCount");

        return bias;
    }

    mutable std::mutex dataMutex;

    ParameterBias globalBias;
    std::map<int, ParameterBias> genreBiases;  // keyed by genre enum

    bool isDirty = false;
};
