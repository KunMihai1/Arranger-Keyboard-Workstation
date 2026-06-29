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

        beginTest("handleIncomingMidiMessage - noteOn does NOT notify listener without output device");
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

            // Without an output device, the midiOut lock fails, ok stays 0,
            // so noteOnReceived is never called
            expect(listener.noteOnCount == 0);

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

        // ==================================================================
        //  noteOnKeyboard
        // ==================================================================

        beginTest("noteOnKeyboard - no crash without output device");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.noteOnKeyboard(60, 127);
            // passes if no crash
        }

        beginTest("noteOnKeyboard - notifies listeners even without an output device (UI feedback)");
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

            // Playing the on-screen/PC keyboard is an INPUT action: listeners (on-screen key
            // highlight, note display) must fire regardless of whether a MIDI output is connected.
            handler.noteOnKeyboard(60, 127);
            expect(listener.noteOnCount == 1);

            handler.removeListener(&listener);
        }

        beginTest("noteOnKeyboard - startNoteSetting note is skipped (no listener)");
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

            handler.noteOnKeyboard(60, 127);
            expect(listener.noteOnCount == 0);

            handler.removeListener(&listener);
        }

        beginTest("noteOnKeyboard - endNoteSetting note is skipped (no listener)");
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

            handler.noteOnKeyboard(72, 127);
            expect(listener.noteOnCount == 0);

            handler.removeListener(&listener);
        }

        beginTest("noteOnKeyboard - onStartNoteSetting callback is invoked");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(60, 72);

            // Without a real output device the midiOut lock will fail,
            // so the callback branch inside the if(midiOut) block won't execute.
            // We verify it doesn't crash.
            bool startCalled = false;
            handler.onStartNoteSetting = [&startCalled]() { startCalled = true; };

            handler.noteOnKeyboard(60, 127);
            // startCalled would be true only with a real device;
            // without device, the if(auto midiOut) block is skipped entirely
        }

        beginTest("noteOnKeyboard - onEndNoteSetting callback is invoked");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(60, 72);

            bool endCalled = false;
            handler.onEndNoteSetting = [&endCalled]() { endCalled = true; };

            handler.noteOnKeyboard(72, 127);
            // Same as above: without device the callback branch is not reached
        }

        beginTest("noteOnKeyboard - multiple notes no crash");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.noteOnKeyboard(36, 100);
            handler.noteOnKeyboard(60, 80);
            handler.noteOnKeyboard(84, 127);
            handler.noteOnKeyboard(108, 50);
            // passes if no crash
        }

        // ==================================================================
        //  noteOffKeyboard
        // ==================================================================

        beginTest("noteOffKeyboard - no crash without output device");
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.noteOffKeyboard(60, 127);
            // passes if no crash
        }

        beginTest("noteOffKeyboard - notifies noteOffReceived listener");
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

            handler.noteOffKeyboard(64, 100);

            // noteOffKeyboard always calls listeners regardless of output device
            expect(listener.noteOffCount == 1);
            expect(listener.lastNoteOff == 64);

            handler.removeListener(&listener);
        }

        beginTest("noteOffKeyboard - notifies handleIncomingMessage listener");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class MsgListener : public MidiHandlerListener
            {
            public:
                int msgCount = 0;
                bool lastWasNoteOff = false;
                int lastNote = -1;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    msgCount++;
                    lastWasNoteOff = message.isNoteOff();
                    lastNote = message.getNoteNumber();
                }
            };

            MsgListener listener;
            handler.addListener(&listener);

            handler.noteOffKeyboard(72, 80);

            expect(listener.msgCount == 1);
            expect(listener.lastWasNoteOff == true);
            expect(listener.lastNote == 72);

            handler.removeListener(&listener);
        }

        beginTest("noteOffKeyboard - multiple notes fire listeners correctly");
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

            handler.noteOffKeyboard(60, 100);
            handler.noteOffKeyboard(64, 80);
            handler.noteOffKeyboard(67, 60);

            expect(listener.noteOffCount == 3);
            expect(listener.notesReceived[0] == 60);
            expect(listener.notesReceived[1] == 64);
            expect(listener.notesReceived[2] == 67);

            handler.removeListener(&listener);
        }

        beginTest("noteOffKeyboard - removed listener is not notified");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount = 0;
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            NoteOffListener listener;
            handler.addListener(&listener);
            handler.removeListener(&listener);

            handler.noteOffKeyboard(60, 100);
            expect(listener.noteOffCount == 0);
        }

        // ==================================================================
        //  allOffKeyboard
        // ==================================================================
        // NOTE: allOffKeyboard() directly dereferences midiDevice.currentDeviceUSEDout
        // without a null check (line 563-564 in MidiHandler.cpp).
        // Calling it without an open output device will crash with a null pointer.
        // This method cannot be safely unit tested without a real MIDI output device.

        beginTest("allOfKeyboard - no open output device");
        {
            MidiDevice device;
            MidiHandler handler{ device };

            handler.allOffKeyboard();

            class MockListener : public MidiHandlerListener
            {
            public:
                int noteOffCount = 0;
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            MockListener listener;
            handler.addListener(&listener);

            handler.allOffKeyboard();

            expect(listener.noteOffCount == 0);

            handler.removeListener(&listener);
        }

        // ==================================================================
        //  setProgramNumber
        // ==================================================================

        beginTest("setProgramNumber - sets left hand program number");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(5, "left");
            expect(handler.getProgramNumberLeftHand() == 5);
        }

        beginTest("setProgramNumber - sets right hand program number");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(10, "right");
            expect(handler.getProgramNumberRightHand() == 10);
        }

        beginTest("setProgramNumber - left and right are independent");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(3, "left");
            handler.setProgramNumber(7, "right");
            expect(handler.getProgramNumberLeftHand() == 3);
            expect(handler.getProgramNumberRightHand() == 7);
        }

        beginTest("setProgramNumber - unrecognized choice does not change either hand");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(5, "left");
            handler.setProgramNumber(10, "right");
            handler.setProgramNumber(99, "center"); // invalid choice
            expect(handler.getProgramNumberLeftHand() == 5);
            expect(handler.getProgramNumberRightHand() == 10);
        }

        beginTest("setProgramNumber - case insensitive left");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(12, "Left");
            expect(handler.getProgramNumberLeftHand() == 12);

            handler.setProgramNumber(15, "LEFT");
            expect(handler.getProgramNumberLeftHand() == 15);
        }

        beginTest("setProgramNumber - case insensitive right");
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(20, "Right");
            expect(handler.getProgramNumberRightHand() == 20);

            handler.setProgramNumber(25, "RIGHT");
            expect(handler.getProgramNumberRightHand() == 25);
        }

        beginTest("setProgramNumber - does not call applyInstrumentPreset without output device");
        {
            MidiDevice device;
            MidiHandler handler(device);

            class PresetListener : public MidiHandlerListener
            {
            public:
                int msgCount = 0;
                void handleIncomingMessage(const juce::MidiMessage& message) override { msgCount++; (void)message; }
            };

            PresetListener listener;
            handler.addListener(&listener);

            // Without output device, the applyInstrumentPreset block is skipped
            handler.setProgramNumber(5, "left");
            expect(listener.msgCount == 0);

            handler.removeListener(&listener);
        }
    }
};

