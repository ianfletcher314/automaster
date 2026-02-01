#pragma once

#include "LookAndFeel.h"
#include "../DSP/MasteringChain.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ProcessingChainView : public juce::Component
{
public:
    // Module identifiers
    enum class Module { EQ, Compressor, Stereo, Limiter };

    ProcessingChainView()
    {
        // Add module buttons
        for (int i = 0; i < 4; ++i)
        {
            auto& btn = moduleButtons[i];
            btn.setClickingTogglesState(false);
            btn.onClick = [this, i]() { selectModule(static_cast<Module>(i)); };
            addAndMakeVisible(btn);
        }

        moduleButtons[0].setButtonText("EQ");
        moduleButtons[1].setButtonText("COMP");
        moduleButtons[2].setButtonText("STEREO");
        moduleButtons[3].setButtonText("LIMITER");

        // Set initial colors
        updateButtonColors();
    }

    void setChain(MasteringChain* chain)
    {
        masteringChain = chain;
    }

    void selectModule(Module mod)
    {
        selectedModule = mod;
        updateButtonColors();

        if (onModuleSelected)
            onModuleSelected(mod);
    }

    Module getSelectedModule() const { return selectedModule; }

    std::function<void(Module)> onModuleSelected;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(AutomasterColors::panelBg);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Chain flow visualization
        auto flowBounds = bounds.reduced(10.0f, 30.0f);
        float moduleWidth = flowBounds.getWidth() / 4.0f;

        // Draw connecting lines
        g.setColour(AutomasterColors::panelBgLight);
        for (int i = 0; i < 3; ++i)
        {
            float x1 = flowBounds.getX() + (i + 0.8f) * moduleWidth;
            float x2 = flowBounds.getX() + (i + 1.2f) * moduleWidth;
            float y = flowBounds.getCentreY();

            // Arrow
            g.drawLine(x1, y, x2, y, 2.0f);

            // Arrowhead
            juce::Path arrow;
            arrow.addTriangle(x2 - 8.0f, y - 4.0f, x2 - 8.0f, y + 4.0f, x2, y);
            g.fillPath(arrow);
        }

        // Draw module boxes
        for (int i = 0; i < 4; ++i)
        {
            auto moduleBounds = juce::Rectangle<float>(
                flowBounds.getX() + i * moduleWidth + 5.0f,
                flowBounds.getY(),
                moduleWidth - 10.0f,
                flowBounds.getHeight()
            );

            // Module background
            juce::Colour moduleColor;
            switch (static_cast<Module>(i))
            {
                case Module::EQ:         moduleColor = AutomasterColors::eqColor; break;
                case Module::Compressor: moduleColor = AutomasterColors::compColor; break;
                case Module::Stereo:     moduleColor = AutomasterColors::stereoColor; break;
                case Module::Limiter:    moduleColor = AutomasterColors::limiterColor; break;
            }

            bool isSelected = (selectedModule == static_cast<Module>(i));
            bool isBypassed = isModuleBypassed(static_cast<Module>(i));

            g.setColour(isBypassed ? AutomasterColors::panelBgLight :
                        (isSelected ? moduleColor.withAlpha(0.4f) : moduleColor.withAlpha(0.2f)));
            g.fillRoundedRectangle(moduleBounds, 4.0f);

            // Border
            g.setColour(isSelected ? moduleColor : moduleColor.withAlpha(0.5f));
            g.drawRoundedRectangle(moduleBounds, 4.0f, isSelected ? 2.0f : 1.0f);

            // Module indicator (active/bypass)
            float indicatorY = moduleBounds.getBottom() + 3.0f;
            g.setColour(isBypassed ? AutomasterColors::textMuted : moduleColor);
            g.fillEllipse(moduleBounds.getCentreX() - 3.0f, indicatorY, 6.0f, 6.0f);
        }

        // Title
        g.setColour(AutomasterColors::textSecondary);
        g.setFont(10.0f);
        g.drawText("SIGNAL CHAIN", bounds.getX(), bounds.getY() + 3.0f,
                   bounds.getWidth(), 15.0f, juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10, 5);
        float buttonWidth = bounds.getWidth() / 4.0f;

        for (int i = 0; i < 4; ++i)
        {
            moduleButtons[i].setBounds(
                bounds.getX() + static_cast<int>(i * buttonWidth),
                bounds.getBottom() - 25,
                static_cast<int>(buttonWidth) - 5,
                22
            );
        }
    }

private:
    bool isModuleBypassed(Module mod) const
    {
        if (masteringChain == nullptr)
            return false;

        switch (mod)
        {
            case Module::EQ:         return masteringChain->getEQ().isBypassed();
            case Module::Compressor: return masteringChain->getCompressor().isBypassed();
            case Module::Stereo:     return masteringChain->getStereoImager().isBypassed();
            case Module::Limiter:    return masteringChain->getLimiter().isBypassed();
        }
        return false;
    }

    void updateButtonColors()
    {
        for (int i = 0; i < 4; ++i)
        {
            bool isSelected = (selectedModule == static_cast<Module>(i));

            juce::Colour moduleColor;
            switch (static_cast<Module>(i))
            {
                case Module::EQ:         moduleColor = AutomasterColors::eqColor; break;
                case Module::Compressor: moduleColor = AutomasterColors::compColor; break;
                case Module::Stereo:     moduleColor = AutomasterColors::stereoColor; break;
                case Module::Limiter:    moduleColor = AutomasterColors::limiterColor; break;
            }

            moduleButtons[i].setColour(juce::TextButton::buttonColourId,
                isSelected ? moduleColor : AutomasterColors::panelBgLight);
            moduleButtons[i].setColour(juce::TextButton::textColourOffId,
                isSelected ? AutomasterColors::textPrimary : AutomasterColors::textSecondary);
        }

        repaint();
    }

    Module selectedModule = Module::EQ;
    std::array<juce::TextButton, 4> moduleButtons;
    MasteringChain* masteringChain = nullptr;
};
