#pragma once
#include <JuceHeader.h>
#include "ArrangerChannels.h"
#include "ArrangerModel.h"   // ArrangerPartType, ArrangerSectionType, ArrangerAfterComplete, TimedBeatEvent
#include <vector>

namespace StyleFileDefaults
{
    constexpr int schemaVersion      = 5;   // v5 adds per-track nttType (v4: style-level original chord)
}

/** One source track in a saved style: events stored ONCE, beats measured from bar 1. */
struct SourceTrackFile
{
    juce::String id, name;
    ArrangerPartType partType = ArrangerPartType::Acc;
    int    channel    = ArrangerChannels::kFirstMelodicChannel;
    int    instrument = ArrangerStyleDefaults::noInstrument;
    double volume     = ArrangerStyleDefaults::fullVolume;
    NttType nttType   = NttType::Chord;   // Phase 7a; default overridden by part-derived migration
    std::vector<TimedBeatEvent> events;   // note-on/off only
    // effects slot intentionally omitted in Part A (reserved for Phase 4+)
};

/** One section as a bar-window into the source tracks. */
struct SectionWindow
{
    juce::String id, name;
    ArrangerSectionType   type          = ArrangerSectionType::Variation;
    int                   startBar       = ArrangerStyleDefaults::firstBar;   // 1-based
    int                   lengthBars     = ArrangerStyleDefaults::minimumLengthBars;
    ArrangerAfterComplete afterComplete  = ArrangerAfterComplete::Loop;
};

/** A whole self-contained arranger style as stored on disk. */
struct ArrangerStyleFile
{
    int schemaVersion = StyleFileDefaults::schemaVersion;
    juce::String id, name;
    double originalTempo = ArrangerStyleDefaults::tempoBpm;
    int timeSigNum = ArrangerStyleDefaults::beatsPerBar, timeSigDenom = ArrangerStyleDefaults::beatUnit;
    int          originalRoot    = PitchClasses::lowest;   // Phase 4: recorded key root (0=C..11=B)
    ChordQuality originalQuality = ChordQuality::Maj;  // ...and quality; default C major
    std::vector<SourceTrackFile> sourceTracks;
    std::vector<SectionWindow>   sections;
};
