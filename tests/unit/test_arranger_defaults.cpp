#include <juce_core/juce_core.h>
#include "Arranger/ArrangerDefaults.h"

namespace ArrangerDefaultsTestValues
{
    const juce::String testName     = "ArrangerDefaults";
    const juce::String testCategory = "Arranger";

    const juce::String singleVariationCoversRecording = "default is a single Variation covering the whole recording";
    const juce::String emptyClampsToOneBar            = "zero or negative bars clamps to a 1-bar Variation";

    constexpr int recordingBars = 8;
    constexpr int noBars        = 0;
    constexpr int oneWindow     = 1;
    constexpr int firstWindow   = 0;
    constexpr int firstBar      = 1;
    constexpr int oneBar        = 1;
}

class ArrangerDefaultsTest : public juce::UnitTest
{
public:
    ArrangerDefaultsTest() : juce::UnitTest (ArrangerDefaultsTestValues::testName, ArrangerDefaultsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerDefaultsTestValues;

        beginTest (singleVariationCoversRecording);
        {
            auto windows = ArrangerDefaults::defaultWindowsForBars (recordingBars);
            expectEquals ((int) windows.size(), oneWindow);
            expect (windows[firstWindow].type == ArrangerSectionType::Variation);
            expectEquals (windows[firstWindow].startBar, firstBar);
            expectEquals (windows[firstWindow].lengthBars, recordingBars);
            expect (windows[firstWindow].afterComplete == ArrangerAfterComplete::Loop);
            expect (windows[firstWindow].id.isNotEmpty());
        }

        beginTest (emptyClampsToOneBar);
        {
            auto windows = ArrangerDefaults::defaultWindowsForBars (noBars);
            expectEquals ((int) windows.size(), oneWindow);
            expectEquals (windows[firstWindow].lengthBars, oneBar);
        }
    }
};
static ArrangerDefaultsTest arrangerDefaultsTest;
