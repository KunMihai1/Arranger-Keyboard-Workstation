#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include "Common/HelperFunctions.h"

/*  The Chord helper page (board StageRevised openPage=chords, PageChords): what it shows of a chord (its note names,
    three voicings drawn on mini keyboards) and where everything goes. Pure: no components, so it is unit-tested
    against the board's measured rects. Rects take board px times the Stage scale, in the content's coordinates. */

namespace ChordHelperModel
{
    inline constexpr int voicingCount = 3;   // root position, 1st inversion, 2nd inversion

    /** The notes' names without octave ("C E G" with a space separator); sharps, as the chord library. */
    juce::String noteNames (const std::vector<int>& notes, const juce::String& separator);

    /** Root position one octave below the chord as played (C3-based, as the board), then each inversion: the
        lowest note moved up an octave. Empty for a chord the library cannot spell. */
    std::array<std::vector<int>, voicingCount> voicings (const Chord& chord);
}

/** One key of the board's .mini keyboard: its box (keyboard coordinates), its MIDI note, and whether it is black. */
struct MiniKey
{
    juce::Rectangle<float> box;
    int note {};
    bool black {};
};

namespace MiniKeyboard
{
    inline constexpr int lowestNote  = 48;   // C3
    inline constexpr int highestNote = 83;   // B5: three octaves

    /** Every key from C3 to B5, the white ones first (the black ones are drawn over them). */
    std::vector<MiniKey> keys (float scale);
}

struct ChordHelperRects
{
    juce::Rectangle<int> search, list, name, hint;
    std::array<juce::Rectangle<int>, ChordHelperModel::voicingCount> voicingTitle, keyboard, voicingNotes;
};

/** Left: search and the chord list (280 wide); right, 22 further: name row, three voicing columns, the hint. */
ChordHelperRects chordHelperLayout (juce::Rectangle<int> content, float scale);

namespace ChordHelperMetrics
{
    inline constexpr int   listColumnWidth    = 280;
    inline constexpr int   columnGap          = 22;
    inline constexpr int   searchHeight       = 26;
    inline constexpr int   searchGap          = 6;
    inline constexpr int   chordRowHeight     = 23;   // .crow
    inline constexpr int   nameHeight         = 38;   // 34 px mono line
    inline constexpr float nameTextSize       = 34.0f;
    inline constexpr int   nameNotesGap       = 14;   // chord notes after the name
    inline constexpr int   nameNotesTop       = 18;   // on the name's baseline
    inline constexpr int   nameNotesHeight    = 14;
    inline constexpr float nameNotesTextSize  = 12.0f;
    inline constexpr int   blockGap           = 10;   // name row, voicings, hint (flex gap 10)
    inline constexpr int   hintMargin         = 4;    // the hint's extra margin-top
    inline constexpr int   voicingGap         = 18;   // between the three columns
    inline constexpr int   voicingGaps        = 2;    // three columns, two gaps
    inline constexpr int   voicingInnerGap    = 6;    // title, keyboard, notes
    inline constexpr int   titleHeight        = 12;
    inline constexpr int   keyboardHeight     = 62;
    inline constexpr int   voicingNotesHeight = 14;
    inline constexpr float voicingNotesTextSize = 12.0f;
    inline constexpr int   hintHeight         = 12;

    inline constexpr int   whiteKeyWidth  = 11;
    inline constexpr int   whiteKeyPitch  = 12;
    inline constexpr int   blackKeyWidth  = 7;
    inline constexpr int   blackKeyHeight = 38;
    inline constexpr int   blackKeyShift  = 4;     // a black key starts this far left of the white edge ...
    inline constexpr int   keyboardCorner = 2;
}
