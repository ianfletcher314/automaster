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
    const juce::Colour compColor       = juce::Colour(0xfff97316);  // Orange (for xover knobs)
    const juce::Colour stereoColor     = juce::Colour(0xff8b5cf6);  // Purple
    const juce::Colour limiterColor    = juce::Colour(0xffef4444);  // Red

    // Compressor band colors - distinct for each band
    const juce::Colour compLowColor    = juce::Colour(0xffef4444);  // Red/warm for low
    const juce::Colour compMidColor    = juce::Colour(0xff22c55e);  // Green for mid
    const juce::Colour compHighColor   = juce::Colour(0xff3b82f6);  // Blue for high

    // EQ band colors (for visualizer and knob accents)
    const juce::Colour eqBand1         = juce::Colour(0xff22c55e);  // Green
    const juce::Colour eqBand2         = juce::Colour(0xff3b82f6);  // Blue
    const juce::Colour eqBand3         = juce::Colour(0xfff59e0b);  // Amber
    const juce::Colour eqBand4         = juce::Colour(0xffec4899);  // Pink
    const juce::Colour eqLowShelf      = juce::Colour(0xff06b6d4);  // Cyan
    const juce::Colour eqHighShelf     = juce::Colour(0xffa855f7);  // Purple
    const juce::Colour eqHPF           = juce::Colour(0xff64748b);  // Gray
    const juce::Colour eqLPF           = juce::Colour(0xff64748b);  // Gray

    inline juce::Colour getEQBandColor(int band) {
        switch(band) {
            case 0: return eqBand1;
            case 1: return eqBand2;
            case 2: return eqBand3;
            case 3: return eqBand4;
            default: return eqColor;
        }
    }
}

// LookAndFeel that draws rotary sliders with a custom arc color and grey inner knob
class ColoredKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ColoredKnobLookAndFeel(juce::Colour c) : color(c) {}

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

        // Value arc with custom color
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(color);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        // Inner knob body
        auto knobRadius = arcRadius * 0.65f;
        juce::Rectangle<float> knobBounds(centre.x - knobRadius, centre.y - knobRadius,
                                           knobRadius * 2.0f, knobRadius * 2.0f);

        // Knob gradient (dark grey)
        juce::ColourGradient knobGradient(AutomasterColors::cardBg, centre.x, centre.y - knobRadius,
                                           AutomasterColors::panelBg, centre.x, centre.y + knobRadius,
                                           false);
        g.setGradientFill(knobGradient);
        g.fillEllipse(knobBounds);

        // Knob outline with accent color hint
        g.setColour(color.withAlpha(0.3f));
        g.drawEllipse(knobBounds, 1.5f);

        // Indicator line
        g.setColour(slider.isEnabled() ? color : AutomasterColors::textMuted);
        juce::Path indicator;
        auto indicatorRadius = knobRadius * 0.8f;
        auto indicatorWidth = lineW * 0.6f;
        indicator.addRoundedRectangle(-indicatorWidth * 0.5f, -indicatorRadius,
                                       indicatorWidth, indicatorRadius * 0.4f, 2.0f);
        g.fillPath(indicator, juce::AffineTransform::rotation(toAngle).translated(centre));
    }

private:
    juce::Colour color;
};

// LookAndFeel for toggle switches - draws pill-shaped toggle instead of 0/1 text
class ToggleSwitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToggleSwitchLookAndFeel(juce::Colour accentColor = AutomasterColors::primary)
        : accent(accentColor) {}

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        bool isOn = button.getToggleState();

        // Make it pill-shaped
        float height = bounds.getHeight();
        float width = juce::jmax(height * 1.8f, bounds.getWidth());
        auto toggleBounds = bounds.withSizeKeepingCentre(width, height);

        // Track background
        g.setColour(isOn ? accent.withAlpha(0.3f) : AutomasterColors::panelBg);
        g.fillRoundedRectangle(toggleBounds, height / 2);

        // Track outline
        g.setColour(isOn ? accent : AutomasterColors::panelBgLight);
        g.drawRoundedRectangle(toggleBounds, height / 2, 1.0f);

        // Thumb circle
        float thumbDiameter = height - 4.0f;
        float thumbX = isOn ? toggleBounds.getRight() - thumbDiameter - 2.0f
                            : toggleBounds.getX() + 2.0f;
        float thumbY = toggleBounds.getY() + 2.0f;

        // Thumb shadow
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillEllipse(thumbX + 1, thumbY + 1, thumbDiameter, thumbDiameter);

        // Thumb
        g.setColour(isOn ? accent : AutomasterColors::textMuted);
        g.fillEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter);

        // Hover highlight
        if (isMouseOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter);
        }
    }

    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override
    {
        // Don't draw any text - the toggle visual is enough
    }

