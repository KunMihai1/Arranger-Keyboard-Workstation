#pragma once
#include <JuceHeader.h>
#include "SectionLanes.h"
#include <vector>

/*  The lane headers left of the Sections lanes (board PageSections .lane-head): "1 Drums" and the track's
    transpose-mode button (.ntt), which cycles None → Parallel → Chord → Fixed. It replaces the editor's strip
    of per-track combos (spec 2026-11-27 sections v2, E5). Empty slots up to eight read "5 —" with no button. */

class SectionLaneHeaders : public juce::Component
{
public:
    struct Track
    {
        juce::String name;   ///< empty: "Track n"
        NttType mode = NttType::Chord;
    };

    SectionLaneHeaders();

    void setTracks (const std::vector<Track>& newTracks);
    const std::vector<Track>& getTracks() const { return tracks; }
    void setScale (float newScale);

    /** A click at `position`: on a track's button, it moves to the next mode and onModeChanged reports it. */
    void clickAt (juce::Point<int> position);
    /** Where track `trackIndex`'s button is drawn and clicked. */
    juce::Rectangle<int> buttonBounds (int trackIndex) const;

    std::function<void (int trackIndex, NttType mode)> onModeChanged;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    juce::Font buttonFont() const;
    juce::String laneLabel (int laneIndex) const;

    std::vector<Track> tracks;
    float scale { SectionLaneMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionLaneHeaders)
};
