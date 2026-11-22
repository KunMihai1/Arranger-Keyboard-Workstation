#pragma once
#include <juce_core/juce_core.h>
#include <vector>

/** Facts of Western music and MIDI notes, shared by the arranger. */
namespace MusicFacts
{
    constexpr int semitonesPerOctave = 12;
    constexpr int lowestMidiNote     = 0;
    constexpr int highestMidiNote    = 127;
}

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

namespace PitchClasses
{
    constexpr int lowest = 0;    // C
    constexpr int none   = -1;   // no chord root / no bass note
}

/** A recognized chord: pitch-class root (0=C..11=B), quality, and the lowest fingered pitch
    class (for Bass Inversion). root=-1 / quality=None means "no chord". */
struct ArrangerChord
{
    int          root     = PitchClasses::none;   // 0..11, -1 = none
    ChordQuality quality  = ChordQuality::None;
    int          bassNote = PitchClasses::none;   // pitch class of lowest note; -1 = use root

    bool isValid() const { return root >= PitchClasses::lowest && quality != ChordQuality::None; }
    bool operator== (const ArrangerChord& other) const
        { return root == other.root && quality == other.quality && bassNote == other.bassNote; }
    bool operator!= (const ArrangerChord& other) const { return ! (*this == other); }
};

/** Chord quality names as written to style files: the text must never change. */
namespace ChordQualityNames
{
    inline const juce::String majorChord     = "Maj";
    inline const juce::String minorChord     = "Min";
    inline const juce::String dominantSeventhChord    = "Dom7";
    inline const juce::String majorSeventhChord    = "Maj7";
    inline const juce::String minorSeventhChord    = "Min7";
    inline const juce::String diminishedChord     = "Dim";
    inline const juce::String halfDiminishedChord = "HalfDim";
    inline const juce::String augmentedChord     = "Aug";
    inline const juce::String suspendedSecondChord    = "Sus2";
    inline const juce::String suspendedFourthChord    = "Sus4";
    inline const juce::String noChord    = "None";
}

inline juce::String toString (ChordQuality quality)
{
    using namespace ChordQualityNames;
    switch (quality) {
        case ChordQuality::Maj:     return majorChord;
        case ChordQuality::Min:     return minorChord;
        case ChordQuality::Dom7:    return dominantSeventhChord;
        case ChordQuality::Maj7:    return majorSeventhChord;
        case ChordQuality::Min7:    return minorSeventhChord;
        case ChordQuality::Dim:     return diminishedChord;
        case ChordQuality::HalfDim: return halfDiminishedChord;
        case ChordQuality::Aug:     return augmentedChord;
        case ChordQuality::Sus2:    return suspendedSecondChord;
        case ChordQuality::Sus4:    return suspendedFourthChord;
        case ChordQuality::None:    return noChord;
    }
    return noChord;
}

inline ChordQuality chordQualityFromString (const juce::String& name)
{
    using namespace ChordQualityNames;
    if (name == majorChord)     return ChordQuality::Maj;
    if (name == minorChord)     return ChordQuality::Min;
    if (name == dominantSeventhChord)    return ChordQuality::Dom7;
    if (name == majorSeventhChord)    return ChordQuality::Maj7;
    if (name == minorSeventhChord)    return ChordQuality::Min7;
    if (name == diminishedChord)     return ChordQuality::Dim;
    if (name == halfDiminishedChord) return ChordQuality::HalfDim;
    if (name == augmentedChord)     return ChordQuality::Aug;
    if (name == suspendedSecondChord)    return ChordQuality::Sus2;
    if (name == suspendedFourthChord)    return ChordQuality::Sus4;
    return ChordQuality::None;
}

/** Semitones above the root (the root itself at 0) of each chord quality. */
namespace ChordShapes
{
    inline const std::vector<int> majorTriad          { 0, 4, 7 };
    inline const std::vector<int> minorTriad          { 0, 3, 7 };
    inline const std::vector<int> dominantSeventh     { 0, 4, 7, 10 };
    inline const std::vector<int> majorSeventh        { 0, 4, 7, 11 };
    inline const std::vector<int> minorSeventh        { 0, 3, 7, 10 };
    inline const std::vector<int> diminishedTriad     { 0, 3, 6 };
    inline const std::vector<int> halfDiminished      { 0, 3, 6, 10 };
    inline const std::vector<int> augmentedTriad      { 0, 4, 8 };
    inline const std::vector<int> suspendedSecond     { 0, 2, 7 };
    inline const std::vector<int> suspendedFourth     { 0, 5, 7 };
}

/** Root-relative semitone intervals (incl. the root at 0) that define each quality.
    Shared by ChordDetector (recognition templates) and ChordTransposer (NTT chord tones) so the
    two can never drift apart. */
inline std::vector<int> chordIntervals (ChordQuality quality)
{
    using namespace ChordShapes;
    switch (quality) {
        case ChordQuality::Maj:     return majorTriad;
        case ChordQuality::Min:     return minorTriad;
        case ChordQuality::Dom7:    return dominantSeventh;
        case ChordQuality::Maj7:    return majorSeventh;
        case ChordQuality::Min7:    return minorSeventh;
        case ChordQuality::Dim:     return diminishedTriad;
        case ChordQuality::HalfDim: return halfDiminished;
        case ChordQuality::Aug:     return augmentedTriad;
        case ChordQuality::Sus2:    return suspendedSecond;
        case ChordQuality::Sus4:    return suspendedFourth;
        case ChordQuality::None:    return majorTriad;   // safe default
    }
    return majorTriad;
}
