#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"
#include "ArrangerTimelineGeometry.h"
#include <vector>
#include <functional>

/** Bar ruler with draggable/resizable colored section regions. Holds a copy of the
    windows; emits a callback whenever they change. Knows nothing about files or the engine. */
class ArrangerTimelineComponent : public juce::Component, public juce::TooltipClient
{
public:
    std::function<void (const std::vector<SectionWindow>&)> onWindowsChanged;
    std::function<void (int sectionIndex)> onSectionSelected;

    /** Tooltip naming the section under the mouse (e.g. "Intro 1  (bars 1-2)"). */
    juce::String getTooltip() override;

    void setTotalBars (int bars);
    void setWindows (const std::vector<SectionWindow>& windows);
    const std::vector<SectionWindow>& getWindows() const { return windows; }

    void setPlayheadBar (double bar);   // for the live preview cursor (1-based, fractional)
    int  getSelectedIndex() const { return selectedIndex; }

    /** Fixed bar width: the component is made this wide per bar and scrolled in a Viewport,
        so bar labels stay readable no matter how many bars there are. */
    static constexpr int kPixelsPerBar = 56;
    int getPreferredWidth() const;     // marginPx*2 + totalBars * kPixelsPerBar
    int xForBar (double bar) const;    // pixel x of a (fractional, 1-based) bar in this component
    int barForX (int pixelX) const;    // nearest 1-based bar line at a pixel x (inverse of xForBar)

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp   (const juce::MouseEvent& event) override;

private:
    static constexpr int kMarginPx = 10;

    ArrangerTimelineGeometry::Layout layout() const;
    juce::Colour colourForType (ArrangerSectionType sectionType) const;

    std::vector<SectionWindow> windows;
    int    totalBars   = ArrangerTimelineGeometry::kFirstBar;
    int    marginPx    = kMarginPx;
    double playheadBar = ArrangerTimelineGeometry::kFirstBar;

    int  selectedIndex = ArrangerStyleDefaults::noIndex;
    ArrangerTimelineGeometry::Hit dragMode = ArrangerTimelineGeometry::Hit::None;
    int  dragGrabOffsetBars {};   // bars between the grab point and the region start (body drag)
    int  dragStartLenValue = ArrangerTimelineGeometry::kMinimumBars;
    bool windowsChangedByDrag = false;  // true once a drag actually moved/resized a window this gesture
};
