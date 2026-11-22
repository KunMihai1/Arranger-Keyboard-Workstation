#include <juce_core/juce_core.h>
#include "Arranger/ArrangerSectionSequencer.h"

/** Auto Fill: the fill between two variations is the one numbered like the variation being LEFT
    (owner, 2026-11-22: Var 2 -> Var 1 plays Fill 2). Spec: docs/superpowers/specs/2026-11-22-autofill-outgoing-variation-design.md */
namespace AutoFillTestValues
{
    const juce::String fillOfOutgoingVariation = "Auto Fill plays the fill of the variation being left";
    const juce::String fillFallsBackToAnyFill  = "Auto Fill falls back to any fill when the outgoing variation has none";
    const juce::String noFillSwitchesDirectly  = "Auto Fill switches directly when the style has no fill";

    const juce::String variationOneId = "var_1";
    const juce::String variationOne   = "Variation 1";
    const juce::String variationTwoId = "var_2";
    const juce::String variationTwo   = "Variation 2";
    const juce::String fillOneId      = "fill_1";
    const juce::String fillOne        = "Fill 1";
    const juce::String fillTwoId      = "fill_2";
    const juce::String fillTwo        = "Fill 2";
    const juce::String fillThreeId    = "fill_3";
    const juce::String fillThree      = "Fill 3";

    constexpr int oneBar          = 1;
    constexpr int beatsPerBar     = 4;
    constexpr int beatUnit        = 4;
    constexpr int variationOneIndex = 0;   // section order in fourSectionStyle()
    constexpr int variationTwoIndex = 1;
    constexpr int fillOneIndex      = 2;
    constexpr int fillTwoIndex      = 3;
    constexpr int firstAddedFillIndex = 2;   // the first fill after the two variations

    // Advance windows (beats) that each cross exactly one bar line: 4, 8, 12, 16.
    constexpr double startBeat   = 0.0;
    constexpr double afterBarOne = 5.0;
    constexpr double afterBarTwo = 9.0;
    constexpr double afterBarThree = 13.0;
    constexpr double afterBarFour  = 17.0;
}

namespace SectionSequencerTestValues
{
    const juce::String testName     = "ArrangerSectionSequencer";
    const juce::String testCategory = "Arranger";

    const juce::String startsOnFirstVariation   = "starts on the first variation";
    const juce::String switchAtNextBar          = "queued switch applies at the next bar boundary, not mid-bar";
    const juce::String doublePressKeepsLast     = "double-press before the boundary keeps only the last request";
    const juce::String loopNeverTransitions     = "a Loop section never auto-transitions";
    const juce::String introFallsThrough        = "Intro plays once then falls into the variation";
    const juce::String fillReturnsToVariation   = "Fill plays once then returns to the variation it was triggered from";
    const juce::String endingRequestsStop       = "Ending plays once then requests stop";
    const juce::String multiBarWindowInOrder    = "a multi-bar window resolves transitions in order";

    const juce::String variationThreeId = "var_3";
    const juce::String variationThree   = "Variation 3";
    const juce::String introOneId       = "intro_1";
    const juce::String introOne         = "Intro 1";
    const juce::String endingOneId      = "ending_1";
    const juce::String endingOne        = "Ending 1";

    constexpr int firstSection  = 0;
    constexpr int secondSection = 1;
    constexpr int thirdSection  = 2;

    constexpr double insideBarOne     = 2.0;
    constexpr double oneBarOfBeats    = 4.0;
    constexpr double threeBarsOfBeats = 12.0;
    constexpr double twentyFiveBars   = 100.0;
    constexpr double sectionStart     = 0.0;   // a new section enters at its bar 0
    constexpr double tolerance        = 1e-9;
    constexpr size_t twoSegments      = 2;
}

using namespace AutoFillTestValues;
using namespace SectionSequencerTestValues;

class ArrangerSectionSequencerTest : public juce::UnitTest
{
public:
    ArrangerSectionSequencerTest() : juce::UnitTest (SectionSequencerTestValues::testName, SectionSequencerTestValues::testCategory) {}

    static ArrangerSection makeSection (const juce::String& id, const juce::String& name,
                                        ArrangerSectionType type, int bars,
                                        ArrangerAfterComplete after)
    {
        ArrangerSection section;
        section.id = id; section.name = name; section.type = type; section.lengthBars = bars; section.afterComplete = after;
        return section;
    }

