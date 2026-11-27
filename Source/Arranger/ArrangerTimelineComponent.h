#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"
#include "ArrangerTimelineGeometry.h"
#include "UI/Sections/SectionLanes.h"
#include <vector>
#include <functional>

/** The Sections editor's lanes (board PageSections): a bar ruler, one lane per recorded track with its notes
    (read-only density), and the draggable / resizable section windows over them. Holds a copy of the windows;
    emits a callback whenever they change. Knows nothing about files or the engine. */
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

    /** The recorded tracks drawn in the lanes (their notes per beat), at `beatsPerBar` beats a bar. */
    void setSourceTracks (const std::vector<SourceTrackFile>& tracks, double beatsPerBar);
    int  getLaneCount() const;

    void setPlayheadBar (double bar);   // for the live preview cursor (1-based, fractional)
    int  getSelectedIndex() const { return selectedIndex; }
    /** Selects a window from outside (the inspector, a jump pad); out of range clears the selection. */
    void setSelectedIndex (int index);

    /** The Stage scale and the zoom level (SectionLanes): together they set the bar width. */
    void setScale (float newScale);
    void setZoomLevel (int level);
    int  getZoomLevel() const { return zoomLevel; }
    int  getBarWidth() const;

    /** The component is made this wide (or the view's width when that is wider) and scrolled in a
        Viewport, so bar labels stay readable no matter how many bars there are. */
    int getPreferredWidth() const;     // totalBars * bar width
    int getPreferredHeight() const;    // the ruler and every lane
    int xForBar (double bar) const;    // pixel x of a (fractional, 1-based) bar in this component
    int barForX (int pixelX) const;    // nearest 1-based bar line at a pixel x (inverse of xForBar)

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp   (const juce::MouseEvent& event) override;

private:
    ArrangerTimelineGeometry::Layout layout() const;
    /** Where window `windowIndex` is drawn and grabbed: its bars, from under the ruler to the bottom. */
    juce::Rectangle<int> windowRegion (int windowIndex, const ArrangerTimelineGeometry::Layout& currentLayout) const;
    static juce::Colour colourForType (ArrangerSectionType sectionType);
    float scaled (float boardPixels) const;
    void paintNotes (juce::Graphics& g, const ArrangerTimelineGeometry::Layout& currentLayout, juce::Rectangle<int> clip) const;
    void paintWindow (juce::Graphics& g, int windowIndex, const ArrangerTimelineGeometry::Layout& currentLayout) const;

    std::vector<SectionWindow> windows;
    int    totalBars   = ArrangerTimelineGeometry::kFirstBar;
    double playheadBar = ArrangerTimelineGeometry::kFirstBar;
    float  scale       = SectionLaneMetrics::referenceScale;
    int    zoomLevel   = SectionLanes::defaultZoomLevel;

    std::vector<std::vector<int>> laneDensity;   // note-ons per beat, per recorded track
    std::vector<int> laneBusiest;                 // each track's busiest beat
    double beatsPerBar = ArrangerStyleDefaults::beatsPerBar;

    int  selectedIndex = ArrangerStyleDefaults::noIndex;
    ArrangerTimelineGeometry::Hit dragMode = ArrangerTimelineGeometry::Hit::None;
    int  dragGrabOffsetBars {};   // bars between the grab point and the region start (body drag)
    int  dragStartLenValue = ArrangerTimelineGeometry::kMinimumBars;
    bool windowsChangedByDrag = false;  // true once a drag actually moved/resized a window this gesture
};
