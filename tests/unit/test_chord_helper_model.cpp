#include <JuceHeader.h>
#include "UI/Chords/ChordHelperModel.h"

/*  The Chord helper page's model: note names, the three voicings, the mini keyboard's keys and the layout, against
    the board (PageChords at scale 1, page origin, measured in Edge: content at (28, 50), 1144 x 299). */

namespace ChordHelperModelTestValues
{
    const juce::String testName     = "ChordHelperModel";
    const juce::String testCategory = "Unit";

    const juce::String namesWithoutOctave = "note names drop the octave and keep sharps";
    const juce::String inversionsRotate   = "root position is an octave below the played chord; each inversion lifts the lowest note";
    const juce::String everyChordFits     = "all 60 library chords: three voicings inside C3-B5";
    const juce::String keysAsBoard        = "the mini keyboard: 21 white and 15 black keys where the board draws them";
    const juce::String layoutAsBoard      = "search, list, name row, voicing columns and hint where the board has them";
    const juce::String layoutScales       = "at the window's scale every rect is the board's times the scale";

    const juce::String space     = " ";
    const juce::String aMinor    = "A Minor";
    const juce::String cSeventh  = "C7";
    const juce::String unknown   = "H Major";
    const juce::String aceNames  = "A C E";
    const juce::String cSharp    = "C#";
    const juce::String nameSeparator = " ";
    const juce::String seventh   = "7";
    const juce::StringArray roots { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::StringArray qualities { "Major", "Minor", "7", "Augmented", "Diminished" };

    const std::vector<int> aMinorRoot   { 57, 60, 64 };
    const std::vector<int> aMinorFirst  { 60, 64, 69 };
    const std::vector<int> aMinorSecond { 64, 69, 72 };
    const std::vector<int> cSeventhRoot   { 48, 52, 55, 58 };
    const std::vector<int> cSeventhSecond { 55, 58, 60, 64 };
    const std::vector<int> cSharpOnly     { 61 };
    constexpr size_t rootPosition = 0;
    constexpr size_t firstInversion = 1;
    constexpr size_t secondInversion = 2;
    constexpr size_t libraryChords = 60;

    constexpr float boardScale  = 1.0f;
    constexpr float windowScale = 1.5364f;
    constexpr float doubleScale = 2.0f;
    constexpr size_t whiteKeys = 21;
    constexpr size_t blackKeys = 15;
    constexpr size_t lastWhiteIndex = 20;
    constexpr int firstNote = 48;
    constexpr int firstBlackNote = 49;
    constexpr int dSharpNote = 51;
    const juce::Rectangle<float> firstWhite { 0.0f, 0.0f, 11.0f, 62.0f };
    const juce::Rectangle<float> firstBlack { 7.0f, 0.0f, 7.0f, 38.0f };
    constexpr float dSharpX = 21.0f;
    constexpr float lastWhiteX = 240.0f;
    constexpr float keyTolerance = 0.001f;

    const juce::Rectangle<int> content { 28, 50, 1144, 299 };
    const juce::Rectangle<float> searchRect { 28.0f, 50.0f, 280.0f, 26.0f };
    const juce::Rectangle<float> listRect   { 28.0f, 82.0f, 280.0f, 267.0f };
    const juce::Rectangle<float> nameRect   { 330.0f, 50.0f, 842.0f, 38.0f };
    const juce::Rectangle<float> hintRect   { 330.0f, 212.0f, 842.0f, 12.0f };
    const float voicingX[] { 330.0f, 616.66f, 903.33f };
    constexpr float voicingWidth = 268.67f;
    constexpr float titleY = 98.0f;
    constexpr float titleHeight = 12.0f;
    constexpr float keyboardY = 116.0f;
    constexpr float keyboardHeight = 62.0f;
    constexpr float notesY = 184.0f;
    constexpr float notesHeight = 14.0f;
    constexpr float boardSlack  = 1.0f;    // the board's fractional columns round to whole pixels
    constexpr float scaledSlack = 2.0f;    // origin and size each round once
    const juce::String versus = " vs ";
}

class ChordHelperModelTest : public juce::UnitTest
{
public:
    ChordHelperModelTest() : juce::UnitTest (ChordHelperModelTestValues::testName, ChordHelperModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ChordHelperModelTestValues;

        beginTest (namesWithoutOctave);
        {
            expectEquals (ChordHelperModel::noteNames (aMinorRoot, space), aceNames);
            expectEquals (ChordHelperModel::noteNames (cSharpOnly, space), cSharp);
            expect (ChordHelperModel::noteNames ({}, space).isEmpty());
        }

        beginTest (inversionsRotate);
        {
            const auto minor = ChordHelperModel::voicings (chordNamed (aMinor));
            expect (minor[rootPosition] == aMinorRoot);
            expect (minor[firstInversion] == aMinorFirst);
            expect (minor[secondInversion] == aMinorSecond);
            const auto seventhChord = ChordHelperModel::voicings (chordNamed (cSeventh));
            expect (seventhChord[rootPosition] == cSeventhRoot);
            expect (seventhChord[secondInversion] == cSeventhSecond);
            for (const auto& voicing : ChordHelperModel::voicings (chordNamed (unknown)))
                expect (voicing.empty());
        }

        beginTest (everyChordFits);
        {
            size_t chords {};
            for (const auto& root : roots)
                for (const auto& quality : qualities)
                {
                    const auto chordName = quality == seventh ? root + quality : root + nameSeparator + quality;
                    for (const auto& voicing : ChordHelperModel::voicings (chordNamed (chordName)))
                    {
                        expect (! voicing.empty(), chordName);
                        for (const auto note : voicing)
                            expect (note >= MiniKeyboard::lowestNote && note <= MiniKeyboard::highestNote, chordName);
                    }
                    ++chords;
                }
            expectEquals (chords, libraryChords);
        }

        beginTest (keysAsBoard);
        {
            const auto keys = MiniKeyboard::keys (boardScale);
            const auto whites = static_cast<size_t> (std::count_if (keys.begin(), keys.end(), [] (const MiniKey& key) { return ! key.black; }));
            expectEquals (whites, whiteKeys);
            expectEquals (keys.size() - whites, blackKeys);
            expect (! keys.empty() && ! keys.front().black && keys.front().note == firstNote && keys.front().box == firstWhite);
            expect (! keys.empty() && keys[lastWhiteIndex].box.getX() == lastWhiteX);
            for (const auto& key : keys)
            {
                if (key.note == firstBlackNote)
                    expect (key.black && key.box == firstBlack);
                if (key.note == dSharpNote)
                    expectWithinAbsoluteError (key.box.getX(), dSharpX, keyTolerance);
            }
            for (const auto& key : MiniKeyboard::keys (doubleScale))
                if (key.note == firstBlackNote)
                    expect (key.box == firstBlack * doubleScale);
        }

        beginTest (layoutAsBoard);
        {
            const auto rects = chordHelperLayout (content, boardScale);
            expectNear (rects.search, searchRect, boardSlack);
            expectNear (rects.list, listRect, boardSlack);
            expectNear (rects.name, nameRect, boardSlack);
            expectNear (rects.hint, hintRect, boardSlack);
            for (size_t column {}; column < ChordHelperModel::voicingCount; ++column)
            {
                expectNear (rects.voicingTitle[column], { voicingX[column], titleY, voicingWidth, titleHeight }, boardSlack);
                expectNear (rects.keyboard[column], { voicingX[column], keyboardY, voicingWidth, keyboardHeight }, boardSlack);
                expectNear (rects.voicingNotes[column], { voicingX[column], notesY, voicingWidth, notesHeight }, boardSlack);
            }
        }

        beginTest (layoutScales);
        {
            const auto scaledContent = (content.toFloat() * windowScale).toNearestInt();
            const auto rects = chordHelperLayout (scaledContent, windowScale);
            expectNear (rects.search, searchRect * windowScale, scaledSlack);
            expectNear (rects.list, listRect * windowScale, scaledSlack);
            expectNear (rects.name, nameRect * windowScale, scaledSlack);
            expectNear (rects.hint, hintRect * windowScale, scaledSlack);
            for (size_t column {}; column < ChordHelperModel::voicingCount; ++column)
                expectNear (rects.keyboard[column],
                            juce::Rectangle<float> { voicingX[column], keyboardY, voicingWidth, keyboardHeight } * windowScale, scaledSlack);
        }
    }

private:
    static Chord chordNamed (const juce::String& chordName)
    {
        Chord chord;
        chord.name = chordName;
        return chord;
    }

    void expectNear (juce::Rectangle<int> actual, juce::Rectangle<float> expected, float slack)
    {
        using namespace ChordHelperModelTestValues;
        const auto message = actual.toString() + versus + expected.toString();
        expectWithinAbsoluteError (static_cast<float> (actual.getX()), expected.getX(), slack, message);
        expectWithinAbsoluteError (static_cast<float> (actual.getY()), expected.getY(), slack, message);
        expectWithinAbsoluteError (static_cast<float> (actual.getRight()), expected.getRight(), slack, message);
        expectWithinAbsoluteError (static_cast<float> (actual.getBottom()), expected.getBottom(), slack, message);
    }
};
static ChordHelperModelTest chordHelperModelTest;