    static ArrangerStyle twoVariations()
    {
        ArrangerStyle st; st.timeSigNum = beatsPerBar; st.timeSigDenom = beatUnit;
        st.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
        st.sections.push_back (makeSection (variationTwoId, variationTwo, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
        return st;
    }

    void runTest() override
    {
        beginTest (startsOnFirstVariation);
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            expectEquals (seq.getActiveIndex(), firstSection);
        }

        beginTest (switchAtNextBar);
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            expect (seq.queue (ArrangerSectionType::Variation, variationTwo));

            auto insideBar = seq.advance (startBeat, insideBarOne);   // still inside bar 1
            expectEquals (seq.getActiveIndex(), firstSection);
            for (auto& segment : insideBar.segments) expectEquals (segment.sectionIndex, firstSection);

            auto acrossBar = seq.advance (insideBarOne, afterBarOne);   // crosses the bar line at beat 4
            expectEquals (seq.getActiveIndex(), secondSection);
            // last segment is the new section, flagged as a change
            expectEquals (acrossBar.segments.back().sectionIndex, secondSection);
            expect (acrossBar.segments.back().sectionChanged);
            expectWithinAbsoluteError (acrossBar.segments.back().localFromBeats, sectionStart, tolerance); // new section enters at its bar 0
        }

        beginTest (doublePressKeepsLast);
        {
            ArrangerSectionSequencer seq;
            ArrangerStyle st = twoVariations();
            st.sections.push_back (makeSection (variationThreeId, variationThree, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Variation, variationTwo);
            seq.queue (ArrangerSectionType::Variation, variationThree); // replaces the pending one
            seq.advance (startBeat, afterBarOne);
            expectEquals (seq.getActiveIndex(), thirdSection);
        }

        beginTest (loopNeverTransitions);
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            auto out = seq.advance (startBeat, twentyFiveBars); // 25 bars
            expectEquals (seq.getActiveIndex(), firstSection);
            expect (! out.stopRequested);
        }

        beginTest (introFallsThrough);
        {
            ArrangerStyle st; st.timeSigNum = beatsPerBar; st.timeSigDenom = beatUnit;
            st.sections.push_back (makeSection (introOneId, introOne, ArrangerSectionType::Intro, oneBar, ArrangerAfterComplete::FallThrough));
            st.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.startAt (firstSection);                 // begin on the intro
            expectEquals (seq.getActiveIndex(), firstSection);

            auto out = seq.advance (startBeat, oneBarOfBeats); // exactly the intro's length
            expectEquals (seq.getActiveIndex(), secondSection); // fell through to the variation
            for (auto& segment : out.segments) expectEquals (segment.sectionIndex, firstSection); // intro played the whole bar
        }

        beginTest (fillReturnsToVariation);
        {
            ArrangerStyle st; st.timeSigNum = beatsPerBar; st.timeSigDenom = beatUnit;
            st.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection (variationTwoId, variationTwo, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection (fillOneId, fillOne, ArrangerSectionType::Fill, oneBar, ArrangerAfterComplete::FallThrough));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Variation, variationTwo);
            seq.advance (startBeat, afterBarOne);          // now on Variation 2
            expectEquals (seq.getActiveIndex(), secondSection);

            seq.queue (ArrangerSectionType::Fill, fillOne);
            seq.advance (afterBarOne, afterBarTwo);          // boundary at 8 -> enter fill
            expectEquals (seq.getActiveIndex(), thirdSection);

            seq.advance (afterBarTwo, afterBarThree);         // boundary at 12 -> fill done -> back to Variation 2
            expectEquals (seq.getActiveIndex(), secondSection);
        }

        beginTest (endingRequestsStop);
        {
            ArrangerStyle st; st.timeSigNum = beatsPerBar; st.timeSigDenom = beatUnit;
            st.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection (endingOneId, endingOne, ArrangerSectionType::Ending, oneBar, ArrangerAfterComplete::Stop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Ending, endingOne);
            seq.advance (startBeat, afterBarOne);          // boundary at 4 -> enter ending
            expectEquals (seq.getActiveIndex(), secondSection);

            auto out = seq.advance (afterBarOne, afterBarTwo); // boundary at 8 -> ending complete
            expect (out.stopRequested);
            auto none = seq.advance (afterBarTwo, afterBarThree); // stopped: no further segments
            expect (none.segments.empty());
        }

