#pragma once

#include "LookAndFeel.h"
#include "../DSP/ReferenceProfile.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class ReferenceWaveform : public juce::Component
{
public:
    ReferenceWaveform()
    {
        formatManager.registerBasicFormats();
    }

    void loadFile(const juce::File& file)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader != nullptr)
        {
            // Read audio for waveform display
            int maxSamples = static_cast<int>(std::min(reader->lengthInSamples,
                                                        static_cast<juce::int64>(reader->sampleRate * 30)));

            juce::AudioBuffer<float> buffer(reader->numChannels, maxSamples);
            reader->read(&buffer, 0, maxSamples, 0, true, true);

            // Generate waveform thumbnail
            generateWaveform(buffer);

            fileName = file.getFileName();
            duration = reader->lengthInSamples / reader->sampleRate;
            hasFile = true;
        }
        else
        {
            hasFile = false;
        }

        repaint();
    }

    void clear()
    {
        waveformPoints.clear();
        hasFile = false;
        fileName = "";
        duration = 0.0;
        repaint();
    }

    void setProfile(const ReferenceProfile* profile)
    {
        referenceProfile = profile;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (!hasFile)
        {
            // Empty state
            g.setColour(AutomasterColors::textMuted);
            g.setFont(13.0f);
            g.drawText("Drop reference track here or click Load",
                       bounds, juce::Justification::centred);
            return;
        }

        // Draw waveform
        auto waveformBounds = bounds.reduced(5.0f, 20.0f);

        // Waveform fill
        g.setColour(AutomasterColors::primary.withAlpha(0.3f));
        juce::Path waveformPath;

        if (!waveformPoints.empty())
        {
            float xStep = waveformBounds.getWidth() / (waveformPoints.size() - 1);
            float centerY = waveformBounds.getCentreY();

            // Upper half
            waveformPath.startNewSubPath(waveformBounds.getX(), centerY);
            for (size_t i = 0; i < waveformPoints.size(); ++i)
            {
                float x = waveformBounds.getX() + i * xStep;
                float y = centerY - waveformPoints[i] * waveformBounds.getHeight() * 0.5f;
                waveformPath.lineTo(x, y);
            }

            // Lower half (mirror)
            for (int i = static_cast<int>(waveformPoints.size()) - 1; i >= 0; --i)
            {
                float x = waveformBounds.getX() + i * xStep;
                float y = centerY + waveformPoints[i] * waveformBounds.getHeight() * 0.5f;
                waveformPath.lineTo(x, y);
            }

            waveformPath.closeSubPath();
            g.fillPath(waveformPath);
        }

        // Waveform outline
        g.setColour(AutomasterColors::primary);
        if (!waveformPoints.empty())
        {
            juce::Path outlinePath;
            float xStep = waveformBounds.getWidth() / (waveformPoints.size() - 1);
            float centerY = waveformBounds.getCentreY();

            outlinePath.startNewSubPath(waveformBounds.getX(),
                                         centerY - waveformPoints[0] * waveformBounds.getHeight() * 0.5f);

            for (size_t i = 1; i < waveformPoints.size(); ++i)
            {
                float x = waveformBounds.getX() + i * xStep;
                float y = centerY - waveformPoints[i] * waveformBounds.getHeight() * 0.5f;
                outlinePath.lineTo(x, y);
            }

            g.strokePath(outlinePath, juce::PathStrokeType(1.0f));
        }

        // File name and duration
        g.setColour(AutomasterColors::textPrimary);
        g.setFont(11.0f);
        g.drawText(fileName, bounds.getX() + 5.0f, bounds.getY() + 2.0f,
                   bounds.getWidth() - 60.0f, 16.0f, juce::Justification::left);

        // Duration
        int minutes = static_cast<int>(duration) / 60;
        int seconds = static_cast<int>(duration) % 60;
        juce::String durationStr = juce::String::formatted("%d:%02d", minutes, seconds);
        g.setColour(AutomasterColors::textSecondary);
        g.drawText(durationStr, bounds.getRight() - 50.0f, bounds.getY() + 2.0f,
                   45.0f, 16.0f, juce::Justification::right);

        // Profile info at bottom
        if (referenceProfile != nullptr && referenceProfile->isProfileValid())
        {
            g.setFont(10.0f);
            juce::String profileInfo = juce::String::formatted(
                "LUFS: %.1f  Width: %.2f  Crest: %.1fdB",
                referenceProfile->getLoudnessRMS(),
                referenceProfile->getStereoWidth(),
                referenceProfile->getCrestFactor());

            g.drawText(profileInfo, bounds.getX() + 5.0f, bounds.getBottom() - 16.0f,
                       bounds.getWidth() - 10.0f, 14.0f, juce::Justification::left);
        }
    }

private:
    void generateWaveform(const juce::AudioBuffer<float>& buffer)
    {
        waveformPoints.clear();

        // Downsample to ~200 points for display
        int numPoints = 200;
        int samplesPerPoint = buffer.getNumSamples() / numPoints;

        if (samplesPerPoint < 1)
            samplesPerPoint = 1;

        for (int i = 0; i < numPoints; ++i)
        {
            int startSample = i * samplesPerPoint;
            int endSample = std::min(startSample + samplesPerPoint, buffer.getNumSamples());

            float maxVal = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                for (int s = startSample; s < endSample; ++s)
                {
                    maxVal = std::max(maxVal, std::abs(buffer.getSample(ch, s)));
                }
            }

            waveformPoints.push_back(maxVal);
        }
    }

    juce::AudioFormatManager formatManager;
    std::vector<float> waveformPoints;
    bool hasFile = false;
    juce::String fileName;
    double duration = 0.0;
    const ReferenceProfile* referenceProfile = nullptr;
};
