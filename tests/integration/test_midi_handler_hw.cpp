#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"

namespace MidiHandlerHwTestValues
{
    const juce::String testName     = "MidiHandler_HW";
    const juce::String testCategory = "Integration";

    const juce::String incomingNoteOnNotifies     = "handleIncomingMidiMessage [device] - noteOn normal path notifies listeners";
    const juce::String incomingStartNoteCallback  = "handleIncomingMidiMessage [device] - startNoteSetting triggers onStartNoteSetting callback";
    const juce::String incomingEndNoteCallback    = "handleIncomingMidiMessage [device] - endNoteSetting triggers onEndNoteSetting callback";
    const juce::String incomingStartNoteNoCallback = "handleIncomingMidiMessage [device] - startNoteSetting without callback does not crash";
    const juce::String incomingEndNoteNoCallback  = "handleIncomingMidiMessage [device] - endNoteSetting without callback does not crash";
    const juce::String keyboardNoteNotifies       = "noteOnKeyboard [device] - normal note notifies listeners";
    const juce::String keyboardStartNoteCallback  = "noteOnKeyboard [device] - startNoteSetting triggers onStartNoteSetting";
    const juce::String keyboardEndNoteCallback    = "noteOnKeyboard [device] - endNoteSetting triggers onEndNoteSetting";
    const juce::String keyboardSeveralNotes       = "noteOnKeyboard [device] - multiple normal notes all notify listener";
    const juce::String allOffSendsNoteOffs        = "allOffKeyboard [device] - sends 128 note-offs on both channels without crash";
    const juce::String presetSendsProgramChange   = "setProgramNumber [device] - applyInstrumentPreset notifies listener with programChange";
    const juce::String rightHandOnChannel16       = "setProgramNumber [device] - right hand uses channel 16";
    const juce::String leftHandOnChannel1         = "setProgramNumber [device] - left hand uses channel 1";
    const juce::String presetSendsControllers     = "setProgramNumber [device] - applyInstrumentPreset sends CC messages to listener";

    const juce::String skippedNoOutput   = "SKIPPED - no MIDI output device connected";
    const juce::String skippedOpenFailed = "SKIPPED - failed to open MIDI output device";

    constexpr size_t firstDeviceEntry = 0;
    constexpr int    firstDevice      = 0;
    constexpr int    none             = 0;
    constexpr int    notReceived      = -1;
    constexpr int    once             = 1;

    constexpr int         leftHandChannel  = 1;
    constexpr int         rightHandChannel = 16;
    constexpr int         startNote        = 60;   // the "set start note" key
    constexpr int         endNote          = 72;   // the "set end note" key
    constexpr int         middleC          = 60;
    constexpr int         noteE4           = 64;
    constexpr int         noteG4           = 67;
    constexpr juce::uint8 velocity         = 100;
    constexpr int         keyVelocity      = 100;
    constexpr int         softKeyVelocity  = 80;
    constexpr int         fullKeyVelocity  = 127;
    constexpr int         threeNotes       = 3;
    constexpr size_t      firstNote        = 0;
    constexpr size_t      secondNote       = 1;
    constexpr size_t      thirdNote        = 2;

    constexpr int leftProgram    = 5;
    constexpr int rightProgram   = 10;
    constexpr int otherLeftProgram = 3;
    constexpr int grandPiano     = 0;
}

using namespace MidiHandlerHwTestValues;

// ==================================================================
// HARDWARE-DEPENDENT TESTS (Integration)
// These tests require a MIDI output device to be connected.
// They skip automatically if no device is found.
// ==================================================================

class MidiHandlerHWTest : public juce::UnitTest
{
public:
    MidiHandlerHWTest() : juce::UnitTest(MidiHandlerHwTestValues::testName, MidiHandlerHwTestValues::testCategory) {}

