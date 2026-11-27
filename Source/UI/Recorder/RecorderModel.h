#pragma once
#include <JuceHeader.h>
#include <array>

/*  The Recorder page (board PageFile, spec part 2 P.9): what its two LCDs say, which buttons work, and where
    everything goes. Pure: no components, so it is unit-tested; RecorderContent paints it and owns the buttons.
    Rects take board px times the Stage scale, in the content's coordinates. */

/** What the page shows: the last recording and the MIDI file played through the current sounds. */
struct RecorderState
{
    double recordedSeconds {};   ///< the recording's length (while recording: the time so far)
    int notes {};                ///< notes played into it (the program changes recorded at its start do not count)
    bool recording {};
    bool playing {};             ///< the recording is playing back
    juce::String fileName;       ///< the MIDI file chosen last; empty: none yet
    bool filePlaying {};

    bool operator== (const RecorderState& other) const;
    bool operator!= (const RecorderState& other) const { return ! (*this == other); }
};

namespace RecorderModel
{
    /** "mm:ss", whole seconds, from 00:00 up to 99:59. */
    juce::String lengthText (double seconds);

    /** "312 notes" / "1 note" / "No notes yet" (true before a take and after an empty one). */
    juce::String notesText (int notes);

    /** The lamp's words at the top right of the recording LCD: "REC ON" / "REC OFF". */
    juce::String recLampText (bool recording);

    /** The file LCD's small status line: "NO FILE LOADED" / "PLAYING" / "STOPPED". */
    juce::String fileStatus (const RecorderState& state);

    /** The file LCD's main line: the file's name, or "Choose a .mid file to play it". */
    juce::String fileTitle (const RecorderState& state);

    bool canPlayRecording (const RecorderState& state);   ///< there are notes to play
    bool canStopRecording (const RecorderState& state);   ///< recording or playing it back
    bool canSaveRecording (const RecorderState& state);   ///< notes, and the recording is finished
    bool canStopFile (const RecorderState& state);        ///< a file is playing
}

namespace RecorderMetrics
{
    inline constexpr int   columnGap        = 40;
    inline constexpr int   headingHeight    = 12;
    inline constexpr int   headingGap       = 10;
    inline constexpr int   recordingLcdHeight = 107;
    inline constexpr int   fileLcdHeight    = 81;
    inline constexpr int   lcdButtonsGap    = 11;
    inline constexpr int   buttonHeight     = 26;
    inline constexpr int   buttonGap        = 8;
    inline constexpr int   hintGap          = 12;
    inline constexpr int   hintHeight       = 14;
    inline constexpr int   lcdPaddingX      = 16;
    inline constexpr int   lcdPaddingTop    = 12;
    inline constexpr int   lcdPaddingBottom = 10;
    inline constexpr int   smallLineHeight  = 14;
    inline constexpr int   timeHeight       = 46;
    inline constexpr float timeTextSize     = 44.0f;
    inline constexpr int   fileTitleHeight  = 24;
    inline constexpr float fileTitleTextSize = 16.0f;
    inline constexpr float smallTextSize    = 12.0f;
    inline constexpr int   columnCount      = 2;
}

/** Board px × scale, in the content's coordinates: left column = the recording, right = the MIDI file. */
struct RecorderRects
{
    juce::Rectangle<int> recordingHeading, recordingLcd, recordingButtons, hint;
    juce::Rectangle<int> recordingLabel, recLamp, time, notes;   ///< inside the recording LCD
    juce::Rectangle<int> fileHeading, fileLcd, fileButtons;
    juce::Rectangle<int> fileStatus, fileTitle, fileSubtitle;    ///< inside the file LCD
};

RecorderRects recorderLayout (juce::Rectangle<int> content, float scale);
