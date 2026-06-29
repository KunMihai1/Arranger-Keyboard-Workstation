#pragma once
#include <juce_core/juce_core.h>
#include <vector>

/** Quality of a recognized chord (extensible; None = unrecognized). */
enum class ChordQuality { None, Maj, Min, Dom7, Maj7, Min7, Dim, HalfDim, Aug, Sus2, Sus4 };

/** Musical role of a part for transposition routing.
    Fixed = Drum/Perc (never transposed); Acc = harmony; Bass = bass line (honours bass inversion). */
enum class PartKind { Fixed, Acc, Bass };

/** How the keyboard is scanned/interpreted for chord recognition.
    Fingered = split-zone template match; SingleFinger = Korg one-finger (1-2 keys); FullKeyboard =
    whole-keyboard, bass-driven recognition. */
enum class ChordMode { Fingered, SingleFinger, FullKeyboard };

/** Per-track Note Transposition Table type (Phase 7a). NoTranspose = identity (drums);
    Parallel = uniform root-interval shift; Chord = role-remap chord tones + scale-snap passing
    tones (Korg's "Chord" NTT); Fixed = same as Chord but kept in the octave nearest the original
    note (no register drift). Unrelated to ChordMode despite the shared word. */
enum class NttType { NoTranspose, Parallel, Chord, Fixed };

/** Curated minor scale used to snap passing tones over Min/Min7 chords (Phase 7a). */
enum class MinorScaleChoice { Dorian, Aeolian, HarmonicMinor };

/** A recognized chord: pitch-class root (0=C..11=B), quality, and the lowest fingered pitch
    class (for Bass Inversion). root=-1 / quality=None means "no chord". */
struct ArrangerChord
{
    int          root     = -1;   // 0..11, -1 = none
    ChordQuality quality  = ChordQuality::None;
    int          bassNote = -1;   // pitch class of lowest note; -1 = use root

    bool isValid() const { return root >= 0 && quality != ChordQuality::None; }
    bool operator== (const ArrangerChord& o) const
        { return root == o.root && quality == o.quality && bassNote == o.bassNote; }
    bool operator!= (const ArrangerChord& o) const { return ! (*this == o); }
};

inline juce::String toString (ChordQuality q)
{
    switch (q) {
        case ChordQuality::Maj:     return "Maj";
        case ChordQuality::Min:     return "Min";
        case ChordQuality::Dom7:    return "Dom7";
        case ChordQuality::Maj7:    return "Maj7";
        case ChordQuality::Min7:    return "Min7";
        case ChordQuality::Dim:     return "Dim";
        case ChordQuality::HalfDim: return "HalfDim";
        case ChordQuality::Aug:     return "Aug";
        case ChordQuality::Sus2:    return "Sus2";
        case ChordQuality::Sus4:    return "Sus4";
        case ChordQuality::None:    return "None";
    }
    return "None";
}

inline ChordQuality chordQualityFromString (const juce::String& s)
{
    if (s == "Maj")     return ChordQuality::Maj;
    if (s == "Min")     return ChordQuality::Min;
    if (s == "Dom7")    return ChordQuality::Dom7;
    if (s == "Maj7")    return ChordQuality::Maj7;
    if (s == "Min7")    return ChordQuality::Min7;
    if (s == "Dim")     return ChordQuality::Dim;
    if (s == "HalfDim") return ChordQuality::HalfDim;
    if (s == "Aug")     return ChordQuality::Aug;
    if (s == "Sus2")    return ChordQuality::Sus2;
    if (s == "Sus4")    return ChordQuality::Sus4;
    return ChordQuality::None;
}

/** Root-relative semitone intervals (incl. the root at 0) that define each quality.
    Shared by ChordDetector (recognition templates) and ChordTransposer (NTT chord tones) so the
    two can never drift apart. */
inline std::vector<int> chordIntervals (ChordQuality q)
{
    switch (q) {
        case ChordQuality::Maj:     return {0,4,7};
        case ChordQuality::Min:     return {0,3,7};
        case ChordQuality::Dom7:    return {0,4,7,10};
        case ChordQuality::Maj7:    return {0,4,7,11};
        case ChordQuality::Min7:    return {0,3,7,10};
        case ChordQuality::Dim:     return {0,3,6};
        case ChordQuality::HalfDim: return {0,3,6,10};
        case ChordQuality::Aug:     return {0,4,8};
        case ChordQuality::Sus2:    return {0,2,7};
        case ChordQuality::Sus4:    return {0,5,7};
        case ChordQuality::None:    return {0,4,7};   // safe default
    }
    return {0,4,7};
}
