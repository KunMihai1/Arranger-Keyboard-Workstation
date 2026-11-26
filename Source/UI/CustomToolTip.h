/*
  ==============================================================================

    CustomToolTip.h
    Created: 9 Jul 2025 6:27:48pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "UI/Stage/StageMetrics.h"

/** The app's tooltip look (owner, 2026-11-25): the page colour, a 1 px rule-line border, silkscreen text. */
namespace StageTooltip
{
    inline constexpr int padding = 8;   // px around the text

    /** All at the stage scale (StageMetrics::referenceScale = board px). */
    juce::Font font (float scale = StageMetrics::referenceScale);
    /** Whole-text size plus padding: nothing is cut. */
    juce::Rectangle<int> sizeFor (const juce::String& text, float scale = StageMetrics::referenceScale);
    void paint (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, float scale = StageMetrics::referenceScale);
}

/** JUCE's TooltipWindow (setTooltip) drawn as StageTooltip. */
class StageTooltipLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override;
};

/** A tooltip window for a hovered control (a strip's track name, the Auto Fill toggle), drawn as StageTooltip. */
class CustomToolTip: public juce::Component
{
public:
    explicit CustomToolTip(const juce::String& text, float scale = StageMetrics::referenceScale);

    void paint(juce::Graphics& g) override;
    void setNewText(const juce::String& newText);

private:
    juce::String text;
    float scale;
};
