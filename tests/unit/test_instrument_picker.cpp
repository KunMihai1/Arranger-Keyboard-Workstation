#include <JuceHeader.h>
#include "InstrumentPicker.h"

namespace InstrumentPickerTestValues
{
    const juce::String testName     = "InstrumentPicker";
    const juce::String testCategory = "Unit";

    const juce::String familiesOfEight = "16 GM families of 8 programs; a program's family is program / 8";
    const juce::String searchAnyCase   = "search matches names in every family, any case; empty text finds nothing";
    const juce::String opensOnCurrent  = "the picker opens on the current program's family, its 8 programs shown";
    const juce::String searchAndBack   = "typing shows matches from every family; clearing returns to the family";
    const juce::String rowChooses      = "a click on an instrument row chooses that program";
    const juce::String familyClick     = "a click on a family shows its programs and clears the search";
    const juce::String layoutFits      = "search, family grid and instrument list sit inside the page, lists side by side";

    const juce::String pianoQuery = "PiAnO";
    const juce::String noQuery;
    constexpr int familyCount    = 16;
    constexpr int perFamily      = 8;
    constexpr int programCount   = 128;
    constexpr int violin         = 40;
    constexpr int stringsFamily  = 5;
    constexpr int pianoMatches   = 6;     // Acoustic Grand, Bright Acoustic, Electric Grand, Honky-tonk, E.Piano 1, 2
    constexpr int thirdRow       = 2;
    constexpr int brassFamily    = 7;
    constexpr int firstBrass     = 56;    // Trumpet
    constexpr int noProgram      = -1;
    constexpr int contentWidth   = 1144;
    constexpr int contentHeight  = 300;
    constexpr float referenceScale = 1.0f;
}

class InstrumentPickerTest : public juce::UnitTest
{
public:
    InstrumentPickerTest() : juce::UnitTest (InstrumentPickerTestValues::testName, InstrumentPickerTestValues::testCategory) {}

    static std::vector<int> familyPrograms (int family)
    {
        std::vector<int> programs;
        for (int offset {}; offset < InstrumentPickerTestValues::perFamily; ++offset)
            programs.push_back (family * InstrumentPickerTestValues::perFamily + offset);
        return programs;
    }

    void runTest() override
    {
        using namespace InstrumentPickerTestValues;

        beginTest (familiesOfEight);
        expectEquals (GmInstruments::families().size(), familyCount);
        expectEquals (getGMInstrumentNames().size(), programCount);
        expectEquals (GmInstruments::familyOf (violin), stringsFamily);

        beginTest (searchAnyCase);
        const auto pianos = GmInstruments::search (pianoQuery);
        expectEquals (static_cast<int> (pianos.size()), pianoMatches);
        expect (GmInstruments::search (noQuery).empty());

        InstrumentPicker picker (violin);
        picker.setSize (contentWidth, contentHeight);

        beginTest (opensOnCurrent);
        expectEquals (picker.getSelectedFamily(), stringsFamily);
        expect (picker.getShownPrograms() == familyPrograms (stringsFamily));

        beginTest (searchAndBack);
        picker.setSearchText (pianoQuery);
        expect (picker.getShownPrograms() == pianos);
        picker.setSearchText (noQuery);
        expect (picker.getShownPrograms() == familyPrograms (stringsFamily));

        beginTest (rowChooses);
        int chosen = noProgram;
        picker.onChosen = [&chosen] (int program) { chosen = program; };
        picker.chooseRow (thirdRow);
        expectEquals (chosen, stringsFamily * perFamily + thirdRow);

        beginTest (familyClick);
        picker.setSearchText (pianoQuery);
        picker.selectFamily (brassFamily);
        expect (picker.getSearchText().isEmpty());
        expect (picker.getShownPrograms() == familyPrograms (brassFamily));
        expectEquals (familyPrograms (brassFamily).front(), firstBrass);

        beginTest (layoutFits);
        const juce::Rectangle<int> content { contentWidth, contentHeight };
        const auto rects = instrumentPickerLayout (content, referenceScale);
        for (const auto& part : { rects.search, rects.familyList, rects.instrumentList })
            expect (content.contains (part));
        expect (rects.familyList.getRight() < rects.instrumentList.getX());
        expect (rects.familyList.getY() >= rects.search.getBottom());
        expectEquals (rects.familyList.getY(), rects.instrumentList.getY());
    }
};

static InstrumentPickerTest instrumentPickerTest;
