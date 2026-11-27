/*
  ==============================================================================

    StagePageFrame.h

    A page over the note area (spec 2026-11-23 §6, boards StageRevised .page,
    PageTracks, PageNotes): page surface and a title row with the page name in
    silkscreen, an optional dim subtitle, a rule, the page's own actions and
    Exit at the right. The owner of the frame lays its children out in the
    content area (onLayoutContent) and hands the title-row actions over with
    setActions (the frame places them; it does not own them).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"
#include "StagePageWidgets.h"

namespace StagePageFrameText
{
    inline const juce::String exit = "Exit";
}

class StagePageFrame : public juce::Component
{
public:
    explicit StagePageFrame (const juce::String& title);

    void setScale (float newScale);
    void setTitle (const juce::String& newTitle);
    juce::String getTitle() const;
    void setSubtitle (const juce::String& newSubtitle);

    /** The page's title-row controls, left to right (SegButton, SilkLabel, TitleDivider); they sit
        right-aligned before Exit. The previous ones are taken off the frame. */
    void setActions (std::vector<juce::Component*> leftToRight);

    /** The board's gap between title-row items and from the title row to the content: 14 on the Style
        page, 12 on the track picker and notes pages. */
    void setRowGap (int newBoardGap);

    /** Called when Exit is clicked. */
    std::function<void()> onExit;
    SegButton& getExitButton();

    /** Where the page's controls go: inside the page padding, below the title row. */
    juce::Rectangle<int> getContentBounds() const;
    juce::Rectangle<float> getTitleBounds() const;
    juce::Rectangle<float> getSubtitleBounds() const;
    /** The rule from the subtitle to the actions; empty when there is no room for a real line. */
    juce::Rectangle<float> getRuleBounds() const;

    /** Lays the page's children out in the content area at the given scale. */
    std::function<void (juce::Rectangle<int> contentArea, float scale)> onLayoutContent;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    float scaled (int referenceLength) const;
    juce::Rectangle<float> boardBand() const;
    juce::Rectangle<float> titleRow() const;
    juce::Font titleFont() const;
    juce::Font subtitleFont() const;
    float actionWidth (juce::Component& action) const;
    float ruleRight() const;

    juce::String title, subtitle;
    std::vector<juce::Component::SafePointer<juce::Component>> actions;
    SegButton exitButton { StagePageFrameText::exit, SegButton::Style::seg };
    int rowGap { StageMetrics::pageRowGap };
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StagePageFrame)
};
