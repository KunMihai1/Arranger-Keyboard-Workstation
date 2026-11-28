#include <JuceHeader.h>
#include "Playback/TempoClock.h"

/*
    The classic track player plays sequences scaled for the tempo set at Start. A tempo change while playing must
    change the speed from that moment on without moving the musical position (owner's TODO: "Change BPM while
    playing"). Before, the player kept its position in wall-clock seconds and only its beat counter followed a new
    tempo, so the bar / beat count jumped and ran apart from the music.
*/
namespace TempoClockTestValues
{
    const juce::String testName     = "Tempo clock";
    const juce::String testCategory = "Unit";

    const juce::String keepsPosition   = "a tempo change keeps the position and changes the speed from then on";
    const juce::String beatsFollow     = "beats count the sequences' own beats, whatever the live tempo";
    const juce::String restartGoesBack = "restart goes back to the start, for sequences scaled for the tempo set then";
    const juce::String ignoresNoTempo  = "a tempo of zero or less is ignored";

    constexpr double startTempo       = 120.0;
    constexpr double slowerTempo      = 60.0;
    constexpr double fasterTempo      = 240.0;
    constexpr double noTempo          = 0.0;
    constexpr double negativeTempo    = -30.0;
    constexpr double oneSecond        = 1.0;
    constexpr double halfSecond       = 0.5;
    constexpr double secondsPerMinute = 60.0;
    constexpr double tolerance        = 1.0e-9;
}

class TempoClockTest : public juce::UnitTest
{
public:
    TempoClockTest() : juce::UnitTest (TempoClockTestValues::testName, TempoClockTestValues::testCategory) {}

    void runTest() override
    {
        using namespace TempoClockTestValues;

        beginTest (keepsPosition);
        {
            TempoClock clock;
            clock.setTempo (startTempo);
            clock.restart();
            expectWithinAbsoluteError (clock.advance (oneSecond), oneSecond, tolerance);

            clock.setTempo (slowerTempo);
            expectWithinAbsoluteError (clock.positionSeconds(), oneSecond, tolerance);   // no jump at the change
            const auto afterSlower = oneSecond + oneSecond * slowerTempo / startTempo;
            expectWithinAbsoluteError (clock.advance (oneSecond), afterSlower, tolerance);

            clock.setTempo (fasterTempo);
            expectWithinAbsoluteError (clock.advance (halfSecond), afterSlower + halfSecond * fasterTempo / startTempo, tolerance);
        }

        beginTest (beatsFollow);
        {
            TempoClock clock;
            clock.setTempo (startTempo);
            clock.restart();
            clock.advance (oneSecond);
            const auto beatsAtStartTempo = oneSecond * startTempo / secondsPerMinute;
            expectWithinAbsoluteError (clock.beats(), beatsAtStartTempo, tolerance);

            clock.setTempo (slowerTempo);
            clock.advance (oneSecond);
            expectWithinAbsoluteError (clock.beats(), beatsAtStartTempo + oneSecond * slowerTempo / secondsPerMinute, tolerance);
        }

        beginTest (restartGoesBack);
        {
            TempoClock clock;
            clock.setTempo (startTempo);
            clock.restart();
            clock.advance (oneSecond);
            clock.setTempo (slowerTempo);
            clock.restart();
            expectWithinAbsoluteError (clock.positionSeconds(), double {}, tolerance);
            expectWithinAbsoluteError (clock.advance (oneSecond), oneSecond, tolerance);   // scaled for 60 now: real time
            expectWithinAbsoluteError (clock.beats(), oneSecond * slowerTempo / secondsPerMinute, tolerance);
        }

        beginTest (ignoresNoTempo);
        {
            TempoClock clock;
            clock.setTempo (startTempo);
            clock.restart();
            clock.setTempo (noTempo);
            clock.setTempo (negativeTempo);
            expectWithinAbsoluteError (clock.getTempo(), startTempo, tolerance);
            expectWithinAbsoluteError (clock.advance (oneSecond), oneSecond, tolerance);
        }
    }
};

static TempoClockTest tempoClockTest;
