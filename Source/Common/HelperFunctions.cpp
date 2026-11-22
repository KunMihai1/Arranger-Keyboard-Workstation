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

namespace ChordDiagrams
{
    struct EmbeddedImage
    {
        const char* data;
        int size;
    };

    struct ChordImages
    {
        const char* upperCaseChordName;
        EmbeddedImage rootPosition, firstInversion, secondInversion;
    };

    // The chord diagrams compiled into BinaryData, by upper-case chord name. Built on first use: the
    // BinaryData pointers are defined in another file, and the order in which files initialise their
    // globals at start-up is not guaranteed.
    const std::vector<ChordImages>& table()
    {
        static const std::vector<ChordImages> chordImages =
        {
        { "C MAJOR", { BinaryData::C_MAJ_ROOT_png, BinaryData::C_MAJ_ROOT_pngSize }, { BinaryData::C_MAJ_INV1_png, BinaryData::C_MAJ_INV1_pngSize }, { BinaryData::C_MAJ_INV2_png, BinaryData::C_MAJ_INV2_pngSize } },
        { "C MINOR", { BinaryData::C_MINOR_ROOT_png, BinaryData::C_MINOR_ROOT_pngSize }, { BinaryData::C_MINOR_INV1_png, BinaryData::C_MINOR_INV1_pngSize }, { BinaryData::C_MINOR_INV2_png, BinaryData::C_MINOR_INV2_pngSize } },
        { "D MAJOR", { BinaryData::D_MAJ_ROOT_png, BinaryData::D_MAJ_ROOT_pngSize }, { BinaryData::D_MAJ_INV1_png, BinaryData::D_MAJ_INV1_pngSize }, { BinaryData::D_MAJ_INV2_png, BinaryData::D_MAJ_INV2_pngSize } },
        { "D MINOR", { BinaryData::D_MINOR_ROOT_png, BinaryData::D_MINOR_ROOT_pngSize }, { BinaryData::D_MINOR_INV1_png, BinaryData::D_MINOR_INV1_pngSize }, { BinaryData::D_MINOR_INV2_png, BinaryData::D_MINOR_INV2_pngSize } },
        { "E MAJOR", { BinaryData::E_MAJ_ROOT_png, BinaryData::E_MAJ_ROOT_pngSize }, { BinaryData::E_MAJ_INV1_png, BinaryData::E_MAJ_INV1_pngSize }, { BinaryData::E_MAJ_INV2_png, BinaryData::E_MAJ_INV2_pngSize } },
        { "E MINOR", { BinaryData::E_MINOR_ROOT_png, BinaryData::E_MINOR_ROOT_pngSize }, { BinaryData::E_MINOR_INV1_png, BinaryData::E_MINOR_INV1_pngSize }, { BinaryData::E_MINOR_INV2_png, BinaryData::E_MINOR_INV2_pngSize } },
        { "F MAJOR", { BinaryData::F_MAJ_ROOT_png, BinaryData::F_MAJ_ROOT_pngSize }, { BinaryData::F_MAJ_INV1_png, BinaryData::F_MAJ_INV1_pngSize }, { BinaryData::F_MAJ_INV2_png, BinaryData::F_MAJ_INV2_pngSize } },
        { "F MINOR", { BinaryData::F_MINOR_ROOT_png, BinaryData::F_MINOR_ROOT_pngSize }, { BinaryData::F_MINOR_INV1_png, BinaryData::F_MINOR_INV1_pngSize }, { BinaryData::F_MINOR_INV2_png, BinaryData::F_MINOR_INV2_pngSize } },
        { "G MAJOR", { BinaryData::G_MAJ_ROOT_png, BinaryData::G_MAJ_ROOT_pngSize }, { BinaryData::G_MAJ_INV1_png, BinaryData::G_MAJ_INV1_pngSize }, { BinaryData::G_MAJ_INV2_png, BinaryData::G_MAJ_INV2_pngSize } },
        { "G MINOR", { BinaryData::G_MINOR_ROOT_png, BinaryData::G_MINOR_ROOT_pngSize }, { BinaryData::G_MINOR_INV1_png, BinaryData::G_MINOR_INV1_pngSize }, { BinaryData::G_MINOR_INV2_png, BinaryData::G_MINOR_INV2_pngSize } },
        { "A MAJOR", { BinaryData::A_MAJ_ROOT_png, BinaryData::A_MAJ_ROOT_pngSize }, { BinaryData::A_MAJ_INV1_png, BinaryData::A_MAJ_INV1_pngSize }, { BinaryData::A_MAJ_INV2_png, BinaryData::A_MAJ_INV2_pngSize } },
        { "A MINOR", { BinaryData::A_MINOR_ROOT_png, BinaryData::A_MINOR_ROOT_pngSize }, { BinaryData::A_MINOR_INV1_png, BinaryData::A_MINOR_INV1_pngSize }, { BinaryData::A_MINOR_INV2_png, BinaryData::A_MINOR_INV2_pngSize } },
        { "B MAJOR", { BinaryData::B_MAJ_ROOT_png, BinaryData::B_MAJ_ROOT_pngSize }, { BinaryData::B_MAJ_INV1_png, BinaryData::B_MAJ_INV1_pngSize }, { BinaryData::B_MAJ_INV2_png, BinaryData::B_MAJ_INV2_pngSize } },
        { "B MINOR", { BinaryData::B_MINOR_ROOT_png, BinaryData::B_MINOR_ROOT_pngSize }, { BinaryData::B_MINOR_INV1_png, BinaryData::B_MINOR_INV1_pngSize }, { BinaryData::B_MINOR_INV2_png, BinaryData::B_MINOR_INV2_pngSize } },
        { "C# MAJOR", { BinaryData::C_SHARP_MAJ_ROOT_png, BinaryData::C_SHARP_MAJ_ROOT_pngSize }, { BinaryData::C_SHARP_MAJ_INV1_png, BinaryData::C_SHARP_MAJ_INV1_pngSize }, { BinaryData::C_SHARP_MAJ_INV2_png, BinaryData::C_SHARP_MAJ_INV2_pngSize } },
        { "C# MINOR", { BinaryData::C_SHARP_MINOR_ROOT_png, BinaryData::C_SHARP_MINOR_ROOT_pngSize }, { BinaryData::C_SHARP_MINOR_INV1_png, BinaryData::C_SHARP_MINOR_INV1_pngSize }, { BinaryData::C_SHARP_MINOR_INV2_png, BinaryData::C_SHARP_MINOR_INV2_pngSize } },
        { "D# MAJOR", { BinaryData::D_SHARP_MAJ_ROOT_png, BinaryData::D_SHARP_MAJ_ROOT_pngSize }, { BinaryData::D_SHARP_MAJ_INV1_png, BinaryData::D_SHARP_MAJ_INV1_pngSize }, { BinaryData::D_SHARP_MAJ_INV2_png, BinaryData::D_SHARP_MAJ_INV2_pngSize } },
        { "D# MINOR", { BinaryData::D_SHARP_MINOR_ROOT_png, BinaryData::D_SHARP_MINOR_ROOT_pngSize }, { BinaryData::D_SHARP_MINOR_INV1_png, BinaryData::D_SHARP_MINOR_INV1_pngSize }, { BinaryData::D_SHARP_MINOR_INV2_png, BinaryData::D_SHARP_MINOR_INV2_pngSize } },
        { "F# MAJOR", { BinaryData::F_SHARP_MAJ_ROOT_png, BinaryData::F_SHARP_MAJ_ROOT_pngSize }, { BinaryData::F_SHARP_MAJ_INV1_png, BinaryData::F_SHARP_MAJ_INV1_pngSize }, { BinaryData::F_SHARP_MAJ_INV2_png, BinaryData::F_SHARP_MAJ_INV2_pngSize } },
        { "F# MINOR", { BinaryData::F_SHARP_MINOR_ROOT_png, BinaryData::F_SHARP_MINOR_ROOT_pngSize }, { BinaryData::F_SHARP_MINOR_INV1_png, BinaryData::F_SHARP_MINOR_INV1_pngSize }, { BinaryData::F_SHARP_MINOR_INV2_png, BinaryData::F_SHARP_MINOR_INV2_pngSize } },
        { "G# MAJOR", { BinaryData::G_SHARP_MAJ_ROOT_png, BinaryData::G_SHARP_MAJ_ROOT_pngSize }, { BinaryData::G_SHARP_MAJ_INV1_png, BinaryData::G_SHARP_MAJ_INV1_pngSize }, { BinaryData::G_SHARP_MAJ_INV2_png, BinaryData::G_SHARP_MAJ_INV2_pngSize } },
        { "G# MINOR", { BinaryData::G_SHARP_MIN_ROOT_png, BinaryData::G_SHARP_MIN_ROOT_pngSize }, { BinaryData::G_SHARP_MINOR_INV1_png, BinaryData::G_SHARP_MINOR_INV1_pngSize }, { BinaryData::G_SHARP_MINOR_INV2_png, BinaryData::G_SHARP_MINOR_INV2_pngSize } },
        { "A# MAJOR", { BinaryData::A_SHARP_MAJ_ROOT_png, BinaryData::A_SHARP_MAJ_ROOT_pngSize }, { BinaryData::A_SHARP_MAJ_INV1_png, BinaryData::A_SHARP_MAJ_INV1_pngSize }, { BinaryData::A_SHARP_MAJ_INV2_png, BinaryData::A_SHARP_MAJ_INV2_pngSize } },
        { "A# MINOR", { BinaryData::A_SHARP_MINOR_ROOT_png, BinaryData::A_SHARP_MINOR_ROOT_pngSize }, { BinaryData::A_SHARP_MINOR_INV1_png, BinaryData::A_SHARP_MINOR_INV1_pngSize }, { BinaryData::A_SHARP_MINOR_INV2_png, BinaryData::A_SHARP_MINOR_INV2_pngSize } },
        { "C DIMINISHED", { BinaryData::C_DIM_ROOT_png, BinaryData::C_DIM_ROOT_pngSize }, { BinaryData::C_DIM_INV1_png, BinaryData::C_DIM_INV1_pngSize }, { BinaryData::C_DIM_INV2_png, BinaryData::C_DIM_INV2_pngSize } },
        { "C AUGMENTED", { BinaryData::C_AUG_ROOT_png, BinaryData::C_AUG_ROOT_pngSize }, { BinaryData::C_AUG_INV1_png, BinaryData::C_AUG_INV1_pngSize }, { BinaryData::C_AUG_INV2_png, BinaryData::C_AUG_INV2_pngSize } },
        { "D DIMINISHED", { BinaryData::D_DIM_ROOT_png, BinaryData::D_DIM_ROOT_pngSize }, { BinaryData::D_DIM_INV1_png, BinaryData::D_DIM_INV1_pngSize }, { BinaryData::D_DIM_INV2_png, BinaryData::D_DIM_INV2_pngSize } },
        { "D AUGMENTED", { BinaryData::D_AUG_ROOT_png, BinaryData::D_AUG_ROOT_pngSize }, { BinaryData::D_AUG_INV1_png, BinaryData::D_AUG_INV1_pngSize }, { BinaryData::D_AUG_INV2_png, BinaryData::D_AUG_INV2_pngSize } },
        { "E DIMINISHED", { BinaryData::E_DIM_ROOT_png, BinaryData::E_DIM_ROOT_pngSize }, { BinaryData::E_DIM_INV1_png, BinaryData::E_DIM_INV1_pngSize }, { BinaryData::E_DIM_INV2_png, BinaryData::E_DIM_INV2_pngSize } },
        { "E AUGMENTED", { BinaryData::E_AUG_ROOT_png, BinaryData::E_AUG_ROOT_pngSize }, { BinaryData::E_AUG_INV1_png, BinaryData::E_AUG_INV1_pngSize }, { BinaryData::E_AUG_INV2_png, BinaryData::E_AUG_INV2_pngSize } },
        { "F DIMINISHED", { BinaryData::F_DIM_ROOT_png, BinaryData::F_DIM_ROOT_pngSize }, { BinaryData::F_DIM_INV1_png, BinaryData::F_DIM_INV1_pngSize }, { BinaryData::F_DIM_INV2_png, BinaryData::F_DIM_INV2_pngSize } },
        { "F AUGMENTED", { BinaryData::F_AUG_ROOT_png, BinaryData::F_AUG_ROOT_pngSize }, { BinaryData::F_AUG_INV1_png, BinaryData::F_AUG_INV1_pngSize }, { BinaryData::F_AUG_INV2_png, BinaryData::F_AUG_INV2_pngSize } },
        { "G DIMINISHED", { BinaryData::G_DIM_ROOT_png, BinaryData::G_DIM_ROOT_pngSize }, { BinaryData::G_DIM_INV1_png, BinaryData::G_DIM_INV1_pngSize }, { BinaryData::G_DIM_INV2_png, BinaryData::G_DIM_INV2_pngSize } },
        { "G AUGMENTED", { BinaryData::G_AUG_ROOT_png, BinaryData::G_AUG_ROOT_pngSize }, { BinaryData::G_AUG_INV1_png, BinaryData::G_AUG_INV1_pngSize }, { BinaryData::G_AUG_INV2_png, BinaryData::G_AUG_INV2_pngSize } },
        { "A DIMINISHED", { BinaryData::A_DIM_ROOT_png, BinaryData::A_DIM_ROOT_pngSize }, { BinaryData::A_DIM_INV1_png, BinaryData::A_DIM_INV1_pngSize }, { BinaryData::A_DIM_INV2_png, BinaryData::A_DIM_INV2_pngSize } },
        { "A AUGMENTED", { BinaryData::A_AUG_ROOT_png, BinaryData::A_AUG_ROOT_pngSize }, { BinaryData::A_AUG_INV1_png, BinaryData::A_AUG_INV1_pngSize }, { BinaryData::A_AUG_INV2_png, BinaryData::A_AUG_INV2_pngSize } },
        { "B DIMINISHED", { BinaryData::B_DIM_ROOT_png, BinaryData::B_DIM_ROOT_pngSize }, { BinaryData::B_DIM_INV1_png, BinaryData::B_DIM_INV1_pngSize }, { BinaryData::B_DIM_INV2_png, BinaryData::B_DIM_INV2_pngSize } },
        { "B AUGMENTED", { BinaryData::B_AUG_ROOT_png, BinaryData::B_AUG_ROOT_pngSize }, { BinaryData::B_AUG_INV1_png, BinaryData::B_AUG_INV1_pngSize }, { BinaryData::B_AUG_INV2_png, BinaryData::B_AUG_INV2_pngSize } },
        { "C# DIMINISHED", { BinaryData::C_SHARP_DIM_ROOT_png, BinaryData::C_SHARP_DIM_ROOT_pngSize }, { BinaryData::C_SHARP_DIM_INV1_png, BinaryData::C_SHARP_DIM_INV1_pngSize }, { BinaryData::C_SHARP_DIM_INV2_png, BinaryData::C_SHARP_DIM_INV2_pngSize } },
        { "C# AUGMENTED", { BinaryData::C_SHARP_AUG_ROOT_png, BinaryData::C_SHARP_AUG_ROOT_pngSize }, { BinaryData::C_SHARP_AUG_INV1_png, BinaryData::C_SHARP_AUG_INV1_pngSize }, { BinaryData::C_SHARP_AUG_INV2_png, BinaryData::C_SHARP_AUG_INV2_pngSize } },
        { "D# DIMINISHED", { BinaryData::D_SHARP_DIM_ROOT_png, BinaryData::D_SHARP_DIM_ROOT_pngSize }, { BinaryData::D_SHARP_DIM_INV1_png, BinaryData::D_SHARP_DIM_INV1_pngSize }, { BinaryData::D_SHARP_DIM_INV2_png, BinaryData::D_SHARP_DIM_INV2_pngSize } },
        { "D# AUGMENTED", { BinaryData::D_SHARP_AUG_ROOT_png, BinaryData::D_SHARP_AUG_ROOT_pngSize }, { BinaryData::D_SHARP_AUG_INV1_png, BinaryData::D_SHARP_AUG_INV1_pngSize }, { BinaryData::D_SHARP_AUG_INV2_png, BinaryData::D_SHARP_AUG_INV2_pngSize } },
        { "F# DIMINISHED", { BinaryData::F_SHARP_DIM_ROOT_png, BinaryData::F_SHARP_DIM_ROOT_pngSize }, { BinaryData::F_SHARP_DIM_INV1_png, BinaryData::F_SHARP_DIM_INV1_pngSize }, { BinaryData::F_SHARP_DIM_INV2_png, BinaryData::F_SHARP_DIM_INV2_pngSize } },
        { "F# AUGMENTED", { BinaryData::F_SHARP_AUG_ROOT_png, BinaryData::F_SHARP_AUG_ROOT_pngSize }, { BinaryData::F_SHARP_AUG_INV1_png, BinaryData::F_SHARP_AUG_INV1_pngSize }, { BinaryData::F_SHARP_AUG_INV2_png, BinaryData::F_SHARP_AUG_INV2_pngSize } },
        { "G# DIMINISHED", { BinaryData::G_SHARP_DIM_ROOT_png, BinaryData::G_SHARP_DIM_ROOT_pngSize }, { BinaryData::G_SHARP_DIM_INV1_png, BinaryData::G_SHARP_DIM_INV1_pngSize }, { BinaryData::G_SHARP_DIM_INV2_png, BinaryData::G_SHARP_DIM_INV2_pngSize } },
        { "G# AUGMENTED", { BinaryData::G_SHARP_AUG_ROOT_png, BinaryData::G_SHARP_AUG_ROOT_pngSize }, { BinaryData::G_SHARP_AUG_INV1_png, BinaryData::G_SHARP_AUG_INV1_pngSize }, { BinaryData::G_SHARP_AUG_INV2_png, BinaryData::G_SHARP_AUG_INV2_pngSize } },
        { "A# DIMINISHED", { BinaryData::A_SHARP_DIM_ROOT_png, BinaryData::A_SHARP_DIM_ROOT_pngSize }, { BinaryData::A_SHARP_DIM_INV1_png, BinaryData::A_SHARP_DIM_INV1_pngSize }, { BinaryData::A_SHARP_DIM_INV2_png, BinaryData::A_SHARP_DIM_INV2_pngSize } },
        { "A# AUGMENTED", { BinaryData::A_SHARP_AUG_ROOT_png, BinaryData::A_SHARP_AUG_ROOT_pngSize }, { BinaryData::A_SHARP_AUG_INV1_png, BinaryData::A_SHARP_AUG_INV1_pngSize }, { BinaryData::A_SHARP_AUG_INV2_png, BinaryData::A_SHARP_AUG_INV2_pngSize } },
        };
        return chordImages;
    }

    juce::Image load(const EmbeddedImage& image)
    {
        return juce::ImageCache::getFromMemory(image.data, image.size);
    }
}

void ChordHelper::loadChordNeeded(Chord& chord)
{
    if (chord.imagesLoaded)
        return;

    const juce::String upperCaseName = chord.name.toUpperCase();
    for (const auto& images : ChordDiagrams::table())
    {
        if (upperCaseName == images.upperCaseChordName)
        {
            chord.imgRoot = ChordDiagrams::load(images.rootPosition);
            chord.imgInv1 = ChordDiagrams::load(images.firstInversion);
            chord.imgInv2 = ChordDiagrams::load(images.secondInversion);
            break;
        }
    }

    chord.imagesLoaded = true;
}