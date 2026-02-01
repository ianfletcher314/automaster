#pragma once

#include "LookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Vertical level meter with peak hold
class LevelMeter : public juce::Component
{
public:
    LevelMeter()
    {
        setOpaque(false);
    }

    void setLevel(float levelDB)
    {
        currentLevel = levelDB;
        if (levelDB > peakLevel)
        {
            peakLevel = levelDB;
            peakHoldCounter = peakHoldSamples;
        }
        repaint();
    }

    void setRange(float minDB, float maxDB)
    {
        minLevel = minDB;
        maxLevel = maxDB;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Calculate level position
        float levelNorm = juce::jmap(currentLevel, minLevel, maxLevel, 0.0f, 1.0f);
        levelNorm = juce::jlimit(0.0f, 1.0f, levelNorm);

        float levelHeight = bounds.getHeight() * levelNorm;
        float levelY = bounds.getBottom() - levelHeight;

        // Draw segmented meter
        int numSegments = 20;
        float segmentHeight = bounds.getHeight() / numSegments;
        float segmentGap = 1.0f;

        for (int i = 0; i < numSegments; ++i)
        {
            float segmentY = bounds.getBottom() - (i + 1) * segmentHeight;
            float segmentLevel = (float)(i + 1) / numSegments;

            if (levelNorm >= segmentLevel - 0.5f / numSegments)
            {
                // Determine color based on level
                juce::Colour segmentColor;
                if (segmentLevel > 0.9f)
                    segmentColor = AutomasterColors::meterRed;
                else if (segmentLevel > 0.75f)
                    segmentColor = AutomasterColors::meterOrange;
                else if (segmentLevel > 0.5f)
                    segmentColor = AutomasterColors::meterYellow;
                else
                    segmentColor = AutomasterColors::meterGreen;

                g.setColour(segmentColor);
                g.fillRoundedRectangle(bounds.getX() + 2.0f, segmentY + segmentGap,
                                       bounds.getWidth() - 4.0f, segmentHeight - segmentGap * 2.0f, 2.0f);
            }
            else
            {
                // Dim segments above level
                g.setColour(AutomasterColors::panelBgLight.withAlpha(0.3f));
                g.fillRoundedRectangle(bounds.getX() + 2.0f, segmentY + segmentGap,
                                       bounds.getWidth() - 4.0f, segmentHeight - segmentGap * 2.0f, 2.0f);
            }
        }

        // Peak hold indicator
        if (peakHoldCounter > 0)
        {
            float peakNorm = juce::jmap(peakLevel, minLevel, maxLevel, 0.0f, 1.0f);
            peakNorm = juce::jlimit(0.0f, 1.0f, peakNorm);
            float peakY = bounds.getBottom() - bounds.getHeight() * peakNorm;

            g.setColour(AutomasterColors::textPrimary);
            g.fillRect(bounds.getX() + 1.0f, peakY - 1.0f, bounds.getWidth() - 2.0f, 2.0f);

            peakHoldCounter--;
            if (peakHoldCounter == 0)
                peakLevel = minLevel;
        }
    }

private:
    float currentLevel = -60.0f;
    float peakLevel = -60.0f;
    float minLevel = -60.0f;
    float maxLevel = 0.0f;
    int peakHoldCounter = 0;
    int peakHoldSamples = 30;  // ~0.5 seconds at 60fps
};

// Horizontal gain reduction meter
class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter()
    {
        setOpaque(false);
    }

    void setGainReduction(float grDB)
    {
        gainReduction = juce::jlimit(0.0f, maxGR, grDB);
        repaint();
    }

    void setMaxGR(float maxDB)
    {
        maxGR = maxDB;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // GR bar (grows from right to left)
        float grNorm = gainReduction / maxGR;
        float grWidth = bounds.getWidth() * grNorm;

        if (grWidth > 0.0f)
        {
            // Color based on amount
            juce::Colour grColor;
            if (grNorm > 0.7f)
                grColor = AutomasterColors::meterRed;
            else if (grNorm > 0.4f)
                grColor = AutomasterColors::meterOrange;
            else
                grColor = AutomasterColors::meterYellow;

            g.setColour(grColor);
            g.fillRoundedRectangle(bounds.getRight() - grWidth, bounds.getY(),
                                   grWidth, bounds.getHeight(), 3.0f);
        }

        // Scale markers
        g.setColour(AutomasterColors::textMuted);
        g.setFont(10.0f);

        // Center line (0 dB)
        g.drawText("0", bounds.getRight() - 15, bounds.getY(), 15, bounds.getHeight(),
                   juce::Justification::centredRight);
    }

private:
    float gainReduction = 0.0f;
    float maxGR = 20.0f;
};

// LUFS meter with integrated/short-term/momentary
class LUFSMeter : public juce::Component
{
public:
    enum DisplayMode { Momentary, ShortTerm, Integrated };

    LUFSMeter()
    {
        setOpaque(false);
    }

