#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include "Chord.h"     // PartKind
#include <vector>
#include <set>
#include <map>
#include <utility>

/** One event to emit, positioned at a monotonic beat within the advance window. */
struct EmittedEvent
{
    double beats = 0.0;
    juce::MidiMessage message;
    PartKind part = PartKind::Fixed;   // Phase 4: how the engine should transpose this note (Fixed = not)
    NttType  ntt  = NttType::Chord;    // Phase 7a: the track's NTT type for this note
};

/**
 * Pure, deterministic loop scheduler. Holds a section's events (in beats within
 * [0, loopLengthBeats)) and, on each advance(from, to), returns the events to emit,
 * wrapping the loop and closing any still-sounding notes at every seam so nothing hangs.
 * No threading, no I/O — fully unit-testable.
 */
class ArrangerScheduler
{
public:
    /** Legacy: every event is non-transposable (PartKind::Fixed). */
    void setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats);
    /** Phase 4: each event carries a PartKind (parallel to `events`) so the engine knows how to
        transpose it. `parts` shorter than `events` pads with Fixed. */
    void setLoop (std::vector<TimedBeatEvent> events, std::vector<PartKind> parts, double loopLengthBeats);
    /** Phase 7a: each event also carries an NttType (parallel to `events`). Shorter vectors pad with
        Fixed / Chord respectively. */
    void setLoop (std::vector<TimedBeatEvent> events, std::vector<PartKind> parts,
                  std::vector<NttType> ntts, double loopLengthBeats);
    void reset();   // forget which notes are currently sounding

    /** Emit note-offs for every currently-sounding note (used when switching sections
        mid-loop, where the normal loop-seam close would not fire). Clears active-note state. */
    std::vector<EmittedEvent> flushActiveNotes (double atBeats);

    /** Advance the monotonic playhead from `fromBeats` to `toBeats` (from <= to). */
    std::vector<EmittedEvent> advance (double fromBeats, double toBeats);

private:
    void trackActiveNote (const juce::MidiMessage& m, PartKind part, NttType ntt);

    std::vector<TimedBeatEvent> sortedEvents;          // sorted by beats
    std::vector<PartKind>       sortedParts;           // aligned with sortedEvents
    std::vector<NttType>        sortedNtt;             // aligned with sortedEvents
    double loopLen = 0.0;
    std::set<std::pair<int,int>> activeNotes;          // (channel, noteNumber) currently sounding
    std::map<std::pair<int,int>, PartKind> activeNoteParts;   // part of each sounding note (for seam offs)
    std::map<std::pair<int,int>, NttType>  activeNoteNtt;     // ntt  of each sounding note (for seam offs)
};
