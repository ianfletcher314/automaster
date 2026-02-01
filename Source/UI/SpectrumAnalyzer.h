#pragma once

#include "LookAndFeel.h"
#include "../DSP/MasteringEQ.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

// FFT size for spectrum analysis
static constexpr int kFFTOrder = 11;
static constexpr int kFFTSize = 1 << kFFTOrder;  // 2048

// Spectrum Analyzer with EQ curve overlay
class SpectrumAnalyzer : public juce::Component
{
public:
    SpectrumAnalyzer()
    {
        setOpaque(false);
    }

    void setEQ(MasteringEQ* eq) { masteringEQ = eq; }

    void pushInputSamples(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            inputFifo[inputFifoIndex++] = samples[i];
            if (inputFifoIndex >= kFFTSize)
            {
                inputFifoIndex = 0;
                processFFT(inputFifo.data(), inputSpectrum.data());
                inputSpectrumReady = true;
            }
        }
    }

    void pushOutputSamples(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outputFifo[outputFifoIndex++] = samples[i];
            if (outputFifoIndex >= kFFTSize)
            {
                outputFifoIndex = 0;
                processFFT(outputFifo.data(), outputSpectrum.data());
                outputSpectrumReady = true;
            }
        }
    }

    void setSampleRate(double sr) { sampleRate = sr; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(0xFF0a0a0a));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Grid
        drawGrid(g, bounds);

        // Draw input spectrum (faint)
        if (inputSpectrumReady)
            drawSpectrum(g, inputSpectrum, juce::Colour(0xFF4488ff), 0.3f, bounds);

        // Draw output spectrum (brighter)
        if (outputSpectrumReady)
            drawSpectrum(g, outputSpectrum, AutomasterColors::primary, 0.6f, bounds);

        // Draw EQ response curve
        if (masteringEQ != nullptr)
            drawEQCurve(g, bounds);

        // Border
        g.setColour(juce::Colour(0xFF333333));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