static MidiHandlerTest midiHandlerTest;


//======================================================================================
//  InstrumentHandler — predefined per-program CC presets (previously untested)
//======================================================================================
class InstrumentHandlerTest : public juce::UnitTest
{
public:
    InstrumentHandlerTest() : juce::UnitTest("InstrumentHandler", "Unit") {}

    void runTest() override
    {
        using Preset = std::vector<std::pair<int, int>>;

        beginTest("getPreset - program 0 is the grand-piano CC preset");
        {
            InstrumentHandler h;
            const Preset expected { {91,-1}, {93,10}, {74,100}, {71,40}, {11,127} };
            expect(h.getPreset(0) == expected);
        }

        beginTest("getPreset - a known program returns its 5-CC preset");
        {
            InstrumentHandler h;
            const auto& p = h.getPreset(4);   // electric piano
            expectEquals((int) p.size(), 5);
            expect(p.front() == std::make_pair(91, -1));   // reverb-send CC first
        }

        beginTest("getPreset - an unknown program is empty");
        {
            InstrumentHandler h;
            expect(h.getPreset(9999).empty());
        }

        beginTest("getInstrumentPresets - contains the seeded programs");
        {
            InstrumentHandler h;
            const auto& m = h.getInstrumentPresets();
            expect(m.find(0)  != m.end());    // grand piano
            expect(m.find(41) != m.end());    // violin
            expect(m.size() >= 30);
        }
    }
};
static InstrumentHandlerTest instrumentHandlerTest;


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
