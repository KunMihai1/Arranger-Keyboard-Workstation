#include <juce_core/juce_core.h>
#include "Arranger/ArrangerTime.h"

namespace ArrangerTimeTestValues
{
    const juce::String testName     = "ArrangerTime";
    const juce::String testCategory = "Arranger";

    const juce::String secondsBeatsAt120   = "seconds <-> beats at 120 BPM";
    const juce::String beatsPerBarCases    = "beats per bar";
    const juce::String barsRoundUp         = "bars for beats rounds up to whole bars";

    constexpr double tolerance     = 1e-9;
    constexpr double tempo120      = 120.0;
    constexpr double oneSecond     = 1.0;
    constexpr double twoBeats      = 2.0;
    constexpr int    fourPerBar    = 4;     // time-signature numerators
    constexpr int    threePerBar   = 3;
    constexpr int    sixPerBar     = 6;
    constexpr int    quarterNote   = 4;     // time-signature denominators
    constexpr int    eighthNote    = 8;
    constexpr double fourBeats     = 4.0;
    constexpr double threeBeats    = 3.0;   // 6/8 = 6 * 4/8
    constexpr double eightBeats    = 8.0;
    constexpr double justOverEight = 8.0001;
    constexpr double noBeats       = 0.0;
    constexpr double justUnderEight = 7.9999;   // tolerance swallows fp noise
    constexpr int    twoBars       = 2;
    constexpr int    threeBars     = 3;
    constexpr int    noBars        = 0;
}

class ArrangerTimeTest : public juce::UnitTest
{
public:
    ArrangerTimeTest() : juce::UnitTest (ArrangerTimeTestValues::testName, ArrangerTimeTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerTimeTestValues;

        beginTest (secondsBeatsAt120);
        {
            expectWithinAbsoluteError (ArrangerTime::secondsToBeats (oneSecond, tempo120), twoBeats, tolerance);
            expectWithinAbsoluteError (ArrangerTime::beatsToSeconds (twoBeats, tempo120), oneSecond, tolerance);
        }

        beginTest (beatsPerBarCases);
        {
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (fourPerBar, quarterNote), fourBeats, tolerance);
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (threePerBar, quarterNote), threeBeats, tolerance);
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (sixPerBar, eighthNote), threeBeats, tolerance);
        }

        beginTest (barsRoundUp);
        {
            expectEquals (ArrangerTime::barsForBeats (eightBeats, fourBeats), twoBars);
            expectEquals (ArrangerTime::barsForBeats (justOverEight, fourBeats), threeBars);
            expectEquals (ArrangerTime::barsForBeats (noBeats, fourBeats), noBars);
            expectEquals (ArrangerTime::barsForBeats (justUnderEight, fourBeats), twoBars);
        }
    }
};

static ArrangerTimeTest arrangerTimeTest;
