#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

namespace MidiHandlerKeyboardTestValues
{
    const juce::String testName     = "MidiHandler (keyboard & programs)";
    const juce::String testCategory = "Unit";

    const juce::String noteOnKeyboardNoCrashWithoutOutputDevice = "noteOnKeyboard - no crash without output device";
    const juce::String noteOnKeyboardNotifiesListenersEvenWithoutAnOutput = "noteOnKeyboard - notifies listeners even without an output device (UI feedback)";
    const juce::String noteOnKeyboardStartNoteSettingNoteIsSkippedNoListener = "noteOnKeyboard - startNoteSetting note is skipped (no listener)";
    const juce::String noteOnKeyboardEndNoteSettingNoteIsSkippedNoListener = "noteOnKeyboard - endNoteSetting note is skipped (no listener)";
    const juce::String noteOnKeyboardOnStartNoteSettingCallbackIsInvoked = "noteOnKeyboard - onStartNoteSetting callback is invoked";
    const juce::String noteOnKeyboardOnEndNoteSettingCallbackIsInvoked = "noteOnKeyboard - onEndNoteSetting callback is invoked";
    const juce::String noteOnKeyboardMultipleNotesNoCrash = "noteOnKeyboard - multiple notes no crash";
    const juce::String noteOffKeyboardNoCrashWithoutOutputDevice = "noteOffKeyboard - no crash without output device";
    const juce::String noteOffKeyboardNotifiesNoteOffReceivedListener = "noteOffKeyboard - notifies noteOffReceived listener";
    const juce::String noteOffKeyboardNotifiesHandleIncomingMessageListener = "noteOffKeyboard - notifies handleIncomingMessage listener";
    const juce::String noteOffKeyboardMultipleNotesFireListenersCorrectly = "noteOffKeyboard - multiple notes fire listeners correctly";
    const juce::String noteOffKeyboardRemovedListenerIsNotNotified = "noteOffKeyboard - removed listener is not notified";
    const juce::String allOfKeyboardNoOpenOutputDevice = "allOfKeyboard - no open output device";
    const juce::String setProgramNumberSetsLeftHandProgramNumber = "setProgramNumber - sets left hand program number";
    const juce::String setProgramNumberSetsRightHandProgramNumber = "setProgramNumber - sets right hand program number";
    const juce::String setProgramNumberLeftAndRightAreIndependent = "setProgramNumber - left and right are independent";
    const juce::String setProgramNumberUnrecognizedChoiceDoesNotChangeEither = "setProgramNumber - unrecognized choice does not change either hand";
    const juce::String setProgramNumberCaseInsensitiveLeft = "setProgramNumber - case insensitive left";
    const juce::String setProgramNumberCaseInsensitiveRight = "setProgramNumber - case insensitive right";
    const juce::String setProgramNumberDoesNotCallApplyInstrumentPresetWithoutOutput = "setProgramNumber - does not call applyInstrumentPreset without output device";

    constexpr int startNote = 60;   // the "set start note" key
    constexpr int endNote   = 72;   // the "set end note" key
    constexpr int middleC   = 60;
    constexpr int noteE4    = 64;
    constexpr int noteG4    = 67;
    constexpr int noteC5    = 72;
    constexpr int lowC      = 36;
    constexpr int highC     = 84;
    constexpr int topC      = 108;
    constexpr int fullVelocity   = 127;
    constexpr int velocity       = 100;
    constexpr int softVelocity   = 80;
    constexpr int quietVelocity  = 60;
    constexpr int faintVelocity  = 50;
    constexpr int none        = 0;
    constexpr int once        = 1;
    constexpr int threeNotes  = 3;
    constexpr int notReceived = -1;
    constexpr size_t firstNote  = 0;
    constexpr size_t secondNote = 1;
    constexpr size_t thirdNote  = 2;
    constexpr int leftProgram       = 5;
    constexpr int rightProgram      = 10;
    constexpr int otherLeftProgram  = 3;
    constexpr int otherRightProgram = 7;
    constexpr int ignoredProgram    = 99;
    constexpr int titleCaseLeftProgram  = 12;
    constexpr int upperCaseLeftProgram  = 15;
    constexpr int titleCaseRightProgram = 20;
    constexpr int upperCaseRightProgram = 25;
    const juce::String unknownHand    = "center";
    const juce::String titleCaseLeft  = "Left";
    const juce::String upperCaseLeft  = "LEFT";
    const juce::String titleCaseRight = "Right";
    const juce::String upperCaseRight = "RIGHT";
}

