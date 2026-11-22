#include "ArrangerTimelineComponent.h"
#include "ArrangerEnums.h"
#include "AppColours.h"
#include <cmath>

using namespace ArrangerTimelineGeometry;

namespace
{
    constexpr int   marginSides        = 2;      // left and right
    constexpr int   regionTop          = 18;     // regions sit below the bar numbers
    constexpr int   regionBottomSpace  = 26;     // ruler + bottom gap taken from the height
    constexpr int   pastLastBar        = 1;      // the bar line after the last bar
    constexpr int   oneWindow          = 1;      // size() - oneWindow is the last window's index
    constexpr int   lastBarOffset      = 1;      // a region of N bars ends N - 1 bars after its start
    constexpr int   barNumberInsetX    = 3;
    constexpr int   barNumberSpareX    = 4;
    constexpr int   barNumberTop       = 0;
    constexpr int   barNumberHeight    = 14;
    constexpr float gridTop            = 0.0f;
    constexpr float selectedAlpha      = 0.85f;
    constexpr float unselectedAlpha    = 0.55f;
    constexpr float regionCornerRadius = 4.0f;
    constexpr float selectedOutline    = 2.0f;
    constexpr float unselectedOutline  = 1.0f;
    constexpr int   labelInsetX        = 6;
    constexpr int   labelInsetY        = 2;
    constexpr int   dragRepeatMs       = 40;     // keep firing mouseDrag while held at an edge
    constexpr int   stopDragRepeat     = 0;
    constexpr int   autoScrollStartPx  = 30;     // distance from the viewport edge that starts scrolling
    constexpr int   autoScrollSpeed    = 30;

    const juce::String tooltipBarsOpen  = "  (bars ";
    const juce::String tooltipBarsDash  = "-";
    const juce::String tooltipBarsClose = ")";
}

Layout ArrangerTimelineComponent::layout() const
{
    return Layout { juce::jmax (kMinimumBars, totalBars), getWidth(), marginPx };
}

int ArrangerTimelineComponent::getPreferredWidth() const
{
    return marginPx * marginSides + juce::jmax (kMinimumBars, totalBars) * kPixelsPerBar;
}

int ArrangerTimelineComponent::xForBar (double bar) const
{
    return (int) std::lround (barToX (bar, layout()));
}

int ArrangerTimelineComponent::barForX (int pixelX) const
{
    return ArrangerTimelineGeometry::xToSnappedBar ((double) pixelX, layout());
}

juce::Colour ArrangerTimelineComponent::colourForType (ArrangerSectionType sectionType) const
{
    switch (sectionType)
    {
        case ArrangerSectionType::Intro:     return AppColours::sectionIntro;
        case ArrangerSectionType::Variation: return AppColours::sectionVariation;
        case ArrangerSectionType::Fill:      return AppColours::sectionFill;
        case ArrangerSectionType::Break:     return AppColours::sectionBreak;
        case ArrangerSectionType::Ending:    return AppColours::sectionEnding;
        case ArrangerSectionType::CountIn:   return AppColours::sectionCountIn;
    }
    return juce::Colours::grey;
}

void ArrangerTimelineComponent::setTotalBars (int bars)     { totalBars = juce::jmax (kMinimumBars, bars); repaint(); }
void ArrangerTimelineComponent::setWindows (const std::vector<SectionWindow>& newWindows)
{
    windows = newWindows;
    if (selectedIndex >= (int) windows.size())   // a removed/shrunk list can leave a stale selection
        selectedIndex = ArrangerStyleDefaults::noIndex;
    repaint();
}
void ArrangerTimelineComponent::setPlayheadBar (double bar) { playheadBar = bar; repaint(); }

juce::String ArrangerTimelineComponent::getTooltip()
{
    const auto pos = getMouseXYRelative();
    const auto currentLayout = layout();
    const int regionHeight = getHeight() - regionBottomSpace;
    for (int windowIndex = (int) windows.size() - oneWindow; windowIndex >= int {}; --windowIndex)   // topmost (last drawn) first
    {
        const auto& window = windows[(size_t) windowIndex];
        auto region = regionBounds (window.startBar, window.lengthBars, currentLayout, regionHeight).withY (regionTop);
        if (region.contains (pos))
        {
            const int endBar = window.startBar + window.lengthBars - lastBarOffset;
            return window.name + tooltipBarsOpen + juce::String (window.startBar)
                 + tooltipBarsDash + juce::String (endBar) + tooltipBarsClose;
        }
    }
    return {};
}

