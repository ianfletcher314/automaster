#pragma once

#include "LookAndFeel.h"
#include "../DSP/SpectralAnalyzer.h"
#include "../DSP/MasteringEQ.h"
#include <juce_gui_basics/juce_gui_basics.h>

class SpectrumAnalyzerUI : public juce::Component, public juce::Timer
{
public:
    SpectrumAnalyzerUI()
    {
        setOpaque(false);
        startTimerHz(30);  // 30fps updates
    }

    ~SpectrumAnalyzerUI() override
    {
        stopTimer();
    }

    void setAnalyzer(SpectralAnalyzer* preAnalyzer, SpectralAnalyzer* postAnalyzer = nullptr)
    {
        preSpectrum = preAnalyzer;
        postSpectrum = postAnalyzer;
    }

    void setEQ(const MasteringEQ* eqModule)
    {
        eq = eqModule;
    }

    void setShowPre(bool show) { showPre = show; }
    void setShowPost(bool show) { showPost = show; }
    void setShowEQCurve(bool show) { showEQCurve = show; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Grid
        drawGrid(g, bounds);

        // Draw spectrums
        if (showPre && preSpectrum != nullptr)
            drawSpectrum(g, bounds, preSpectrum->getMagnitudeSpectrum(), AutomasterColors::spectrumPre, 0.5f);

        if (showPost && postSpectrum != nullptr)
            drawSpectrum(g, bounds, postSpectrum->getMagnitudeSpectrum(), AutomasterColors::spectrumPost, 0.8f);

        // Draw EQ curve
        if (showEQCurve && eq != nullptr)
            drawEQCurve(g, bounds);
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    void drawGrid(juce::Graphics& g, const juce::Rectangle<float>& bounds)
    {
        g.setColour(AutomasterColors::panelBgLight.withAlpha(0.3f));

        // Frequency lines (logarithmic)
        const float freqs[] = { 100.0f, 1000.0f, 10000.0f };
        for (float freq : freqs)
        {
            float x = frequencyToX(freq, bounds);
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        }

        // dB lines
        for (float db = -60.0f; db <= 0.0f; db += 12.0f)
        {
            float y = dBToY(db, bounds);
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        }

        // Labels
        g.setColour(AutomasterColors::textMuted);
        g.setFont(9.0f);

        g.drawText("100", frequencyToX(100.0f, bounds) - 15.0f, bounds.getBottom() - 15.0f, 30.0f, 12.0f,
                   juce::Justification::centred);
        g.drawText("1k", frequencyToX(1000.0f, bounds) - 15.0f, bounds.getBottom() - 15.0f, 30.0f, 12.0f,
                   juce::Justification::centred);
        g.drawText("10k", frequencyToX(10000.0f, bounds) - 15.0f, bounds.getBottom() - 15.0f, 30.0f, 12.0f,
                   juce::Justification::centred);

        g.drawText("0dB", bounds.getX() + 2.0f, dBToY(0.0f, bounds) - 6.0f, 30.0f, 12.0f,
                   juce::Justification::left);
        g.drawText("-24", bounds.getX() + 2.0f, dBToY(-24.0f, bounds) - 6.0f, 30.0f, 12.0f,
                   juce::Justification::left);
        g.drawText("-48", bounds.getX() + 2.0f, dBToY(-48.0f, bounds) - 6.0f, 30.0f, 12.0f,
                   juce::Justification::left);
    }

    void drawSpectrum(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                      const std::array<float, SpectralAnalyzer::NUM_BINS>& spectrum,
                      juce::Colour color, float alpha)
    {
        juce::Path spectrumPath;
        bool pathStarted = false;

        float binWidth = 44100.0f / (SpectralAnalyzer::FFT_SIZE);  // Assume 44.1kHz

        for (int i = 1; i < SpectralAnalyzer::NUM_BINS; ++i)
        {
            float freq = i * binWidth;
            if (freq < 20.0f || freq > 20000.0f)
                continue;

            float x = frequencyToX(freq, bounds);
            float y = dBToY(spectrum[i], bounds);

            if (!pathStarted)
            {
                spectrumPath.startNewSubPath(x, y);
                pathStarted = true;
            }
            else
            {
                spectrumPath.lineTo(x, y);
            }
        }

        // Close path for fill
        juce::Path fillPath = spectrumPath;
        fillPath.lineTo(bounds.getRight(), bounds.getBottom());
        fillPath.lineTo(bounds.getX(), bounds.getBottom());
        fillPath.closeSubPath();

        // Fill with gradient
        g.setColour(color.withAlpha(alpha * 0.3f));
        g.fillPath(fillPath);

        // Stroke
        g.setColour(color.withAlpha(alpha));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }

    void drawEQCurve(juce::Graphics& g, const juce::Rectangle<float>& bounds)
    {
        auto response = eq->getMagnitudeResponse();

        juce::Path eqPath;
        bool pathStarted = false;

        for (int i = 0; i < MasteringEQ::RESPONSE_SIZE; ++i)
        {
            float normalizedX = static_cast<float>(i) / (MasteringEQ::RESPONSE_SIZE - 1);
            float freq = 20.0f * std::pow(1000.0f, normalizedX);

            float x = frequencyToX(freq, bounds);
            float y = dBToY(response[i], bounds);

            // Center around 0dB for EQ curve
            float centerY = dBToY(0.0f, bounds);
            y = centerY - (centerY - y);

            if (!pathStarted)
            {
                eqPath.startNewSubPath(x, y);
                pathStarted = true;
            }
            else
            {
                eqPath.lineTo(x, y);
            }
        }

        // Draw EQ curve
        g.setColour(AutomasterColors::spectrumEQ);
        g.strokePath(eqPath, juce::PathStrokeType(2.0f));

        // Draw fill for boost/cut
        juce::Path fillPath;
        float zeroY = dBToY(0.0f, bounds);

        fillPath.startNewSubPath(bounds.getX(), zeroY);
        for (int i = 0; i < MasteringEQ::RESPONSE_SIZE; ++i)
        {
            float normalizedX = static_cast<float>(i) / (MasteringEQ::RESPONSE_SIZE - 1);
            float freq = 20.0f * std::pow(1000.0f, normalizedX);
            float x = frequencyToX(freq, bounds);
            float y = dBToY(response[i], bounds);
            float centerY = dBToY(0.0f, bounds);
            y = centerY - (centerY - y);
            fillPath.lineTo(x, y);
        }
        fillPath.lineTo(bounds.getRight(), zeroY);
        fillPath.closeSubPath();

        g.setColour(AutomasterColors::spectrumEQ.withAlpha(0.2f));
        g.fillPath(fillPath);
    }

    float frequencyToX(float freq, const juce::Rectangle<float>& bounds) const
    {
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float normalizedX = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
        return bounds.getX() + bounds.getWidth() * normalizedX;
    }

    float dBToY(float dB, const juce::Rectangle<float>& bounds) const
    {
        float minDB = -60.0f;
        float maxDB = 6.0f;
        float normalizedY = (dB - minDB) / (maxDB - minDB);
        return bounds.getBottom() - bounds.getHeight() * normalizedY;
    }

    SpectralAnalyzer* preSpectrum = nullptr;
    SpectralAnalyzer* postSpectrum = nullptr;
    const MasteringEQ* eq = nullptr;

    bool showPre = true;
    bool showPost = true;
    bool showEQCurve = true;
};
