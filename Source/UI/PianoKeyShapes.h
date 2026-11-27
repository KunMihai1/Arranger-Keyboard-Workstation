#pragma once

#include <juce_graphics/juce_graphics.h>

/*  The 88 keys of the on-screen piano (A0 = MIDI 21 to C8 = MIDI 108) as the board StageRevised draws them:
    52 white keys across the width with a small gap, black keys narrower and shorter, set off the white-key
    boundary like a real keyboard (C# and F# to the left, D# and A# to the right, G# centred).
    Pure geometry: no component, no drawing. */
namespace PianoKeyShapes
{
    inline constexpr int lowestNote  = 21;    // A0
    inline constexpr int highestNote = 108;   // C8
    inline constexpr int whiteKeys   = 52;

    /** True for C#, D#, F#, G# and A#. */
    bool isBlack (int midiNote);

    /** White keys below this note (A0 = 0, B0 = 1, C1 = 2 …). */
    int whiteKeysBelow (int midiNote);

    /** The key's drawn area for a keyboard of this size; empty outside A0..C8. */
    juce::Rectangle<float> shapeOf (int midiNote, float keyboardWidth, float keyboardHeight);
}
