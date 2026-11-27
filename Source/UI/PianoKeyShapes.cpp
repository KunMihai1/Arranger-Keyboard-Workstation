#include "PianoKeyShapes.h"

namespace
{
    // Board StageRevised, keys region 1200 x 140: .wk is 22 wide on a 1200/52 step, .bk 14 x 88 set 7 left of the
    // white-key boundary plus its offset.
    namespace Board
    {
        constexpr float width        = 1200.0f;
        constexpr float whiteWidth   = 22.0f;
        constexpr float blackWidth   = 14.0f;
        constexpr float blackHalf    = 7.0f;
        constexpr float keyHeight    = 140.0f;
        constexpr float blackHeight  = 88.0f;
        constexpr float leftOffset   = -2.0f;   // C#, F#
        constexpr float rightOffset  = 2.0f;    // D#, A#
        constexpr float centred      = 0.0f;    // G#
    }

    constexpr int semitonesPerOctave = 12;
    // Pitch classes (semitones above C) of the black keys
    constexpr int cSharp = 1;
    constexpr int dSharp = 3;
    constexpr int fSharp = 6;
    constexpr int gSharp = 8;
    constexpr int aSharp = 10;

    int pitchClassOf (int midiNote) { return midiNote % semitonesPerOctave; }

    float boardOffsetOf (int pitchClass)
    {
        switch (pitchClass)
        {
            case cSharp: case fSharp: return Board::leftOffset;
            case dSharp: case aSharp: return Board::rightOffset;
            default:                  return Board::centred;
        }
    }
}

namespace PianoKeyShapes
{
    bool isBlack (int midiNote)
    {
        const auto pitchClass = pitchClassOf (midiNote);
        return pitchClass == cSharp || pitchClass == dSharp || pitchClass == fSharp || pitchClass == gSharp || pitchClass == aSharp;
    }

    int whiteKeysBelow (int midiNote)
    {
        int whites {};
        for (int note = lowestNote; note < midiNote; ++note)
            if (! isBlack (note))
                ++whites;
        return whites;
    }

    juce::Rectangle<float> shapeOf (int midiNote, float keyboardWidth, float keyboardHeight)
    {
        if (midiNote < lowestNote || midiNote > highestNote)
            return {};

        const auto step  = keyboardWidth / static_cast<float> (whiteKeys);
        const auto scale = keyboardWidth / Board::width;
        const auto left  = static_cast<float> (whiteKeysBelow (midiNote)) * step;

        if (! isBlack (midiNote))
            return { left, float {}, Board::whiteWidth * scale, keyboardHeight };

        return { left + (boardOffsetOf (pitchClassOf (midiNote)) - Board::blackHalf) * scale, float {},
                 Board::blackWidth * scale, keyboardHeight * Board::blackHeight / Board::keyHeight };
    }
}
