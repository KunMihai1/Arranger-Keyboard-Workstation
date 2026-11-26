/*
  ==============================================================================

    HelperFunction.cpp
    Created: 12 Oct 2025 8:32:50pm
    Author:  Kisuke

  ==============================================================================
*/

#include "HelperFunctions.h"

namespace NoteNaming
{
    constexpr int lowestMidiNote        = 0;
    constexpr int highestMidiNote       = 127;
    constexpr int semitonesPerOctave    = 12;
    constexpr int octaveOfLowestMidiNote = -1;   // MIDI note 0 is C-1, so middle C (60) is C4
    constexpr int notFound              = -1;
    constexpr int startOfText           = 0;
    constexpr juce::juce_wchar negativeOctaveSign = '-';

    const juce::String invalidNoteName = "Invalid";
    const juce::StringArray pitchClassNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

namespace ChordSpelling
{
    constexpr int middleC = 60;   // chord roots are voiced in the octave starting at middle C

    const juce::String rootAndQualitySeparator = " ";
    const juce::String majorQuality      = "Major";
    const juce::String minorQuality      = "Minor";
    const juce::String seventhQuality    = "7";
    const juce::String augmentedQuality  = "Augmented";
    const juce::String diminishedQuality = "Diminished";

    // A name without a separator is a seventh chord written like "C7": the root is all but that suffix.
    constexpr int seventhSuffixLength = 1;

    // Semitones above the root.
    const std::vector<int> majorTriad      { 0, 4, 7 };
    const std::vector<int> minorTriad      { 0, 3, 7 };
    const std::vector<int> dominantSeventh { 0, 4, 7, 10 };
    const std::vector<int> augmentedTriad  { 0, 4, 8 };
    const std::vector<int> diminishedTriad { 0, 3, 6 };
}

juce::String MapHelper::intToStringNote(int note)
{
    using namespace NoteNaming;
    if (note < lowestMidiNote || note > highestMidiNote)
        return invalidNoteName;

    const int octave = note / semitonesPerOctave + octaveOfLowestMidiNote;
    return pitchClassNames[note % semitonesPerOctave] + juce::String(octave);
}

int MapHelper::stringToIntNote(juce::String noteString)
{
    using namespace NoteNaming;
    noteString = noteString.trim().toUpperCase();

    int firstOctaveCharacter = notFound;
    for (int characterIndex {}; characterIndex < noteString.length(); ++characterIndex)
    {
        if (juce::CharacterFunctions::isDigit(noteString[characterIndex]) || noteString[characterIndex] == negativeOctaveSign)
        {
            firstOctaveCharacter = characterIndex;
            break;
        }
    }

    if (firstOctaveCharacter == notFound)
        return notFound;

    const juce::String pitchClassPart = noteString.substring(startOfText, firstOctaveCharacter);
    const juce::String octavePart     = noteString.substring(firstOctaveCharacter);

    const int pitchClass = pitchClassNames.indexOf(pitchClassPart);
    if (pitchClass == notFound)
        return notFound;

    const int octave   = octavePart.getIntValue();
    const int midiNote = (octave - octaveOfLowestMidiNote) * semitonesPerOctave + pitchClass;

    if (midiNote < lowestMidiNote || midiNote > highestMidiNote)
        return notFound;

    return midiNote;
}

std::vector<int> ChordHelper::getNotesForChord(const Chord& chord)
{
    using namespace ChordSpelling;
    const juce::String& name = chord.name;
    juce::String root;
    juce::String quality;

    if (name.contains(rootAndQualitySeparator))
    {
        root    = name.upToFirstOccurrenceOf(rootAndQualitySeparator, false, false);
        quality = name.fromFirstOccurrenceOf(rootAndQualitySeparator, false, false).trim();
    }
    else
    {
        root    = name.dropLastCharacters(seventhSuffixLength);
        quality = seventhQuality;
    }

    const int pitchClass = NoteNaming::pitchClassNames.indexOf(root);
    if (pitchClass == NoteNaming::notFound)
        return {};

    const int rootNote = middleC + pitchClass;

    std::vector<int> intervals;
    if (quality.equalsIgnoreCase(majorQuality))
        intervals = majorTriad;
    else if (quality.equalsIgnoreCase(minorQuality))
        intervals = minorTriad;
    else if (quality.equalsIgnoreCase(seventhQuality))
        intervals = dominantSeventh;
    else if (quality.equalsIgnoreCase(augmentedQuality))
        intervals = augmentedTriad;
    else if (quality.equalsIgnoreCase(diminishedQuality))
        intervals = diminishedTriad;

    std::vector<int> notes;
    for (int interval : intervals)
        notes.push_back(rootNote + interval);

    return notes;
}
