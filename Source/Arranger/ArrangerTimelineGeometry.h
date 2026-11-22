#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ArrangerTimelineGeometry
{
    constexpr int kFirstBar      = 1;   // bars are 1-based on the timeline
    constexpr int kMinimumBars   = 1;
    constexpr int kMinimumPixels = 1;

    struct Layout
    {
        int totalBars = kFirstBar;
        int widthPx   {};
        int marginPx  {};   // equal left/right margin
    };

    enum class Hit { None, Body, LeftEdge, RightEdge };

    constexpr int kEdgeGrabPx = 6;

    double pixelsPerBar (const Layout& layout);
    double barToX (double bar, const Layout& layout);            // bar is 1-based, may be fractional
    int    xToSnappedBar (double pixelX, const Layout& layout);  // nearest bar line, clamp [1, totalBars+1]
    juce::Rectangle<int> regionBounds (int startBar, int lengthBars, const Layout& layout, int heightPx);
    Hit    hitTest (int pixelX, const juce::Rectangle<int>& region);
}
