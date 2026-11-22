/*
  ==============================================================================

    HelperFunction.h
    Created: 12 Oct 2025 8:32:50pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>


struct Chord
{
    juce::String name;
    juce::Image imgRoot, imgInv1, imgInv2;

    bool imagesLoaded = false;
};

/**
 * @class MapHelper
 * @brief Utility class for converting between MIDI note numbers and string representations.
 */
class MapHelper {
public:
    /**
     * @brief Converts a MIDI note number to its string representation.
     * @param note MIDI note number (0-127)
     * @return String representation of the note, e.g., "C4". Returns "Invalid" if out of range.
     */
    static juce::String intToStringNote(int note);

    /**
     * @brief Converts a note string to a MIDI note number.
     * @param noteString Note string, e.g., "C4", "D#3"
     * @return MIDI note number (0-127), or -1 if the string is invalid.
     */
    static int stringToIntNote(juce::String noteString);

private:
};

class ChordHelper
{
public:
    static std::vector<int> getNotesForChord(const Chord& chord);

    static void loadChordNeeded(Chord& chord);
};