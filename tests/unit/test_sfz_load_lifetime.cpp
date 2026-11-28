#include <JuceHeader.h>
#include "Audio/AudioHandler.h"
#include "MidiHandler.h"

/*  AudioHandler::loadSfz loads on background threads that write into the handler (its synths, its format manager).
    They used to be detached (juce::Thread::launch capturing `this`), so closing the app - or switching styles and
    then closing - mid-load let them write into a destroyed handler. Destroying the handler now waits for the loads
    that are running and drops the queued ones. Observable from outside: on Windows a file a loader still reads cannot
    be deleted, so right after the handler is gone the samples must be free. (Without the fix a loader is still
    reading here, or writes into freed memory and takes the runner down.) */

namespace SfzLoadLifetimeTestValues
{
    const juce::String testName     = "SFZ load lifetime";
    const juce::String testCategory = "Unit";

    const juce::String destroyWaitsForLoads =
        "destroying the handler while SFZs load waits for them: nothing still reads the samples afterwards";

    const juce::String fixtureFolderName = "sfz-load-lifetime-test";
    const juce::String toneFileName      = "long-tone.wav";
    const juce::String sfzFileName       = "long-tone.sfz";
    const juce::String sfzContents       = "<region> sample=long-tone.wav lokey=0 hikey=127";

    constexpr double sampleRate      = 48000.0;
    constexpr double toneSeconds     = 60.0;    // megabytes to read: every loader is still busy when the handler goes
    constexpr int    toneChannelCount = 1;
    constexpr int    toneBitDepth    = 16;
    constexpr int    firstSample     = 0;
    constexpr int    firstMidiChannel = 1;
    constexpr int    lastMidiChannel  = 16;
}

using namespace SfzLoadLifetimeTestValues;

class SfzLoadLifetimeTest : public juce::UnitTest
{
public:
    SfzLoadLifetimeTest() : juce::UnitTest (SfzLoadLifetimeTestValues::testName, SfzLoadLifetimeTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (destroyWaitsForLoads);

        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (fixtureFolderName);
        folder.deleteRecursively();
        folder.createDirectory();
        writeSilence (folder.getChildFile (toneFileName));
        folder.getChildFile (sfzFileName).replaceWithText (sfzContents);

        {
            MidiDevice device;
            MidiHandler handler (device);
            auto synth = std::make_unique<AudioHandler> (handler);
            for (int midiChannel = firstMidiChannel; midiChannel <= lastMidiChannel; ++midiChannel)
                synth->loadSfz (folder.getChildFile (sfzFileName), midiChannel);
            synth.reset();   // the app closing mid-load
        }

        expect (folder.deleteRecursively());
    }

private:
    static void writeSilence (const juce::File& file)
    {
        const int length = juce::roundToInt (toneSeconds * sampleRate);
        juce::AudioBuffer<float> tone (toneChannelCount, length);
        tone.clear();

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sampleRate, toneChannelCount, toneBitDepth, {}, {}));
        if (writer != nullptr)
        {
            stream.release();   // the writer owns the stream now
            writer->writeFromAudioSampleBuffer (tone, firstSample, length);
        }
    }
};

static SfzLoadLifetimeTest sfzLoadLifetimeTest;
