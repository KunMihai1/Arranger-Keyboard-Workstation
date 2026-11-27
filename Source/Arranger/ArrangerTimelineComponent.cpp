#include "ArrangerTimelineComponent.h"
#include "ArrangerEnums.h"
#include "AppColours.h"
#include "UI/Stage/StageFonts.h"
#include <cmath>

using namespace ArrangerTimelineGeometry;

namespace
{
    constexpr int   oneWindow          = 1;      // size() - oneWindow is the last window's index
    constexpr int   lastBarOffset      = 1;      // a region of N bars ends N - 1 bars after its start
    constexpr int   noMargin           = 0;      // the board starts bar 1 at the lanes' left edge
    constexpr float half               = 0.5f;
    constexpr float glowSpread         = 1.0f;   // the selected window's outer ring and the playhead's glow
    constexpr float outlineWidth       = 1.0f;
    constexpr float shadowDrop         = 1.0f;   // the label's text-shadow: 0 1px
    constexpr int   dragRepeatMs       = 40;     // keep firing mouseDrag while held at an edge
    constexpr int   stopDragRepeat     = 0;
    constexpr int   autoScrollStartPx  = 30;     // distance from the viewport edge that starts scrolling
    constexpr int   autoScrollSpeed    = 30;
    constexpr int   glowWidthFactor    = 3;      // the playhead's glow: three times its width
    const juce::String tooltipBarsOpen  = "  (bars ";
    const juce::String tooltipBarsDash  = "-";
    const juce::String tooltipBarsClose = ")";
}

float ArrangerTimelineComponent::scaled (float boardPixels) const { return boardPixels * scale; }

Layout ArrangerTimelineComponent::layout() const
{
    return Layout { juce::jmax (kMinimumBars, totalBars), getWidth(), noMargin };
}

int ArrangerTimelineComponent::getBarWidth() const { return SectionLanes::barWidth (zoomLevel, scale); }

int ArrangerTimelineComponent::getPreferredWidth() const
{
    return juce::jmax (kMinimumBars, totalBars) * getBarWidth();
}

int ArrangerTimelineComponent::getPreferredHeight() const
{
    return SectionLanes::contentHeight (getLaneCount(), scale);
}

int ArrangerTimelineComponent::getLaneCount() const { return SectionLanes::laneCount (static_cast<int> (laneNotes.size())); }

int ArrangerTimelineComponent::xForBar (double bar) const
{
    return (int) std::lround (barToX (bar, layout()));
}

int ArrangerTimelineComponent::barForX (int pixelX) const
{
    return ArrangerTimelineGeometry::xToSnappedBar ((double) pixelX, layout());
}

juce::Colour ArrangerTimelineComponent::colourForType (ArrangerSectionType sectionType)
{
    switch (sectionType)
    {
        case ArrangerSectionType::Intro:     return StagePalette::sectionIntro;
        case ArrangerSectionType::Variation: return StagePalette::sectionVariation;
        case ArrangerSectionType::Fill:      return StagePalette::sectionFill;
        case ArrangerSectionType::Break:     return StagePalette::sectionBreak;
        case ArrangerSectionType::Ending:    return StagePalette::sectionEnding;
        case ArrangerSectionType::CountIn:   break;
    }
    return StagePalette::sectionCountIn;
}

void ArrangerTimelineComponent::setTotalBars (int bars)     { totalBars = juce::jmax (kMinimumBars, bars); repaint(); }

void ArrangerTimelineComponent::setWindows (const std::vector<SectionWindow>& newWindows)
{
    windows = newWindows;
    if (selectedIndex >= (int) windows.size())   // a removed/shrunk list can leave a stale selection
        selectedIndex = ArrangerStyleDefaults::noIndex;
    repaint();
}

void ArrangerTimelineComponent::setSelectedIndex (int index)
{
    selectedIndex = juce::isPositiveAndBelow (index, (int) windows.size()) ? index : ArrangerStyleDefaults::noIndex;
    repaint();
}

void ArrangerTimelineComponent::setSourceTracks (const std::vector<SourceTrackFile>& tracks, double newBeatsPerBar)
{
    beatsPerBar = newBeatsPerBar;
    laneNotes.clear();
    lanePitches.clear();
    for (const auto& track : tracks)
    {
        laneNotes.push_back (SectionLanes::laneNotes (track.events));
        lanePitches.push_back (SectionLanes::pitchRange (laneNotes.back()));
    }
    repaint();
}

void ArrangerTimelineComponent::setPlayheadBar (double bar) { playheadBar = bar; repaint(); }
void ArrangerTimelineComponent::setScale (float newScale)   { scale = newScale; repaint(); }
void ArrangerTimelineComponent::setZoomLevel (int level)    { zoomLevel = SectionLanes::clampZoomLevel (level); repaint(); }