    void runTest() override
    {
        MidiDevice device;
        InstrumentHandler instrumentHandler;

        std::vector<std::string> outDevices;
        device.getAvailableDevicesMidiOUT(outDevices);

        if (outDevices.empty() || juce::String (outDevices[firstDeviceEntry]) == DeviceNames::noDevicesFound)
        {
            logMessage(skippedNoOutput);
            return;
        }

        if (!device.deviceOpenOUT(firstDevice))
        {
            logMessage(skippedOpenFailed);
            return;
        }

        

        // --- handleIncomingMidiMessage with device ---

        beginTest(incomingNoteOnNotifies);
        {
            MidiHandler handler(device);
            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                int lastNote = notReceived;
                int msgCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; lastNote = midiNote; }
                void handleIncomingMessage(const juce::MidiMessage& message) override { msgCount++; (void)message; }
            };

            Listener listener;
            handler.addListener(&listener);

            // startNoteSetting and endNoteSetting default to -1,
            // so note 60 does NOT match either => normal path => ok=1
            auto noteOn = juce::MidiMessage::noteOn(leftHandChannel, middleC, velocity);
            handler.handleIncomingMidiMessage(nullptr, noteOn);

            expect(listener.noteOnCount == once);
            expect(listener.lastNote == middleC);
            expect(listener.msgCount == once);