private:
    juce::Colour accent;
};

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

        // Use primary accent color for non-custom sliders
        auto accentColor = AutomasterColors::primary;

        // Background track
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                     0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(AutomasterColors::panelBgLight);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        // Value arc - use accent color
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);

            g.setColour(accentColor);
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

        // Knob outline with accent color hint
        g.setColour(accentColor.withAlpha(0.3f));
        g.drawEllipse(knobBounds, 1.5f);

        // Indicator line - use accent color
        g.setColour(slider.isEnabled() ? accentColor : AutomasterColors::textMuted);
        juce::Path indicator;
        auto indicatorRadius = knobRadius * 0.8f;
        auto indicatorWidth = lineW * 0.6f;
        indicator.addRoundedRectangle(-indicatorWidth * 0.5f, -indicatorRadius,
                                       indicatorWidth, indicatorRadius * 0.4f, 2.0f);
        g.fillPath(indicator, juce::AffineTransform::rotation(toAngle).translated(centre));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Draw as a nice pill-shaped toggle switch
        float toggleWidth = juce::jmin(bounds.getWidth(), 36.0f);
        float toggleHeight = juce::jmin(bounds.getHeight(), 18.0f);

        auto toggleBounds = juce::Rectangle<float>(
            bounds.getCentreX() - toggleWidth / 2,
            bounds.getCentreY() - toggleHeight / 2,
            toggleWidth, toggleHeight);

        bool isOn = button.getToggleState();

        // Track background
        g.setColour(isOn ? AutomasterColors::primary.withAlpha(0.3f) : AutomasterColors::panelBg);
        g.fillRoundedRectangle(toggleBounds, toggleHeight / 2);

        // Track border
        g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::panelBgLight.brighter(0.2f));
        g.drawRoundedRectangle(toggleBounds, toggleHeight / 2, 1.0f);

        // Thumb
        float thumbSize = toggleHeight - 4.0f;
        float thumbX = isOn ? toggleBounds.getRight() - thumbSize - 2.0f : toggleBounds.getX() + 2.0f;
        float thumbY = toggleBounds.getCentreY() - thumbSize / 2;

        // Thumb shadow
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillEllipse(thumbX + 1, thumbY + 1, thumbSize, thumbSize);

        // Thumb body
        g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::textSecondary);
        g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

        // Thumb highlight
        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);
        }
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

        // Check if this is a small toggle button (like the switches)
        bool isToggleSwitch = button.getClickingTogglesState() && bounds.getWidth() < 50 && bounds.getHeight() < 30;

        if (isToggleSwitch)
        {
            // Draw as pill-shaped toggle switch
            float cornerRadius = bounds.getHeight() / 2.0f;
            bool isOn = button.getToggleState();

            // Track background
            g.setColour(isOn ? AutomasterColors::primary.withAlpha(0.3f) : AutomasterColors::panelBg);
            g.fillRoundedRectangle(bounds, cornerRadius);

            // Track border
            g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::panelBgLight.brighter(0.2f));
            g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

            // Thumb
            float thumbSize = bounds.getHeight() - 4.0f;
            float thumbX = isOn ? bounds.getRight() - thumbSize - 2.0f : bounds.getX() + 2.0f;
            float thumbY = bounds.getCentreY() - thumbSize / 2.0f;

            // Thumb shadow
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillEllipse(thumbX + 1, thumbY + 1, thumbSize, thumbSize);

            // Thumb body
            g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::textSecondary);
            g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

            // Hover highlight
            if (isMouseOverButton)
            {
                g.setColour(juce::Colours::white.withAlpha(0.15f));
                g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);
            }
        }
        else
        {
            // Regular button
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
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Don't draw text for small toggle switches
        bool isToggleSwitch = button.getClickingTogglesState() && bounds.getWidth() < 50 && bounds.getHeight() < 30;
        if (isToggleSwitch)
            return;

        // Regular button text
        auto font = getTextButtonFont(button, button.getHeight());
        g.setFont(font);
        g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                              : juce::TextButton::textColourOffId)
                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

        auto yIndent = juce::jmin(4, button.proportionOfHeight(0.3f));
        auto cornerSize = juce::jmin(button.getHeight(), button.getWidth()) / 2;
        auto fontHeight = juce::roundToInt(font.getHeight() * 0.6f);
        auto leftIndent = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        auto rightIndent = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        auto textWidth = button.getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(button.getButtonText(),
                             leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                             juce::Justification::centred, 2);
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