void ArrangerTimelineComponent::paint (juce::Graphics& g)
{
    const auto currentLayout = layout();
    const double barWidth = pixelsPerBar (currentLayout);
    const auto clip = g.getClipBounds();   // only paint what's actually visible (cheap when scrolled)
    g.fillAll (AppColours::timelineBackground);

    // bar grid + numbers — restricted to the visible bar range
    int firstBar = (int) std::floor (((double) clip.getX()     - marginPx) / barWidth) + kFirstBar;
    int lastBar  = (int) std::ceil  (((double) clip.getRight() - marginPx) / barWidth) + kFirstBar;
    firstBar = juce::jlimit (kFirstBar, totalBars + pastLastBar, firstBar);
    lastBar  = juce::jlimit (kFirstBar, totalBars + pastLastBar, lastBar);

    for (int bar = firstBar; bar <= lastBar; ++bar)
    {
        const int barX = (int) std::lround (barToX ((double) bar, currentLayout));
        g.setColour (AppColours::timelineBarLine);
        g.drawVerticalLine (barX, gridTop, (float) getHeight());
        if (bar <= totalBars)
        {
            g.setColour (AppColours::timelineBarNumber);
            g.drawText (juce::String (bar), barX + barNumberInsetX, barNumberTop, kPixelsPerBar - barNumberSpareX,
                        barNumberHeight, juce::Justification::left);
        }
    }

    // section regions (skip those outside the visible clip)
    const int regionHeight = getHeight() - regionBottomSpace;
    for (int windowIndex {}; windowIndex < (int) windows.size(); ++windowIndex)
    {
        const auto& window = windows[(size_t) windowIndex];
        auto region = regionBounds (window.startBar, window.lengthBars, currentLayout, regionHeight)
                          .withY (regionTop);
        if (region.getRight() < clip.getX() || region.getX() > clip.getRight())
            continue;
        const bool isSelected = windowIndex == selectedIndex;
        auto typeColour = colourForType (window.type);
        g.setColour (typeColour.withAlpha (isSelected ? selectedAlpha : unselectedAlpha));
        g.fillRoundedRectangle (region.toFloat(), regionCornerRadius);
        g.setColour (isSelected ? juce::Colours::white : typeColour);
        g.drawRoundedRectangle (region.toFloat(), regionCornerRadius, isSelected ? selectedOutline : unselectedOutline);
        g.setColour (juce::Colours::white);
        g.drawText (window.name, region.reduced (labelInsetX, labelInsetY), juce::Justification::centredLeft, true);
    }

    // playhead
    const int playheadX = (int) std::lround (barToX (playheadBar, currentLayout));
    g.setColour (juce::Colours::orange);
    g.drawVerticalLine (playheadX, (float) regionTop, (float) (regionTop + regionHeight));
}

void ArrangerTimelineComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto currentLayout = layout();
    const int regionHeight = getHeight() - regionBottomSpace;
    selectedIndex = ArrangerStyleDefaults::noIndex;
    dragMode = Hit::None;
    windowsChangedByDrag = false;   // a bare click (no drag) must NOT rebuild the style

    for (int windowIndex = (int) windows.size() - oneWindow; windowIndex >= int {}; --windowIndex)
    {
        const auto& window = windows[(size_t) windowIndex];
        auto region = regionBounds (window.startBar, window.lengthBars, currentLayout, regionHeight).withY (regionTop);
        if (event.y >= region.getY() && event.y <= region.getBottom())
        {
            const auto hit = ArrangerTimelineGeometry::hitTest (event.x, region);
            if (hit != Hit::None)
            {
                selectedIndex      = windowIndex;
                dragMode           = hit;
                dragStartLenValue  = window.lengthBars;
                dragGrabOffsetBars = ArrangerTimelineGeometry::xToSnappedBar ((double) event.x, currentLayout) - window.startBar;
                juce::Desktop::getInstance().beginDragAutoRepeat (dragRepeatMs);
                break;
            }
        }
    }

    if (onSectionSelected) onSectionSelected (selectedIndex);
    repaint();
}

void ArrangerTimelineComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (selectedIndex == ArrangerStyleDefaults::noIndex || dragMode == Hit::None) return;
    const auto currentLayout = layout();
    auto& window = windows[(size_t) selectedIndex];

    const int oldStart = window.startBar, oldLen = window.lengthBars;   // detect a *real* edit (not jitter)
    const int barUnderCursor = xToSnappedBar ((double) event.x, currentLayout);

    if (dragMode == Hit::Body)
    {
        // Absolute positioning via the grab offset stays correct while the view auto-scrolls.
        window.startBar = juce::jlimit (kFirstBar, totalBars + pastLastBar - dragStartLenValue, barUnderCursor - dragGrabOffsetBars);
    }
    else if (dragMode == Hit::LeftEdge)
    {
        const int newStart = juce::jlimit (kFirstBar, window.startBar + window.lengthBars - kMinimumBars, barUnderCursor);
        window.lengthBars = (window.startBar + window.lengthBars) - newStart;
        window.startBar   = newStart;
    }
    else if (dragMode == Hit::RightEdge)
    {
        const int newEnd = juce::jlimit (window.startBar + kMinimumBars, totalBars + pastLastBar, barUnderCursor);
        window.lengthBars = newEnd - window.startBar;
    }

    // Only count this gesture as a window edit if a bar value actually moved. Sub-bar mouse jitter
    // during a plain click leaves the snapped values unchanged -> stays a "click to jump", not an edit
    // (an edit triggers rebuildPreview, which resets the sequencer and would drop a queued switch).
    if (window.startBar != oldStart || window.lengthBars != oldLen)
        windowsChangedByDrag = true;

    // Drag near a viewport edge -> scroll horizontally to follow.
    if (auto* viewport = findParentComponentOfClass<juce::Viewport>())
    {
        const auto inViewport = viewport->getLocalPoint (this, event.getPosition());
        viewport->autoScroll (inViewport.x, inViewport.y, autoScrollStartPx, autoScrollSpeed);
    }

    repaint();
}

void ArrangerTimelineComponent::mouseUp (const juce::MouseEvent&)
{
    juce::Desktop::getInstance().beginDragAutoRepeat (stopDragRepeat);   // stop the auto-scroll repeat
    // Only rebuild the style if a real drag edited the windows. A bare click is a "jump to section"
    // gesture (it queues a switch); rebuilding here would reset the sequencer and drop that switch.
    if (windowsChangedByDrag && onWindowsChanged)
        onWindowsChanged (windows);
    dragMode = Hit::None;
    windowsChangedByDrag = false;
}
