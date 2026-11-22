#pragma once
#include <JuceHeader.h>
#include "AppColours.h"

class PlayScreenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PlayScreenLookAndFeel()
    {
        setColour(juce::TextButton::textColourOffId,        AppColours::titleText);
        setColour(juce::TextButton::textColourOnId,         AppColours::titleText);
        setColour(juce::TextButton::buttonColourId,         AppColours::accent2);
        setColour(juce::TextButton::buttonOnColourId,       AppColours::accent3);
        setColour(juce::ToggleButton::textColourId,         AppColours::titleText);
        setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
        setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);
    }

    void drawButtonBackground(juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override
    {
        constexpr float edgeInset        = 0.5f;   // keeps the outline on whole pixels
        constexpr float cornerRadius     = 6.0f;
        constexpr float pressShade       = 0.15f;
        constexpr float hoverOutlineAlpha = 1.0f;
        constexpr float idleOutlineAlpha  = 0.75f;
        constexpr float outlineThickness = 1.5f;
        auto bounds = button.getLocalBounds().toFloat().reduced(edgeInset);

        juce::Colour fill = AppColours::cardBg;
        if (isButtonDown)
            fill = fill.darker(pressShade);
        else if (isMouseOverButton)
            fill = fill.brighter(pressShade);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(backgroundColour.withAlpha(isMouseOverButton ? hoverOutlineAlpha : idleOutlineAlpha));
        g.drawRoundedRectangle(bounds, cornerRadius, outlineThickness);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override
    {
        constexpr float buttonFontHeight = 13.0f;
        return juce::Font(buttonFontHeight);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayScreenLookAndFeel)
};
