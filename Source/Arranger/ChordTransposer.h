#pragma once
#include "Chord.h"
#include "NttScales.h"

/**
 * Note Transposition Table (NTT, Phase 7a): maps each accompaniment note from the recorded
 * (original) chord onto the played (active) chord per the track's NttType. Chord-tone roles are
 * remapped to the active chord; passing tones snap to the active chord's scale. Pure: no JUCE
 * GUI/threads, no allocation on the per-note path; read on the engine's timer thread.
 */
class ChordTransposer
{
public:
    void setOriginalChord (ArrangerChord recorded) { original = recorded; }
    void setActiveChord   (ArrangerChord played)   { active = played; }
    void setBassInversion (bool shouldInvert)      { bassInversion = shouldInvert; }
    bool isBassInversionOn() const                 { return bassInversion; }

    void setMinorScale (MinorScaleChoice c)        { minorScale = c; }
    void setNttEnabled (bool enabled)              { nttEnabled = enabled; }

    /** Map one note for a part using the given NTT type. Fixed parts and invalid chords return the
        note unchanged; when NTT is globally disabled every type behaves as NoTranspose. */
    int transpose (int noteNumber, PartKind part, NttType ntt) const;

    /** Back-compat overload (Chord type). */
    int transpose (int noteNumber, PartKind part) const { return transpose (noteNumber, part, NttType::Chord); }

private:
    int snapToScale  (int noteVal) const;
    int wrapToOctave (int origin, int n) const;

    ArrangerChord    original   { 0, ChordQuality::Maj, 0 };   // default C major
    ArrangerChord    active;
    bool             bassInversion = false;
    MinorScaleChoice minorScale    = MinorScaleChoice::Dorian;
    bool             nttEnabled     = true;
};
