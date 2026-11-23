#include <JuceHeader.h>
#include "UI/Stage/StagePageFrame.h"
#include "UI/Stage/StageMetrics.h"

namespace StagePageFrameTestValues
{
    const juce::String testName     = "StagePageFrame";
    const juce::String testCategory = "Unit";

    const juce::String contentBelowTitle = "the content area sits inside the page padding, below the title row";
    const juce::String layoutOnResize    = "resizing lays the content out at the current scale";
    const juce::String pageTitle         = "Recorder";

    constexpr float ownerScale  = 1.6f;
    constexpr int   pageWidth   = 1920;
    constexpr int   pageHeight  = 585;
    constexpr int   noCalls     = 0;
    constexpr int   oneCall     = 1;
    constexpr float tolerance   = 0.0001f;
}

class StagePageFrameTest : public juce::UnitTest
{
public:
    StagePageFrameTest() : juce::UnitTest (StagePageFrameTestValues::testName, StagePageFrameTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StagePageFrameTestValues;

        beginTest (contentBelowTitle);
        {
            StagePageFrame frame { pageTitle };
            frame.setScale (ownerScale);
            frame.setSize (pageWidth, pageHeight);
            const auto content = frame.getContentBounds();
            expectEquals (frame.getTitle(), pageTitle);
            expectEquals (content.getX(), juce::roundToInt (StageMetrics::pagePaddingSide * ownerScale));
            expectGreaterThan (content.getY(), juce::roundToInt (StageMetrics::pagePaddingTop * ownerScale));
            expect (frame.getLocalBounds().contains (content));
            expectEquals (content.getRight(), pageWidth - juce::roundToInt (StageMetrics::pagePaddingSide * ownerScale));
        }

        beginTest (layoutOnResize);
        {
            StagePageFrame frame { pageTitle };
            int calls {};
            float scaleSeen {};
            juce::Rectangle<int> areaSeen;
            frame.onLayoutContent = [&] (juce::Rectangle<int> area, float scale) { ++calls; areaSeen = area; scaleSeen = scale; };
            expectEquals (calls, noCalls);
            frame.setScale (ownerScale);
            frame.setSize (pageWidth, pageHeight);
            expectGreaterOrEqual (calls, oneCall);
            expect (areaSeen == frame.getContentBounds());
            expectWithinAbsoluteError (scaleSeen, ownerScale, tolerance);
        }
    }
};
static StagePageFrameTest stagePageFrameTest;
