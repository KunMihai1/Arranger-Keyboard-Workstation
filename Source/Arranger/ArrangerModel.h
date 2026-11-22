#pragma once
#include <JuceHeader.h>
#include "Chord.h"     // ChordQuality (style-level original chord)
#include <vector>

/** Schema version for arranger styles, bumped when the model changes. */
static constexpr int ARRANGER_SCHEMA_VERSION = 2;

/** Defaults and facts shared by every arranger class. */
namespace ArrangerStyleDefaults
{
    constexpr double tempoBpm          = 120.0;   // BPM used when a style or a caller gives none
    constexpr int    beatsPerBar       = 4;       // 4/4
    constexpr int    beatUnit          = 4;
    constexpr double quarterNotesPerWholeNote = 4.0;
    constexpr double secondsPerMinute  = 60.0;
    constexpr int    minimumLengthBars = 1;
    constexpr int    firstBar          = 1;       // bars are counted from 1 in the UI and the style file
    constexpr double sectionStartBeat  = 0.0;
    constexpr double fullVolume        = 100.0;   // a track's volume, 0..100 (sent as CC7 0..127)
    constexpr int    firstChannel      = 1;
    constexpr int    noInstrument      = -1;      // keep the device's current program
    constexpr int    noIndex           = -1;      // no section / scheduler selected
}

/** Names of the sections a new style starts with (written to .style files). */
namespace DefaultSectionNames
{
    inline const juce::String variation   = "Variation 1";
    inline const juce::String variationId = "var_1";
    inline const juce::String intro       = "Intro 1";
    inline const juce::String introId     = "intro_1";
    inline const juce::String fill        = "Fill 1";
    inline const juce::String fillId      = "fill_1";
    inline const juce::String ending      = "Ending 1";
    inline const juce::String endingId    = "ending_1";
}

/** A melodic track whose name contains this (any case) is the bass part. */
namespace TrackRoles
{
    inline const juce::String bassNameMarker = "bass";
}

/** How close two beat positions must be to count as the same, for floating-point beat maths. */
namespace BeatTolerance
{
    constexpr double sameBeat     = 1e-9;    // positions, bar boundaries
    constexpr double windowEdge   = 1e-12;   // end of a processing window
    constexpr double wholeBars    = 1e-6;    // rounding a beat count up to whole bars
}

/** Musical role of a track (drives transposition behaviour in later phases). */
enum class ArrangerPartType { Drum, Perc, Bass, Acc };

/** Style-element type (the section buttons). */
enum class ArrangerSectionType { Intro, Variation, Fill, Break, Ending, CountIn };

/** What a section does when its own length elapses: loop forever, fall through to
    the return variation (Intro/Fill), or stop playback (Ending). */
enum class ArrangerAfterComplete { Loop, FallThrough, Stop };

/** One timestamped MIDI event, positioned in beats within its section's loop. */
struct TimedBeatEvent
{
    double beats = ArrangerStyleDefaults::sectionStartBeat;   // [0, loopLengthBeats)
    juce::MidiMessage message;
};

/** One accompaniment track within a section. */
struct ArrangerTrack
{
    juce::String id;
    juce::String name;
    ArrangerPartType partType = ArrangerPartType::Acc;
    int instrument = ArrangerStyleDefaults::noInstrument;
    int channel = ArrangerStyleDefaults::firstChannel;
    double volume = ArrangerStyleDefaults::fullVolume;
    NttType nttType = NttType::Chord;      // Phase 7a: per-track transposition type

    std::vector<TimedBeatEvent> pattern;   // Phase 1: the section's loop for this track
};

/** One style element / section (e.g. Variation 1), an independent loop. */
struct ArrangerSection
{
    juce::String id;
    juce::String name;
    ArrangerSectionType type = ArrangerSectionType::Variation;
    int lengthBars = ArrangerStyleDefaults::minimumLengthBars;   // this section's own loop length
    ArrangerAfterComplete afterComplete = ArrangerAfterComplete::Loop; // Phase 2 transport rule
    std::vector<ArrangerTrack> tracks;
};

/** A whole arranger style. */
struct ArrangerStyle
{
    int schemaVersion = ARRANGER_SCHEMA_VERSION;
    juce::String id;
    juce::String name;
    double originalTempo = ArrangerStyleDefaults::tempoBpm;   // BPM patterns were authored at
    int timeSigNum = ArrangerStyleDefaults::beatsPerBar;
    int timeSigDenom = ArrangerStyleDefaults::beatUnit;

    // Phase 4: the chord/key the accompaniment was recorded in (the "from" reference the NTT maps
    // out of). Defaults to C major; auto-detected at authoring and editable, persisted in the .style.
    int          originalRoot    = PitchClasses::lowest;   // 0=C .. 11=B
    ChordQuality originalQuality = ChordQuality::Maj;

    std::vector<ArrangerSection> sections;
};
