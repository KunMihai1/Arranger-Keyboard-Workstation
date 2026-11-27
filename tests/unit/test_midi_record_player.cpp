#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

namespace MidiRecordPlayerTestValues
{
    const juce::String testName     = "MidiRecordPlayer";
    const juce::String testCategory = "Unit";

    const juce::String leftHandNoteOnRemapped   = "remapChannel - channel 1 -> 14, note-on preserved";
    const juce::String rightHandNoteOffRemapped = "remapChannel - channel 16 -> 15, note-off preserved";
    const juce::String otherChannelsUnchanged   = "remapChannel - other channels are left unchanged";
    const juce::String controllerRemapped       = "remapChannel - controller on ch1 remapped, type + data preserved";
    const juce::String programChangeRemapped    = "remapChannel - program change on ch16 remapped to 15";
    const juce::String pitchWheelRemapped       = "remapChannel - pitch wheel on ch1 remapped, value preserved";
    const juce::String unhandledTypeUnchanged   = "remapChannel - an unhandled message type is returned unchanged (channel kept)";
    const juce::String defaultsAreEmpty         = "defaults - program numbers 0 and no recorded events";
    const juce::String stateMachine             = "state machine - start arms + clears; stop guard returns false/true";
    const juce::String captureOnlyWhileRecording = "capture - handleIncomingMessage records (remapped) only while recording";
    const juce::String serializationRoundTrip   = "serialization round-trips through in-memory streams (no disk)";
    const juce::String garbageInputFails        = "readRecordingFromStream - garbage input fails cleanly with an error message";
    const juce::String notesCounted             = "the Recorder page's count: note-ons only, not the program changes recorded at the start";
    const juce::String lengthOfRecording        = "the recording's length: to Stop, at least to its last event; running while recording";
    const juce::String filePlayingReported      = "file playback is reported while it plays";

    const juce::String wrongChannelMessage   = "a parsed note was on the wrong channel";
    const juce::String notesLostMessage      = "note numbers did not survive the round-trip";
    const char garbageInput[] = "not a midi file at all";

    // The record player moves the hands' channels: 1 -> 14 and 16 -> 15.
    constexpr int leftHandChannel          = 1;
    constexpr int recordedLeftHandChannel  = 14;
    constexpr int rightHandChannel         = 16;
    constexpr int recordedRightHandChannel = 15;
    constexpr int untouchedChannel         = 5;
    constexpr int styleChannel             = 2;

    constexpr int         middleC       = 60;
    constexpr int         noteE4        = 64;
    constexpr juce::uint8 velocity      = 100;
    constexpr juce::uint8 softVelocity  = 90;
    constexpr juce::uint8 lowestVelocity = 1;
    constexpr int         volumeController = 7;
    constexpr int         controllerValue  = 100;
    constexpr int         programNumber    = 42;
    constexpr int         pitchWheelValue  = 12000;
    constexpr int         pressureValue    = 90;

    constexpr int    defaultProgram   = 0;
    constexpr int    noEvents         = 0;
    constexpr int    oneEvent         = 1;
    constexpr int    twoNoteOns       = 2;
    constexpr int    atLeastOneNoteOff = 1;
    constexpr double recordingStart   = 0.0;
    constexpr double halfSecond       = 0.5;
    constexpr double oneSecond        = 1.0;
    constexpr double tempo            = 120.0;
    constexpr int    recordedPrograms = 2;     // startRecording's presets: one program change per hand
}

using namespace MidiRecordPlayerTestValues;

