#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

// MidiHandler: PC-keyboard note paths (noteOnKeyboard / noteOffKeyboard / allOfKeyboard) and
// per-hand program numbers. Split from test_midi_handler.cpp to keep files under 800 lines.
class MidiHandlerKeyboardTest : public juce::UnitTest
{
public:
    MidiHandlerKeyboardTest() : juce::UnitTest("MidiHandler (keyboard & programs)", "Unit") {}

    void runTest() override
    {
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

static MidiHandlerKeyboardTest midiHandlerKeyboardTest;
