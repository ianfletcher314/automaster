#pragma once

#include "../DSP/AnalysisEngine.h"
#include <array>
#include <vector>

class FeatureExtractor
{
public:
    // Feature vector size for ML model input
    static constexpr int NUM_FEATURES = 50;

    struct FeatureVector
    {
        std::array<float, NUM_FEATURES> features;
        std::array<std::string, NUM_FEATURES> featureNames;
        bool isValid = false;
    };

    FeatureExtractor() = default;

    // Extract normalized feature vector from analysis results
    FeatureVector extract(const AnalysisEngine::AnalysisResults& analysis)
    {
        FeatureVector vec;
        int idx = 0;

        // Spectral features (16 features)
        // Octave band energies (8 bands, averaged from 32)
        for (int octave = 0; octave < 8; ++octave)
        {
            float sum = 0.0f;
            for (int band = octave * 4; band < (octave + 1) * 4; ++band)
                sum += analysis.bandEnergies[band];
            vec.features[idx] = normalizeDB(sum / 4.0f, -60.0f, 0.0f);
            vec.featureNames[idx] = "octave_" + std::to_string(octave);
            idx++;
        }

        // Spectral centroid (normalized to 0-1 where 0=low, 1=bright)
        vec.features[idx] = normalize(analysis.spectral.centroid, 500.0f, 8000.0f);
        vec.featureNames[idx++] = "spectral_centroid";

        // Spectral spread
        vec.features[idx] = normalize(analysis.spectral.spread, 500.0f, 5000.0f);
        vec.featureNames[idx++] = "spectral_spread";

        // Spectral flatness (0=tonal, 1=noisy)
        vec.features[idx] = analysis.spectral.flatness;
        vec.featureNames[idx++] = "spectral_flatness";

        // Spectral slope
        vec.features[idx] = normalize(analysis.spectral.slope, -8.0f, 0.0f);
        vec.featureNames[idx++] = "spectral_slope";

        // Spectral rolloff
        vec.features[idx] = normalize(analysis.spectral.rolloff, 2000.0f, 16000.0f);
        vec.featureNames[idx++] = "spectral_rolloff";

        // Spectral balance ratios (3 features)
        float totalEnergy = 0.0f;
        for (float e : analysis.bandEnergies)
            totalEnergy += std::max(0.0f, e + 60.0f);  // Shift to positive

        float lowEnergy = 0.0f, midEnergy = 0.0f, highEnergy = 0.0f;
        for (int i = 0; i < 10; ++i)
            lowEnergy += std::max(0.0f, analysis.bandEnergies[i] + 60.0f);
        for (int i = 10; i < 22; ++i)
            midEnergy += std::max(0.0f, analysis.bandEnergies[i] + 60.0f);
        for (int i = 22; i < 32; ++i)
            highEnergy += std::max(0.0f, analysis.bandEnergies[i] + 60.0f);

        if (totalEnergy > 0.0f)
        {
            vec.features[idx] = lowEnergy / totalEnergy;
            vec.featureNames[idx++] = "low_ratio";
            vec.features[idx] = midEnergy / totalEnergy;
            vec.featureNames[idx++] = "mid_ratio";
            vec.features[idx] = highEnergy / totalEnergy;
            vec.featureNames[idx++] = "high_ratio";
        }
        else
        {
            vec.features[idx++] = 0.33f;
            vec.features[idx++] = 0.34f;
            vec.features[idx++] = 0.33f;
        }

        // Dynamics features (8 features)
        // Crest factors per band
        for (int band = 0; band < 3; ++band)
        {
            vec.features[idx] = normalize(analysis.dynamics.crestFactors[band], 3.0f, 20.0f);
            vec.featureNames[idx++] = "crest_band_" + std::to_string(band);
        }

        // Transient density
        vec.features[idx] = normalize(analysis.dynamics.transientDensity, 0.0f, 100.0f);
        vec.featureNames[idx++] = "transient_density";

        // Dynamic range
        vec.features[idx] = normalize(analysis.dynamics.dynamicRange, 6.0f, 30.0f);
        vec.featureNames[idx++] = "dynamic_range";

        // Average crest factor
        float avgCrest = (analysis.dynamics.crestFactors[0] +
                          analysis.dynamics.crestFactors[1] +
                          analysis.dynamics.crestFactors[2]) / 3.0f;
        vec.features[idx] = normalize(avgCrest, 3.0f, 20.0f);
        vec.featureNames[idx++] = "avg_crest";

        // Crest factor variance (indicator of frequency-dependent dynamics)
        float crestVar = 0.0f;
        for (int i = 0; i < 3; ++i)
            crestVar += std::pow(analysis.dynamics.crestFactors[i] - avgCrest, 2.0f);
        vec.features[idx] = normalize(std::sqrt(crestVar / 3.0f), 0.0f, 5.0f);
        vec.featureNames[idx++] = "crest_variance";

        // Loudness features (6 features)
        vec.features[idx] = normalizeDB(analysis.momentaryLUFS, -40.0f, 0.0f);
        vec.featureNames[idx++] = "momentary_lufs";

        vec.features[idx] = normalizeDB(analysis.shortTermLUFS, -40.0f, 0.0f);
        vec.featureNames[idx++] = "short_term_lufs";

        vec.features[idx] = normalizeDB(analysis.integratedLUFS, -40.0f, 0.0f);
        vec.featureNames[idx++] = "integrated_lufs";

        vec.features[idx] = normalizeDB(analysis.truePeak, -20.0f, 0.0f);
        vec.featureNames[idx++] = "true_peak";

        vec.features[idx] = normalize(analysis.loudnessRange, 0.0f, 20.0f);
        vec.featureNames[idx++] = "loudness_range";

        // Headroom
        float headroom = analysis.truePeak - analysis.shortTermLUFS;
        vec.features[idx] = normalize(headroom, 0.0f, 20.0f);
        vec.featureNames[idx++] = "headroom";

        // Stereo features (10 features)
        vec.features[idx] = normalize(analysis.stereo.correlation, -1.0f, 1.0f);
        vec.featureNames[idx++] = "correlation";

        vec.features[idx] = normalize(analysis.stereo.width, 0.0f, 2.0f);
        vec.featureNames[idx++] = "width";

        vec.features[idx] = normalize(analysis.stereo.balance, -1.0f, 1.0f);
        vec.featureNames[idx++] = "balance";

        // Per-band correlation
        for (int band = 0; band < 3; ++band)
        {
            vec.features[idx] = normalize(analysis.stereo.bandCorrelation[band], -1.0f, 1.0f);
            vec.featureNames[idx++] = "corr_band_" + std::to_string(band);
        }

        // Per-band width
        for (int band = 0; band < 3; ++band)
        {
            vec.features[idx] = normalize(analysis.stereo.bandWidth[band], 0.0f, 2.0f);
            vec.featureNames[idx++] = "width_band_" + std::to_string(band);
        }

        // Reference match (1 feature)
        vec.features[idx] = analysis.hasReference ?
            normalize(analysis.referenceMatchScore, 0.0f, 100.0f) : 0.5f;
        vec.featureNames[idx++] = "reference_match";

        // Fill remaining with zeros if any
        while (idx < NUM_FEATURES)
        {
            vec.features[idx] = 0.0f;
            vec.featureNames[idx] = "reserved_" + std::to_string(idx);
            idx++;
        }

        vec.isValid = true;
        return vec;
    }

