#include <JuceHeader.h>
#include "Audio/MidiChannelSplitter.h"
#include "MidiHandler.h"
#include "Audio/AudioHandler.h"

/*  The audio callback must not allocate or wait on a lock (wiki: audio thread rule). The per-channel MIDI split keeps
    its reserved storage block after block, and taking the MIDI block never waits for the MIDI thread. */

namespace AudioThreadTestValues
{
    const juce::String testName     = "Audio thread real-time rules";
    const juce::String testCategory = "Unit";

    const juce::String routesByChannel  = "the block's events reach their own channel, at their sample position";
    const juce::String keepsStorage     = "after prepare, a full block every callback never reallocates a channel buffer";
    const juce::String neverWaits       = "taking the MIDI block does not wait while the MIDI thread holds the queue";
    const juce::String laterBlockGetsIt = "what was queued then reaches the synth in a later block";
    const juce::String noSfzReported    = "a note on a channel without an SFZ is reported on the message thread, once";

    constexpr int firstChannel   = 1;
    constexpr int secondChannel  = 2;
    constexpr int lastChannel    = 16;
    constexpr int stereo        = 2;
    constexpr int thirdChannel  = 3;
    constexpr int firstChannelBit = 1;   // channel 1's bit in the report's mask
    constexpr int thirdChannelBit = firstChannelBit << (thirdChannel - firstChannel);
    constexpr int noReport      = 0;
    constexpr int note           = 60;
    constexpr int otherNote      = 64;
    constexpr juce::uint8 velocity = 100;
    constexpr int earlySample    = 10;
    constexpr int lateSample     = 300;
    constexpr int oneEvent       = 1;
    constexpr int noEvents       = 0;
    constexpr int reservedEvents = 256;
    constexpr int callbacks      = 50;
    const juce::uint8 sysExBytes[] = { 0x7e, 0x7f, 0x09, 0x01 };   // GM on: has no channel

    constexpr int blockStart     = 0;
    constexpr int blockSize      = 512;
    constexpr int parkTimeoutMs  = 5000;   // the MIDI thread waits at most this long inside the lock
    constexpr int returnWithinMs = 1000;   // the audio side must be back long before the MIDI thread lets go
    constexpr int keyboardNote   = 60;
    constexpr float keyboardVelocity = 0.8f;
}

using namespace AudioThreadTestValues;

/** Parks the MIDI thread inside MidiHandler's locked section until released. */
struct ParkingListener : public MidiHandlerListener
{
    juce::WaitableEvent inside, release;
    void noteOnReceived (int) override
    {
        inside.signal();
        release.wait (parkTimeoutMs);
    }
};

class AudioThreadRealtimeTest : public juce::UnitTest
{
public:
    AudioThreadRealtimeTest() : juce::UnitTest (AudioThreadTestValues::testName, AudioThreadTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (routesByChannel);
        {
            MidiChannelSplitter splitter;
            splitter.prepare (reservedEvents);
            juce::MidiBuffer block;
            block.addEvent (juce::MidiMessage::noteOn (firstChannel, note, velocity), earlySample);
            block.addEvent (juce::MidiMessage::noteOn (lastChannel, otherNote, velocity), lateSample);
            block.addEvent (juce::MidiMessage::createSysExMessage (sysExBytes, static_cast<int> (sizeof (sysExBytes))), earlySample);
            splitter.split (block);

            expectEquals (splitter.channel (firstChannel).getNumEvents(), oneEvent);
            expectEquals (splitter.channel (lastChannel).getNumEvents(), oneEvent);
            expectEquals (splitter.channel (secondChannel).getNumEvents(), noEvents);
            expectEquals ((*splitter.channel (lastChannel).begin()).samplePosition, lateSample);
            expectEquals ((*splitter.channel (lastChannel).begin()).getMessage().getNoteNumber(), otherNote);

            splitter.split ({});
            expectEquals (splitter.channel (firstChannel).getNumEvents(), noEvents);   // each block starts empty
        }

        beginTest (keepsStorage);
        {
            MidiChannelSplitter splitter;
            splitter.prepare (reservedEvents);
            juce::MidiBuffer block;
            for (int eventIndex {}; eventIndex < reservedEvents; ++eventIndex)
                block.addEvent (juce::MidiMessage::noteOn (firstChannel, note, velocity), eventIndex);

            splitter.split (block);
            const auto* storage = splitter.channel (firstChannel).data.begin();
            bool sameStorage = true;
            for (int callback {}; callback < callbacks; ++callback)
            {
                splitter.split (block);
                sameStorage = sameStorage && splitter.channel (firstChannel).data.begin() == storage;
            }
            expect (sameStorage);
            expectEquals (splitter.channel (firstChannel).getNumEvents(), reservedEvents);
        }

        beginTest (neverWaits);
        {
            MidiDevice device;
            MidiHandler handler (device);
            handler.setInternalSynthListening (true);
            ParkingListener parking;
            handler.addListener (&parking);

            juce::WaitableEvent midiDone;
            juce::Thread::launch ([&handler, &midiDone]
            {
                handler.handleIncomingMidiMessage (nullptr, juce::MidiMessage::noteOn (firstChannel, keyboardNote, keyboardVelocity));
                midiDone.signal();
            });
            expect (parking.inside.wait (parkTimeoutMs));

            juce::WaitableEvent audioDone;
            juce::MidiBuffer audioBlock;
            juce::Thread::launch ([&handler, &audioBlock, &audioDone]
            {
                handler.getNextMidiBlock (audioBlock, blockStart, blockSize);
                audioDone.signal();
            });
            expect (audioDone.wait (returnWithinMs));

            // Whatever happened above, let both threads finish before their references go out of scope.
            parking.release.signal();
            audioDone.wait (parkTimeoutMs);

            beginTest (laterBlockGetsIt);
            juce::MidiBuffer nextBlock;
            expect (midiDone.wait (parkTimeoutMs));   // the MIDI thread has queued the note and left
            handler.getNextMidiBlock (nextBlock, blockStart, blockSize);
            expectGreaterOrEqual (audioBlock.getNumEvents() + nextBlock.getNumEvents(), oneEvent);
            handler.removeListener (&parking);
        }

        beginTest (noSfzReported);
        {
            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            int reported = noReport;
            synth.onNoSfzForChannels = [&reported] (int channelMask) { reported = channelMask; };
            handler.setInternalSynthListening (true);
            handler.injectMidiMessage (juce::MidiMessage::noteOn (thirdChannel, note, velocity));

            juce::AudioBuffer<float> output (stereo, blockSize);
            synth.audioDeviceIOCallbackWithContext (nullptr, int {}, output.getArrayOfWritePointers(), stereo, blockSize, {});
            synth.reportChannelsWithoutSfz();
            expectEquals (reported, thirdChannelBit);

            reported = noReport;
            synth.reportChannelsWithoutSfz();
            expectEquals (reported, noReport);   // reported once, not again
        }
    }
};

static AudioThreadRealtimeTest audioThreadRealtimeTest;
