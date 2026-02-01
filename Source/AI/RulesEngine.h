#pragma once

#include "../DSP/AnalysisEngine.h"
#include "../DSP/ParameterGenerator.h"
#include "../DSP/ReferenceProfile.h"
#include <array>
#include <string>

class RulesEngine
{
public:
    // Mastering mode
    enum class Mode
    {
        Instant,    // Generate from analysis only
        Reference,  // Match loaded reference
        Genre       // Match genre preset
    };

    RulesEngine() = default;

    // Set operating mode
    void setMode(Mode newMode) { mode = newMode; }
    Mode getMode() const { return mode; }

    // Set target loudness
    void setTargetLUFS(float lufs)
    {
        targetLUFS = juce::jlimit(-24.0f, -6.0f, lufs);
    }
    float getTargetLUFS() const { return targetLUFS; }

    // Set genre for genre mode
    void setGenre(ReferenceProfile::Genre newGenre)
    {
        genre = newGenre;
        if (mode == Mode::Genre)
            genreProfile = ReferenceProfile::createGenrePreset(genre);
    }
    ReferenceProfile::Genre getGenre() const { return genre; }

    // Set reference profile for reference mode
    void setReferenceProfile(const ReferenceProfile& profile)
    {
        referenceProfile = profile;
    }

    // Generate processing parameters based on current mode
    ParameterGenerator::GeneratedParameters generateParameters(
        const AnalysisEngine::AnalysisResults& analysis)
    {
        ParameterGenerator::GeneratedParameters params;

        switch (mode)
        {
            case Mode::Instant:
                params = generateInstantParameters(analysis);
                break;

            case Mode::Reference:
                params = generateReferenceParameters(analysis);
                break;

            case Mode::Genre:
                params = generateGenreParameters(analysis);
                break;
        }

        // Apply safety limits
        applySafetyLimits(params);

        // Apply genre-specific adjustments even in Instant mode
        if (mode == Mode::Instant && genre != ReferenceProfile::Genre::Auto)
            applyGenreHints(params, genre);

        return params;
    }

    // Auto-detect genre from analysis
    ReferenceProfile::Genre detectGenre(const AnalysisEngine::AnalysisResults& analysis)
    {
        // Simple genre detection based on spectral and dynamics features
        float subEnergy = (analysis.bandEnergies[0] + analysis.bandEnergies[1] +
                          analysis.bandEnergies[2]) / 3.0f;
        float avgEnergy = 0.0f;
        for (float e : analysis.bandEnergies)
            avgEnergy += e;
        avgEnergy /= 32.0f;

        float subRatio = subEnergy - avgEnergy;
        float crest = (analysis.dynamics.crestFactors[0] +
                      analysis.dynamics.crestFactors[1] +
                      analysis.dynamics.crestFactors[2]) / 3.0f;
        float width = analysis.stereo.width;

        // Electronic: heavy sub, low crest, wide stereo
        if (subRatio > 3.0f && crest < 8.0f && width > 1.2f)
            return ReferenceProfile::Genre::Electronic;

        // Hip-Hop: heavy sub, moderate crest, narrower
        if (subRatio > 2.0f && crest < 10.0f && width < 1.1f)
            return ReferenceProfile::Genre::HipHop;

        // Classical: high crest factor, wide
        if (crest > 16.0f && width > 1.1f)
            return ReferenceProfile::Genre::Classical;

        // Jazz: high crest factor, moderate width
        if (crest > 14.0f)
            return ReferenceProfile::Genre::Jazz;

        // Metal: low crest, mid-scooped
        float midEnergy = (analysis.bandEnergies[12] + analysis.bandEnergies[13] +
                          analysis.bandEnergies[14]) / 3.0f;
        if (crest < 8.0f && midEnergy < avgEnergy - 2.0f)
            return ReferenceProfile::Genre::Metal;

        // Rock: moderate crest, balanced
        if (crest > 9.0f && crest < 14.0f)
            return ReferenceProfile::Genre::Rock;

        // Default to Pop
        return ReferenceProfile::Genre::Pop;
    }

    // Get description of current rules being applied
    std::string getRulesDescription() const
    {
        std::string desc;

        switch (mode)
        {
            case Mode::Instant:
                desc = "Instant Master: Analyzing spectral balance, dynamics, and stereo field.";
                break;
            case Mode::Reference:
                desc = "Reference Match: Matching to loaded reference track.";
                break;
            case Mode::Genre:
                desc = "Genre Match: Matching to " + getGenreName(genre) + " profile.";
                break;
        }

        desc += " Target: " + std::to_string(static_cast<int>(targetLUFS)) + " LUFS.";

        return desc;
    }

