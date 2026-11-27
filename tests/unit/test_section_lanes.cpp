#include <JuceHeader.h>
#include "UI/Sections/SectionLanes.h"
#include "UI/Sections/SectionLaneHeaders.h"
#include "Arranger/ArrangerTimelineComponent.h"

/*  The Sections editor's lanes (board PageSections, spec part 2 P.6): today's timeline had only a bar ruler and
    the windows; the board adds one lane per recorded track with its notes, the transpose mode in the lane
    header (it was a separate strip of combos) and zoom (the bar width was fixed). */

namespace SectionLanesTestValues
{
    const juce::String testName     = "SectionLanes";
    const juce::String testCategory = "Unit";

    const juce::String eightSlotsAlwaysShow = "at least the eight track slots show; more tracks, more lanes";
    const juce::String lanesStackUnderRuler = "lanes stack under the ruler, each 22 board px, scaled";
    const juce::String densityCountsBeats   = "note-ons are counted per beat; note-offs and events past the end are not";
    const juce::String densityFades         = "an empty beat is clear; busier beats are more opaque, the busiest fully lit";
    const juce::String rulerMarks           = "a strong line every 4 bars, numbers on every other bar, from bar 1";
    const juce::String labelsNameLength     = "a window's label is its name and its length, one bar singular";
    const juce::String modesCycle           = "the transpose button cycles None, Parallel, Chord, Fixed and back";
    const juce::String zoomSteps            = "zoom goes from a quarter to twice today's 56 px bar, today's width by default";
    const juce::String headersTestName      = "SectionLaneHeaders";
    const juce::String buttonsInTheirLanes  = "each track's button sits at the right of its own lane; empty slots have none";
    const juce::String clickCyclesMode      = "a click on a button moves that track to the next mode and reports it; elsewhere nothing";
    const juce::String timelineTestName     = "ArrangerTimelineLanes";
    const juce::String zoomSetsWidth        = "the timeline is bars x the zoomed, scaled bar width";
    const juce::String lanesSetHeight       = "the timeline is the ruler and the lanes high, at least eight lanes";
    const juce::String selectFromOutside    = "a section can be selected from outside; out of range or removed clears it";
    const juce::String drumsName            = "Drums";
    const juce::String bassName             = "Bass";

    const juce::String varName      = "Var 1";
    const juce::String fillName     = "Fill 1";
    const char* const varLabelUtf8  = "Var 1 \xc2\xb7 8 bars";   // "Var 1 · 8 bars"
    const char* const fillLabelUtf8 = "Fill 1 \xc2\xb7 1 bar";   // "Fill 1 · 1 bar"
    const juce::String noneText     = "None";
    const juce::String parallelText = "Parallel";
    const juce::String chordText    = "Chord";
    const juce::String fixedText    = "Fixed";

    constexpr int    noTracks       = 0;
    constexpr int    fourTracks     = 4;
    constexpr int    eightSlots     = 8;
    constexpr int    tenTracks      = 10;
    constexpr int    firstLane      = 0;
    constexpr int    thirdLane      = 2;
    constexpr int    laneWidth      = 600;
    constexpr float  boardScale     = 1.0f;
    constexpr float  doubleScale    = 2.0f;
    constexpr int    rulerPx        = 18;
    constexpr int    lanePx         = 22;
    constexpr int    twoLanesDown   = 44;       // 2 × 22
    constexpr int    eightLanesHigh = 194;      // 18 + 8 × 22
    constexpr int    doubled        = 2;
    constexpr int    channel        = 1;
    constexpr int    middleC        = 60;
    constexpr int    noteE          = 64;
    constexpr int    noteG          = 67;
    constexpr juce::uint8 velocity  = 100;
    constexpr double firstBeat      = 0.0;
    constexpr double sameBeatLater  = 0.5;
    constexpr double thirdBeat      = 2.25;
    constexpr double pastTheEnd     = 9.0;
    constexpr int    fourBeats      = 4;
    constexpr int    twoNotes       = 2;
    constexpr int    oneNote        = 1;
    constexpr int    noNotes        = 0;
    constexpr int    busiest        = 4;
    constexpr float  clear          = 0.0f;
    constexpr float  fullyLit       = 1.0f;
    constexpr float  tolerance      = 0.001f;
    constexpr int    bar1            = 1;
    constexpr int    bar2            = 2;
    constexpr int    bar3            = 3;
    constexpr int    bar4            = 4;
    constexpr int    bar5            = 5;
    constexpr int    bar9            = 9;
    constexpr int    oneBar         = 1;
    constexpr int    eightBars      = 8;
    constexpr int    todaysBarPx    = 56;
    constexpr int    quarterBarPx   = 14;
    constexpr int    twiceBarPx     = 112;
    constexpr int    belowRange     = -5;
    constexpr int    aboveRange     = 99;
    constexpr int    nextLevel      = 1;
    constexpr int    headerWidthPx  = 132;
    constexpr int    secondTrack    = 1;
    constexpr int    emptySlot      = 5;
    constexpr int    tenBars        = 10;
    constexpr int    threeTracks    = 3;
    constexpr int    secondWindow   = 1;
    constexpr int    missingWindow  = 5;
    constexpr int    noSelection    = -1;
    constexpr int    windowStart    = 3;
    constexpr int    windowLength   = 2;
    constexpr size_t beat1           = 0;
    constexpr size_t beat2           = 1;
    constexpr size_t beat3           = 2;
    constexpr size_t beat4           = 3;
}