    void setLevels(float momentary, float shortTerm, float integrated)
    {
        momentaryLUFS = momentary;
        shortTermLUFS = shortTerm;
        integratedLUFS = integrated;
        repaint();
    }

    void setTarget(float targetDB)
    {
        targetLUFS = targetDB;
    }

    void setDisplayMode(DisplayMode mode)
    {
        displayMode = mode;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Current value based on mode
        float currentValue = momentaryLUFS;
        juce::String modeLabel = "M";
        switch (displayMode)
        {
            case Momentary:
                currentValue = momentaryLUFS;
                modeLabel = "M";
                break;
            case ShortTerm:
                currentValue = shortTermLUFS;
                modeLabel = "S";
                break;
            case Integrated:
                currentValue = integratedLUFS;
                modeLabel = "I";
                break;
        }

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Value display
        g.setColour(AutomasterColors::textPrimary);
        g.setFont(juce::Font(24.0f, juce::Font::bold));

        juce::String valueStr = currentValue > -60.0f ?
            juce::String(currentValue, 1) : "-∞";
        g.drawText(valueStr, bounds.reduced(5.0f), juce::Justification::centred);

        // Mode indicator
        g.setColour(AutomasterColors::primary);
        g.setFont(12.0f);
        g.drawText(modeLabel, bounds.getX() + 5.0f, bounds.getY() + 5.0f, 20.0f, 15.0f,
                   juce::Justification::topLeft);

        // Target indicator
        if (targetLUFS > -30.0f)
        {
            float diff = currentValue - targetLUFS;
            juce::Colour diffColor = std::abs(diff) < 1.0f ? AutomasterColors::success :
                                     (diff > 0.0f ? AutomasterColors::warning : AutomasterColors::secondary);

            g.setColour(diffColor);
            g.setFont(10.0f);
            juce::String diffStr = (diff >= 0.0f ? "+" : "") + juce::String(diff, 1);
            g.drawText(diffStr, bounds.getRight() - 35.0f, bounds.getBottom() - 20.0f, 30.0f, 15.0f,
                       juce::Justification::bottomRight);
        }
    }

private:
    float momentaryLUFS = -100.0f;
    float shortTermLUFS = -100.0f;
    float integratedLUFS = -100.0f;
    float targetLUFS = -14.0f;
    DisplayMode displayMode = ShortTerm;
};

// Correlation meter (-1 to +1)
class CorrelationMeter : public juce::Component
{
public:
    CorrelationMeter()
    {
        setOpaque(false);
    }

    void setCorrelation(float corr)
    {
        correlation = juce::jlimit(-1.0f, 1.0f, corr);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float centerX = bounds.getCentreX();

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Center line (mono)
        g.setColour(AutomasterColors::panelBgLight);
        g.fillRect(centerX - 0.5f, bounds.getY(), 1.0f, bounds.getHeight());

        // Indicator position
        float indicatorX = centerX + (bounds.getWidth() * 0.5f - 4.0f) * correlation;

        // Color based on correlation
        juce::Colour indicatorColor;
        if (correlation > 0.5f)
            indicatorColor = AutomasterColors::meterGreen;
        else if (correlation > 0.0f)
            indicatorColor = AutomasterColors::meterYellow;
        else if (correlation > -0.5f)
            indicatorColor = AutomasterColors::meterOrange;
        else
            indicatorColor = AutomasterColors::meterRed;

        // Draw indicator
        g.setColour(indicatorColor);
        g.fillRoundedRectangle(indicatorX - 3.0f, bounds.getY() + 2.0f, 6.0f,
                               bounds.getHeight() - 4.0f, 2.0f);

        // Labels
        g.setColour(AutomasterColors::textMuted);
        g.setFont(9.0f);
        g.drawText("L", bounds.getX(), bounds.getY(), 12.0f, bounds.getHeight(),
                   juce::Justification::centredLeft);
        g.drawText("R", bounds.getRight() - 12.0f, bounds.getY(), 12.0f, bounds.getHeight(),
                   juce::Justification::centredRight);
    }

private:
    float correlation = 1.0f;
};

// Match percentage indicator
class MatchIndicator : public juce::Component
{
public:
    MatchIndicator()
    {
        setOpaque(false);
    }

    void setMatch(float matchPercent)
    {
        match = juce::jlimit(0.0f, 100.0f, matchPercent);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background track
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Progress bar
        float barWidth = bounds.getWidth() * (match / 100.0f);
        if (barWidth > 0.0f)
        {
            // Gradient from red (0%) to green (100%)
            juce::Colour barColor = juce::Colour::fromHSV(match / 300.0f, 0.7f, 0.8f, 1.0f);
            g.setColour(barColor);
            g.fillRoundedRectangle(bounds.getX(), bounds.getY(), barWidth, bounds.getHeight(), 4.0f);
        }

        // Percentage text
        g.setColour(AutomasterColors::textPrimary);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(juce::String(static_cast<int>(match)) + "%", bounds.toNearestInt(),
                   juce::Justification::centred);
    }

private:
    float match = 0.0f;
};
