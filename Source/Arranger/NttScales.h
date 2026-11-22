#pragma once
#include "Chord.h"
#include <array>

namespace ScaleSizes
{
    constexpr int wholeTone   = 6;
    constexpr int heptatonic  = 7;
    constexpr int octatonic   = 8;
    constexpr int maxDegrees  = 8;   // room for the largest scale
}

/** Scale degrees in semitones above the chord root; unused trailing entries are 0. */
namespace ScaleDegrees
{
    using Degrees = std::array<int, ScaleSizes::maxDegrees>;
    constexpr Degrees ionian           { { 0,2,4,5,7,9,11,0 } };
    constexpr Degrees mixolydian       { { 0,2,4,5,7,9,10,0 } };
    constexpr Degrees dorian           { { 0,2,3,5,7,9,10,0 } };
    constexpr Degrees aeolian          { { 0,2,3,5,7,8,10,0 } };
    constexpr Degrees harmonicMinor    { { 0,2,3,5,7,8,11,0 } };
    constexpr Degrees wholeHalfDiminished { { 0,2,3,5,6,8,9,11 } };
    constexpr Degrees locrian          { { 0,1,3,5,6,8,10,0 } };
    constexpr Degrees wholeTone        { { 0,2,4,6,8,10,0,0 } };
}

/** A snap scale: `count` valid semitone degrees (from the active chord root) in `degrees`. Only the
    first `count` entries are meaningful. Data-only, allocation-free; read on the engine timer thread. */
struct NttScale { int count = ScaleSizes::heptatonic; ScaleDegrees::Degrees degrees = ScaleDegrees::ionian; };

/** The curated minor scale for Min/Min7 (the one user-selectable row). */
inline NttScale minorScaleFor (MinorScaleChoice choice)
{
    switch (choice)
    {
        case MinorScaleChoice::Aeolian:       return { ScaleSizes::heptatonic, ScaleDegrees::aeolian };
        case MinorScaleChoice::HarmonicMinor: return { ScaleSizes::heptatonic, ScaleDegrees::harmonicMinor };
        case MinorScaleChoice::Dorian:
        default:                              return { ScaleSizes::heptatonic, ScaleDegrees::dorian };
    }
}

/** Scale used to snap passing tones, chosen by the held (active) chord's quality. */
inline NttScale scaleForQuality (ChordQuality quality, MinorScaleChoice minor)
{
    switch (quality)
    {
        case ChordQuality::Maj:
        case ChordQuality::Maj7:    return { ScaleSizes::heptatonic, ScaleDegrees::ionian };
        case ChordQuality::Dom7:
        case ChordQuality::Sus2:
        case ChordQuality::Sus4:    return { ScaleSizes::heptatonic, ScaleDegrees::mixolydian };
        case ChordQuality::Min:
        case ChordQuality::Min7:    return minorScaleFor (minor);
        case ChordQuality::Dim:     return { ScaleSizes::octatonic,  ScaleDegrees::wholeHalfDiminished };
        case ChordQuality::HalfDim: return { ScaleSizes::heptatonic, ScaleDegrees::locrian };
        case ChordQuality::Aug:     return { ScaleSizes::wholeTone,  ScaleDegrees::wholeTone };
        case ChordQuality::None:
        default:                    return { ScaleSizes::heptatonic, ScaleDegrees::ionian };
    }
}
