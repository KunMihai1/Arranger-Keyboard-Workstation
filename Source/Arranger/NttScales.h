#pragma once
#include "Chord.h"
#include <array>

/** A snap scale: `count` valid semitone degrees (from the active chord root) in `degrees`. Only the
    first `count` entries are meaningful. Data-only, allocation-free; read on the engine timer thread. */
struct NttScale { int count = 7; std::array<int,8> degrees { { 0,2,4,5,7,9,11,0 } }; };

/** The curated minor scale for Min/Min7 (the one user-selectable row). */
inline NttScale minorScaleFor (MinorScaleChoice c)
{
    switch (c)
    {
        case MinorScaleChoice::Aeolian:       return { 7, { { 0,2,3,5,7,8,10,0 } } };
        case MinorScaleChoice::HarmonicMinor: return { 7, { { 0,2,3,5,7,8,11,0 } } };
        case MinorScaleChoice::Dorian:
        default:                              return { 7, { { 0,2,3,5,7,9,10,0 } } };
    }
}

/** Scale used to snap passing tones, chosen by the held (active) chord's quality. */
inline NttScale scaleForQuality (ChordQuality q, MinorScaleChoice minor)
{
    switch (q)
    {
        case ChordQuality::Maj:
        case ChordQuality::Maj7:    return { 7, { { 0,2,4,5,7,9,11,0 } } };   // Ionian
        case ChordQuality::Dom7:
        case ChordQuality::Sus2:
        case ChordQuality::Sus4:    return { 7, { { 0,2,4,5,7,9,10,0 } } };   // Mixolydian
        case ChordQuality::Min:
        case ChordQuality::Min7:    return minorScaleFor (minor);
        case ChordQuality::Dim:     return { 8, { { 0,2,3,5,6,8,9,11 } } };   // whole-half diminished
        case ChordQuality::HalfDim: return { 7, { { 0,1,3,5,6,8,10,0 } } };   // Locrian
        case ChordQuality::Aug:     return { 6, { { 0,2,4,6,8,10,0,0 } } };   // whole-tone
        case ChordQuality::None:
        default:                    return { 7, { { 0,2,4,5,7,9,11,0 } } };
    }
}
