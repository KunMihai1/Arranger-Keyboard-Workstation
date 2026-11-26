#include <JuceHeader.h>
#include "UI/chordBrowser.h"

namespace ChordBrowserTestValues
{
    const juce::String testName     = "ChordBrowser";
    const juce::String testCategory = "Unit";

    const juce::String escapeReported = "Esc in the search box is reported, so the Chords page can close (the box keeps Esc to itself)";
    const juce::String opensOnFirst   = "the page opens showing the first chord";
    const juce::String clickShowsAndReports = "a click on a row shows that chord and reports it (it plays)";
    const juce::String searchFilters  = "the search keeps the chords whose name contains the text, any case; the shown chord stays";

    const juce::String cMajor   = "C Major";
    const juce::String cMinor   = "C Minor";
    const juce::String aMinor   = "A Minor";
    const juce::String minorQuery = "MINOR";
    const juce::String noMatch  = "xyz";
    constexpr int allRows       = 3;
    constexpr int minorRows     = 2;
    constexpr int noRows        = 0;
    constexpr int secondRow     = 1;
    constexpr int firstRow      = 0;
}

class ChordBrowserTest : public juce::UnitTest
{
public:
    ChordBrowserTest() : juce::UnitTest (ChordBrowserTestValues::testName, ChordBrowserTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ChordBrowserTestValues;

        beginTest (escapeReported);
        {
            ChordBrowserComponent browser { {} };
            juce::TextEditor* search = nullptr;
            for (auto* child : browser.getChildren())
                if (auto* editor = dynamic_cast<juce::TextEditor*> (child))
                    search = editor;
            expect (search != nullptr);

            bool escaped = false;
            browser.onEscape = [&escaped] { escaped = true; };
            if (search != nullptr && search->onEscapeKey)
                search->onEscapeKey();
            expect (escaped);
        }

        beginTest (opensOnFirst);
        {
            ChordBrowserComponent browser { library() };
            expectEquals (browser.getChosenName(), cMajor);
            expectEquals (browser.getNumShown(), allRows);
        }

        beginTest (clickShowsAndReports);
        {
            ChordBrowserComponent browser { library() };
            juce::String reported;
            browser.onChordChosen = [&reported] (const Chord& chord) { reported = chord.name; };
            browser.chooseShownRow (secondRow);
            expectEquals (browser.getChosenName(), cMinor);
            expectEquals (reported, cMinor);
        }

        beginTest (searchFilters);
        {
            ChordBrowserComponent browser { library() };
            browser.setSearchText (minorQuery);
            expectEquals (browser.getNumShown(), minorRows);
            expectEquals (browser.getChosenName(), cMajor);   // filtered out of the list, still shown on the right
            browser.chooseShownRow (secondRow);
            expectEquals (browser.getChosenName(), aMinor);
            browser.setSearchText (noMatch);
            expectEquals (browser.getNumShown(), noRows);
            browser.chooseShownRow (firstRow);                  // nothing there: nothing changes
            expectEquals (browser.getChosenName(), aMinor);
            browser.setSearchText ({});
            expectEquals (browser.getNumShown(), allRows);
        }
    }

private:
    static std::vector<Chord> library()
    {
        using namespace ChordBrowserTestValues;
        std::vector<Chord> chords;
        for (const auto& chordName : { cMajor, cMinor, aMinor })
        {
            Chord chord;
            chord.name = chordName;
            chords.push_back (chord);
        }
        return chords;
    }
};
static ChordBrowserTest chordBrowserTest;
