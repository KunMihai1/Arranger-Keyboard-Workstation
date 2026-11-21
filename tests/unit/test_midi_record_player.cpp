#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"



//======================================================================================
//  MidiRecordPlayer::remapChannel — record-player channel remap (previously untested)
//======================================================================================
class MidiRecordPlayerTest : public juce::UnitTest
{
public:
    MidiRecordPlayerTest() : juce::UnitTest("MidiRecordPlayer", "Unit") {}

    void runTest() override
    {
        beginTest("remapChannel - channel 1 -> 14, note-on preserved");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100));
            expect(m.isNoteOn());
            expectEquals(m.getChannel(), 14);
            expectEquals(m.getNoteNumber(), 60);
            expectEquals((int) m.getVelocity(), 100);
        }

        beginTest("remapChannel - channel 16 -> 15, note-off preserved");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::noteOff(16, 64));
            expect(m.isNoteOff());
            expectEquals(m.getChannel(), 15);
            expectEquals(m.getNoteNumber(), 64);
        }

        beginTest("remapChannel - other channels are left unchanged");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::noteOn(5, 60, (juce::uint8) 90));
            expectEquals(m.getChannel(), 5);
        }

        beginTest("remapChannel - controller on ch1 remapped, type + data preserved");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::controllerEvent(1, 7, 100));
            expect(m.isController());
            expectEquals(m.getChannel(), 14);
            expectEquals(m.getControllerNumber(), 7);
            expectEquals(m.getControllerValue(), 100);
        }

        beginTest("remapChannel - program change on ch16 remapped to 15");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::programChange(16, 42));
            expect(m.isProgramChange());
            expectEquals(m.getChannel(), 15);
            expectEquals(m.getProgramChangeNumber(), 42);
        }

        beginTest("remapChannel - pitch wheel on ch1 remapped, value preserved");
        {
            MidiRecordPlayer p;
            auto m = p.remapChannel(juce::MidiMessage::pitchWheel(1, 12000));
            expect(m.isPitchWheel());
            expectEquals(m.getChannel(), 14);
            expectEquals(m.getPitchWheelValue(), 12000);
        }

        beginTest("remapChannel - an unhandled message type is returned unchanged (channel kept)");
        {
            MidiRecordPlayer p;
            // channel pressure is none of the remapped types -> returned as-is, channel NOT remapped
            auto m = p.remapChannel(juce::MidiMessage::channelPressureChange(1, 90));
            expect(m.isChannelPressure());
            expectEquals(m.getChannel(), 1);
        }

        beginTest("defaults - program numbers 0 and no recorded events");
        {
            MidiRecordPlayer p;
            expectEquals(p.getProgramLeftHand(), 0);
            expectEquals(p.getProgramRightHand(), 0);
            expectEquals(p.getSizeRecorded(), 0);
        }

        beginTest("state machine - start arms + clears; stop guard returns false/true");
        {
            MidiRecordPlayer p;
            p.applyPresetFunction = [] {};                 // startRecording invokes this; stub it
            p.getAllRecordedEvents().push_back(RecordedEvent{ juce::MidiMessage::noteOn(1,60,(juce::uint8)1), 0.0 });

            expect(p.stopRecording() == false);            // not recording yet -> guard returns false
            p.startRecording();
            expect(p.getIsRecording());
            expectEquals(p.getSizeRecorded(), 0);          // startRecording cleared the buffer
            expect(p.stopRecording() == true);             // was recording -> true
            expect(! p.getIsRecording());
        }

        beginTest("capture - handleIncomingMessage records (remapped) only while recording");
        {
            MidiRecordPlayer p;
            p.applyPresetFunction = [] {};

            p.handleIncomingMessage(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));  // not recording
            expectEquals(p.getSizeRecorded(), 0);          // ignored

            p.startRecording();
            p.handleIncomingMessage(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));  // ch1 -> remapped to 14
            expectEquals(p.getSizeRecorded(), 1);
            const auto& ev = p.getAllRecordedEvents().back();
            expect(ev.message.isNoteOn());
            expectEquals(ev.message.getChannel(), 14);     // remapChannel applied on capture
            expectEquals(ev.message.getNoteNumber(), 60);
            expect(ev.timeFromStart >= 0.0);
            p.stopRecording();
        }

        beginTest("serialization round-trips through in-memory streams (no disk)");
        {
            MidiRecordPlayer p;
            // Populate the in-memory buffer directly (skip the timer/wall-clock recording path).
            auto& evs = p.getAllRecordedEvents();
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOn (2, 60, (juce::uint8)100), 0.0 });
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOn (2, 64, (juce::uint8)100), 0.5 });
            evs.push_back(RecordedEvent{ juce::MidiMessage::noteOff(2, 60),                  1.0 });

            juce::MemoryOutputStream out;
            juce::String err;
            expect(p.writeRecordingToStream(out, err, 120.0), err);

            juce::MemoryInputStream in(out.getData(), out.getDataSize(), false);
            expect(p.readRecordingFromStream(in, err), err);

            int noteOns = 0, noteOffs = 0; bool chOk = true; bool has60 = false, has64 = false;
            for (const auto& e : p.getParsedEvents())
            {
                if (e.message.isNoteOn())  { ++noteOns;  if (e.message.getChannel() != 2) chOk = false;
                                             if (e.message.getNoteNumber() == 60) has60 = true;
                                             if (e.message.getNoteNumber() == 64) has64 = true; }
                else if (e.message.isNoteOff()) { ++noteOffs; if (e.message.getChannel() != 2) chOk = false; }
            }
            expectEquals(noteOns, 2);
            expect(noteOffs >= 1);
            expect(chOk, "a parsed note was on the wrong channel");
            expect(has60 && has64, "note numbers did not survive the round-trip");
        }

        beginTest("readRecordingFromStream - garbage input fails cleanly with an error message");
        {
            MidiRecordPlayer p;
            const char junk[] = "not a midi file at all";
            juce::MemoryInputStream in(junk, sizeof(junk), false);
            juce::String err;
            expect(! p.readRecordingFromStream(in, err));   // returns false
            expect(err.isNotEmpty());                        // and sets an error message
        }
    }
};
static MidiRecordPlayerTest midiRecordPlayerTest;