class SectionLanesTest : public juce::UnitTest
{
public:
    SectionLanesTest() : juce::UnitTest (SectionLanesTestValues::testName, SectionLanesTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionLanesTestValues;
        using namespace SectionLanes;

        beginTest (eightSlotsAlwaysShow);
        expectEquals (laneCount (noTracks), eightSlots);
        expectEquals (laneCount (fourTracks), eightSlots);
        expectEquals (laneCount (tenTracks), tenTracks);

        beginTest (lanesStackUnderRuler);
        {
            const auto first = laneBounds (firstLane, laneWidth, boardScale);
            expectEquals (first.getY(), rulerPx);
            expectEquals (first.getHeight(), lanePx);
            expectEquals (first.getWidth(), laneWidth);
            expectEquals (laneBounds (thirdLane, laneWidth, boardScale).getY(), rulerPx + twoLanesDown);
            expectEquals (laneBounds (thirdLane, laneWidth, doubleScale).getY(), (rulerPx + twoLanesDown) * doubled);
            expectEquals (contentHeight (eightSlots, boardScale), eightLanesHigh);
            expectEquals (contentHeight (eightSlots, doubleScale), eightLanesHigh * doubled);
        }

        beginTest (densityCountsBeats);
        {
            const std::vector<TimedBeatEvent> events {
                { firstBeat,     juce::MidiMessage::noteOn (channel, middleC, velocity) },
                { sameBeatLater, juce::MidiMessage::noteOn (channel, noteE, velocity) },
                { sameBeatLater, juce::MidiMessage::noteOff (channel, middleC) },
                { thirdBeat,     juce::MidiMessage::noteOn (channel, noteG, velocity) },
                { pastTheEnd,    juce::MidiMessage::noteOn (channel, middleC, velocity) },
            };
            const auto density = beatDensity (events, fourBeats);
            expectEquals ((int) density.size(), fourBeats);
            if (density.size() == (size_t) fourBeats)
            {
                expectEquals (density[beat1], twoNotes);
                expectEquals (density[beat2], noNotes);
                expectEquals (density[beat3], oneNote);
                expectEquals (density[beat4], noNotes);
            }
        }

        beginTest (densityFades);
        expectWithinAbsoluteError (densityAlpha (noNotes, busiest), clear, tolerance);
        expect (densityAlpha (oneNote, busiest) > clear);
        expect (densityAlpha (twoNotes, busiest) > densityAlpha (oneNote, busiest));
        expectWithinAbsoluteError (densityAlpha (busiest, busiest), fullyLit, tolerance);
        expectWithinAbsoluteError (densityAlpha (noNotes, noNotes), clear, tolerance);   // an empty track: no divide by zero

        beginTest (rulerMarks);
        expect (isStrongBarLine (bar1) && isStrongBarLine (bar5) && isStrongBarLine (bar9));
        expect (! isStrongBarLine (bar2) && ! isStrongBarLine (bar4));
        expect (showsBarNumber (bar1) && showsBarNumber (bar3) && showsBarNumber (bar5));
        expect (! showsBarNumber (bar2) && ! showsBarNumber (bar4));

        beginTest (labelsNameLength);
        expectEquals (windowLabel (varName, eightBars), juce::String::fromUTF8 (varLabelUtf8));
        expectEquals (windowLabel (fillName, oneBar), juce::String::fromUTF8 (fillLabelUtf8));

        beginTest (modesCycle);
        expect (nextTransposeMode (NttType::NoTranspose) == NttType::Parallel);
        expect (nextTransposeMode (NttType::Parallel) == NttType::Chord);
        expect (nextTransposeMode (NttType::Chord) == NttType::Fixed);
        expect (nextTransposeMode (NttType::Fixed) == NttType::NoTranspose);
        expectEquals (transposeModeText (NttType::NoTranspose), noneText);
        expectEquals (transposeModeText (NttType::Parallel), parallelText);
        expectEquals (transposeModeText (NttType::Chord), chordText);
        expectEquals (transposeModeText (NttType::Fixed), fixedText);

        beginTest (zoomSteps);
        expectEquals (barWidth (defaultZoomLevel, boardScale), todaysBarPx);
        expectEquals (barWidth (clampZoomLevel (belowRange), boardScale), quarterBarPx);
        expectEquals (barWidth (clampZoomLevel (aboveRange), boardScale), twiceBarPx);
        expectEquals (barWidth (defaultZoomLevel, doubleScale), todaysBarPx * doubled);
        for (int level {}; level + nextLevel < zoomLevelCount(); ++level)
            expect (zoomFactor (level) < zoomFactor (level + nextLevel));
    }
};
static SectionLanesTest sectionLanesTest;

