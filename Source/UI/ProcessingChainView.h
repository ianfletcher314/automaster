#pragma once

#include "LookAndFeel.h"
#include "../DSP/MasteringChain.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ProcessingChainView : public juce::Component
{
public:
    enum class Module { EQ, Compressor, Stereo, Limiter };

    ProcessingChainView()
    {
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

        // Simple dark background
        g.setColour(juce::Colour(0xFF1a1a1a));
        g.fillRoundedRectangle(bounds, 4.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(2, 4);
        int buttonWidth = bounds.getWidth() / 4;
        int gap = 4;

        for (int i = 0; i < 4; ++i)
        {
            moduleButtons[i].setBounds(
                bounds.getX() + i * buttonWidth + gap / 2,
                bounds.getY(),
                buttonWidth - gap,
                bounds.getHeight()
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
            bool isBypassed = isModuleBypassed(static_cast<Module>(i));

            juce::Colour moduleColor;
            switch (static_cast<Module>(i))
            {
                case Module::EQ:         moduleColor = AutomasterColors::eqColor; break;
                case Module::Compressor: moduleColor = AutomasterColors::compColor; break;
                case Module::Stereo:     moduleColor = AutomasterColors::stereoColor; break;
                case Module::Limiter:    moduleColor = AutomasterColors::limiterColor; break;
            }

            if (isBypassed)
                moduleColor = AutomasterColors::textMuted;

            moduleButtons[i].setColour(juce::TextButton::buttonColourId,
                isSelected ? moduleColor : juce::Colour(0xFF2a2a2a));
            moduleButtons[i].setColour(juce::TextButton::buttonOnColourId, moduleColor);
            moduleButtons[i].setColour(juce::TextButton::textColourOffId,
                isSelected ? juce::Colours::white : AutomasterColors::textSecondary);
            moduleButtons[i].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        }

        repaint();
    }

    Module selectedModule = Module::EQ;
    std::array<juce::TextButton, 4> moduleButtons;
    MasteringChain* masteringChain = nullptr;
};
