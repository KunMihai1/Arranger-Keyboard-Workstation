#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

class MidiHandlerTest : public juce::UnitTest
{
public:
    MidiHandlerTest() : juce::UnitTest("MidiHandler", "Unit") {}

    void runTest() override
    {
        beginTest("re-pressing the same chord re-sends it (not suppressed as a duplicate)");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(0, 100);   // notes < 100 are in the chord (left) zone

            int validSends = 0;
            handler.onChordChanged = [&](const ArrangerChord& c) { if (c.isValid()) ++validSends; };

            // Press C major -> one valid send.
            handler.noteOnKeyboard(60, 100); handler.noteOnKeyboard(64, 100); handler.noteOnKeyboard(67, 100);
            expect(validSends >= 1);
            const int afterFirst = validSends;

            // Release everything (chord memory holds; no send), then press the SAME C major again.
            handler.noteOffKeyboard(60, 0); handler.noteOffKeyboard(64, 0); handler.noteOffKeyboard(67, 0);
            handler.noteOnKeyboard(60, 100); handler.noteOnKeyboard(64, 100); handler.noteOnKeyboard(67, 100);

            // Must be re-sent — the engine may have reset to its home key between presses (e.g. a 2nd Play).
            expect(validSends > afterFirst);
        }

        // ---- heldChord (Phase 5 / held-chord seed) ----

        beginTest("heldChord - valid only while notes are physically held (not from Chord Memory)");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(0, 100);   // chord zone = notes < 100
            handler.setChordMemory(true);            // memory ON: current() remembers after release

            expect(! handler.heldChord().isValid());                 // nothing held yet

            handler.noteOnKeyboard(60, 100); handler.noteOnKeyboard(64, 100); handler.noteOnKeyboard(67, 100); // C major
            expect(handler.heldChord().isValid());
            expectEquals(handler.heldChord().root, 0);               // C

