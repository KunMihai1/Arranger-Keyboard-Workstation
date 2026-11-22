#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerSourceBuilder.h"

namespace ArrangerSourceBuilderTestValues
{
    const juce::String testName     = "ArrangerSourceBuilder";
    const juce::String testCategory = "Arranger";

    const juce::String channelsAndBeats  = "melodic tracks get 2,3,...; percussion gets 10; events convert to beats";
    const juce::String overflowDropped   = "more than 11 melodic tracks: overflow tracks are dropped, not crashed";

    const juce::String keysName    = "Keys";
    const juce::String bassName    = "Bass";
    const juce::String drumsName   = "Drums";
    const juce::String melodicPrefix = "M";

    constexpr int         recordedChannel = 1;
    constexpr int         middleC         = 60;
    constexpr int         bassNote        = 40;
    constexpr int         kickNote        = 36;
    constexpr juce::uint8 velocity        = 100;
    constexpr double      startSeconds    = 0.0;
    constexpr double      oneSecond       = 1.0;    // 2 beats at 120 BPM
    constexpr double      halfSecond      = 0.5;
    constexpr int         electricPiano   = 4;
    constexpr double      keysVolume      = 90.0;
    constexpr double      tempo120        = 120.0;
    constexpr double      tolerance       = 1e-9;

    constexpr int    threeTracks        = 3;
    constexpr size_t firstTrack         = 0;
    constexpr size_t secondTrack        = 1;
    constexpr size_t thirdTrack         = 2;
    constexpr int    firstMelodicChannel  = 2;
    constexpr int    secondMelodicChannel = 3;
    constexpr int    drumChannel        = 10;
    constexpr int    twoEvents          = 2;
    constexpr size_t noteOnEvent        = 0;
    constexpr size_t noteOffEvent       = 1;
    constexpr double twoBeats           = 2.0;
    constexpr int    tooManyMelodic     = 13;
    constexpr int    melodicChannels    = 11;   // melodic channels available
}

class ArrangerSourceBuilderTest : public juce::UnitTest
{
public:
    ArrangerSourceBuilderTest() : juce::UnitTest (ArrangerSourceBuilderTestValues::testName, ArrangerSourceBuilderTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerSourceBuilderTestValues;

        beginTest (channelsAndBeats);
        {
            TrackEntry keys; keys.type = TrackType::Melodic; keys.displayName = keysName;
            keys.instrumentAssociated = electricPiano; keys.volumeAssociated = keysVolume;
            keys.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, middleC, velocity), startSeconds);
            keys.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), oneSecond); // 1s -> 2 beats @120

            TrackEntry bass; bass.type = TrackType::Melodic; bass.displayName = bassName;
            bass.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, bassNote, velocity), startSeconds);
            bass.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, bassNote), oneSecond);

            TrackEntry drums; drums.type = TrackType::Percussion; drums.displayName = drumsName;
            drums.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, kickNote, velocity), startSeconds);
            drums.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, kickNote), halfSecond);

            std::vector<TrackEntry> tracks { keys, drums, bass };
            auto sources = ArrangerSourceBuilder::fromTrackEntries (tracks, tempo120);

            expectEquals ((int) sources.size(), threeTracks);
            // order preserved; channels allocated by type
            expectEquals (sources[firstTrack].channel, firstMelodicChannel);   // first melodic
            expect (sources[firstTrack].partType != ArrangerPartType::Drum);
            expectEquals (sources[secondTrack].channel, drumChannel);          // percussion
            expect (sources[secondTrack].partType == ArrangerPartType::Drum);
            expectEquals (sources[thirdTrack].channel, secondMelodicChannel);  // second melodic

            // part-type classification: "Keys" -> Acc, "Bass" -> Bass (name-based), "Drums" -> Drum
            expect (sources[firstTrack].partType == ArrangerPartType::Acc);
            expect (sources[thirdTrack].partType == ArrangerPartType::Bass);

            // events converted + channel overridden
            expectEquals ((int) sources[firstTrack].events.size(), twoEvents);
            expectWithinAbsoluteError (sources[firstTrack].events[noteOffEvent].beats, twoBeats, tolerance);
            expectEquals (sources[firstTrack].events[noteOnEvent].message.getChannel(), firstMelodicChannel);
            expectEquals (sources[firstTrack].instrument, electricPiano);
            expectWithinAbsoluteError (sources[firstTrack].volume, keysVolume, tolerance);
        }

        beginTest (overflowDropped);
        {
            std::vector<TrackEntry> tracks;
            for (int trackIndex {}; trackIndex < tooManyMelodic; ++trackIndex)
            {
                TrackEntry track; track.type = TrackType::Melodic; track.displayName = melodicPrefix + juce::String (trackIndex);
                track.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, middleC, velocity), startSeconds);
                track.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), halfSecond);
                tracks.push_back (track);
            }
            auto sources = ArrangerSourceBuilder::fromTrackEntries (tracks, tempo120);
            expectEquals ((int) sources.size(), melodicChannels);
        }
    }
};
static ArrangerSourceBuilderTest arrangerSourceBuilderTest;