private:
    void processFFT(float* fifoData, float* spectrumData)
    {
        // Copy data and apply window
        std::copy(fifoData, fifoData + kFFTSize, fftData.begin());
        window.multiplyWithWindowingTable(fftData.data(), kFFTSize);

        // Zero pad second half
        for (int i = kFFTSize; i < kFFTSize * 2; ++i)
            fftData[i] = 0.0f;

        // Perform FFT
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        // Copy to spectrum with smoothing
        for (int i = 0; i < kFFTSize / 2; ++i)
        {
            float newVal = fftData[i];
            spectrumData[i] = spectrumData[i] * 0.8f + newVal * 0.2f;  // Smooth
        }
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colour(0xFF222222));

        // Horizontal grid (dB)
        for (int db = -12; db <= 12; db += 6)
        {
            float y = dbToY((float)db, bounds.getHeight());
            g.drawHorizontalLine((int)(bounds.getY() + y), bounds.getX() + 5, bounds.getRight() - 5);
        }

        // Zero line brighter
        g.setColour(juce::Colour(0xFF333333));
        float zeroY = dbToY(0.0f, bounds.getHeight());
        g.drawHorizontalLine((int)(bounds.getY() + zeroY), bounds.getX() + 5, bounds.getRight() - 5);

        // Vertical grid (frequency)
        g.setColour(juce::Colour(0xFF222222));
        float freqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
        for (float freq : freqs)
        {
            float x = freqToX(freq, bounds.getWidth());
            g.drawVerticalLine((int)(bounds.getX() + x), bounds.getY() + 5, bounds.getBottom() - 5);
        }

        // Labels
        g.setColour(AutomasterColors::textMuted);
        g.setFont(juce::FontOptions(8.0f));
        const char* freqLabels[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
        for (int i = 0; i < 8; ++i)
        {
            float x = freqToX(freqs[i], bounds.getWidth());
            g.drawText(freqLabels[i], (int)(bounds.getX() + x - 12), (int)(bounds.getBottom() - 12), 24, 10, juce::Justification::centred);
        }

        // dB labels
        g.drawText("+12", (int)(bounds.getX() + 2), (int)(bounds.getY() + dbToY(12.0f, bounds.getHeight()) - 5), 24, 10, juce::Justification::left);
        g.drawText("0dB", (int)(bounds.getX() + 2), (int)(bounds.getY() + zeroY - 5), 24, 10, juce::Justification::left);
        g.drawText("-12", (int)(bounds.getX() + 2), (int)(bounds.getY() + dbToY(-12.0f, bounds.getHeight()) - 5), 24, 10, juce::Justification::left);
    }

    void drawSpectrum(juce::Graphics& g, const std::array<float, kFFTSize / 2>& spectrum,
                      juce::Colour colour, float alpha, juce::Rectangle<float> bounds)
    {
        juce::Path spectrumPath;
        bool pathStarted = false;

        for (float x = 0; x < bounds.getWidth(); x += 1.0f)
        {
            float freq = 20.0f * std::pow(1000.0f, x / bounds.getWidth());
            int bin = (int)(freq * kFFTSize / sampleRate);
            if (bin < 0) bin = 0;
            if (bin >= kFFTSize / 2) bin = kFFTSize / 2 - 1;

            float magnitude = spectrum[bin];

            // Interpolate between bins
            if (bin > 0 && bin < kFFTSize / 2 - 1)
            {
                float frac = (freq * kFFTSize / sampleRate) - bin;
                magnitude = spectrum[bin] * (1.0f - frac) + spectrum[bin + 1] * frac;
            }

            magnitude *= 20.0f;  // Boost for visibility
            float db = magnitude > 0.0001f ? 20.0f * std::log10(magnitude) : -60.0f;
            db = juce::jlimit(-60.0f, 6.0f, db);
            float normalizedDb = (db + 60.0f) / 66.0f;
            float y = bounds.getHeight() * (1.0f - normalizedDb);

            if (!pathStarted)
            {
                spectrumPath.startNewSubPath(bounds.getX() + x, bounds.getY() + y);
                pathStarted = true;
            }
            else
            {
                spectrumPath.lineTo(bounds.getX() + x, bounds.getY() + y);
            }
        }

        // Fill
        juce::Path fillPath = spectrumPath;
        fillPath.lineTo(bounds.getRight(), bounds.getBottom());
        fillPath.lineTo(bounds.getX(), bounds.getBottom());
        fillPath.closeSubPath();
        g.setColour(colour.withAlpha(alpha * 0.3f));
        g.fillPath(fillPath);

        // Stroke
        g.setColour(colour.withAlpha(alpha));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));
    }

    void drawEQCurve(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        float zeroY = dbToY(0.0f, bounds.getHeight());

        // Draw individual band curves first (colored, semi-transparent)
        for (int band = 0; band < 4; ++band)
        {
            if (masteringEQ->isBandActive(band))
                drawBandCurve(g, bounds, band, AutomasterColors::getEQBandColor(band));
        }

        // Draw shelves
        if (masteringEQ->isLowShelfActive())
            drawFilterCurve(g, bounds, [this](float freq) {
                return masteringEQ->getLowShelfMagnitudeAtFrequency(freq);
            }, AutomasterColors::eqLowShelf);

        if (masteringEQ->isHighShelfActive())
            drawFilterCurve(g, bounds, [this](float freq) {
                return masteringEQ->getHighShelfMagnitudeAtFrequency(freq);
            }, AutomasterColors::eqHighShelf);

        // Draw combined EQ curve (white, on top)
        juce::Path eqPath;
        bool pathStarted = false;

        for (float x = 0; x < bounds.getWidth(); x += 2.0f)
        {
            float freq = 20.0f * std::pow(1000.0f, x / bounds.getWidth());
            float magnitude = masteringEQ->getMagnitudeAtFrequency(freq);
            float db = juce::Decibels::gainToDecibels(magnitude, -24.0f);
            db = juce::jlimit(-18.0f, 18.0f, db);
            float y = dbToY(db, bounds.getHeight());

            if (!pathStarted)
            {
                eqPath.startNewSubPath(bounds.getX() + x, bounds.getY() + y);
                pathStarted = true;
            }
            else
            {
                eqPath.lineTo(bounds.getX() + x, bounds.getY() + y);
            }
        }

        // Fill under combined curve
        juce::Path fillPath = eqPath;
        fillPath.lineTo(bounds.getRight(), bounds.getY() + zeroY);
        fillPath.lineTo(bounds.getX(), bounds.getY() + zeroY);
        fillPath.closeSubPath();

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillPath(fillPath);

        // Combined curve stroke (white)
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.strokePath(eqPath, juce::PathStrokeType(2.0f));
    }

    void drawBandCurve(juce::Graphics& g, juce::Rectangle<float> bounds, int band, juce::Colour color)
    {
        juce::Path bandPath;
        bool pathStarted = false;
        float zeroY = dbToY(0.0f, bounds.getHeight());

        for (float x = 0; x < bounds.getWidth(); x += 2.0f)
        {
            float freq = 20.0f * std::pow(1000.0f, x / bounds.getWidth());
            float magnitude = masteringEQ->getBandMagnitudeAtFrequency(band, freq);
            float db = juce::Decibels::gainToDecibels(magnitude, -24.0f);
            db = juce::jlimit(-18.0f, 18.0f, db);
            float y = dbToY(db, bounds.getHeight());

            if (!pathStarted)
            {
                bandPath.startNewSubPath(bounds.getX() + x, bounds.getY() + y);
                pathStarted = true;
            }
            else
            {
                bandPath.lineTo(bounds.getX() + x, bounds.getY() + y);
            }
        }

        // Fill under curve
        juce::Path fillPath = bandPath;
        fillPath.lineTo(bounds.getRight(), bounds.getY() + zeroY);
        fillPath.lineTo(bounds.getX(), bounds.getY() + zeroY);
        fillPath.closeSubPath();

        g.setColour(color.withAlpha(0.15f));
        g.fillPath(fillPath);

        // Glow effect
        g.setColour(color.withAlpha(0.3f));
        g.strokePath(bandPath, juce::PathStrokeType(4.0f));
        g.setColour(color.withAlpha(0.7f));
        g.strokePath(bandPath, juce::PathStrokeType(2.0f));
    }

    template<typename MagnitudeFunc>
    void drawFilterCurve(juce::Graphics& g, juce::Rectangle<float> bounds, MagnitudeFunc getMag, juce::Colour color)
    {
        juce::Path filterPath;
        bool pathStarted = false;
        float zeroY = dbToY(0.0f, bounds.getHeight());

        for (float x = 0; x < bounds.getWidth(); x += 2.0f)
        {
            float freq = 20.0f * std::pow(1000.0f, x / bounds.getWidth());
            float magnitude = getMag(freq);
            float db = juce::Decibels::gainToDecibels(magnitude, -24.0f);
            db = juce::jlimit(-18.0f, 18.0f, db);
            float y = dbToY(db, bounds.getHeight());

            if (!pathStarted)
            {
                filterPath.startNewSubPath(bounds.getX() + x, bounds.getY() + y);
                pathStarted = true;
            }
            else
            {
                filterPath.lineTo(bounds.getX() + x, bounds.getY() + y);
            }
        }

        // Fill
        juce::Path fillPath = filterPath;
        fillPath.lineTo(bounds.getRight(), bounds.getY() + zeroY);
        fillPath.lineTo(bounds.getX(), bounds.getY() + zeroY);
        fillPath.closeSubPath();

        g.setColour(color.withAlpha(0.1f));
        g.fillPath(fillPath);

        // Stroke
        g.setColour(color.withAlpha(0.5f));
        g.strokePath(filterPath, juce::PathStrokeType(2.0f));
    }

    float freqToX(float freq, float width) const
    {
        float minLog = std::log10(20.0f);
        float maxLog = std::log10(20000.0f);
        float freqLog = std::log10(juce::jmax(20.0f, freq));
        return width * (freqLog - minLog) / (maxLog - minLog);
    }

    float dbToY(float db, float height) const
    {
        return height * 0.5f * (1.0f - db / 18.0f);
    }

    MasteringEQ* masteringEQ = nullptr;
    double sampleRate = 44100.0;

    // FFT
    juce::dsp::FFT fft { kFFTOrder };
    juce::dsp::WindowingFunction<float> window { kFFTSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, kFFTSize * 2> fftData = {};

    // Input spectrum
    std::array<float, kFFTSize> inputFifo = {};
    std::array<float, kFFTSize / 2> inputSpectrum = {};
    int inputFifoIndex = 0;
    bool inputSpectrumReady = false;

    // Output spectrum
    std::array<float, kFFTSize> outputFifo = {};
    std::array<float, kFFTSize / 2> outputSpectrum = {};
    int outputFifoIndex = 0;
    bool outputSpectrumReady = false;
};
