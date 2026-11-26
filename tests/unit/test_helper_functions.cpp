#include <juce_core/juce_core.h>
#include "HelperFunctions.h"

namespace HelperFunctionsTestValues
{
    const juce::String testName     = "MapHelper";
    const juce::String testCategory = "Unit";

    const juce::String intToStringNoteValidNotes = "intToStringNote - valid notes";
    const juce::String intToStringNoteInvalidNotes = "intToStringNote - invalid notes";
    const juce::String stringToIntNoteValidStrings = "stringToIntNote - valid strings";
    const juce::String stringToIntNoteInvalidStrings = "stringToIntNote - invalid strings";
    const juce::String intStringInt = "int-string-int";
    const juce::String majorChordsReturnCorrectIntervalsRootMaj3 = "Major chords return correct intervals (root, maj3, perf5)";
    const juce::String minorChordsReturnCorrectIntervalsRootMin3 = "Minor chords return correct intervals (root, min3, perf5)";
    const juce::String diminishedChordsRootMin3Dim5 = "Diminished chords (root, min3, dim5)";
    const juce::String augmentedChordsRootMaj3Aug5 = "Augmented chords (root, maj3, aug5)";
    const juce::String sharpRootNotes = "Sharp root notes";
    const juce::String invalidRootReturnsEmpty = "Invalid root returns empty";
    const juce::String case7thChord = "7th chord";
    const juce::String getNotesForChordUnknownRootReturnsEmptyForEvery = "getNotesForChord - unknown root returns empty for every quality";

    const juce::String chordHelperTestName = "ChordHelper";
    constexpr int middleC = 60;
    constexpr int concertA = 69;
    constexpr int lowestNote = 0;
    constexpr int highestNote = 127;
    constexpr int belowRange = -1;
    constexpr int aboveRange = 128;
    constexpr int notANote = -1;
    const juce::String middleCName = "C4";
    const juce::String concertAName = "A4";
    const juce::String lowestNoteName = "C-1";
    const juce::String invalidName = "Invalid";
    const juce::String nonsenseName = "XYZ";
    const juce::String cMajor = "C Major";
    const juce::String aMajor = "A Major";
    const juce::String dMinor = "D Minor";
    const juce::String cDiminished = "C Diminished";
    const juce::String cAugmented = "C Augmented";
    const juce::String fSharpMinor = "F# Minor";
    const juce::String unknownRootChord = "X Major";
    const juce::String cSeventh = "C7";
    const juce::String notANoteRoot = "H ";
    const juce::StringArray qualityNames { "Major", "Minor", "Diminished", "Augmented" };
    const std::vector<int> cMajorNotes { 60, 64, 67 };
    const std::vector<int> aMajorNotes { 69, 73, 76 };
    const std::vector<int> dMinorNotes { 62, 65, 69 };
    const std::vector<int> cDiminishedNotes { 60, 63, 66 };
    const std::vector<int> cAugmentedNotes { 60, 64, 68 };
    const std::vector<int> fSharpMinorNotes { 66, 69, 73 };
    const std::vector<int> cSeventhNotes { 60, 64, 67, 70 };
}

using namespace HelperFunctionsTestValues;
class MapHelperTest : public juce::UnitTest
{
public:
    MapHelperTest() : juce::UnitTest (HelperFunctionsTestValues::testName, HelperFunctionsTestValues::testCategory) {}
    void runTest() override
    {
        beginTest(intToStringNoteValidNotes);
        expect(MapHelper::intToStringNote(middleC) == middleCName);   
        expect(MapHelper::intToStringNote(concertA) == concertAName);   
        expect(MapHelper::intToStringNote(lowestNote) == lowestNoteName);  


        beginTest(intToStringNoteInvalidNotes);
        expect(MapHelper::intToStringNote(belowRange) == invalidName);
        expect(MapHelper::intToStringNote(aboveRange) == invalidName);


        beginTest(stringToIntNoteValidStrings);
        expect(MapHelper::stringToIntNote(middleCName) == middleC);
        expect(MapHelper::stringToIntNote(concertAName) == concertA);


        beginTest(stringToIntNoteInvalidStrings);
        expect(MapHelper::stringToIntNote(nonsenseName) == notANote);
        expect(MapHelper::stringToIntNote({}) == notANote);


        beginTest(intStringInt);
        
        for (int note = lowestNote; note <= highestNote; ++note)
        {
            auto str = MapHelper::intToStringNote(note);
            expect(MapHelper::stringToIntNote(str) == note);
        }
    }
};

class ChordHelperTest : public juce::UnitTest
{
public:
    ChordHelperTest() : juce::UnitTest(chordHelperTestName, testCategory) {}

    void runTest() override
    {
        beginTest(majorChordsReturnCorrectIntervalsRootMaj3);
        {
            Chord chord; chord.name = cMajor;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == cMajorNotes);  
        }
        {
            Chord chord; chord.name = aMajor;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == aMajorNotes);  
        }
        beginTest(minorChordsReturnCorrectIntervalsRootMin3);
        {
            Chord chord; chord.name = dMinor;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == dMinorNotes);  
        }

        beginTest(diminishedChordsRootMin3Dim5);
        {
            Chord chord; chord.name = cDiminished;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == cDiminishedNotes);
        }

        beginTest(augmentedChordsRootMaj3Aug5);
        {
            Chord chord; chord.name = cAugmented;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == cAugmentedNotes);
        }

        beginTest(sharpRootNotes);
        {
            Chord chord; chord.name = fSharpMinor;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == fSharpMinorNotes);  
        }

        beginTest(invalidRootReturnsEmpty);
        {
            Chord chord; chord.name = unknownRootChord;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes.empty());
        }

        beginTest(case7thChord);
        {
            Chord chord; chord.name = cSeventh;
            auto notes = ChordHelper::getNotesForChord(chord);
            expect(notes == cSeventhNotes);
        }

        beginTest(getNotesForChordUnknownRootReturnsEmptyForEvery);
        {
            for (const auto& quality : qualityNames)
            {
                Chord chord; chord.name = notANoteRoot + quality;   // H is not a note
                expect(ChordHelper::getNotesForChord(chord).empty());
            }
        }
    }

};


static MapHelperTest mapHelperTest;

static ChordHelperTest chordHelperTest;
