#include <JuceHeader.h>
#include "UI/Stage/StageScale.h"
#include "UI/Stage/StageMetrics.h"

namespace StageScaleTestValues
{
    const juce::String testName     = "StageScale";
    const juce::String testCategory = "Unit";

    const juce::String ownerScreen      = "1920 x 1032 scales the reference by 1.6";
    const juce::String referenceSize    = "1200 x 645 gives the reference regions exactly";
    const juce::String narrowWindow     = "a narrow window is limited by its width";
    const juce::String shortWindow      = "a very short window keeps the note area at zero or more";
    const juce::String regionsAreStacked = "the regions are stacked top to bottom without gaps";

    constexpr int ownerWidth   = 1920;
    constexpr int ownerHeight  = 1032;
    constexpr float ownerScale = 1.6f;
    constexpr int ownerPanel   = 198;   // 124 x 1.6 = 198.4
    constexpr int ownerFelt    = 6;     // 4 x 1.6 = 6.4
    constexpr int ownerKeys    = 224;   // 140 x 1.6
    constexpr int ownerSlip    = 19;    // 12 x 1.6 = 19.2
    constexpr int ownerNotes   = ownerHeight - ownerPanel - ownerFelt - ownerKeys - ownerSlip;

    constexpr int narrowWidth  = 1000;
    constexpr int narrowHeight = 900;
    constexpr int shortWidth   = 1920;
    constexpr int shortHeight  = 500;

    constexpr float scaleTolerance = 0.0001f;
}

class StageScaleTest : public juce::UnitTest
{
    void expectStacked (const StageRegions& regions, juce::Rectangle<int> bounds)
    {
        expectEquals (regions.panel.getY(), bounds.getY());
        expectEquals (regions.felt.getY(), regions.panel.getBottom());
        expectEquals (regions.noteArea.getY(), regions.felt.getBottom());
        expectEquals (regions.keys.getY(), regions.noteArea.getBottom());
        expectEquals (regions.slip.getY(), regions.keys.getBottom());
        expectEquals (regions.slip.getBottom(), bounds.getBottom());

        for (const auto& region : { regions.panel, regions.felt, regions.noteArea, regions.keys, regions.slip })
        {
            expectEquals (region.getX(), bounds.getX());
            expectEquals (region.getWidth(), bounds.getWidth());
        }
    }

public:
    StageScaleTest() : juce::UnitTest (StageScaleTestValues::testName, StageScaleTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageScaleTestValues;

        beginTest (ownerScreen);
        {
            const juce::Rectangle<int> bounds { ownerWidth, ownerHeight };
            const auto regions = StageScale::regionsFor (bounds);
            expectWithinAbsoluteError (regions.scale, ownerScale, scaleTolerance);
            expectEquals (regions.panel.getHeight(), ownerPanel);
            expectEquals (regions.felt.getHeight(), ownerFelt);
            expectEquals (regions.keys.getHeight(), ownerKeys);
            expectEquals (regions.slip.getHeight(), ownerSlip);
            expectEquals (regions.noteArea.getHeight(), ownerNotes);
        }

        beginTest (referenceSize);
        {
            const juce::Rectangle<int> bounds { StageMetrics::referenceWidth, StageMetrics::referenceHeight };
            const auto regions = StageScale::regionsFor (bounds);
            expectWithinAbsoluteError (regions.scale, StageMetrics::referenceScale, scaleTolerance);
            expectEquals (regions.panel.getHeight(), StageMetrics::panelHeight);
            expectEquals (regions.felt.getHeight(), StageMetrics::feltHeight);
            expectEquals (regions.noteArea.getHeight(), StageMetrics::noteAreaHeight);
            expectEquals (regions.keys.getHeight(), StageMetrics::keysHeight);
            expectEquals (regions.slip.getHeight(), StageMetrics::slipHeight);
        }

        beginTest (narrowWindow);
        {
            const juce::Rectangle<int> bounds { narrowWidth, narrowHeight };
            const auto regions = StageScale::regionsFor (bounds);
            const auto widthScale = static_cast<float> (narrowWidth) / static_cast<float> (StageMetrics::referenceWidth);
            expectWithinAbsoluteError (regions.scale, widthScale, scaleTolerance);
        }

        beginTest (shortWindow);
        {
            const juce::Rectangle<int> bounds { shortWidth, shortHeight };
            const auto regions = StageScale::regionsFor (bounds);
            const auto heightScale = static_cast<float> (shortHeight) / static_cast<float> (StageMetrics::referenceHeight);
            expectWithinAbsoluteError (regions.scale, heightScale, scaleTolerance);
            expectGreaterOrEqual (regions.noteArea.getHeight(), int {});
        }

        beginTest (regionsAreStacked);
        {
            for (const juce::Rectangle<int> bounds : { juce::Rectangle<int> { ownerWidth, ownerHeight },
                                                       juce::Rectangle<int> { StageMetrics::referenceWidth, StageMetrics::referenceHeight },
                                                       juce::Rectangle<int> { narrowWidth, narrowHeight },
                                                       juce::Rectangle<int> { shortWidth, shortHeight } })
                expectStacked (StageScale::regionsFor (bounds), bounds);
        }
    }
};
static StageScaleTest stageScaleTest;
