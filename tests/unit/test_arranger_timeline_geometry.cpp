#include <juce_gui_basics/juce_gui_basics.h>
#include "Arranger/ArrangerTimelineGeometry.h"

namespace ArrangerTimelineGeometryTestValues
{
    const juce::String testName     = "ArrangerTimelineGeometry";
    const juce::String testCategory = "Arranger";

    const juce::String barsMapToX       = "bar 1 maps to the left margin, bar 9 (end) to the right edge";
    const juce::String xSnapsToBarLine  = "x snaps to the nearest bar line, clamped to [1, totalBars+1]";
    const juce::String regionSpansBars  = "region bounds span its bars";
    const juce::String hitTestZones     = "hit-test detects left edge, right edge, body, and miss";

    // 8 bars across an 820 px view with 10 px margins: 800 usable px, 100 px per bar.
    constexpr int    totalBars   = 8;
    constexpr int    viewWidth   = 820;
    constexpr int    margin      = 10;
    constexpr double tolerance   = 1e-6;

    constexpr double firstBar    = 1.0;
    constexpr double endBar      = 9.0;
    constexpr double middleBar   = 5.0;
    constexpr double leftMarginX = 10.0;
    constexpr double endX        = 810.0;
    constexpr double middleX     = 410.0;

    constexpr double atBarOne       = 10.0;
    constexpr double justBeforeHalf = 59.0;    // under 50 px past bar 1: stays on bar 1
    constexpr double justAfterHalf  = 61.0;    // over 50 px: snaps to bar 2
    constexpr double farLeft        = -100.0;
    constexpr double farRight       = 9999.0;
    constexpr int    barOne         = 1;
    constexpr int    barTwo         = 2;
    constexpr int    lastBarLine    = 9;       // totalBars + 1

    constexpr int regionStartBar = 5;
    constexpr int regionBars     = 2;
    constexpr int regionHeight   = 40;
    constexpr int regionX        = 410;
    constexpr int regionWidth    = 200;        // 2 bars * 100 px

    constexpr int nearLeftEdge  = 412;         // the region spans x 410..610
    constexpr int nearRightEdge = 608;
    constexpr int insideBody    = 500;
    constexpr int outside       = 700;
}

class ArrangerTimelineGeometryTest : public juce::UnitTest
{
public:
    ArrangerTimelineGeometryTest() : juce::UnitTest (ArrangerTimelineGeometryTestValues::testName, ArrangerTimelineGeometryTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerTimelineGeometry;
        using namespace ArrangerTimelineGeometryTestValues;
        Layout view { totalBars, viewWidth, margin };

        beginTest (barsMapToX);
        {
            expectWithinAbsoluteError (barToX (firstBar, view), leftMarginX, tolerance);
            expectWithinAbsoluteError (barToX (endBar, view), endX, tolerance);       // 8 bars later
            expectWithinAbsoluteError (barToX (middleBar, view), middleX, tolerance); // 4 bars in
        }

        beginTest (xSnapsToBarLine);
        {
            expectEquals (xToSnappedBar (atBarOne, view), barOne);
            expectEquals (xToSnappedBar (justBeforeHalf, view), barOne);
            expectEquals (xToSnappedBar (justAfterHalf, view), barTwo);
            expectEquals (xToSnappedBar (farLeft, view), barOne);       // clamp low
            expectEquals (xToSnappedBar (farRight, view), lastBarLine); // clamp high (totalBars+1)
        }

        beginTest (regionSpansBars);
        {
            auto region = regionBounds (regionStartBar, regionBars, view, regionHeight);
            expectEquals (region.getX(), regionX);
            expectEquals (region.getWidth(), regionWidth);
            expectEquals (region.getHeight(), regionHeight);
        }

        beginTest (hitTestZones);
        {
            auto region = regionBounds (regionStartBar, regionBars, view, regionHeight);
            expect (hitTest (nearLeftEdge, region) == Hit::LeftEdge);
            expect (hitTest (nearRightEdge, region) == Hit::RightEdge);
            expect (hitTest (insideBody, region) == Hit::Body);
            expect (hitTest (outside, region) == Hit::None);
        }
    }
};
static ArrangerTimelineGeometryTest arrangerTimelineGeometryTest;
