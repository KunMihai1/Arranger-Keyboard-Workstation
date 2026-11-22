#include <JuceHeader.h>
#include "Audio/AudioHandler.h"
#include "MidiHandler.h"

/*
    MidiHandler queues notes for the internal SFZ synth, and only the synth's audio callback empties
    that queue. With the External MIDI engine no audio callback runs, so the queue used to grow for the
    whole session and then fire every note at once into the synth's first block after switching to
    Internal SFZ (spec: docs/superpowers/specs/2026-11-22-stale-synth-midi-backlog-design.md).
    These tests drive AudioHandler through the same start/stop calls JUCE's AudioDeviceManager makes.
*/
namespace SynthQueueTestValues
{
    const juce::String testName     = "Internal synth MIDI queue";
    const juce::String testCategory = "Unit";

    const juce::String notesBeforeStartAreDropped = "notes played before the internal synth starts are not replayed into it";
    const juce::String notesWhileRunningArrive    = "notes played while the internal synth runs reach it";
    const juce::String notesAfterStopAreNotKept   = "notes played after the internal synth stops are not kept for it";
    const juce::String restartGetsOnlyNewNotes    = "a restarted synth gets only the notes played after the restart";

    const juce::String fakeDeviceName = "Fake output";
    const juce::String fakeDeviceType = "Fake";

    constexpr int    midiChannel       = 1;
    constexpr int    firstNote         = 48;
    constexpr int    playedNoteCount   = 100;
    constexpr int    eventsPerNote     = 2;    // a note-on and its note-off
    constexpr juce::uint8 velocity     = 100;
    constexpr double sampleRate        = 48000.0;
    constexpr int    blockSize         = 512;
    constexpr int    blockStart        = 0;
    constexpr int    noEvents          = 0;
    constexpr int    bitDepth          = 32;
    constexpr int    noLatency         = 0;

    // A held note across a stop/restart: its note-off is dropped while the synth is stopped, so the
    // restart itself must silence what was sounding.
    const juce::String heldNoteSilencedOnRestart = "a note still held when the synth stopped is silent after it restarts";
    const juce::String fixtureFolderName = "synth-queue-test";
    const juce::String toneFileName      = "tone.wav";
    const juce::String sfzFileName       = "tone.sfz";
    const juce::String sfzContents       = "<region> sample=tone.wav lokey=0 hikey=127";
    constexpr int    toneChannelCount    = 1;
    constexpr int    toneBitDepth        = 16;
    constexpr double toneSeconds         = 4.0;    // far longer than the test runs: never ends by itself
    constexpr double toneFrequency       = 440.0;
    constexpr float  toneLevel           = 0.5f;
    constexpr int    stereo              = 2;
    constexpr int    firstSample         = 0;
    constexpr int    heldNote            = 60;
    constexpr int    loadAttempts        = 300;
    constexpr int    attemptPauseMs      = 10;
    constexpr float  audibleLevel        = 0.001f;   // -60 dBFS
}

/** The smallest AudioIODevice AudioHandler::audioDeviceAboutToStart can read its rate and size from. */
class FakeAudioDevice : public juce::AudioIODevice
{
public:
    FakeAudioDevice() : juce::AudioIODevice (SynthQueueTestValues::fakeDeviceName, SynthQueueTestValues::fakeDeviceType) {}

    juce::StringArray getOutputChannelNames() override { return {}; }
    juce::StringArray getInputChannelNames() override { return {}; }
    juce::Array<double> getAvailableSampleRates() override { return { SynthQueueTestValues::sampleRate }; }
    juce::Array<int> getAvailableBufferSizes() override { return { SynthQueueTestValues::blockSize }; }
    int getDefaultBufferSize() override { return SynthQueueTestValues::blockSize; }
    juce::String open (const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start (juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return SynthQueueTestValues::blockSize; }
    double getCurrentSampleRate() override { return SynthQueueTestValues::sampleRate; }
    int getCurrentBitDepth() override { return SynthQueueTestValues::bitDepth; }
    juce::BigInteger getActiveOutputChannels() const override { return {}; }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return SynthQueueTestValues::noLatency; }
    int getInputLatencyInSamples() override { return SynthQueueTestValues::noLatency; }
};

