#pragma once

#include "DSPUtils.h"
#include "SpectralAnalyzer.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <string>
#include <memory>

class ReferenceProfile
{
public:
    static constexpr int NUM_BANDS = 32;

    // Genre presets
    enum class Genre
    {
        Auto,
        Pop,
        Rock,
        Electronic,
        HipHop,
        Jazz,
        Classical,
        Metal,
        RnB,
        Country,
        Custom
    };

    ReferenceProfile() = default;

    // Load reference from audio file
    bool loadFromFile(const juce::File& file)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (!reader)
            return false;

        // Read entire file (or first 60 seconds for long files)
        int maxSamples = static_cast<int>(std::min(reader->lengthInSamples,
                                                    static_cast<juce::int64>(reader->sampleRate * 60)));

        juce::AudioBuffer<float> buffer(reader->numChannels, maxSamples);
        reader->read(&buffer, 0, maxSamples, 0, true, true);

        return analyzeBuffer(buffer, reader->sampleRate);
    }

    // Analyze audio buffer to create profile
    bool analyzeBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        if (buffer.getNumSamples() < 1024)
            return false;

        profileSampleRate = sampleRate;
        profileDurationSeconds = buffer.getNumSamples() / sampleRate;

        // Set up analyzer
        SpectralAnalyzer analyzer;
        analyzer.prepare(sampleRate, 4096);

        // Analyze spectral content
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        if (numChannels >= 2)
        {
            analyzer.pushStereoSamples(buffer.getReadPointer(0),
                                        buffer.getReadPointer(1),
                                        numSamples);
        }
        else
        {
            analyzer.pushSamples(buffer.getReadPointer(0), numSamples);
        }

        // Store spectral envelope
        auto features = analyzer.getSpectralFeatures();
        spectralEnvelope = features.bandEnergies;
        spectralCentroid = features.centroid;
        spectralSlope = features.slope;
        spectralFlatness = features.flatness;

        // Calculate loudness (simple RMS-based for profile)
        float sumSquared = 0.0f;
        float peakValue = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* samples = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                sumSquared += samples[i] * samples[i];
                peakValue = std::max(peakValue, std::abs(samples[i]));
            }
        }

        float rms = std::sqrt(sumSquared / (numSamples * numChannels));
        loudnessRMS = DSPUtils::linearToDecibels(rms);
        peakLevel = DSPUtils::linearToDecibels(peakValue);
        crestFactor = peakLevel - loudnessRMS;

        // Calculate stereo characteristics
        if (numChannels >= 2)
        {
            float sumMid2 = 0.0f, sumSide2 = 0.0f;
            float sumL2 = 0.0f, sumR2 = 0.0f, sumLR = 0.0f;

            const float* left = buffer.getReadPointer(0);
            const float* right = buffer.getReadPointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                float mid = (left[i] + right[i]) * 0.5f;
                float side = (left[i] - right[i]) * 0.5f;
                sumMid2 += mid * mid;
                sumSide2 += side * side;
                sumL2 += left[i] * left[i];
                sumR2 += right[i] * right[i];
                sumLR += left[i] * right[i];
            }

            stereoWidth = sumMid2 > 1e-10f ? std::sqrt(sumSide2 / sumMid2) : 1.0f;
            float denom = std::sqrt(sumL2 * sumR2);
            stereoCorrelation = denom > 1e-10f ? sumLR / denom : 1.0f;
        }

        isValid = true;
        return true;
    }

    // Create preset profile for genre
    static ReferenceProfile createGenrePreset(Genre genre)
    {
        ReferenceProfile profile;
        profile.genre = genre;
        profile.isValid = true;

        // Default neutral profile
        profile.spectralEnvelope.fill(-30.0f);
        profile.loudnessRMS = -18.0f;
        profile.peakLevel = -1.0f;
        profile.crestFactor = 12.0f;
        profile.stereoWidth = 1.0f;
        profile.stereoCorrelation = 0.8f;
        profile.spectralCentroid = 2000.0f;
        profile.spectralSlope = -3.0f;

        switch (genre)
        {
            case Genre::Pop:
                // Bright, loud, wide
                profile.spectralSlope = -2.5f;
                profile.loudnessRMS = -14.0f;
                profile.crestFactor = 8.0f;
                profile.stereoWidth = 1.2f;
                // Boost presence (2-5kHz)
                for (int i = 16; i < 24; ++i)
                    profile.spectralEnvelope[i] += 3.0f;
                break;

            case Genre::Rock:
                // Punchy, mid-forward
                profile.spectralSlope = -3.0f;
                profile.loudnessRMS = -12.0f;
                profile.crestFactor = 10.0f;
                profile.stereoWidth = 1.1f;
                // Boost low-mids and presence
                for (int i = 8; i < 14; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                for (int i = 18; i < 22; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                break;

            case Genre::Electronic:
                // Sub-heavy, bright, very wide
                profile.spectralSlope = -2.0f;
                profile.loudnessRMS = -10.0f;
                profile.crestFactor = 6.0f;
                profile.stereoWidth = 1.4f;
                // Heavy sub bass
                for (int i = 0; i < 6; ++i)
                    profile.spectralEnvelope[i] += 4.0f;
                // Bright highs
                for (int i = 24; i < 32; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                break;

            case Genre::HipHop:
                // Sub-heavy, clear vocals, moderate width
                profile.spectralSlope = -2.5f;
                profile.loudnessRMS = -11.0f;
                profile.crestFactor = 7.0f;
                profile.stereoWidth = 1.0f;
                // Heavy sub
                for (int i = 0; i < 5; ++i)
                    profile.spectralEnvelope[i] += 5.0f;
                // Vocal presence
                for (int i = 14; i < 20; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                break;

            case Genre::Jazz:
                // Natural, dynamic, moderate width
                profile.spectralSlope = -4.0f;
                profile.loudnessRMS = -20.0f;
                profile.crestFactor = 16.0f;
                profile.stereoWidth = 1.0f;
                // Natural curve
                break;

            case Genre::Classical:
                // Very dynamic, natural, wide
                profile.spectralSlope = -4.5f;
                profile.loudnessRMS = -23.0f;
                profile.crestFactor = 20.0f;
                profile.stereoWidth = 1.3f;
                break;

            case Genre::Metal:
                // Aggressive, loud, mid-scooped
                profile.spectralSlope = -2.5f;
                profile.loudnessRMS = -10.0f;
                profile.crestFactor = 6.0f;
                profile.stereoWidth = 1.2f;
                // Mid scoop
                for (int i = 10; i < 16; ++i)
                    profile.spectralEnvelope[i] -= 3.0f;
                // Boost lows and highs
                for (int i = 0; i < 8; ++i)
                    profile.spectralEnvelope[i] += 3.0f;
                for (int i = 22; i < 32; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                break;

            case Genre::RnB:
                // Warm, smooth, sub-heavy
                profile.spectralSlope = -3.5f;
                profile.loudnessRMS = -13.0f;
                profile.crestFactor = 9.0f;
                profile.stereoWidth = 1.1f;
                // Warm low end
                for (int i = 2; i < 8; ++i)
                    profile.spectralEnvelope[i] += 3.0f;
                // Smooth highs (slight roll-off)
                for (int i = 26; i < 32; ++i)
                    profile.spectralEnvelope[i] -= 2.0f;
                break;

            case Genre::Country:
                // Natural, mid-forward, moderate dynamics
                profile.spectralSlope = -3.5f;
                profile.loudnessRMS = -14.0f;
                profile.crestFactor = 11.0f;
                profile.stereoWidth = 1.0f;
                // Vocal/guitar presence
                for (int i = 12; i < 20; ++i)
                    profile.spectralEnvelope[i] += 2.0f;
                break;

            default:
                break;
        }

        return profile;
    }

    // Compare current audio to reference and return match score (0-100)
    float calculateMatchScore(const std::array<float, NUM_BANDS>& currentSpectrum,
                              float currentLoudness,
                              float currentWidth,
                              float currentCorrelation) const
    {
        if (!isValid)
            return 0.0f;

        float score = 0.0f;

        // Spectral match (40% weight)
        float spectralDiff = 0.0f;
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            float diff = std::abs(currentSpectrum[i] - spectralEnvelope[i]);
            spectralDiff += std::min(diff, 12.0f);  // Cap at 12dB difference
        }
        float spectralScore = std::max(0.0f, 100.0f - (spectralDiff / NUM_BANDS) * 8.33f);
        score += spectralScore * 0.4f;

        // Loudness match (25% weight)
        float loudnessDiff = std::abs(currentLoudness - loudnessRMS);
        float loudnessScore = std::max(0.0f, 100.0f - loudnessDiff * 5.0f);
        score += loudnessScore * 0.25f;

        // Stereo width match (20% weight)
        float widthDiff = std::abs(currentWidth - stereoWidth);
        float widthScore = std::max(0.0f, 100.0f - widthDiff * 50.0f);
        score += widthScore * 0.2f;

        // Correlation match (15% weight)
        float corrDiff = std::abs(currentCorrelation - stereoCorrelation);
        float corrScore = std::max(0.0f, 100.0f - corrDiff * 100.0f);
        score += corrScore * 0.15f;

        return score;
    }

    // Getters
    bool isProfileValid() const { return isValid; }
    Genre getGenre() const { return genre; }
    const std::array<float, NUM_BANDS>& getSpectralEnvelope() const { return spectralEnvelope; }
    float getLoudnessRMS() const { return loudnessRMS; }
    float getPeakLevel() const { return peakLevel; }
    float getCrestFactor() const { return crestFactor; }
    float getStereoWidth() const { return stereoWidth; }
    float getStereoCorrelation() const { return stereoCorrelation; }
    float getSpectralCentroid() const { return spectralCentroid; }
    float getSpectralSlope() const { return spectralSlope; }
    float getDuration() const { return profileDurationSeconds; }
    std::string getName() const { return profileName; }

    void setName(const std::string& name) { profileName = name; }

private:
    bool isValid = false;
    Genre genre = Genre::Auto;
    std::string profileName = "Untitled";

    double profileSampleRate = 44100.0;
    float profileDurationSeconds = 0.0f;

    // Spectral characteristics
    std::array<float, NUM_BANDS> spectralEnvelope = {};
    float spectralCentroid = 0.0f;
    float spectralSlope = 0.0f;
    float spectralFlatness = 0.0f;

    // Loudness characteristics
    float loudnessRMS = -18.0f;
    float peakLevel = -1.0f;
    float crestFactor = 12.0f;

    // Stereo characteristics
    float stereoWidth = 1.0f;
    float stereoCorrelation = 0.8f;
};
