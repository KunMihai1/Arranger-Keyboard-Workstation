#include "ArrangerTimelineGeometry.h"
#include <cmath>

namespace ArrangerTimelineGeometry
{
    namespace
    {
        constexpr int    marginSides   = 2;     // left and right
        constexpr double roundToNearest = 0.5;
        constexpr int    regionTop     = 0;
        constexpr int    pastLastBar   = 1;     // the bar line after the last bar is a valid snap target
    }

    double pixelsPerBar (const Layout& layout)
    {
        const int usable = juce::jmax (kMinimumPixels, layout.widthPx - marginSides * layout.marginPx);
        return (double) usable / (double) juce::jmax (kMinimumBars, layout.totalBars);
    }

    double barToX (double bar, const Layout& layout)
    {
        return (double) layout.marginPx + (bar - kFirstBar) * pixelsPerBar (layout);
    }

    int xToSnappedBar (double pixelX, const Layout& layout)
    {
        const double bar = (pixelX - (double) layout.marginPx) / pixelsPerBar (layout) + kFirstBar;
        const int snapped = (int) std::floor (bar + roundToNearest);
        return juce::jlimit (kFirstBar, layout.totalBars + pastLastBar, snapped);
    }

    juce::Rectangle<int> regionBounds (int startBar, int lengthBars, const Layout& layout, int heightPx)
    {
        const int left  = (int) std::lround (barToX ((double) startBar, layout));
        const int right = (int) std::lround (barToX ((double) (startBar + juce::jmax (kMinimumBars, lengthBars)), layout));
        return juce::Rectangle<int> (left, regionTop, juce::jmax (kMinimumPixels, right - left), heightPx);
    }

    Hit hitTest (int pixelX, const juce::Rectangle<int>& region)
    {
        if (pixelX < region.getX() - kEdgeGrabPx || pixelX > region.getRight() + kEdgeGrabPx)
            return Hit::None;
        if (std::abs (pixelX - region.getX())     <= kEdgeGrabPx) return Hit::LeftEdge;
        if (std::abs (pixelX - region.getRight())  <= kEdgeGrabPx) return Hit::RightEdge;
        if (pixelX >= region.getX() && pixelX <= region.getRight())     return Hit::Body;
        return Hit::None;
    }
}
