#include "ChordTransposer.h"
#include <juce_core/juce_core.h>
#include <cstdlib>

namespace
{
    int mod12 (int x) { return ((x % 12) + 12) % 12; }
    int clampMidi (int n) { while (n < 0) n += 12; while (n > 127) n -= 12; return n; }

    int indexOfDegree (const std::vector<int>& tones, int deg)
    {
        for (int i = 0; i < (int) tones.size(); ++i) if (tones[i] == deg) return i;
        return -1;
    }
}

int ChordTransposer::snapToScale (int noteVal) const
{
    const NttScale sc = scaleForQuality (active.quality, minorScale);
    const int pc = mod12 (noteVal);
    int best = noteVal, bestDist = 1000;
    for (int i = 0; i < sc.count; ++i)
    {
        const int spc = mod12 (active.root + sc.degrees[i]);
        const int up  = noteVal + mod12 (spc - pc);   // nearest note >= noteVal with class spc
        for (int cand : { up, up - 12 })
        {
            const int d = std::abs (cand - noteVal);
            if (d < bestDist || (d == bestDist && cand < best)) { best = cand; bestDist = d; }
        }
    }
    return best;
}

int ChordTransposer::wrapToOctave (int origin, int n) const
{
    while (n - origin > 12) n -= 12;   // more than an octave above -> fold down
    while (origin - n > 12) n += 12;   // more than an octave below -> fold up
    return clampMidi (n);
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
    if (part == PartKind::Bass && bassInversion && active.bassNote >= 0)
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
    if (interval == 0 && active.quality == original.quality)
        return noteNumber;

    // Chord / Fixed: remap chord-tone roles onto the active chord; snap passing tones to its scale.
    const int  deg  = mod12 (noteNumber - original.root);
    const auto orig = chordIntervals (original.quality);
    const auto act  = chordIntervals (active.quality);
    const int  idx  = indexOfDegree (orig, deg);

    int result;
    if (idx >= 0 && idx < (int) act.size())
        result = noteNumber + interval + (act[idx] - orig[idx]);   // same role of the active chord
    else
        result = snapToScale (noteNumber + interval);              // passing tone -> active scale

    if (ntt == NttType::Fixed)
    {
        // Place the resulting pitch class in the octave nearest the original note (no register drift).
        const int pc = mod12 (result);
        const int up = noteNumber + mod12 (pc - mod12 (noteNumber));
        result = (up - noteNumber <= 6) ? up : up - 12;
        return clampMidi (result);
    }

    return wrapToOctave (noteNumber, result);
}