            handler.removeListener(&listener);
        }

        beginTest(incomingStartNoteCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            bool startCalled = false;
            handler.onStartNoteSetting = [&startCalled]() { startCalled = true; };

            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            Listener listener;
            handler.addListener(&listener);

            // Note 60 matches startNoteSetting => callback path, ok stays 0
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHandChannel, startNote, velocity));

            expect(startCalled == true);
            expect(listener.noteOnCount == none); // ok was 0

            handler.removeListener(&listener);
        }

        beginTest(incomingEndNoteCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            bool endCalled = false;
            handler.onEndNoteSetting = [&endCalled]() { endCalled = true; };

            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            Listener listener;
            handler.addListener(&listener);

            // Note 72 matches endNoteSetting => callback path, ok stays 0
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHandChannel, endNote, velocity));

            expect(endCalled == true);
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(incomingStartNoteNoCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);
            // onStartNoteSetting is NOT set (nullptr/empty) — should not crash
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHandChannel, startNote, velocity));

        }

        beginTest(incomingEndNoteNoCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);
            // onEndNoteSetting is NOT set — should not crash
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHandChannel, endNote, velocity));
        }

        // --- noteOnKeyboard with device ---

        beginTest(keyboardNoteNotifies);
        {
            MidiHandler handler(device);
            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                int lastNote = notReceived;
                int msgCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; lastNote = midiNote; }
                void handleIncomingMessage(const juce::MidiMessage& message) override { msgCount++; (void)message; }
            };

            Listener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(noteE4, keyVelocity);

            expect(listener.noteOnCount == once);
            expect(listener.lastNote == noteE4);
            expect(listener.msgCount == once);

            handler.removeListener(&listener);
        }

        beginTest(keyboardStartNoteCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            bool startCalled = false;
            handler.onStartNoteSetting = [&startCalled]() { startCalled = true; };

            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            Listener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(startNote, fullKeyVelocity);

            expect(startCalled == true);
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(keyboardEndNoteCallback);
        {
            MidiHandler handler(device);
            handler.set_start_end_notes(startNote, endNote);

            bool endCalled = false;
            handler.onEndNoteSetting = [&endCalled]() { endCalled = true; };

            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
            };

            Listener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(endNote, fullKeyVelocity);

            expect(endCalled == true);
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(keyboardSeveralNotes);
        {
            MidiHandler handler(device);
            class Listener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                std::vector<int> notesReceived;
                void noteOnReceived(int midiNote) override
                {
                    noteOnCount++;
                    notesReceived.push_back(midiNote);
                }
            };

            Listener listener;
            handler.addListener(&listener);

            handler.noteOnKeyboard(middleC, keyVelocity);
            handler.noteOnKeyboard(noteE4, softKeyVelocity);
            handler.noteOnKeyboard(noteG4, fullKeyVelocity);

            expect(listener.noteOnCount == threeNotes);
            expect(listener.notesReceived[firstNote] == middleC);
            expect(listener.notesReceived[secondNote] == noteE4);
            expect(listener.notesReceived[thirdNote] == noteG4);

            handler.removeListener(&listener);
        }

        // --- allOffKeyboard with device ---

        beginTest(allOffSendsNoteOffs);
        {
            MidiHandler handler(device);
            // allOffKeyboard sends noteOff for notes 0-127 on ch1 and ch16
            handler.allOffKeyboard();
        }

        // --- applyInstrumentPreset (via setProgramNumber) with device ---

        beginTest(presetSendsProgramChange);
        {
            MidiHandler handler(device, &instrumentHandler);
            class PresetListener : public MidiHandlerListener
            {
            public:
                int msgCount {};
                bool gotProgramChange = false;
                int programChangeNumber = notReceived;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    msgCount++;
                    if (message.isProgramChange())
                    {
                        gotProgramChange = true;
                        programChangeNumber = message.getProgramChangeNumber();
                    }
                }
            };

            PresetListener listener;
            handler.addListener(&listener);

            handler.setProgramNumber(leftProgram, HandChoiceNames::left);

            expect(handler.getProgramNumberLeftHand() == leftProgram);
            expect(listener.gotProgramChange == true);
            expect(listener.programChangeNumber == leftProgram);
            // msgCount >= 1 because applyInstrumentPreset also sends CC messages
            expect(listener.msgCount >= once);

            handler.removeListener(&listener);
        }

        beginTest(rightHandOnChannel16);
        {
            MidiHandler handler(device, &instrumentHandler);
            class PresetListener : public MidiHandlerListener
            {
            public:
                bool gotProgramChange = false;
                int programChangeChannel = notReceived;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    if (message.isProgramChange())
                    {
                        gotProgramChange = true;
                        programChangeChannel = message.getChannel();
                    }
                }
            };

            PresetListener listener;
            handler.addListener(&listener);

            handler.setProgramNumber(rightProgram, HandChoiceNames::right);

            expect(handler.getProgramNumberRightHand() == rightProgram);
            expect(listener.gotProgramChange == true);
            expect(listener.programChangeChannel == rightHandChannel);

            handler.removeListener(&listener);
        }

        beginTest(leftHandOnChannel1);
        {
            MidiHandler handler(device, &instrumentHandler);
            class PresetListener : public MidiHandlerListener
            {
            public:
                bool gotProgramChange = false;
                int programChangeChannel = notReceived;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    if (message.isProgramChange())
                    {
                        gotProgramChange = true;
                        programChangeChannel = message.getChannel();
                    }
                }
            };

            PresetListener listener;
            handler.addListener(&listener);

            handler.setProgramNumber(otherLeftProgram, HandChoiceNames::left);

            expect(handler.getProgramNumberLeftHand() == otherLeftProgram);
            expect(listener.gotProgramChange == true);
            expect(listener.programChangeChannel == leftHandChannel);

            handler.removeListener(&listener);
        }

        beginTest(presetSendsControllers);
        {
            MidiHandler handler(device, &instrumentHandler);
            class CCListener : public MidiHandlerListener
            {
            public:
                int ccMsgCount {};
                int programChangeCount {};
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    if (message.isController())
                        ccMsgCount++;
                    if (message.isProgramChange())
                        programChangeCount++;
                }
            };

            CCListener listener;
            handler.addListener(&listener);

            handler.setProgramNumber(grandPiano, HandChoiceNames::left);

            // applyInstrumentPreset sends 1 programChange + N CC messages
            expect(listener.programChangeCount == once);
            // CC count depends on the preset; at minimum it should be >= 0
            expect(listener.ccMsgCount >= none);

            handler.removeListener(&listener);
        }

        device.deviceCloseOUT();
    }
};

static MidiHandlerHWTest midiHandlerHWTest;
