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
    const juce::String notesPairOnOff       = "each note-on pairs with its release into one note; one never released lasts to the track's end";
    const juce::String notesStackByPitch    = "in a lane the lowest pitch sits at the bottom, the highest at the top, a single pitch in the middle";
    const juce::String labelStartsUncovered = "a label starts after the windows drawn over the start of its window; fully covered, none";
    constexpr int      spanStart            = 0;
    constexpr int      coverStart           = 40;
    constexpr int      secondCoverEnd       = 90;
    constexpr int      spanEnd              = 200;
    const juce::String rulerMarks           ="a strong line every 4 bars, numbers on every other bar, from bar 1";
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
    const juce::String labelUnderWindowHidden = "a window's label is hidden under the windows drawn over it (two at one bar printed over each other)";
    const juce::String otherVarName         = "Variation 7 longer name";
    constexpr int      edgePx               = 2;
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
    constexpr double trackEnd       = 3.0;
    constexpr juce::uint8 silent    = 0;
    constexpr int    threeNotes     = 3;
    constexpr size_t note1          = 0;
    constexpr size_t note2          = 1;
    constexpr size_t note3          = 2;
    constexpr double beatTolerance  = 0.0001;
    constexpr float  positionTolerance = 0.01f;
    constexpr float  half           = 0.5f;
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

        beginTest (notesPairOnOff);
        {
            const std::vector<TimedBeatEvent> events {
                { firstBeat,     juce::MidiMessage::noteOn (channel, middleC, velocity) },
                { sameBeatLater, juce::MidiMessage::noteOn (channel, noteE, velocity) },
                { sameBeatLater, juce::MidiMessage::noteOff (channel, middleC) },
                { thirdBeat,     juce::MidiMessage::noteOn (channel, noteG, velocity) },   // never released
                { trackEnd,      juce::MidiMessage::noteOn (channel, noteE, silent) },     // velocity 0 = release
            };
            const auto notes = laneNotes (events);
            expectEquals ((int) notes.size(), threeNotes);
            if (notes.size() == (size_t) threeNotes)
            {
                expectEquals (notes[note1].pitch, middleC);
                expectWithinAbsoluteError (notes[note1].startBeat, firstBeat, beatTolerance);
                expectWithinAbsoluteError (notes[note1].lengthBeats, sameBeatLater - firstBeat, beatTolerance);
                expectEquals (notes[note2].pitch, noteE);
                expectWithinAbsoluteError (notes[note2].lengthBeats, trackEnd - sameBeatLater, beatTolerance);
                expectEquals (notes[note3].pitch, noteG);
                expectWithinAbsoluteError (notes[note3].lengthBeats, trackEnd - thirdBeat, beatTolerance);   // held to the end
            }
            const auto range = pitchRange (notes);
            expectEquals (range.low, middleC);
            expectEquals (range.high, noteG);
        }

        beginTest (notesStackByPitch);
        for (const auto scale : { boardScale, doubleScale })
        {
            using namespace SectionLaneMetrics;
            const auto lane = laneBounds (firstLane, laneWidth, scale);
            const PitchRange range { middleC, noteG };
            const float bandTop    = (float) lane.getY() + noteInsetY * scale;
            const float lowestTop  = bandTop + (noteHeight - noteThickness) * scale;
            expectWithinAbsoluteError (noteTop (middleC, range, lane, scale), lowestTop, positionTolerance);
            expectWithinAbsoluteError (noteTop (noteG, range, lane, scale), bandTop, positionTolerance);
            const float middle = noteTop (noteE, range, lane, scale);
            expect (middle > bandTop && middle < lowestTop);
            expectWithinAbsoluteError (noteTop (middleC, { middleC, middleC }, lane, scale), (bandTop + lowestTop) * half, positionTolerance);
        }

        beginTest (labelStartsUncovered);
        {
            const juce::Range<int> window { spanStart, spanEnd };
            expectEquals (labelStart (window, {}), spanStart);
            expectEquals (labelStart (window, { { coverStart, spanEnd } }), spanStart);                       // covered further right only
            expectEquals (labelStart (window, { { spanStart, coverStart } }), coverStart);                    // starts where the cover ends
            expectEquals (labelStart (window, { { coverStart, secondCoverEnd }, { spanStart, coverStart } }), secondCoverEnd);   // covers in a row
            expect (labelStart (window, { { spanStart, spanEnd } }) >= spanEnd);                              // fully covered: no label
        }

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

        beginTest (labelUnderWindowHidden);
        {
            // Variation 1 (bars 1-8) under Fill 1 (bars 1-2), as two Adds at the same bar leave them: inside the
            // fill only the fill's label shows, whatever the variation is called.
            auto render = [] (const juce::String& lowerName)
            {
                SectionWindow lower;
                lower.type = ArrangerSectionType::Variation;
                lower.name = lowerName;
                lower.startBar = bar1;
                lower.lengthBars = eightBars;
                SectionWindow upper;
                upper.type = ArrangerSectionType::Fill;
                upper.name = fillName;
                upper.startBar = bar1;
                upper.lengthBars = windowLength;
                ArrangerTimelineComponent timeline;
                timeline.setTotalBars (tenBars);
                timeline.setWindows ({ lower, upper });
                timeline.setSize (timeline.getPreferredWidth(), timeline.getPreferredHeight());
                juce::Image image (juce::Image::ARGB, timeline.getWidth(), timeline.getHeight(), true, juce::SoftwareImageType());
                juce::Graphics g (image);
                timeline.paintEntireComponent (g, false);
                return image;
            };
            const auto named   = render (varName);
            const auto renamed = render (otherVarName);
            const int fillRight = windowLength * todaysBarPx - edgePx;   // inside the fill, clear of its border
            bool insideSame = true, outsideDiffers = false;
            for (int column {}; column < named.getWidth(); ++column)
                for (int row {}; row < named.getHeight(); ++row)
                {
                    const bool same = named.getPixelAt (column, row) == renamed.getPixelAt (column, row);
                    if (column < fillRight) insideSame = insideSame && same;
                    else                    outsideDiffers = outsideDiffers || ! same;
                }
            expect (insideSame);
            expect (outsideDiffers);   // the variation's label still shows where nothing covers it
        }
    }
};
static ArrangerTimelineLanesTest arrangerTimelineLanesTest;