        beginTest (multiBarWindowInOrder);
        {
            ArrangerStyle st; st.timeSigNum = beatsPerBar; st.timeSigDenom = beatUnit;
            st.sections.push_back (makeSection (introOneId, introOne, ArrangerSectionType::Intro, oneBar, ArrangerAfterComplete::FallThrough));
            st.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.startAt (firstSection);
            auto out = seq.advance (startBeat, threeBarsOfBeats);  // intro bar, then 2 variation bars, all in one call
            expect (out.segments.size() >= twoSegments);
            expectEquals (out.segments.front().sectionIndex, firstSection); // starts on intro
            expectEquals (out.segments.back().sectionIndex, secondSection);  // ends on variation
            expectEquals (seq.getActiveIndex(), secondSection);
        }

        runAutoFillTests();
    }

    static ArrangerStyle autoFillStyle (bool withFillOne, bool withFillTwo)
    {
        ArrangerStyle style;
        style.timeSigNum   = beatsPerBar;
        style.timeSigDenom = beatUnit;
        style.sections.push_back (makeSection (variationOneId, variationOne, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
        style.sections.push_back (makeSection (variationTwoId, variationTwo, ArrangerSectionType::Variation, oneBar, ArrangerAfterComplete::Loop));
        if (withFillOne)
            style.sections.push_back (makeSection (fillOneId, fillOne, ArrangerSectionType::Fill, oneBar, ArrangerAfterComplete::FallThrough));
        if (withFillTwo)
            style.sections.push_back (makeSection (fillTwoId, fillTwo, ArrangerSectionType::Fill, oneBar, ArrangerAfterComplete::FallThrough));
        return style;
    }

    void runAutoFillTests()
    {

        beginTest (fillOfOutgoingVariation);
        {
            ArrangerSectionSequencer sequencer;
            sequencer.setStyle (autoFillStyle (true, true));
            sequencer.setAutoFillEnabled (true);

            sequencer.queue (ArrangerSectionType::Variation, variationTwo);   // leaving Variation 1
            sequencer.advance (startBeat, afterBarOne);
            expectEquals (sequencer.getActiveIndex(), fillOneIndex);
            sequencer.advance (afterBarOne, afterBarTwo);
            expectEquals (sequencer.getActiveIndex(), variationTwoIndex);

            sequencer.queue (ArrangerSectionType::Variation, variationOne);   // leaving Variation 2
            sequencer.advance (afterBarTwo, afterBarThree);
            expectEquals (sequencer.getActiveIndex(), fillTwoIndex);
            sequencer.advance (afterBarThree, afterBarFour);
            expectEquals (sequencer.getActiveIndex(), variationOneIndex);
        }

        beginTest (fillFallsBackToAnyFill);
        {
            // Fills: Fill 3 first, then Fill 2. Leaving Variation 1 (no Fill 1) must fall back to the FIRST
            // fill, Fill 3 - not to Fill 2, which the old target-numbered lookup would have picked.
            ArrangerStyle style = autoFillStyle (false, false);
            style.sections.push_back (makeSection (fillThreeId, fillThree, ArrangerSectionType::Fill, oneBar, ArrangerAfterComplete::FallThrough));
            style.sections.push_back (makeSection (fillTwoId, fillTwo, ArrangerSectionType::Fill, oneBar, ArrangerAfterComplete::FallThrough));

            ArrangerSectionSequencer sequencer;
            sequencer.setStyle (style);
            sequencer.setAutoFillEnabled (true);

            sequencer.queue (ArrangerSectionType::Variation, variationTwo);   // leaving Variation 1
            sequencer.advance (startBeat, afterBarOne);
            expectEquals (sequencer.getActiveIndex(), firstAddedFillIndex);    // Fill 3
            sequencer.advance (afterBarOne, afterBarTwo);
            expectEquals (sequencer.getActiveIndex(), variationTwoIndex);
        }

        beginTest (noFillSwitchesDirectly);
        {
            ArrangerSectionSequencer sequencer;
            sequencer.setStyle (autoFillStyle (false, false));
            sequencer.setAutoFillEnabled (true);

            sequencer.queue (ArrangerSectionType::Variation, variationTwo);
            sequencer.advance (startBeat, afterBarOne);
            expectEquals (sequencer.getActiveIndex(), variationTwoIndex);
        }
    }
};

static ArrangerSectionSequencerTest arrangerSectionSequencerTest;