// Custom toggle switch component (pill-shaped)
class ToggleSwitch : public juce::Component
{
public:
    ToggleSwitch() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }

    void setToggleState(bool shouldBeOn)
    {
        if (isOn != shouldBeOn)
        {
            isOn = shouldBeOn;
            repaint();
        }
    }

    bool getToggleState() const { return isOn; }

    std::function<void(bool)> onStateChange;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        float cornerRadius = bounds.getHeight() / 2.0f;

        // Track background
        g.setColour(isOn ? AutomasterColors::primary.withAlpha(0.3f) : AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Track border
        g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::panelBgLight.brighter(0.2f));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        // Thumb
        float thumbSize = bounds.getHeight() - 4.0f;
        float thumbX = isOn ? bounds.getRight() - thumbSize - 2.0f : bounds.getX() + 2.0f;
        float thumbY = bounds.getCentreY() - thumbSize / 2.0f;

        // Thumb shadow
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillEllipse(thumbX + 1, thumbY + 1, thumbSize, thumbSize);

        // Thumb body
        g.setColour(isOn ? AutomasterColors::primary : AutomasterColors::textSecondary);
        g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

        // Thumb highlight on hover
        if (isMouseOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        isOn = !isOn;
        repaint();
        if (onStateChange)
            onStateChange(isOn);
    }

    void mouseEnter(const juce::MouseEvent&) override { isMouseOver = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { isMouseOver = false; repaint(); }

private:
    bool isOn = false;
    bool isMouseOver = false;
};

// Rainbow button LookAndFeel - sophisticated aurora/synthwave gradient
// Styled button with a solid color (or gradient for rainbow effect)
class StyledButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StyledButtonLookAndFeel(juce::Colour baseColor) : color(baseColor) {}

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 6.0f;

        // Gradient from lighter at top to darker at bottom
        juce::ColourGradient gradient(
            color.brighter(0.2f), bounds.getX(), bounds.getY(),
            color.darker(0.2f), bounds.getX(), bounds.getBottom(),
            false);

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Subtle inner glow at top
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.reduced(2.0f).withHeight(bounds.getHeight() * 0.4f), cornerSize);

        // Darken on press, subtle brighten on hover
        if (isButtonDown)
        {
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else if (isMouseOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }

        // Subtle border
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool, bool) override
    {
        auto bounds = button.getLocalBounds();

        // Bold white text with subtle shadow
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));

        // Subtle shadow
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawText(button.getButtonText(), bounds.translated(1, 1), juce::Justification::centred);

        // Main text
        g.setColour(juce::Colours::white);
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }

private:
    juce::Colour color;
};

// Rainbow/aurora gradient button (legacy alias)
class RainbowButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 6.0f;

        // Sophisticated aurora gradient - deep jewel tones
        juce::ColourGradient gradient(
            juce::Colour(0xFFE040FB), bounds.getX(), bounds.getCentreY(),  // Magenta/pink
            juce::Colour(0xFF00BCD4), bounds.getRight(), bounds.getCentreY(),  // Teal
            false);

        // Rich, sophisticated color stops
        gradient.addColour(0.25, juce::Colour(0xFF7C4DFF));  // Deep purple
        gradient.addColour(0.5, juce::Colour(0xFF448AFF));   // Rich blue
        gradient.addColour(0.75, juce::Colour(0xFF00E5FF));  // Cyan

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Subtle inner glow
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds.reduced(2.0f).withHeight(bounds.getHeight() * 0.4f), cornerSize);

        // Darken on press, subtle brighten on hover
        if (isButtonDown)
        {
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else if (isMouseOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }

        // Subtle border
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool, bool) override
    {
        auto bounds = button.getLocalBounds();

        // Bold white text with subtle shadow
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));

        // Subtle shadow
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawText(button.getButtonText(), bounds.translated(1, 1), juce::Justification::centred);

        // Main text
        g.setColour(juce::Colours::white);
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
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
