#include <juce_core/juce_core.h>
#include "Arranger/ArrangerChannels.h"

namespace ArrangerChannelsTestValues
{
    const juce::String testName     = "ArrangerChannels";
    const juce::String testCategory = "Arranger";

    const juce::String melodicChannelsInOrder = "melodic channels are 2..9 then 11..13, skipping reserved";
    const juce::String neverReserved          = "never returns a reserved channel";
    const juce::String overflowIsNoChannel    = "overflow past 11 melodic tracks returns -1";

    // Channel 1 and 16 are the hands, 10 is drums, 14 and 15 hold recorded hands.
    const int expectedChannels[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 };
    constexpr int melodicTrackCount = 11;
    constexpr int leftHandChannel   = 1;
    constexpr int drumChannel       = 10;
    constexpr int recordedLeft      = 14;
    constexpr int recordedRight     = 15;
    constexpr int rightHandChannel  = 16;
    constexpr int firstOverflow     = 11;
    constexpr int farOverflow       = 50;
    constexpr int noChannel         = -1;
}

class ArrangerChannelsTest : public juce::UnitTest
{
public:
    ArrangerChannelsTest() : juce::UnitTest (ArrangerChannelsTestValues::testName, ArrangerChannelsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerChannelsTestValues;

        beginTest (melodicChannelsInOrder);
        {
            for (int trackIndex {}; trackIndex < melodicTrackCount; ++trackIndex)
                expectEquals (ArrangerChannels::melodicChannelForIndex (trackIndex), expectedChannels[trackIndex]);
        }

        beginTest (neverReserved);
        {
            for (int trackIndex {}; trackIndex < melodicTrackCount; ++trackIndex)
            {
                const int ch = ArrangerChannels::melodicChannelForIndex (trackIndex);
                expect (ch != leftHandChannel && ch != drumChannel && ch != recordedLeft && ch != recordedRight && ch != rightHandChannel);
            }
        }

        beginTest (overflowIsNoChannel);
        {
            expectEquals (ArrangerChannels::melodicChannelForIndex (firstOverflow), noChannel);
            expectEquals (ArrangerChannels::melodicChannelForIndex (farOverflow), noChannel);
        }
    }
};
static ArrangerChannelsTest arrangerChannelsTest;
