#pragma once
#include <JuceHeader.h>
#include "Arranger/ArrangerModel.h"
#include "Arranger/Chord.h"
#include <vector>

/*  The Sections editor's lanes (board PageSections, spec part 2 P.6): one lane per recorded track under a bar
    ruler, the recording's notes as a read-only density view, the section windows over them. Pure: the
    timeline component and the lane headers paint from these, so the geometry is unit-tested.
    Lengths are board px; callers multiply by the Stage scale. */

namespace SectionLaneMetrics
{
    inline constexpr int   rulerHeight     = 18;   // the bar numbers' strip above the lanes
    inline constexpr int   laneHeight      = 22;
    inline constexpr int   minimumLanes    = 8;    // a style has eight track slots: empty ones still show
    inline constexpr int   headerWidth     = 132;  // track name + transpose button, left of the lanes
    inline constexpr int   headerPaddingX  = 8;
    inline constexpr int   noteInsetY      = 5;    // a note cell inside its lane
    inline constexpr int   noteHeight      = 12;
    inline constexpr int   noteGap         = 1;    // between two beats' cells
    inline constexpr int   barNumberInsetX = 3;
    inline constexpr int   barNumberTop    = 2;
    inline constexpr float barNumberSize   = 10.0f;
    inline constexpr int   labelInsetX     = 6;
    inline constexpr int   labelTop        = 3;
    inline constexpr float labelSize       = 11.0f;
    inline constexpr float laneNameSize    = 12.0f;
    inline constexpr int   gripWidth       = 3;
    inline constexpr int   gripHeight      = 22;
    inline constexpr int   playheadWidth   = 2;
    inline constexpr int   windowCorner    = 2;
    inline constexpr int   strongLineEvery = 4;    // a stronger bar line every 4 bars
    inline constexpr int   numberEvery     = 2;    // bar numbers on bars 1, 3, 5, …
    inline constexpr int   baseBarWidth    = 56;   // today's bar width: zoom 1
    inline constexpr float referenceScale  = 1.0f; // the board's own size
}

namespace SectionLanes
{
    /** The lanes shown for `trackCount` recorded tracks: at least the eight slots. */
    int laneCount (int trackCount);

    /** Lane `laneIndex` (0-based) across `width`, in a component whose top is the ruler's top. */
    juce::Rectangle<int> laneBounds (int laneIndex, int width, float scale);

    /** The whole lanes area's height: the ruler and every lane. */
    int contentHeight (int laneTotal, float scale);

    /** Note-ons per beat over `totalBeats` beats (events past the end are dropped). */
    std::vector<int> beatDensity (const std::vector<TimedBeatEvent>& events, int totalBeats);

    /** A beat cell's opacity: 0 for an empty beat, rising with the notes in it up to the busiest beat. */
    float densityAlpha (int notes, int busiestBeat);

    bool isStrongBarLine (int bar);   ///< bars 1, 5, 9, …
    bool showsBarNumber (int bar);    ///< bars 1, 3, 5, …

    /** A window's label: "Var 1 · 8 bars", "Fill 1 · 1 bar". */
    juce::String windowLabel (const juce::String& name, int lengthBars);

    /** The transpose mode a click moves to: None → Parallel → Chord → Fixed → None. */
    NttType nextTransposeMode (NttType mode);
    juce::String transposeModeText (NttType mode);   ///< "None" / "Parallel" / "Chord" / "Fixed"

    /** Zoom: levels from a quarter of today's bar width to twice it; today's width is the default. */
    inline constexpr int defaultZoomLevel = 3;
    int zoomLevelCount();
    int clampZoomLevel (int level);
    float zoomFactor (int level);
    /** One bar's width at a zoom level and the Stage scale, whole px (at least one). */
    int barWidth (int zoomLevel, float scale);
}