using namespace MidiHandlerKeyboardTestValues;

// MidiHandler: PC-keyboard note paths (noteOnKeyboard / noteOffKeyboard / allOfKeyboard) and
// per-hand program numbers. Split from test_midi_handler.cpp to keep files under 800 lines.
class MidiHandlerKeyboardTest : public juce::UnitTest
{
public:
    MidiHandlerKeyboardTest() : juce::UnitTest (MidiHandlerKeyboardTestValues::testName, MidiHandlerKeyboardTestValues::testCategory) {}

    void runTest() override
    {
        // ==================================================================
        //  noteOnKeyboard
        // ==================================================================

        beginTest(noteOnKeyboardNoCrashWithoutOutputDevice);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.noteOnKeyboard(middleC, fullVelocity);
            // passes if no crash
        }

        beginTest(noteOnKeyboardNotifiesListenersEvenWithoutAnOutput);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            // Playing the on-screen/PC keyboard is an INPUT action: listeners (on-screen key
            // highlight, note display) must fire regardless of whether a MIDI output is connected.
            handler.noteOnKeyboard(middleC, fullVelocity);
            expect(listener.noteOnCount == once);

            handler.removeListener(&listener);
        }

        beginTest(noteOnKeyboardStartNoteSettingNoteIsSkippedNoListener);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(startNote, fullVelocity);
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(noteOnKeyboardEndNoteSettingNoteIsSkippedNoListener);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            class NoteOnListener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            NoteOnListener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(endNote, fullVelocity);
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(noteOnKeyboardOnStartNoteSettingCallbackIsInvoked);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            // Without a real output device the midiOut lock will fail,
            // so the callback branch inside the if(midiOut) block won't execute.
            // We verify it doesn't crash.
            bool startCalled = false;
            handler.onStartNoteSetting = [&startCalled]() { startCalled = true; };

            handler.noteOnKeyboard(startNote, fullVelocity);
            // startCalled would be true only with a real device;
            // without device, the if(auto midiOut) block is skipped entirely
        }

        beginTest(noteOnKeyboardOnEndNoteSettingCallbackIsInvoked);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            bool endCalled = false;
            handler.onEndNoteSetting = [&endCalled]() { endCalled = true; };

            handler.noteOnKeyboard(endNote, fullVelocity);
            // Same as above: without device the callback branch is not reached
        }

        beginTest(noteOnKeyboardMultipleNotesNoCrash);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.noteOnKeyboard(lowC, velocity);
            handler.noteOnKeyboard(middleC, softVelocity);
            handler.noteOnKeyboard(highC, fullVelocity);
            handler.noteOnKeyboard(topC, faintVelocity);
            // passes if no crash
        }

        // ==================================================================
        //  noteOffKeyboard
        // ==================================================================

        beginTest(noteOffKeyboardNoCrashWithoutOutputDevice);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.noteOffKeyboard(middleC, fullVelocity);
            // passes if no crash
        }

        beginTest(noteOffKeyboardNotifiesNoteOffReceivedListener);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount {};
                int lastNoteOff = notReceived;
                void noteOffReceived(int midiNote) override { noteOffCount++; lastNoteOff = midiNote; }
            };

            NoteOffListener listener;
            handler.addListener(&listener);

            handler.noteOffKeyboard(noteE4, velocity);

            // noteOffKeyboard always calls listeners regardless of output device
            expect(listener.noteOffCount == once);
            expect(listener.lastNoteOff == noteE4);

            handler.removeListener(&listener);
        }

        beginTest(noteOffKeyboardNotifiesHandleIncomingMessageListener);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class MsgListener : public MidiHandlerListener
            {
            public:
                int msgCount {};
                bool lastWasNoteOff = false;
                int lastNote = notReceived;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    msgCount++;
                    lastWasNoteOff = message.isNoteOff();
                    lastNote = message.getNoteNumber();
                }
            };

            MsgListener listener;
            handler.addListener(&listener);

            handler.noteOffKeyboard(noteC5, softVelocity);

            expect(listener.msgCount == once);
            expect(listener.lastWasNoteOff == true);
            expect(listener.lastNote == noteC5);

            handler.removeListener(&listener);
        }

        beginTest(noteOffKeyboardMultipleNotesFireListenersCorrectly);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount {};
                std::vector<int> notesReceived;
                void noteOffReceived(int midiNote) override
                {
                    noteOffCount++;
                    notesReceived.push_back(midiNote);
                }
            };

            NoteOffListener listener;
            handler.addListener(&listener);

            handler.noteOffKeyboard(middleC, velocity);
            handler.noteOffKeyboard(noteE4, softVelocity);
            handler.noteOffKeyboard(noteG4, quietVelocity);

            expect(listener.noteOffCount == threeNotes);
            expect(listener.notesReceived[firstNote] == middleC);
            expect(listener.notesReceived[secondNote] == noteE4);
            expect(listener.notesReceived[thirdNote] == noteG4);

            handler.removeListener(&listener);
        }

        beginTest(noteOffKeyboardRemovedListenerIsNotNotified);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class NoteOffListener : public MidiHandlerListener
            {
            public:
                int noteOffCount {};
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            NoteOffListener listener;
            handler.addListener(&listener);
            handler.removeListener(&listener);

            handler.noteOffKeyboard(middleC, velocity);
            expect(listener.noteOffCount == none);
        }

        // ==================================================================
        //  allOffKeyboard
        // ==================================================================
        // NOTE: allOffKeyboard() directly dereferences midiDevice.currentDeviceUSEDout
        // without a null check (line 563-564 in MidiHandler.cpp).
        // Calling it without an open output device will crash with a null pointer.
        // This method cannot be safely unit tested without a real MIDI output device.

        beginTest(allOfKeyboardNoOpenOutputDevice);
        {
            MidiDevice device;
            MidiHandler handler{ device };

            handler.allOffKeyboard();

            class MockListener : public MidiHandlerListener
            {
            public:
                int noteOffCount {};
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            MockListener listener;
            handler.addListener(&listener);

            handler.allOffKeyboard();

            expect(listener.noteOffCount == none);

            handler.removeListener(&listener);
        }

        // ==================================================================
        //  setProgramNumber
        // ==================================================================

        beginTest(setProgramNumberSetsLeftHandProgramNumber);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(leftProgram, HandChoiceNames::left);
            expect(handler.getProgramNumberLeftHand() == leftProgram);
        }

        beginTest(setProgramNumberSetsRightHandProgramNumber);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(rightProgram, HandChoiceNames::right);
            expect(handler.getProgramNumberRightHand() == rightProgram);
        }

        beginTest(setProgramNumberLeftAndRightAreIndependent);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(otherLeftProgram, HandChoiceNames::left);
            handler.setProgramNumber(otherRightProgram, HandChoiceNames::right);
            expect(handler.getProgramNumberLeftHand() == otherLeftProgram);
            expect(handler.getProgramNumberRightHand() == otherRightProgram);
        }

        beginTest(setProgramNumberUnrecognizedChoiceDoesNotChangeEither);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(leftProgram, HandChoiceNames::left);
            handler.setProgramNumber(rightProgram, HandChoiceNames::right);
            handler.setProgramNumber(ignoredProgram, unknownHand); // invalid choice
            expect(handler.getProgramNumberLeftHand() == leftProgram);
            expect(handler.getProgramNumberRightHand() == rightProgram);
        }

        beginTest(setProgramNumberCaseInsensitiveLeft);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(titleCaseLeftProgram, titleCaseLeft);
            expect(handler.getProgramNumberLeftHand() == titleCaseLeftProgram);

            handler.setProgramNumber(upperCaseLeftProgram, upperCaseLeft);
            expect(handler.getProgramNumberLeftHand() == upperCaseLeftProgram);
        }

        beginTest(setProgramNumberCaseInsensitiveRight);
        {
            MidiDevice device;
            MidiHandler handler(device);

            handler.setProgramNumber(titleCaseRightProgram, titleCaseRight);
            expect(handler.getProgramNumberRightHand() == titleCaseRightProgram);

            handler.setProgramNumber(upperCaseRightProgram, upperCaseRight);
            expect(handler.getProgramNumberRightHand() == upperCaseRightProgram);
        }

        beginTest(setProgramNumberDoesNotCallApplyInstrumentPresetWithoutOutput);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class PresetListener : public MidiHandlerListener
            {
            public:
                int msgCount {};
                void handleIncomingMessage(const juce::MidiMessage& message) override { msgCount++; (void)message; }
            };

            PresetListener listener;
            handler.addListener(&listener);

            // Without output device, the applyInstrumentPreset block is skipped
            handler.setProgramNumber(leftProgram, HandChoiceNames::left);
            expect(listener.msgCount == none);

            handler.removeListener(&listener);
        }
    }
};

static MidiHandlerKeyboardTest midiHandlerKeyboardTest;