    // Get raw feature values (unnormalized) for display
    std::vector<std::pair<std::string, float>> getRawFeatures(
        const AnalysisEngine::AnalysisResults& analysis)
    {
        std::vector<std::pair<std::string, float>> features;

        features.emplace_back("Spectral Centroid (Hz)", analysis.spectral.centroid);
        features.emplace_back("Spectral Slope (dB/oct)", analysis.spectral.slope);
        features.emplace_back("Spectral Flatness", analysis.spectral.flatness);

        features.emplace_back("Crest Factor Low", analysis.dynamics.crestFactors[0]);
        features.emplace_back("Crest Factor Mid", analysis.dynamics.crestFactors[1]);
        features.emplace_back("Crest Factor High", analysis.dynamics.crestFactors[2]);
        features.emplace_back("Transient Density", analysis.dynamics.transientDensity);
        features.emplace_back("Dynamic Range", analysis.dynamics.dynamicRange);

        features.emplace_back("LUFS Momentary", analysis.momentaryLUFS);
        features.emplace_back("LUFS Short-term", analysis.shortTermLUFS);
        features.emplace_back("LUFS Integrated", analysis.integratedLUFS);
        features.emplace_back("True Peak", analysis.truePeak);

        features.emplace_back("Stereo Correlation", analysis.stereo.correlation);
        features.emplace_back("Stereo Width", analysis.stereo.width);
        features.emplace_back("Balance", analysis.stereo.balance);

        if (analysis.hasReference)
            features.emplace_back("Reference Match %", analysis.referenceMatchScore);

        return features;
    }

private:
    float normalize(float value, float min, float max)
    {
        return juce::jlimit(0.0f, 1.0f, (value - min) / (max - min));
    }

    float normalizeDB(float db, float minDB, float maxDB)
    {
        if (db < minDB - 10.0f)  // Below noise floor
            return 0.0f;
        return normalize(db, minDB, maxDB);
    }
};
