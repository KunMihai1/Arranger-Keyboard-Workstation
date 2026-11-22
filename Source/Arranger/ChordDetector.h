#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Chord.h"
#include "ArrangerModel.h"   // TimedBeatEvent
#include <set>
#include <vector>

/**
 * Tracks the notes held in the chord-recognition zone and reports the recognized ArrangerChord.
 *
 * Split mode (default): every held note is assumed to be a chord tone, so the held
 * pitch-class set is matched directly against interval templates (Fingered recognition).
 * Full-Keyboard mode: held notes mix chord + melody, so recognition only updates when >=3
 * notes form a known chord (trying the whole set, then the lowest 3-4), and otherwise the last
 * recognized chord is held (hysteresis) so a passing melody note never re-triggers the accomp.
 *
 * Pure: no JUCE GUI/threads. Lives on the MIDI input thread; only its ArrangerChord result crosses to
 * the engine (via a mailbox), never its internal state.
 */
class ChordDetector
{
public:
    void  noteOn  (int noteNumber);
    void  noteOff (int noteNumber);
    void  reset();

    void  setMode (ChordMode newMode) { mode = newMode; recompute(); }
    ChordMode getMode() const { return mode; }

    /** Best chord for the currently-held notes, or ArrangerChord::none (root=-1) if unrecognized. */
    ArrangerChord current() const { return recognized; }

    /** Whether any notes are physically held in the chord zone right now. Distinguishes "a chord is
        being held" from "Chord Memory is still remembering a released chord". */
    bool hasHeldNotes() const { return ! heldNotes.empty(); }

private:
    void  recompute();                                   // re-run recognition after a held-note change
    static ArrangerChord recognizeSet (const std::set<int>& notes);   // direct template match on a note set
    static ArrangerChord recognizeSingleFinger (const std::set<int>& notes);   // Korg one-finger: 1-2 keys

    std::set<int> heldNotes;     // absolute note numbers
    ChordMode     mode = ChordMode::Fingered;
    ArrangerChord         recognized;    // current recognized chord (held across non-matches in full mode)
};

/** Offline key/chord finder for authoring: pitch-class histogram of an accompaniment's note-ons â†’
    best root + major/minor. Empty/ambiguous input falls back to C major. Not used in playback. */
ArrangerChord detectKeyFromEvents (const std::vector<TimedBeatEvent>& events);
