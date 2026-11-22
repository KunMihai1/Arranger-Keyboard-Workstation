#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include <vector>

/** One contiguous slice of an advance window during which a single section is active.
    `localFromBeats`/`localToBeats` are section-relative (feed the section's scheduler). */
struct SectionSegment
{
    int    sectionIndex   {};
    double localFromBeats = ArrangerStyleDefaults::sectionStartBeat;
    double localToBeats   = ArrangerStyleDefaults::sectionStartBeat;
    bool   sectionChanged = false; // true when a switch happened at this segment's start
};

/** Result of advancing the sequencer. */
struct SequencerStep
{
    std::vector<SectionSegment> segments;
    bool                        stopRequested = false; // an Ending finished
};

/**
 * Pure section state machine. Decides which section is active across a monotonic beat
 * window, applying queued switches and one-shot completions only at bar boundaries.
 * Emits no MIDI; the engine maps segments onto per-section schedulers. Fully unit-testable.
 */
class ArrangerSectionSequencer
{
public:
    void setStyle (const ArrangerStyle& style);   // records metadata, picks initial section
    void reset();                                 // back to the initial section at beat 0
    void startAt (int sectionIndex);              // begin on a specific section (e.g. Intro)

    /** Queue a switch to the section matching type (+name as tiebreaker), applied at the
        next bar boundary. Last call before the boundary wins. False if no such section. */
    bool queue (ArrangerSectionType type, const juce::String& name);

    /** When enabled, queuing a Variation while a Variation plays inserts the Fill of the variation
        being left (Fill N when leaving Variation N, else any Fill) as a one-bar transition into the
        new variation. */
    void setAutoFillEnabled (bool enabled) { autoFillEnabled = enabled; }

    /** Advance the monotonic playhead from `fromBeats` to `toBeats` (from <= to). */
    SequencerStep advance (double fromBeats, double toBeats);

    int    getActiveIndex() const { return activeIndex; }
    double getBeatsPerBar() const { return beatsPerBar; }
    double getActiveStartAbs() const { return activeStartAbs; }  // abs beat the active section began on

private:
    struct Info
    {
        juce::String          id, name;
        ArrangerSectionType   type  = ArrangerSectionType::Variation;
        int                   lengthBars = ArrangerStyleDefaults::minimumLengthBars;
        ArrangerAfterComplete after = ArrangerAfterComplete::Loop;
    };

    int  firstVariationIndex() const;
    int  lastSectionIndex() const;
    int  findSection (ArrangerSectionType type, const juce::String& name) const;
    void applyBoundary (double boundaryAbs, SequencerStep& step);

    std::vector<Info> sections;
    double beatsPerBar    = ArrangerStyleDefaults::beatsPerBar;

    int    activeIndex    {};
    int    returnIndex    {};      // variation to fall back to after Intro/Fill
    double activeStartAbs = ArrangerStyleDefaults::sectionStartBeat;   // absolute beat where the active section began
    int    pendingIndex   = ArrangerStyleDefaults::noIndex;    // queued target, -1 = none
    bool   stopped        = false;
    bool   autoFillEnabled   = false; // insert a Fill when switching variations
    int    fillReturnOverride = ArrangerStyleDefaults::noIndex;   // variation to land on after an auto-fill (-1 = normal fill return)
};
