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

    const juce::String sameLookEverywhere = "panel contents keep the board's proportions, centred, on any window";
    const juce::String everythingVisible  = "every region stays inside the window on any screen shape";

    constexpr int ultraWideWidth  = 2560;   // 21:9
    constexpr int ultraWideHeight = 1080;
    constexpr int ultraWideContentWidth = 2009;   // 1200 x 1080 / 645 = 2009.3
    constexpr int ultraWideContentX     = 275;    // (2560 - 2009) / 2, rounded down
    constexpr int squareishWidth  = 1024;   // 4:3
    constexpr int squareishHeight = 768;
    constexpr int portraitWidth   = 1080;
    constexpr int portraitHeight  = 1920;
    constexpr int scaledLaptopWidth  = 1280; // 1920 x 1080 at 150 % Windows scaling
    constexpr int scaledLaptopHeight = 720;
    constexpr int tinyWidth  = 320;
    constexpr int tinyHeight = 200;
    constexpr int roundingSlack = 1;   // px lost or gained by rounding each length

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

        beginTest (sameLookEverywhere);
        {
            const juce::Rectangle<int> ultraWide { ultraWideWidth, ultraWideHeight };
            const auto wideRegions = StageScale::regionsFor (ultraWide);
            expectEquals (wideRegions.panelContent.getWidth(), ultraWideContentWidth);
            expectEquals (wideRegions.panelContent.getX(), ultraWideContentX);
            expectEquals (wideRegions.panelContent.getY(), wideRegions.panel.getY());
            expectEquals (wideRegions.panelContent.getHeight(), wideRegions.panel.getHeight());

            const juce::Rectangle<int> owner { ownerWidth, ownerHeight };
            expect (StageScale::regionsFor (owner).panelContent == StageScale::regionsFor (owner).panel);
        }

        beginTest (everythingVisible);
        {
            for (const juce::Rectangle<int> bounds : { juce::Rectangle<int> { ultraWideWidth, ultraWideHeight },
                                                       juce::Rectangle<int> { squareishWidth, squareishHeight },
                                                       juce::Rectangle<int> { portraitWidth, portraitHeight },
                                                       juce::Rectangle<int> { scaledLaptopWidth, scaledLaptopHeight },
                                                       juce::Rectangle<int> { tinyWidth, tinyHeight },
                                                       juce::Rectangle<int> { ownerWidth, ownerHeight } })
            {
                const auto regions = StageScale::regionsFor (bounds);
                expectStacked (regions, bounds);
                expectGreaterOrEqual (regions.noteArea.getHeight(), int {});
                expect (regions.panel.contains (regions.panelContent), bounds.toString());
                expectEquals (regions.panelContent.getHeight(), regions.panel.getHeight());
                expectGreaterThan (regions.panelContent.getWidth(), int {});
                expectLessOrEqual (std::abs (regions.panelContent.getCentreX() - regions.panel.getCentreX()), roundingSlack);

                // The panel contents are the board scaled uniformly: width / height = 1200 / 124 up to rounding.
                const auto expectedWidth = static_cast<float> (regions.panelContent.getHeight())
                                         * static_cast<float> (StageMetrics::referenceWidth) / static_cast<float> (StageMetrics::panelHeight);
                expectWithinAbsoluteError (static_cast<float> (regions.panelContent.getWidth()), expectedWidth,
                                           static_cast<float> (StageMetrics::referenceWidth) / static_cast<float> (StageMetrics::panelHeight) * roundingSlack);
            }
        }
    }
};
static StageScaleTest stageScaleTest;
