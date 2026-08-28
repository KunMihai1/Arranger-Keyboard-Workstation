#pragma once
#include <JuceHeader.h>
#include "Chord.h"     // ChordQuality (style-level original chord)
#include <vector>

/** Schema version for arranger styles, bumped when the model changes. */
static constexpr int ARRANGER_SCHEMA_VERSION = 2;

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
    double beats = 0.0;            // [0, loopLengthBeats)
    juce::MidiMessage message;
};

/** One accompaniment track within a section. */
struct ArrangerTrack
{
    juce::String id;
    juce::String name;
    ArrangerPartType partType = ArrangerPartType::Acc;
    int instrument = -1;
    int channel = 1;
    double volume = 100.0;
    NttType nttType = NttType::Chord;      // Phase 7a: per-track transposition type

    std::vector<TimedBeatEvent> pattern;   // Phase 1: the section's loop for this track
};

/** One style element / section (e.g. Variation 1), an independent loop. */
struct ArrangerSection
{
    juce::String id;
    juce::String name;
    ArrangerSectionType type = ArrangerSectionType::Variation;
    int lengthBars = 1;                    // this section's own loop length
    ArrangerAfterComplete afterComplete = ArrangerAfterComplete::Loop; // Phase 2 transport rule
    std::vector<ArrangerTrack> tracks;
};

/** A whole arranger style. */
struct ArrangerStyle
{
    int schemaVersion = ARRANGER_SCHEMA_VERSION;
    juce::String id;
    juce::String name;
    double originalTempo = 120.0;          // BPM patterns were authored at
    int timeSigNum = 4;
    int timeSigDenom = 4;

    // Phase 4: the chord/key the accompaniment was recorded in (the "from" reference the NTT maps
    // out of). Defaults to C major; auto-detected at authoring and editable, persisted in the .style.
    int          originalRoot    = 0;                  // 0=C .. 11=B
    ChordQuality originalQuality = ChordQuality::Maj;

    std::vector<ArrangerSection> sections;
};