    static std::string getGenreName(ReferenceProfile::Genre g)
    {
        switch (g)
        {
            case ReferenceProfile::Genre::Auto:       return "Auto";
            case ReferenceProfile::Genre::Pop:        return "Pop";
            case ReferenceProfile::Genre::Rock:       return "Rock";
            case ReferenceProfile::Genre::Electronic: return "Electronic";
            case ReferenceProfile::Genre::HipHop:     return "Hip-Hop";
            case ReferenceProfile::Genre::Jazz:       return "Jazz";
            case ReferenceProfile::Genre::Classical:  return "Classical";
            case ReferenceProfile::Genre::Metal:      return "Metal";
            case ReferenceProfile::Genre::RnB:        return "R&B";
            case ReferenceProfile::Genre::Country:    return "Country";
            case ReferenceProfile::Genre::Custom:     return "Custom";
            default:                                  return "Unknown";
        }
    }

private:
    ParameterGenerator::GeneratedParameters generateInstantParameters(
        const AnalysisEngine::AnalysisResults& analysis)
    {
        return parameterGenerator.generateFromAnalysis(analysis, targetLUFS);
    }

    ParameterGenerator::GeneratedParameters generateReferenceParameters(
        const AnalysisEngine::AnalysisResults& analysis)
    {
        if (!referenceProfile.isProfileValid())
            return generateInstantParameters(analysis);

        return parameterGenerator.generateFromReference(analysis, referenceProfile, 1.0f);
    }

    ParameterGenerator::GeneratedParameters generateGenreParameters(
        const AnalysisEngine::AnalysisResults& analysis)
    {
        return parameterGenerator.generateFromReference(analysis, genreProfile, 0.7f);
    }

    void applySafetyLimits(ParameterGenerator::GeneratedParameters& params)
    {
        // EQ safety: max ±6dB boost, ±9dB cut
        params.eq.lowShelfGain = juce::jlimit(-9.0f, 6.0f, params.eq.lowShelfGain);
        params.eq.highShelfGain = juce::jlimit(-9.0f, 6.0f, params.eq.highShelfGain);

        for (int i = 0; i < 4; ++i)
            params.eq.bandGain[i] = juce::jlimit(-9.0f, 6.0f, params.eq.bandGain[i]);

        // Compressor safety
        for (int i = 0; i < 3; ++i)
        {
            params.comp.threshold[i] = juce::jlimit(-40.0f, 0.0f, params.comp.threshold[i]);
            params.comp.ratio[i] = juce::jlimit(1.0f, 10.0f, params.comp.ratio[i]);
            params.comp.attack[i] = juce::jlimit(0.1f, 100.0f, params.comp.attack[i]);
            params.comp.release[i] = juce::jlimit(10.0f, 1000.0f, params.comp.release[i]);
        }

        // Stereo safety
        params.stereo.globalWidth = juce::jlimit(0.5f, 1.5f, params.stereo.globalWidth);

        // Limiter safety
        params.limiter.ceiling = juce::jlimit(-3.0f, 0.0f, params.limiter.ceiling);
        params.limiter.autoGain = juce::jlimit(-12.0f, 12.0f, params.limiter.autoGain);
    }

    void applyGenreHints(ParameterGenerator::GeneratedParameters& params,
                         ReferenceProfile::Genre g)
    {
        // Subtle genre-specific adjustments
        switch (g)
        {
            case ReferenceProfile::Genre::Electronic:
                // More limiting, wider
                params.limiter.autoGain += 2.0f;
                params.stereo.globalWidth *= 1.1f;
                break;

            case ReferenceProfile::Genre::HipHop:
                // Heavy sub, tighter
                params.eq.lowShelfGain += 1.0f;
                params.stereo.monoBassEnabled = true;
                break;

            case ReferenceProfile::Genre::Jazz:
            case ReferenceProfile::Genre::Classical:
                // Less compression, more dynamics
                for (int i = 0; i < 3; ++i)
                    params.comp.threshold[i] += 4.0f;
                params.limiter.autoGain -= 3.0f;
                break;

            case ReferenceProfile::Genre::Metal:
                // More aggressive compression
                for (int i = 0; i < 3; ++i)
                {
                    params.comp.threshold[i] -= 2.0f;
                    params.comp.ratio[i] += 1.0f;
                }
                break;

            default:
                break;
        }

        // Re-apply safety after genre hints
        applySafetyLimits(params);
    }

    Mode mode = Mode::Instant;
    float targetLUFS = -14.0f;
    ReferenceProfile::Genre genre = ReferenceProfile::Genre::Auto;

    ReferenceProfile referenceProfile;
    ReferenceProfile genreProfile;

    ParameterGenerator parameterGenerator;
};