class SectionLaneHeadersTest : public juce::UnitTest
{
public:
    SectionLaneHeadersTest() : juce::UnitTest (SectionLanesTestValues::headersTestName, SectionLanesTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionLanesTestValues;
        SectionLaneHeaders headers;
        headers.setTracks ({ { drumsName, NttType::Chord }, { bassName, NttType::Fixed } });
        headers.setBounds (int {}, int {}, headerWidthPx, SectionLanes::contentHeight (eightSlots, boardScale));

        beginTest (buttonsInTheirLanes);
        for (int track {}; track <= secondTrack; ++track)
        {
            const auto lane   = SectionLanes::laneBounds (track, headerWidthPx, boardScale);
            const auto button = headers.buttonBounds (track);
            expect (! button.isEmpty());
            expect (lane.contains (button));
            expectEquals (button.getRight(), headerWidthPx - SectionLaneMetrics::headerPaddingX);
        }
        expect (headers.buttonBounds (emptySlot).isEmpty());

        beginTest (clickCyclesMode);
        {
            int reportedTrack = noSelection;
            NttType reportedMode = NttType::Chord;
            headers.onModeChanged = [&] (int track, NttType mode) { reportedTrack = track; reportedMode = mode; };
            headers.clickAt (headers.buttonBounds (secondTrack).getCentre());
            expectEquals (reportedTrack, secondTrack);
            expect (reportedMode == NttType::NoTranspose);
            expect (headers.getTracks()[(size_t) secondTrack].mode == NttType::NoTranspose);

            reportedTrack = noSelection;
            headers.clickAt (SectionLanes::laneBounds (secondTrack, headerWidthPx, boardScale).getTopLeft().translated (SectionLaneMetrics::headerPaddingX, int {}));
            expectEquals (reportedTrack, noSelection);   // the name is not a button
        }
    }
};
static SectionLaneHeadersTest sectionLaneHeadersTest;

class ArrangerTimelineLanesTest : public juce::UnitTest
{
public:
    ArrangerTimelineLanesTest() : juce::UnitTest (SectionLanesTestValues::timelineTestName, SectionLanesTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionLanesTestValues;

        beginTest (zoomSetsWidth);
        {
            ArrangerTimelineComponent timeline;
            timeline.setTotalBars (tenBars);
            expectEquals (timeline.getPreferredWidth(), tenBars * todaysBarPx);
            timeline.setZoomLevel (SectionLanes::clampZoomLevel (belowRange));
            expectEquals (timeline.getPreferredWidth(), tenBars * quarterBarPx);
            timeline.setZoomLevel (SectionLanes::defaultZoomLevel);
            timeline.setScale (doubleScale);
            expectEquals (timeline.getPreferredWidth(), tenBars * todaysBarPx * doubled);
        }

        beginTest (lanesSetHeight);
        {
            ArrangerTimelineComponent timeline;
            timeline.setSourceTracks (std::vector<SourceTrackFile> (static_cast<size_t> (threeTracks)), ArrangerStyleDefaults::beatsPerBar);
            expectEquals (timeline.getLaneCount(), eightSlots);
            expectEquals (timeline.getPreferredHeight(), eightLanesHigh);
            timeline.setSourceTracks (std::vector<SourceTrackFile> (static_cast<size_t> (tenTracks)), ArrangerStyleDefaults::beatsPerBar);
            expectEquals (timeline.getLaneCount(), tenTracks);
        }

        beginTest (selectFromOutside);
        {
            ArrangerTimelineComponent timeline;
            SectionWindow window;
            window.startBar = windowStart;
            window.lengthBars = windowLength;
            timeline.setWindows ({ window, window });
            timeline.setSelectedIndex (secondWindow);
            expectEquals (timeline.getSelectedIndex(), secondWindow);
            timeline.setSelectedIndex (missingWindow);
            expectEquals (timeline.getSelectedIndex(), noSelection);
            timeline.setSelectedIndex (secondWindow);
            timeline.setWindows ({ window });
            expectEquals (timeline.getSelectedIndex(), noSelection);
        }
    }
};
static ArrangerTimelineLanesTest arrangerTimelineLanesTest;