            handler.noteOffKeyboard(60, 0); handler.noteOffKeyboard(64, 0); handler.noteOffKeyboard(67, 0);
            // Chord Memory still *remembers* C major, but nothing is physically held -> heldChord() invalid.
            expect(! handler.heldChord().isValid());
        }

        // ---- setChordMode (Phase 5) ----

        beginTest("setChordMode - Single-Finger recognizes Korg one-finger chords");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(0, 100);
            handler.setChordMode(ChordMode::SingleFinger);

            ArrangerChord last;
            handler.onChordChanged = [&](const ArrangerChord& c) { last = c; };

            handler.noteOnKeyboard(60, 100);   // C alone -> C major
            expect(last.isValid()); expectEquals(last.root, 0);
            expect(last.quality == ChordQuality::Maj);

            handler.noteOnKeyboard(58, 100);   // + Bb (black key to the left) -> C minor
            expect(last.quality == ChordQuality::Min);
        }

        beginTest("setChordMode - Full-Keyboard scans notes above the split point");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(0, 50);   // split at 50: in Fingered, notes >=50 are out of zone
            handler.setChordMode(ChordMode::FullKeyboard);

            int valids = 0;
            handler.onChordChanged = [&](const ArrangerChord& c) { if (c.isValid()) ++valids; };

            // 60/64/67 are ABOVE the split (would be ignored in Fingered) -> Full-Keyboard still reads them.
            handler.noteOnKeyboard(60, 100); handler.noteOnKeyboard(64, 100); handler.noteOnKeyboard(67, 100);
            expect(valids >= 1);
        }

        // ---- setPlayableRange ----

        beginTest("setPlayableRange - 25 keys");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(25);
            expect(device.get_minNote() == 36);
            expect(device.get_maxNote() == 60);
        }

        beginTest("setPlayableRange - 49 keys");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(49);
            expect(device.get_minNote() == 40);
            expect(device.get_maxNote() == 88);
        }

        beginTest("setPlayableRange - 61 keys");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(61);
            expect(device.get_minNote() == 36);
            expect(device.get_maxNote() == 96);
        }

        beginTest("setPlayableRange - 88 keys");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(88);
            expect(device.get_minNote() == 21);
            expect(device.get_maxNote() == 108);
        }

        beginTest("setPlayableRange - unrecognized key count leaves defaults");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(37);  
            
            expect(device.get_minNote() == 0);
            expect(device.get_maxNote() == 127);
        }

        // ---- set_start_end_notes ----

        beginTest("set_start_end_notes - stores values");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(36, 96);
            // verified indirectly: no crash, internal state set
            // further verified through playBackSettingsChanged below
        }

        // ---- set_left_right_bounds ----

        beginTest("set_left_right_bounds - stores values");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(55, 72);
            // verified indirectly through setCorrectChannelBasedOnHand below
        }

        // ---- channelForHand ----

        beginTest("channelForHand - note below right bound uses channel 1");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(55, 60);

            // note 59 is below rightHandBound (60), so channel should be 1
            expectEquals(handler.channelForHand(59), 1);
        }

        beginTest("channelForHand - note at right bound uses channel 16");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(55, 60);
            expectEquals(handler.channelForHand(60), 16);   // note >= rightHandBound
        }

        beginTest("channelForHand - note above right bound uses channel 16");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(55, 60);
            expectEquals(handler.channelForHand(80), 16);
        }

        beginTest("channelForHand - no bounds set defaults to channel 1");
        {
            MidiDevice device;
            MidiHandler handler(device);
            // rightHandBoundSetting is -1 by default => first condition false => channel 1
            expectEquals(handler.channelForHand(60), 1);
        }

        // ---- Program Numbers ----

        beginTest("getProgramNumberLeftHand - default is 0");
        {
            MidiDevice device;
            MidiHandler handler(device);
            expect(handler.getProgramNumberLeftHand() == 0);
        }

        beginTest("getProgramNumberRightHand - default is 0");
        {
            MidiDevice device;
            MidiHandler handler(device);
            expect(handler.getProgramNumberRightHand() == 0);
        }

        // ---- playBackSettingsChanged ----

        beginTest("playBackSettingsChanged - applies all settings");
        {
            MidiDevice device;
            MidiHandler handler(device);

            PlayBackSettings settings;
            settings.startNote = 36;
            settings.endNote = 96;
            settings.leftHandBound = 55;
            settings.rightHandBound = 72;

            handler.playBackSettingsChanged(settings);
            // internally calls set_start_end_notes and set_left_right_bounds
            // verify channel routing works with the new bounds
            expectEquals(handler.channelForHand(71), 1);   // below 72 => ch 1
            expectEquals(handler.channelForHand(72), 16);  // at 72 => ch 16
        }

        // ---- handlePlayableRange ----

        beginTest("handlePlayableRange - known key count with positive nrKeys");
        {
            MidiDevice device;
            MidiHandler handler(device);
            int result = handler.handlePlayableRange("07cf", "6803", 61, false);
            expect(result == 1);
            expect(device.get_minNote() == 36);
            expect(device.get_maxNote() == 96);
        }

        beginTest("handlePlayableRange - keyboard input mode");
        {
            MidiDevice device;
            MidiHandler handler(device);
            int result = handler.handlePlayableRange("07cf", "6803", 49, true);
            expect(result == 1);
            expect(device.get_minNote() == 40);
            expect(device.get_maxNote() == 88);
        }

        beginTest("handlePlayableRange - negative nrKeys with keyboard input still sets range");
        {
            MidiDevice device;
            MidiHandler handler(device);
            // isKeyboardInput = true, so it goes through setPlayableRange(-5)
            // setPlayableRange doesn't match any known size, so defaults remain
            int result = handler.handlePlayableRange("07cf", "6803", -5, true);
            expect(result == 1);
        }

        // ---- Listener add/remove ----

        beginTest("addListener and removeListener - no crash");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            class MockListener : public MidiHandlerListener
            {
            public:
                int noteOnCount = 0;
                int noteOffCount = 0;
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            MockListener listener;
            handler.addListener(&listener);
            handler.removeListener(&listener);
            // no crash = pass
        }

        // ---- getNextMidiBlock ----

        beginTest("getNextMidiBlock - empty buffer when no messages");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 0);
        }

        // ==================================================================
        //  handleIncomingMidiMessage
        // ==================================================================

        beginTest("handleIncomingMidiMessage - noteOn message is buffered");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            handler.handleIncomingMidiMessage(nullptr, noteOn);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);
        }

        beginTest("handleIncomingMidiMessage - noteOff message is buffered");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto noteOff = juce::MidiMessage::noteOff(1, 60, (juce::uint8)0);
            handler.handleIncomingMidiMessage(nullptr, noteOff);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);
        }

        beginTest("handleIncomingMidiMessage - non-note message is buffered");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto cc = juce::MidiMessage::controllerEvent(1, 64, 127);
            handler.handleIncomingMidiMessage(nullptr, cc);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);
        }

        beginTest("handleIncomingMidiMessage - multiple messages buffered then cleared");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 60, (juce::uint8)0));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 64, (juce::uint8)80));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 3);

            // After getNextMidiBlock the internal buffer should be cleared
            juce::MidiBuffer buffer2;
            handler.getNextMidiBlock(buffer2, 0, 512);
            expect(buffer2.getNumEvents() == 0);
        }

        // ---- chord-zone mute must apply to the INTERNAL synth feed too (physical-MIDI path) ----
        // Regression: physical-keyboard left-zone notes were sounding on the internal SFZ synth
        // even with chord-zone mute on, because handleIncomingMidiMessage enqueued the raw note
        // into incomingMidiMessages unconditionally (the PC-keyboard path already gated it).

        beginTest("handleIncomingMidiMessage - muted chord-zone note-ON is NOT fed to the internal synth");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(0, 100);   // notes < 100 are in the chord (left) zone
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            // A physical-keyboard left-zone note: drives the chord, must NOT sound on the internal synth.
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 48, (juce::uint8)100));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 0);   // muted onset never reaches the SFZ feed
        }

        beginTest("handleIncomingMidiMessage - chord-zone note-ON IS fed to the internal synth when mute is off");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(0, 100);
            handler.setChordZoneMute(false);        // normal playing: the note must sound
            handler.setArrangerEngaged(true);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 48, (juce::uint8)100));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);
        }

        beginTest("handleIncomingMidiMessage - muted chord-zone note-OFF still passes (no stuck notes)");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(0, 100);
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            // Onset is muted, but the release must always reach the synth so nothing can hang.
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 48, (juce::uint8)0));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);
        }

        beginTest("handleIncomingMidiMessage - right-hand note still sounds while chord zone is muted");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(0, 100);   // split at 100; notes >= 100 are the right (melody) zone
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 108, (juce::uint8)100));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, 0, 512);
            expect(buffer.getNumEvents() == 1);   // only the left zone is muted
        }

        beginTest("handleIncomingMidiMessage - noteOff notifies noteOffReceived listener");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount = 0;
                int lastNoteOff = -1;
                void noteOffReceived(int midiNote) override { noteOffCount++; lastNoteOff = midiNote; }
            };

            NoteOffListener listener;
            handler.addListener(&listener);

            auto noteOff = juce::MidiMessage::noteOff(1, 64, (juce::uint8)0);
            handler.handleIncomingMidiMessage(nullptr, noteOff);

            expect(listener.noteOffCount == 1);
            expect(listener.lastNoteOff == 64);

            handler.removeListener(&listener);
        }

        beginTest("handleIncomingMidiMessage - noteOff notifies handleIncomingMessage listener");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class MsgListener : public MidiHandlerListener
            {
            public:
                int msgCount = 0;
                bool lastWasNoteOff = false;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    msgCount++;
                    lastWasNoteOff = message.isNoteOff();
                }
            };

            MsgListener listener;
            handler.addListener(&listener);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 72, (juce::uint8)0));

            expect(listener.msgCount == 1);
            expect(listener.lastWasNoteOff == true);

            handler.removeListener(&listener);
        }

        beginTest("handleIncomingMidiMessage - noteOn notifies listener even without an output device");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount = 0;
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            handler.handleIncomingMidiMessage(nullptr, noteOn);

            // Playability is decided from the note range FIRST, independent of any external
            // MIDI-OUT device (5c75d4f). startNoteSetting/endNoteSetting default to -1, so note 60
            // sets ok = 1 and the listener is notified even with no device open -- this is what
            // makes the internal SFZ sound from a physical keyboard. Gating this on the midiOut
            // lock was the bug; asserting 0 here would re-assert that bug.
            expect(listener.noteOnCount == 1);

            handler.removeListener(&listener);
        }

        beginTest("handleIncomingMidiMessage - startNoteSetting note does not trigger normal noteOn path");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(60, 72);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount = 0;
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            // Note 60 matches startNoteSetting
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));
            expect(listener.noteOnCount == 0);

            handler.removeListener(&listener);
        }

        beginTest("handleIncomingMidiMessage - endNoteSetting note does not trigger normal noteOn path");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(60, 72);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount = 0;
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            // Note 72 matches endNoteSetting
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, 72, (juce::uint8)100));
            expect(listener.noteOnCount == 0);

            handler.removeListener(&listener);
        }

        beginTest("handleIncomingMidiMessage - noteOff fires for multiple notes");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount = 0;
                std::vector<int> notesReceived;
                void noteOffReceived(int midiNote) override
                {
                    noteOffCount++;
                    notesReceived.push_back(midiNote);
                }
            };

            NoteOffListener listener;
            handler.addListener(&listener);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 60, (juce::uint8)0));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 64, (juce::uint8)0));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, 67, (juce::uint8)0));

            expect(listener.noteOffCount == 3);
            expect(listener.notesReceived[0] == 60);
            expect(listener.notesReceived[1] == 64);
            expect(listener.notesReceived[2] == 67);

            handler.removeListener(&listener);
        }
    }
};

static MidiHandlerTest midiHandlerTest;
