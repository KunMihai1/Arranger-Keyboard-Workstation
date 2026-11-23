/*
  ==============================================================================

    StagePageFrame.h

    A page over the note area (spec 2026-11-23 §6, board StageRevised .page):
    page surface, silkscreen title with a rule, and a content area. Phase 1
    uses it for the "legacy" pages that host today's controls until each page
    is redesigned; the owner of the frame lays its children out in
    onLayoutContent.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"

class StagePageFrame : public juce::Component
{
public:
    explicit StagePageFrame (const juce::String& title);

    void setScale (float newScale);
    juce::String getTitle() const;

    /** Where the page's controls go: inside the page padding, below the title row. */
    juce::Rectangle<int> getContentBounds() const;

    /** Lays the page's children out in the content area at the given scale. */
    std::function<void (juce::Rectangle<int> contentArea, float scale)> onLayoutContent;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    float scaled (int referenceLength) const;
    float titleHeight() const;

    juce::String title;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StagePageFrame)
};