class InternalSynthMidiQueueTest : public juce::UnitTest
{
public:
    InternalSynthMidiQueueTest() : juce::UnitTest (SynthQueueTestValues::testName, SynthQueueTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SynthQueueTestValues;

        beginTest (notesBeforeStartAreDropped);
        {
            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            FakeAudioDevice output;

            playNotes (handler, playedNoteCount);       // External MIDI session: nothing drains the queue
            synth.audioDeviceAboutToStart (&output);    // the user switches to Internal SFZ

            expectEquals (eventsTheSynthWouldTake (handler), noEvents);
        }

        beginTest (notesWhileRunningArrive);
        {
            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            FakeAudioDevice output;

            synth.audioDeviceAboutToStart (&output);
            playNotes (handler, playedNoteCount);

            expectEquals (eventsTheSynthWouldTake (handler), playedNoteCount * eventsPerNote);
        }

        beginTest (notesAfterStopAreNotKept);
        {
            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            FakeAudioDevice output;

            synth.audioDeviceAboutToStart (&output);
            synth.audioDeviceStopped();                 // the user switches back to External MIDI
            playNotes (handler, playedNoteCount);

            expectEquals (eventsTheSynthWouldTake (handler), noEvents);
        }

        beginTest (restartGetsOnlyNewNotes);
        {
            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            FakeAudioDevice output;

            synth.audioDeviceAboutToStart (&output);
            synth.audioDeviceStopped();
            playNotes (handler, playedNoteCount);
            synth.audioDeviceAboutToStart (&output);
            playNotes (handler, playedNoteCount);

            expectEquals (eventsTheSynthWouldTake (handler), playedNoteCount * eventsPerNote);
        }

        beginTest (heldNoteSilencedOnRestart);
        {
            const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (fixtureFolderName);
            folder.createDirectory();
            writeTone (folder.getChildFile (toneFileName));
            folder.getChildFile (sfzFileName).replaceWithText (sfzContents);

            MidiDevice device;
            MidiHandler handler (device);
            AudioHandler synth (handler);
            FakeAudioDevice output;

            synth.audioDeviceAboutToStart (&output);
            synth.loadSfz (folder.getChildFile (sfzFileName), midiChannel);

            // The SFZ loads on a background thread; until it has, a note-on is consumed without sound.
            bool sounding = false;
            for (int attempt {}; attempt < loadAttempts && ! sounding; ++attempt)
            {
                handler.injectMidiMessage (juce::MidiMessage::noteOn (midiChannel, heldNote, velocity));
                sounding = renderBlockLevel (synth) > audibleLevel;
                if (! sounding)
                    juce::Thread::sleep (attemptPauseMs);
            }
            expect (sounding);

            synth.audioDeviceStopped();                 // engine switched to External MIDI with the key held
            handler.injectMidiMessage (juce::MidiMessage::noteOff (midiChannel, heldNote, velocity));   // not queued now
            synth.audioDeviceAboutToStart (&output);    // and back to Internal SFZ

            expectLessThan (renderBlockLevel (synth), audibleLevel);

            folder.deleteRecursively();
        }
    }

private:
    static void writeTone (const juce::File& file)
    {
        using namespace SynthQueueTestValues;
        const int length = juce::roundToInt (toneSeconds * sampleRate);
        juce::AudioBuffer<float> tone (toneChannelCount, length);
        for (int sampleIndex {}; sampleIndex < length; ++sampleIndex)
            tone.setSample (firstSample, sampleIndex,
                            toneLevel * (float) std::sin (juce::MathConstants<double>::twoPi * toneFrequency * sampleIndex / sampleRate));

        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sampleRate, toneChannelCount, toneBitDepth, {}, {}));
        if (writer != nullptr)
        {
            stream.release();   // the writer owns the stream now
            writer->writeFromAudioSampleBuffer (tone, firstSample, length);
        }
    }

    /** Runs one audio block through the synth and returns its peak level. */
    static float renderBlockLevel (AudioHandler& synth)
    {
        using namespace SynthQueueTestValues;
        juce::AudioBuffer<float> block (stereo, blockSize);
        block.clear();
        synth.audioDeviceIOCallbackWithContext (nullptr, int {}, block.getArrayOfWritePointers(), stereo, blockSize, {});
        return block.getMagnitude (firstSample, blockSize);
    }

    static void playNotes (MidiHandler& handler, int noteCount)
    {
        using namespace SynthQueueTestValues;
        for (int noteIndex {}; noteIndex < noteCount; ++noteIndex)
        {
            handler.handleIncomingMidiMessage (nullptr, juce::MidiMessage::noteOn (midiChannel, firstNote, velocity));
            handler.handleIncomingMidiMessage (nullptr, juce::MidiMessage::noteOff (midiChannel, firstNote, velocity));
        }
    }

    /** What the synth's next audio block would receive (the same call its audio callback makes). */
    static int eventsTheSynthWouldTake (MidiHandler& handler)
    {
        using namespace SynthQueueTestValues;
        juce::MidiBuffer block;
        handler.getNextMidiBlock (block, blockStart, blockSize);
        return block.getNumEvents();
    }
};

static InternalSynthMidiQueueTest internalSynthMidiQueueTest;