juce::Rectangle<int> ArrangerTimelineComponent::windowRegion (int windowIndex, const Layout& currentLayout) const
{
    const auto& window = windows[(size_t) windowIndex];
    const int top = juce::roundToInt (scaled ((float) SectionLaneMetrics::rulerHeight));
    return regionBounds (window.startBar, window.lengthBars, currentLayout, juce::jmax (kMinimumPixels, getHeight() - top)).withY (top);
}

juce::String ArrangerTimelineComponent::getTooltip()
{
    const auto pos = getMouseXYRelative();
    const auto currentLayout = layout();
    for (int windowIndex = (int) windows.size() - oneWindow; windowIndex >= int {}; --windowIndex)   // topmost (last drawn) first
    {
        const auto& window = windows[(size_t) windowIndex];
        if (windowRegion (windowIndex, currentLayout).contains (pos))
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
    using namespace SectionLaneMetrics;
    const auto currentLayout = layout();
    const double barWidth = pixelsPerBar (currentLayout);
    const auto clip = g.getClipBounds();   // only paint what's actually visible (cheap when scrolled)

    g.fillAll (StagePalette::timelineLanes);

    // Bar lines (a stronger one every 4 bars) and numbers on every other bar — the visible range only.
    int firstBar = (int) std::floor ((double) clip.getX()     / barWidth) + kFirstBar;
    int lastBar  = (int) std::ceil  ((double) clip.getRight() / barWidth) + kFirstBar;
    firstBar = juce::jlimit (kFirstBar, totalBars + kPastLastBar, firstBar);
    lastBar  = juce::jlimit (kFirstBar, totalBars + kPastLastBar, lastBar);
    g.setFont (StageFonts::lcd (scaled (barNumberSize)));
    for (int bar = firstBar; bar <= lastBar; ++bar)
    {
        const int barX = (int) std::lround (barToX ((double) bar, currentLayout));
        g.setColour (SectionLanes::isStrongBarLine (bar) ? StagePalette::timelineBarLineStrong : StagePalette::timelineBarLine);
        g.drawVerticalLine (barX, float {}, (float) getHeight());
        if (bar <= totalBars && SectionLanes::showsBarNumber (bar))
        {
            g.setColour (StagePalette::laneBarNumber);
            g.drawText (juce::String (bar), juce::Rectangle<float> ((float) barX + scaled (barNumberInsetX), scaled (barNumberTop),
                                                                   (float) barWidth * SectionLaneMetrics::numberEvery, scaled (rulerHeight - barNumberTop)),
                        juce::Justification::topLeft, false);
        }
    }

    paintNotes (g, currentLayout, clip);

    for (int windowIndex {}; windowIndex < (int) windows.size(); ++windowIndex)
    {
        const auto region = windowRegion (windowIndex, currentLayout);
        if (region.getRight() >= clip.getX() && region.getX() <= clip.getRight())   // skip those outside the clip
            paintWindow (g, windowIndex, currentLayout);
    }

    // Playhead, with its glow.
    const float playheadX = (float) barToX (playheadBar, currentLayout);
    const float width = scaled (playheadWidth);
    g.setColour (StagePalette::tempoAccent.withAlpha (StagePalette::playheadGlowAlpha));
    g.fillRect (juce::Rectangle<float> (playheadX - width * glowWidthFactor * half, float {}, width * glowWidthFactor, (float) getHeight()));
    g.setColour (StagePalette::tempoAccent);
    g.fillRect (juce::Rectangle<float> (playheadX - width * half, float {}, width, (float) getHeight()));
}

void ArrangerTimelineComponent::paintNotes (juce::Graphics& g, const Layout& currentLayout, juce::Rectangle<int> clip) const
{
    using namespace SectionLaneMetrics;
    // Each note as a bar from its start to its release, high notes higher in the lane (a mini piano roll).
    const double beatWidth = pixelsPerBar (currentLayout) / juce::jmax (half, (float) beatsPerBar);
    const double firstVisibleBeat = (double) clip.getX() / beatWidth;
    const double lastVisibleBeat  = (double) clip.getRight() / beatWidth;
    g.setColour (StagePalette::laneNote);

    for (size_t lane {}; lane < laneNotes.size(); ++lane)
    {
        const auto laneBox = SectionLanes::laneBounds ((int) lane, getWidth(), scale);
        for (const auto& note : laneNotes[lane])
        {
            if (note.startBeat > lastVisibleBeat)
                break;   // in start order: the rest are further right
            if (note.startBeat + note.lengthBeats < firstVisibleBeat)
                continue;
            const float left  = (float) (note.startBeat * beatWidth);
            const float width = juce::jmax (scaled (outlineWidth), (float) (note.lengthBeats * beatWidth) - scaled (noteGap));
            g.fillRect (juce::Rectangle<float> (left, SectionLanes::noteTop (note.pitch, lanePitches[lane], laneBox, scale),
                                                width, scaled (noteThickness)));
        }
    }
}

void ArrangerTimelineComponent::paintWindow (juce::Graphics& g, int windowIndex, const Layout& currentLayout) const
{
    using namespace SectionLaneMetrics;
    const auto& window = windows[(size_t) windowIndex];
    const auto region = windowRegion (windowIndex, currentLayout).toFloat();
    const bool isSelected = windowIndex == selectedIndex;
    const auto typeColour = colourForType (window.type);
    const float corner = scaled (windowCorner);

    g.setColour (typeColour.withAlpha (isSelected ? StagePalette::windowSelectedFillAlpha : StagePalette::windowFillAlpha));
    g.fillRoundedRectangle (region, corner);
    if (isSelected)
    {
        g.setColour (StagePalette::windowLabel.withAlpha (StagePalette::windowSelectedGlowAlpha));
        g.drawRoundedRectangle (region.expanded (glowSpread), corner, outlineWidth);
    }
    g.setColour (isSelected ? StagePalette::windowLabel : typeColour);
    g.drawRoundedRectangle (region.reduced (outlineWidth * half), corner, outlineWidth);

    // Label "name · n bars", with the board's text-shadow; cut at the window's edge, and hidden under the
    // windows drawn over this one (two windows added at the same bar no longer print over each other).
    {
        juce::Graphics::ScopedSaveState keepClip (g);
        std::vector<juce::Range<int>> covers;
        for (int above = windowIndex + oneWindow; above < (int) windows.size(); ++above)
        {
            const auto cover = windowRegion (above, currentLayout);
            g.excludeClipRegion (cover);
            covers.push_back (cover.getHorizontalRange());
        }
        // A partly covered window shows its whole label after what covers its start.
        const int start = SectionLanes::labelStart (windowRegion (windowIndex, currentLayout).getHorizontalRange(), covers);
        const auto text = SectionLanes::windowLabel (window.name, window.lengthBars);
        const auto labelArea = region.withLeft ((float) start).withTrimmedLeft (scaled (labelInsetX)).withTrimmedTop (scaled (labelTop))
                                     .withHeight (scaled (labelSize + labelTop)).withTrimmedRight (scaled (labelInsetX));
        g.setFont (StageFonts::lcd (scaled (labelSize)));
        g.setColour (StagePalette::windowLabelShadow);
        g.drawText (text, labelArea.translated (float {}, shadowDrop), juce::Justification::topLeft, true);
        g.setColour (StagePalette::windowLabel);
        g.drawText (text, labelArea, juce::Justification::topLeft, true);
    }

    if (isSelected)   // grips on both edges
    {
        g.setColour (StagePalette::windowLabel.withAlpha (StagePalette::gripAlpha));
        const float gripW = scaled (gripWidth), gripH = scaled (gripHeight);
        for (const float edgeX : { region.getX(), region.getRight() })
            g.fillRoundedRectangle (juce::Rectangle<float> (gripW, gripH).withCentre ({ edgeX, region.getCentreY() }), outlineWidth);
    }
}

void ArrangerTimelineComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto currentLayout = layout();

    selectedIndex = ArrangerStyleDefaults::noIndex;
    dragMode = Hit::None;
    windowsChangedByDrag = false;   // a bare click (no drag) must NOT rebuild the style

    for (int windowIndex = (int) windows.size() - oneWindow; windowIndex >= int {}; --windowIndex)
    {
        const auto& window = windows[(size_t) windowIndex];
        const auto region = windowRegion (windowIndex, currentLayout);
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
        window.startBar = juce::jlimit (kFirstBar, totalBars + kPastLastBar - dragStartLenValue, barUnderCursor - dragGrabOffsetBars);
    }
    else if (dragMode == Hit::LeftEdge)
    {
        const int newStart = juce::jlimit (kFirstBar, window.startBar + window.lengthBars - kMinimumBars, barUnderCursor);
        window.lengthBars = (window.startBar + window.lengthBars) - newStart;
        window.startBar   = newStart;
    }
    else if (dragMode == Hit::RightEdge)
    {
        const int newEnd = juce::jlimit (window.startBar + kMinimumBars, totalBars + kPastLastBar, barUnderCursor);
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
