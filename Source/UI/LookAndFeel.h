#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Purple/Blue AI Theme for Automaster
namespace AutomasterColors
{
    // Background colors
    const juce::Colour background      = juce::Colour(0xff0d0d12);  // Very dark blue-black
    const juce::Colour panelBg         = juce::Colour(0xff161620);  // Dark panel
    const juce::Colour panelBgLight    = juce::Colour(0xff1e1e2a);  // Lighter panel
    const juce::Colour cardBg          = juce::Colour(0xff252535);  // Card background

    // Accent colors - Purple/Blue AI theme
    const juce::Colour primary         = juce::Colour(0xff7c3aed);  // Vibrant purple
    const juce::Colour primaryLight    = juce::Colour(0xff8b5cf6);  // Light purple
    const juce::Colour primaryDark     = juce::Colour(0xff5b21b6);  // Dark purple
    const juce::Colour secondary       = juce::Colour(0xff3b82f6);  // Blue
    const juce::Colour accent          = juce::Colour(0xff06b6d4);  // Cyan

    // Text colors
    const juce::Colour textPrimary     = juce::Colour(0xfff1f5f9);  // Near white
    const juce::Colour textSecondary   = juce::Colour(0xff94a3b8);  // Gray
    const juce::Colour textMuted       = juce::Colour(0xff64748b);  // Muted gray

    // Meter colors
    const juce::Colour meterGreen      = juce::Colour(0xff22c55e);
    const juce::Colour meterYellow     = juce::Colour(0xffeab308);
    const juce::Colour meterRed        = juce::Colour(0xffef4444);
    const juce::Colour meterOrange     = juce::Colour(0xfff97316);

    // Status colors
    const juce::Colour success         = juce::Colour(0xff10b981);
    const juce::Colour warning         = juce::Colour(0xfff59e0b);
    const juce::Colour error           = juce::Colour(0xffef4444);

    // Spectrum colors
    const juce::Colour spectrumPre     = juce::Colour(0xff64748b);  // Gray for input
    const juce::Colour spectrumPost    = juce::Colour(0xff8b5cf6);  // Purple for output
    const juce::Colour spectrumEQ      = juce::Colour(0xff06b6d4);  // Cyan for EQ curve

    // Module-specific colors
    const juce::Colour eqColor         = juce::Colour(0xff3b82f6);  // Blue
    const juce::Colour compColor       = juce::Colour(0xfff97316);  // Orange
    const juce::Colour stereoColor     = juce::Colour(0xff8b5cf6);  // Purple
    const juce::Colour limiterColor    = juce::Colour(0xffef4444);  // Red
}

class AutomasterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AutomasterLookAndFeel()
    {
        // Set default colors
        setColour(juce::ResizableWindow::backgroundColourId, AutomasterColors::background);
        setColour(juce::TextButton::buttonColourId, AutomasterColors::panelBg);
        setColour(juce::TextButton::buttonOnColourId, AutomasterColors::primary);
        setColour(juce::TextButton::textColourOffId, AutomasterColors::textPrimary);
        setColour(juce::TextButton::textColourOnId, AutomasterColors::textPrimary);
        setColour(juce::ComboBox::backgroundColourId, AutomasterColors::panelBg);
        setColour(juce::ComboBox::textColourId, AutomasterColors::textPrimary);
        setColour(juce::ComboBox::outlineColourId, AutomasterColors::panelBgLight);
        setColour(juce::PopupMenu::backgroundColourId, AutomasterColors::panelBg);
        setColour(juce::PopupMenu::textColourId, AutomasterColors::textPrimary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, AutomasterColors::primary);
        setColour(juce::Label::textColourId, AutomasterColors::textPrimary);
        setColour(juce::Slider::thumbColourId, AutomasterColors::primary);
        setColour(juce::Slider::trackColourId, AutomasterColors::panelBgLight);
        setColour(juce::Slider::backgroundColourId, AutomasterColors::panelBg);
        setColour(juce::TabbedComponent::backgroundColourId, AutomasterColors::panelBg);
        setColour(juce::TabbedComponent::outlineColourId, AutomasterColors::panelBgLight);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = radius * 0.1f;
        auto arcRadius = radius - lineW * 0.5f;

        juce::Point<float> centre = bounds.getCentre();

        // Background track
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                     0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(AutomasterColors::panelBgLight);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        // Value arc
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);

            // Gradient from primary to accent
            juce::ColourGradient gradient(AutomasterColors::primary, centre.x, centre.y,
                                           AutomasterColors::accent, centre.x + radius, centre.y + radius,
                                           true);
            g.setGradientFill(gradient);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        // Knob body
        auto knobRadius = arcRadius * 0.65f;
        juce::Rectangle<float> knobBounds(centre.x - knobRadius, centre.y - knobRadius,
                                           knobRadius * 2.0f, knobRadius * 2.0f);

        // Knob gradient
        juce::ColourGradient knobGradient(AutomasterColors::cardBg, centre.x, centre.y - knobRadius,
                                           AutomasterColors::panelBg, centre.x, centre.y + knobRadius,
                                           false);
        g.setGradientFill(knobGradient);
        g.fillEllipse(knobBounds);

        // Knob outline
        g.setColour(AutomasterColors::panelBgLight);
        g.drawEllipse(knobBounds, 1.0f);

        // Indicator line
        g.setColour(slider.isEnabled() ? AutomasterColors::primary : AutomasterColors::textMuted);
        juce::Path indicator;
        auto indicatorRadius = knobRadius * 0.8f;
        auto indicatorWidth = lineW * 0.6f;
        indicator.addRoundedRectangle(-indicatorWidth * 0.5f, -indicatorRadius,
                                       indicatorWidth, indicatorRadius * 0.4f, 2.0f);
        g.fillPath(indicator, juce::AffineTransform::rotation(toAngle).translated(centre));
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 4.0f;
            auto bounds = juce::Rectangle<float>(x, y, width, height);
            auto trackX = bounds.getCentreX() - trackWidth * 0.5f;

            // Track background
            g.setColour(AutomasterColors::panelBgLight);
            g.fillRoundedRectangle(trackX, y, trackWidth, height, 2.0f);

            // Track fill
            auto fillHeight = maxSliderPos - sliderPos;
            g.setColour(AutomasterColors::primary);
            g.fillRoundedRectangle(trackX, sliderPos, trackWidth, fillHeight, 2.0f);

            // Thumb
            auto thumbSize = 12.0f;
            g.setColour(AutomasterColors::textPrimary);
            g.fillEllipse(bounds.getCentreX() - thumbSize * 0.5f, sliderPos - thumbSize * 0.5f,
                         thumbSize, thumbSize);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos,
                                              maxSliderPos, style, slider);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 6.0f;

        juce::Colour baseColour = button.getToggleState() ?
            AutomasterColors::primary : AutomasterColors::panelBgLight;

        if (isButtonDown)
            baseColour = baseColour.darker(0.2f);
        else if (isMouseOverButton)
            baseColour = baseColour.brighter(0.1f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border for non-toggled buttons
        if (!button.getToggleState())
        {
            g.setColour(AutomasterColors::panelBg.brighter(0.2f));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0, 0, width, height).reduced(1.0f);
        auto cornerSize = 4.0f;

        g.setColour(AutomasterColors::panelBgLight);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(AutomasterColors::panelBg.brighter(0.2f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Arrow
        juce::Path arrow;
        auto arrowX = width - 15.0f;
        auto arrowY = height * 0.5f;
        arrow.addTriangle(arrowX - 4.0f, arrowY - 2.0f,
                          arrowX + 4.0f, arrowY - 2.0f,
                          arrowX, arrowY + 3.0f);
        g.setColour(AutomasterColors::textSecondary);
        g.fillPath(arrow);
    }

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        auto bounds = button.getActiveArea().toFloat();
        auto isFrontTab = button.isFrontTab();

        juce::Colour bgColour = isFrontTab ? AutomasterColors::primary.withAlpha(0.2f) :
                                 AutomasterColors::panelBg;

        if (isMouseOver && !isFrontTab)
            bgColour = bgColour.brighter(0.1f);

        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.reduced(2.0f, 0.0f), 4.0f);

        // Bottom indicator for active tab
        if (isFrontTab)
        {
            g.setColour(AutomasterColors::primary);
            g.fillRoundedRectangle(bounds.getX() + 4.0f, bounds.getBottom() - 3.0f,
                                   bounds.getWidth() - 8.0f, 2.0f, 1.0f);
        }

        // Text
        g.setColour(isFrontTab ? AutomasterColors::textPrimary : AutomasterColors::textSecondary);
        g.setFont(13.0f);
        g.drawText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return juce::Font(13.0f);
    }
};

// Custom component for glowing buttons (AI status indicator)
class GlowButton : public juce::TextButton
{
public:
    GlowButton(const juce::String& text) : juce::TextButton(text)
    {
        setColour(juce::TextButton::buttonColourId, AutomasterColors::primary.withAlpha(0.3f));
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Glow effect when active
        if (getToggleState())
        {
            g.setColour(AutomasterColors::primary.withAlpha(0.15f));
            g.fillRoundedRectangle(bounds.expanded(4.0f), 10.0f);
            g.fillRoundedRectangle(bounds.expanded(2.0f), 8.0f);
        }

        // Button background
        juce::Colour bgColour = getToggleState() ?
            AutomasterColors::primary : AutomasterColors::panelBgLight;

        if (isMouseOver())
            bgColour = bgColour.brighter(0.1f);

        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.reduced(1.0f), 6.0f);

        // Text
        g.setColour(AutomasterColors::textPrimary);
        g.setFont(14.0f);
        g.drawText(getButtonText(), bounds.toNearestInt(), juce::Justification::centred);
    }
};
