#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerMidiHex.h"

namespace ArrangerMidiHexTestValues
{
    const juce::String testName     = "ArrangerMidiHex";
    const juce::String testCategory = "Arranger";

    const juce::String noteOnRoundTrip  = "note-on round-trips through hex";
    const juce::String noteOffRoundTrip = "note-off round-trips through hex";
    const juce::String upperCaseNoSeparators = "hex string is uppercase bytes with no separators";

    constexpr int         drumChannel   = 10;
    constexpr int         kickNote      = 36;
    constexpr juce::uint8 velocity      = 100;
    constexpr int         bassChannel   = 2;
    constexpr int         middleC       = 60;
    constexpr int         firstChannel  = 1;
    constexpr int         hexNote       = 0x24;
    constexpr juce::uint8 hexVelocity   = 0x64;
    const juce::String    expectedHex   = "902464";   // 90 24 64: note-on channel 1, note 0x24, velocity 0x64
}

class ArrangerMidiHexTest : public juce::UnitTest
{
public:
    ArrangerMidiHexTest() : juce::UnitTest (ArrangerMidiHexTestValues::testName, ArrangerMidiHexTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerMidiHexTestValues;

        beginTest (noteOnRoundTrip);
        {
            auto message = juce::MidiMessage::noteOn (drumChannel, kickNote, velocity);
            auto back = ArrangerMidiHex::fromHex (ArrangerMidiHex::toHex (message));
            expect (back.isNoteOn());
            expectEquals (back.getChannel(), drumChannel);
            expectEquals (back.getNoteNumber(), kickNote);
            expectEquals ((int) back.getVelocity(), (int) velocity);
        }

        beginTest (noteOffRoundTrip);
        {
            auto message = juce::MidiMessage::noteOff (bassChannel, middleC);
            auto back = ArrangerMidiHex::fromHex (ArrangerMidiHex::toHex (message));
            expect (back.isNoteOff());
            expectEquals (back.getChannel(), bassChannel);
            expectEquals (back.getNoteNumber(), middleC);
        }

        beginTest (upperCaseNoSeparators);
        {
            auto message = juce::MidiMessage::noteOn (firstChannel, hexNote, hexVelocity);
            expectEquals (ArrangerMidiHex::toHex (message), expectedHex);
        }
    }
};
static ArrangerMidiHexTest arrangerMidiHexTest;
