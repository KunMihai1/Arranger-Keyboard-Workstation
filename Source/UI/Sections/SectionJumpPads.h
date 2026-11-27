#pragma once
#include <JuceHeader.h>
#include "UI/Stage/StageMetrics.h"
#include <vector>

/*  The jump pads under the Sections lanes (board PageSections .pad): "Jump", then one rubber pad per section
    with its LED (lit while that section plays) and its name. A click reports the pad; the editor selects that
    section and, while previewing, switches to it at the next bar — what a click on its window does. */

namespace SectionJumpPadValues
{
    inline constexpr int noPad = -1;   ///< no section playing
}

class SectionJumpPads : public juce::Component
{
public:
    SectionJumpPads();

    void setPads (const juce::StringArray& names);
    void setPlayingIndex (int index);   ///< the lit LED; out of range: none
    int getPlayingIndex() const { return playingIndex; }
    void setScale (float newScale);

    juce::Rectangle<int> padBounds (int padIndex) const;   ///< the key's box (LED and name above / below it)
    void clickAt (juce::Point<int> position);

    std::function<void (int padIndex)> onPadClicked;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    float scaled (float boardPixels) const { return boardPixels * scale; }
    float padsLeft() const;

    juce::StringArray names;
    int playingIndex { SectionJumpPadValues::noPad };
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionJumpPads)
};
