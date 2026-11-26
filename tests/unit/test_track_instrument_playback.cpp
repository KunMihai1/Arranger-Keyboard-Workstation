#include <JuceHeader.h>
#include "CurrentStyleComponent.h"

namespace TrackInstrumentPlaybackTestValues
{
    const juce::String testName     = "CurrentStyleComponent (a track's chosen instrument is played)";
    const juce::String testCategory = "Unit";

    const juce::String startPlaysChosen   = "Start plays the instrument chosen on the strip, not the file's or the one baked at load";
    const juce::String configTakesStrip   = "a saved arranger configuration plays the strip's instrument and volume";
    const juce::String configKeepsUnset   = "a strip with no instrument keeps the configuration's instrument";
    const juce::String choiceIsReported   = "choosing an instrument on a strip reports its channel and program";
    const juce::String noNoteOn           = "the sequence has no note-on";

    const juce::String styleName = "Style 1";
    constexpr int    trackChannel     = 1;
    constexpr int    filesProgram     = 0;     // Acoustic Grand Piano, in the MIDI file
    constexpr int    programAtLoad    = 24;    // Nylon guitar: the strip's instrument when the style loaded
    constexpr int    chosenProgram    = 40;    // Violin: picked on the strip after the load
    constexpr int    configProgram    = 33;    // Electric Bass in the saved configuration
    constexpr int    noteNumber       = 60;
    constexpr juce::uint8 noteVelocity = 100;
    constexpr double fileTempo        = 120.0;
    constexpr double atStart          = 0.0;
    constexpr double noteOnSeconds    = 0.5;
    constexpr double noteOffSeconds   = 1.0;
    constexpr double configVolume     = 100.0;
    constexpr double stripVolume      = 70.0;
    constexpr int    firstSlot        = 0;
    constexpr int    secondSlot       = 1;
    constexpr int    notReported      = -1;
}

/*
    Owner (2026-11-25): "when selecting an instrument for a track, the change isn't reflected when playing".
    Loading a style bakes a program change into each track's sequence (with the instrument the entry held then) next
    to the MIDI file's own program changes; Start sent the newly chosen program and then the sequence switched it back.
    A saved arranger configuration only took the strips' volumes.
*/
class TrackInstrumentPlaybackTest : public juce::UnitTest
{
public:
    TrackInstrumentPlaybackTest() : juce::UnitTest (TrackInstrumentPlaybackTestValues::testName, TrackInstrumentPlaybackTestValues::testCategory) {}

    void runTest() override
    {
        checkStartPlaysChosenInstrument();
        checkConfigurationTakesStrip();
        checkChoiceIsReported();
    }

private:
    /** The program the channel plays at the sequence's first note-on (the last program change before it). */
    static int programAtFirstNote (const juce::MidiMessageSequence& sequence, int& channelOut)
    {
        int program = TrackInstrumentPlaybackTestValues::notReported;
        std::map<int, int> programPerChannel;
        for (int eventIndex {}; eventIndex < sequence.getNumEvents(); ++eventIndex)
        {
            const auto& message = sequence.getEventPointer (eventIndex)->message;
            if (message.isProgramChange())
                programPerChannel[message.getChannel()] = message.getProgramChangeNumber();
            else if (message.isNoteOn())
            {
                channelOut = message.getChannel();
                const auto found = programPerChannel.find (channelOut);
                return found != programPerChannel.end() ? found->second : program;
            }
        }
        return program;
    }

    void checkStartPlaysChosenInstrument()
    {
        using namespace TrackInstrumentPlaybackTestValues;
        beginTest (startPlaysChosen);

        TrackEntry entry;
        entry.uuid = TrackEntry::generateUUID();
        entry.type = TrackType::Melodic;
        entry.originalBPM = fileTempo;
        entry.instrumentAssociated = programAtLoad;
        entry.originalSequenceTicks.addEvent (juce::MidiMessage::programChange (trackChannel, filesProgram).withTimeStamp (atStart));
        entry.originalSequenceTicks.addEvent (juce::MidiMessage::noteOn (trackChannel, noteNumber, noteVelocity).withTimeStamp (noteOnSeconds));
        entry.originalSequenceTicks.addEvent (juce::MidiMessage::noteOff (trackChannel, noteNumber).withTimeStamp (noteOffSeconds));
        entry.originalSequenceTicks.updateMatchedPairs();

        std::unordered_map<juce::Uuid, TrackEntry*> entries { { entry.uuid, &entry } };
        CurrentStyleComponent panel (styleName, entries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});   // an output, as the SFZ engine gives
        auto* strip = panel.getAllTracks()[firstSlot];
        strip->setUUID (entry.uuid);
        strip->setInstrumentNumber (programAtLoad);

        panel.applyBPMchangeBeforePlayback (fileTempo, true);   // what loading the style does
        strip->chooseInstrument (chosenProgram);                // the owner picks Violin
        panel.startPlaying();

        int channel = notReported;
        const auto program = programAtFirstNote (entry.sequence, channel);
        expect (channel != notReported, noNoteOn);
        expectEquals (program, chosenProgram);

        panel.stopPlaying (false);
    }

    void checkConfigurationTakesStrip()
    {
        using namespace TrackInstrumentPlaybackTestValues;
        beginTest (configTakesStrip);

        juce::OwnedArray<Track> strips;
        auto* chosen = strips.add (new Track());
        chosen->setUUID (juce::Uuid());
        chosen->setInstrumentNumber (chosenProgram);
        chosen->setVolumeSlider (stripVolume);
        auto* unset = strips.add (new Track());
        unset->setUUID (juce::Uuid());
        unset->setVolumeSlider (stripVolume);

        ArrangerStyle style;
        ArrangerSection section;
        for (auto* strip : strips)
        {
            ArrangerTrack track;
            track.id = strip->getUsedID().toString();
            track.instrument = configProgram;
            track.volume = configVolume;
            section.tracks.push_back (track);
        }
        style.sections.push_back (section);

        CurrentStyleComponent::applyLiveTrackSettings (style, strips);

        const auto& tracks = style.sections.front().tracks;
        expectEquals (tracks[firstSlot].instrument, chosenProgram);
        expectEquals (tracks[firstSlot].volume, stripVolume);

        beginTest (configKeepsUnset);
        expectEquals (tracks[secondSlot].instrument, configProgram);
        expectEquals (tracks[secondSlot].volume, stripVolume);
    }

    void checkChoiceIsReported()
    {
        using namespace TrackInstrumentPlaybackTestValues;
        beginTest (choiceIsReported);

        std::unordered_map<juce::Uuid, TrackEntry*> noEntries;
        CurrentStyleComponent panel (styleName, noEntries, {}, {});
        int reportedChannel = notReported;
        int reportedProgram = notReported;
        panel.onTrackInstrumentChosen = [&] (int channel, int program)
        {
            reportedChannel = channel;
            reportedProgram = program;
        };
        auto* strip = panel.getAllTracks()[secondSlot];
        strip->chooseInstrument (chosenProgram);

        expectEquals (reportedChannel, strip->getChannel());
        expectEquals (reportedProgram, chosenProgram);
    }
};

static TrackInstrumentPlaybackTest trackInstrumentPlaybackTest;
