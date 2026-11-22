#include "ChordTransposer.h"
#include <juce_core/juce_core.h>
#include <cstdlib>

namespace
{
    using MusicFacts::semitonesPerOctave;

    constexpr int degreeNotFound     = -1;
    constexpr int fartherThanAnyNote = 1000;   // starting "best distance" before any candidate
    constexpr int halfOctave         = 6;      // Fixed NTT: within this many semitones above, stay up
    constexpr int sameRoot           = 0;

    int pitchClassOf (int noteNumber) { return ((noteNumber % semitonesPerOctave) + semitonesPerOctave) % semitonesPerOctave; }

    int clampMidi (int noteNumber)
    {
        while (noteNumber < MusicFacts::lowestMidiNote)  noteNumber += semitonesPerOctave;
        while (noteNumber > MusicFacts::highestMidiNote) noteNumber -= semitonesPerOctave;
        return noteNumber;
    }

    int indexOfDegree (const std::vector<int>& tones, int degree)
    {
        for (int toneIndex {}; toneIndex < (int) tones.size(); ++toneIndex) if (tones[toneIndex] == degree) return toneIndex;
        return degreeNotFound;
    }
}

int ChordTransposer::snapToScale (int noteNumber) const
{
    const NttScale scale = scaleForQuality (active.quality, minorScale);
    const int pitchClass = pitchClassOf (noteNumber);
    int bestNote = noteNumber, bestDistance = fartherThanAnyNote;
    for (int degreeIndex {}; degreeIndex < scale.count; ++degreeIndex)
    {
        const int scalePitchClass = pitchClassOf (active.root + scale.degrees[degreeIndex]);
        const int noteAbove  = noteNumber + pitchClassOf (scalePitchClass - pitchClass);   // nearest note >= noteNumber with class scalePitchClass
        for (int candidate : { noteAbove, noteAbove - semitonesPerOctave })
        {
            const int distance = std::abs (candidate - noteNumber);
            if (distance < bestDistance || (distance == bestDistance && candidate < bestNote)) { bestNote = candidate; bestDistance = distance; }
        }
    }
    return bestNote;
}

int ChordTransposer::wrapToOctave (int origin, int noteNumber) const
{
    while (noteNumber - origin > semitonesPerOctave) noteNumber -= semitonesPerOctave;   // more than an octave above -> fold down
    while (origin - noteNumber > semitonesPerOctave) noteNumber += semitonesPerOctave;   // more than an octave below -> fold up
    return clampMidi (noteNumber);
}

int ChordTransposer::transpose (int noteNumber, PartKind part, NttType ntt) const
{
    if (part == PartKind::Fixed || ! active.isValid() || ! original.isValid())
        return noteNumber;

    // Master bypass: play the recorded home key, no exceptions. This must short-circuit the WHOLE
    // function rather than rewrite `ntt`, because the bass-inversion branch below returns early and
    // would otherwise never see the bypass — leaving the bass transposed while every other part
    // stayed in the recorded key.
    if (! nttEnabled)
        return noteNumber;

    // Bass inversion (slash chords) layers on top, independent of NTT *type*: re-base the WHOLE bass
    // line onto the played bass note, preserving its shape (a C/E voicing moves the line down to E).
    // Independent of the per-track type, but NOT of the master switch above.
    if (part == PartKind::Bass && bassInversion && active.bassNote >= PitchClasses::lowest)
        return clampMidi (noteNumber + (active.bassNote - original.root));

    if (ntt == NttType::NoTranspose)
        return noteNumber;

    const int interval = active.root - original.root;

    if (ntt == NttType::Parallel)
        return wrapToOctave (noteNumber, noteNumber + interval);

    // Faithful home-key reproduction: when the played chord matches the recorded chord's root &
    // quality, the source is already correct, so return it verbatim instead of snapping its
    // chromatic passing tones (this is how Korg reproduces the source chord). Bass inversion, which
    // can differ while root/quality match, is handled above for the Bass part.
    if (interval == sameRoot && active.quality == original.quality)
        return noteNumber;

    // Chord / Fixed: remap chord-tone roles onto the active chord; snap passing tones to its scale.
    const int  degree  = pitchClassOf (noteNumber - original.root);
    const auto originalTones = chordIntervals (original.quality);
    const auto activeTones  = chordIntervals (active.quality);
    const int  roleIndex  = indexOfDegree (originalTones, degree);

    int result;
    if (roleIndex >= int {} && roleIndex < (int) activeTones.size())
        result = noteNumber + interval + (activeTones[roleIndex] - originalTones[roleIndex]);   // same role of the active chord
    else
        result = snapToScale (noteNumber + interval);              // passing tone -> active scale

    if (ntt == NttType::Fixed)
    {
        // Place the resulting pitch class in the octave nearest the original note (no register drift).
        const int pitchClass = pitchClassOf (result);
        const int noteAbove = noteNumber + pitchClassOf (pitchClass - pitchClassOf (noteNumber));
        result = (noteAbove - noteNumber <= halfOctave) ? noteAbove : noteAbove - semitonesPerOctave;
        return clampMidi (result);
    }

    return wrapToOctave (noteNumber, result);
}