//======================================================================================
//  MidiRecordPlayer::remapChannel — record-player channel remap (previously untested)
//======================================================================================
class MidiRecordPlayerTest : public juce::UnitTest
{
public:
    MidiRecordPlayerTest() : juce::UnitTest(MidiRecordPlayerTestValues::testName, MidiRecordPlayerTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(leftHandNoteOnRemapped);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::noteOn(leftHandChannel, middleC, velocity));
            expect(message.isNoteOn());
            expectEquals(message.getChannel(), recordedLeftHandChannel);
            expectEquals(message.getNoteNumber(), middleC);
            expectEquals((int) message.getVelocity(), (int) velocity);
        }

        beginTest(rightHandNoteOffRemapped);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::noteOff(rightHandChannel, noteE4));
            expect(message.isNoteOff());
            expectEquals(message.getChannel(), recordedRightHandChannel);
            expectEquals(message.getNoteNumber(), noteE4);
        }

        beginTest(otherChannelsUnchanged);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::noteOn(untouchedChannel, middleC, softVelocity));
            expectEquals(message.getChannel(), untouchedChannel);
        }

        beginTest(controllerRemapped);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::controllerEvent(leftHandChannel, volumeController, controllerValue));
            expect(message.isController());
            expectEquals(message.getChannel(), recordedLeftHandChannel);
            expectEquals(message.getControllerNumber(), volumeController);
            expectEquals(message.getControllerValue(), controllerValue);
        }

        beginTest(programChangeRemapped);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::programChange(rightHandChannel, programNumber));
            expect(message.isProgramChange());
            expectEquals(message.getChannel(), recordedRightHandChannel);
            expectEquals(message.getProgramChangeNumber(), programNumber);
        }

        beginTest(pitchWheelRemapped);
        {
            MidiRecordPlayer player;
            auto message = player.remapChannel(juce::MidiMessage::pitchWheel(leftHandChannel, pitchWheelValue));
            expect(message.isPitchWheel());
            expectEquals(message.getChannel(), recordedLeftHandChannel);
            expectEquals(message.getPitchWheelValue(), pitchWheelValue);
        }

        beginTest(unhandledTypeUnchanged);
        {
            MidiRecordPlayer player;
            // channel pressure is none of the remapped types -> returned as-is, channel NOT remapped
            auto message = player.remapChannel(juce::MidiMessage::channelPressureChange(leftHandChannel, pressureValue));
            expect(message.isChannelPressure());
            expectEquals(message.getChannel(), leftHandChannel);
        }

        beginTest(defaultsAreEmpty);
        {
            MidiRecordPlayer player;
            expectEquals(player.getProgramLeftHand(), defaultProgram);
            expectEquals(player.getProgramRightHand(), defaultProgram);
            expectEquals(player.getSizeRecorded(), noEvents);
        }

        beginTest(stateMachine);
        {
            MidiRecordPlayer player;
            player.applyPresetFunction = [] {};                 // startRecording invokes this; stub it
            player.getAllRecordedEvents().push_back(RecordedEvent{ juce::MidiMessage::noteOn(leftHandChannel,middleC,lowestVelocity), recordingStart });

            expect(player.stopRecording() == false);            // not recording yet -> guard returns false
            player.startRecording();
            expect(player.getIsRecording());
            expectEquals(player.getSizeRecorded(), noEvents);   // startRecording cleared the buffer
            expect(player.stopRecording() == true);             // was recording -> true
            expect(! player.getIsRecording());
        }

        beginTest(captureOnlyWhileRecording);
        {
            MidiRecordPlayer player;
            player.applyPresetFunction = [] {};

            player.handleIncomingMessage(juce::MidiMessage::noteOn(leftHandChannel, middleC, velocity));  // not recording
            expectEquals(player.getSizeRecorded(), noEvents);   // ignored

            player.startRecording();
            player.handleIncomingMessage(juce::MidiMessage::noteOn(leftHandChannel, middleC, velocity));  // ch1 -> remapped to 14
            expectEquals(player.getSizeRecorded(), oneEvent);
            const auto& ev = player.getAllRecordedEvents().back();
            expect(ev.message.isNoteOn());
            expectEquals(ev.message.getChannel(), recordedLeftHandChannel);     // remapChannel applied on capture
            expectEquals(ev.message.getNoteNumber(), middleC);
            expect(ev.timeFromStart >= recordingStart);
            player.stopRecording();
        }

        beginTest(serializationRoundTrip);
        {
            MidiRecordPlayer player;
            // Populate the in-memory buffer directly (skip the timer/wall-clock recording path).
            auto& evs = player.getAllRecordedEvents();
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOn (styleChannel, middleC, velocity), recordingStart });
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOn (styleChannel, noteE4, velocity),  halfSecond });
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOff(styleChannel, middleC),           oneSecond });

            juce::MemoryOutputStream out;
            juce::String err;
            expect(player.writeRecordingToStream(out, err, tempo), err);

            juce::MemoryInputStream in(out.getData(), out.getDataSize(), false);
            expect(player.readRecordingFromStream(in, err), err);

            int noteOns {}, noteOffs {}; bool chOk = true; bool has60 = false, has64 = false;
            for (const auto& event : player.getParsedEvents())
            {
                if (event.message.isNoteOn())  { ++noteOns;  if (event.message.getChannel() != styleChannel) chOk = false;
                                                 if (event.message.getNoteNumber() == middleC) has60 = true;
                                                 if (event.message.getNoteNumber() == noteE4) has64 = true; }
                else if (event.message.isNoteOff()) { ++noteOffs; if (event.message.getChannel() != styleChannel) chOk = false; }
            }
            expectEquals(noteOns, twoNoteOns);
            expect(noteOffs >= atLeastOneNoteOff);
            expect(chOk, wrongChannelMessage);
            expect(has60 && has64, notesLostMessage);
        }

        beginTest(notesCounted);
        {
            MidiRecordPlayer player;
            auto& events = player.getAllRecordedEvents();
            for (int hand {}; hand < recordedPrograms; ++hand)
                events.push_back(RecordedEvent{ juce::MidiMessage::programChange (styleChannel, programNumber), recordingStart });
            events.push_back(RecordedEvent{ juce::MidiMessage::noteOn (styleChannel, middleC, velocity), halfSecond });
            events.push_back(RecordedEvent{ juce::MidiMessage::noteOff (styleChannel, middleC), oneSecond });
            events.push_back(RecordedEvent{ juce::MidiMessage::noteOn (styleChannel, noteE4, velocity), oneSecond });
            expectEquals(player.getRecordedNoteCount(), twoNoteOns);
        }

        beginTest(lengthOfRecording);
        {
            MidiRecordPlayer player;
            player.applyPresetFunction = [] {};
            expectEquals(player.getRecordedSeconds(), recordingStart);
            player.startRecording();
            expect(player.getRecordedSeconds() >= recordingStart);
            player.stopRecording();
            const auto lengthAtStop = player.getRecordedSeconds();
            expect(lengthAtStop >= recordingStart);
            expectEquals(player.getRecordedSeconds(), lengthAtStop);   // stopped: it no longer runs

            player.getAllRecordedEvents().push_back(RecordedEvent{ juce::MidiMessage::noteOn (styleChannel, middleC, velocity), oneSecond });
            expect(player.getRecordedSeconds() >= oneSecond);
        }

        beginTest(filePlayingReported);
        {
            MidiRecordPlayer player;
            expect(! player.getIsPlayingFile());
            player.startRecordingFilePlaying();
            expect(player.getIsPlayingFile());
            player.stopRecordingFilePlaying();
            expect(! player.getIsPlayingFile());
        }

        beginTest(garbageInputFails);
        {
            MidiRecordPlayer player;
            juce::MemoryInputStream in(garbageInput, sizeof(garbageInput), false);
            juce::String err;
            expect(! player.readRecordingFromStream(in, err));   // returns false
            expect(err.isNotEmpty());                        // and sets an error message
        }
    }
};
static MidiRecordPlayerTest midiRecordPlayerTest;
