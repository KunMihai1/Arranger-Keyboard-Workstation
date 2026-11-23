#include <JuceHeader.h>
#include "UI/chordBrowser.h"

namespace ChordBrowserTestValues
{
    const juce::String testName     = "ChordBrowser";
    const juce::String testCategory = "Unit";

    const juce::String escapeReported = "Esc in the search box is reported, so the Chords page can close (the box keeps Esc to itself)";
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
            if (search != nullptr)
                browser.textEditorEscapeKeyPressed (*search);
            expect (escaped);
        }
    }
};
static ChordBrowserTest chordBrowserTest;
