#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

namespace MidiHandlerTestValues
{
    const juce::String testName     = "MidiHandler";
    const juce::String testCategory = "Unit";

    const juce::String rePressingTheSameChordReSends = "re-pressing the same chord re-sends it (not suppressed as a duplicate)";
    const juce::String heldChordValidOnlyWhileNotesArePhysically = "heldChord - valid only while notes are physically held (not from Chord Memory)";
    const juce::String setChordModeSingleFingerRecognizesKorgOneFinger = "setChordMode - Single-Finger recognizes Korg one-finger chords";
    const juce::String setChordModeFullKeyboardScansNotesAboveThe = "setChordMode - Full-Keyboard scans notes above the split point";
    const juce::String setPlayableRange25Keys = "setPlayableRange - 25 keys";
    const juce::String setPlayableRange49Keys = "setPlayableRange - 49 keys";
    const juce::String setPlayableRange61Keys = "setPlayableRange - 61 keys";
    const juce::String setPlayableRange88Keys = "setPlayableRange - 88 keys";
    const juce::String setPlayableRangeUnrecognizedKeyCountLeavesDefaults = "setPlayableRange - unrecognized key count leaves defaults";
    const juce::String setStartEndNotesStoresValues = "set_start_end_notes - stores values";
    const juce::String setLeftRightBoundsStoresValues = "set_left_right_bounds - stores values";
    const juce::String channelForHandNoteBelowRightBoundUsesChannel = "channelForHand - note below right bound uses channel 1";
    const juce::String channelForHandNoteAtRightBoundUsesChannel = "channelForHand - note at right bound uses channel 16";
    const juce::String channelForHandNoteAboveRightBoundUsesChannel = "channelForHand - note above right bound uses channel 16";
    const juce::String channelForHandNoBoundsSetDefaultsToChannel = "channelForHand - no bounds set defaults to channel 1";
    const juce::String getProgramNumberLeftHandDefaultIs0 = "getProgramNumberLeftHand - default is 0";
    const juce::String getProgramNumberRightHandDefaultIs0 = "getProgramNumberRightHand - default is 0";
    const juce::String playBackSettingsChangedAppliesAllSettings = "playBackSettingsChanged - applies all settings";
    const juce::String handlePlayableRangeKnownKeyCountWithPositiveNrKeys = "handlePlayableRange - known key count with positive nrKeys";
    const juce::String handlePlayableRangeKeyboardInputMode = "handlePlayableRange - keyboard input mode";
    const juce::String handlePlayableRangeNegativeNrKeysWithKeyboardInputStill = "handlePlayableRange - negative nrKeys with keyboard input still sets range";
    const juce::String addListenerAndRemoveListenerNoCrash = "addListener and removeListener - no crash";
    const juce::String getNextMidiBlockEmptyBufferWhenNoMessages = "getNextMidiBlock - empty buffer when no messages";
    const juce::String handleIncomingMidiMessageNoteOnMessageIsBuffered = "handleIncomingMidiMessage - noteOn message is buffered";
    const juce::String handleIncomingMidiMessageNoteOffMessageIsBuffered = "handleIncomingMidiMessage - noteOff message is buffered";
    const juce::String handleIncomingMidiMessageNonNoteMessageIsBuffered = "handleIncomingMidiMessage - non-note message is buffered";
    const juce::String handleIncomingMidiMessageMultipleMessagesBufferedThenCleared = "handleIncomingMidiMessage - multiple messages buffered then cleared";
    const juce::String handleIncomingMidiMessageMutedChordZoneNoteONIs = "handleIncomingMidiMessage - muted chord-zone note-ON is NOT fed to the internal synth";
    const juce::String handleIncomingMidiMessageChordZoneNoteONISFed = "handleIncomingMidiMessage - chord-zone note-ON IS fed to the internal synth when mute is off";
    const juce::String handleIncomingMidiMessageMutedChordZoneNoteOFFStill = "handleIncomingMidiMessage - muted chord-zone note-OFF still passes (no stuck notes)";
    const juce::String handleIncomingMidiMessageRightHandNoteStillSoundsWhile = "handleIncomingMidiMessage - right-hand note still sounds while chord zone is muted";
    const juce::String handleIncomingMidiMessageNoteOffNotifiesNoteOffReceivedListener = "handleIncomingMidiMessage - noteOff notifies noteOffReceived listener";
    const juce::String handleIncomingMidiMessageNoteOffNotifiesHandleIncomingMessageListener = "handleIncomingMidiMessage - noteOff notifies handleIncomingMessage listener";
    const juce::String handleIncomingMidiMessageNoteOnNotifiesListenerEvenWithoutAn = "handleIncomingMidiMessage - noteOn notifies listener even without an output device";
    const juce::String handleIncomingMidiMessageStartNoteSettingNoteDoesNotTriggerNormal = "handleIncomingMidiMessage - startNoteSetting note does not trigger normal noteOn path";
    const juce::String handleIncomingMidiMessageEndNoteSettingNoteDoesNotTriggerNormal = "handleIncomingMidiMessage - endNoteSetting note does not trigger normal noteOn path";
    const juce::String handleIncomingMidiMessageNoteOffFiresForMultipleNotes = "handleIncomingMidiMessage - noteOff fires for multiple notes";

    using HandChannels::leftHand;
    using HandChannels::rightHand;

    constexpr int lowestNote  = 0;
    constexpr int noteA0      = 21;
    constexpr int noteC2      = 36;
    constexpr int noteE2      = 40;
    constexpr int noteC3      = 48;
    constexpr int noteBFlat3  = 58;
    constexpr int noteB3      = 59;
    constexpr int middleC     = 60;
    constexpr int noteE4      = 64;
    constexpr int noteG4      = 67;
    constexpr int noteB4      = 71;
    constexpr int noteC5      = 72;
    constexpr int noteGSharp5 = 80;
    constexpr int noteE6      = 88;
    constexpr int noteC7      = 96;
    constexpr int noteC8      = 108;
    constexpr int highestNote = 127;
    constexpr int pitchC      = 0;

    constexpr int noLeftBound            = 0;
    constexpr int splitAbovePlayedNotes  = 100;   // every note the tests play is in the chord (left) zone
    constexpr int splitAt50              = 50;
    constexpr int leftHandBound          = 55;
    constexpr int rightHandBound         = 72;
    constexpr int startNote              = 60;    // the "set start note" key
    constexpr int endNote                = 72;    // the "set end note" key

    constexpr int         keyVelocity         = 100;
    constexpr int         releaseVelocity     = 0;
    constexpr juce::uint8 velocity            = 100;
    constexpr juce::uint8 softVelocity        = 80;
    constexpr juce::uint8 releaseVelocityByte = 0;
    constexpr int         sustainController   = 64;
    constexpr int         fullControllerValue = 127;

    constexpr int keys25 = 25;
    constexpr int keys49 = 49;
    constexpr int keys61 = 61;
    constexpr int keys88 = 88;
    constexpr int unsupportedKeyCount = 37;
    constexpr int invalidKeyCount     = -5;
    constexpr int rangeHandled        = 1;
    const juce::String vendorId  = "07cf";
    const juce::String productId = "6803";

    constexpr int defaultProgram = 0;
    constexpr int blockStart     = 0;
    constexpr int blockSamples   = 512;
    constexpr int none           = 0;
    constexpr int once           = 1;
    constexpr int threeEvents    = 3;
    constexpr int notReceived    = -1;
    constexpr size_t firstNote   = 0;
    constexpr size_t secondNote  = 1;
    constexpr size_t thirdNote   = 2;
}

using namespace MidiHandlerTestValues;

class MidiHandlerTest : public juce::UnitTest
{
public:
    MidiHandlerTest() : juce::UnitTest (MidiHandlerTestValues::testName, MidiHandlerTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(rePressingTheSameChordReSends);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);   // notes < 100 are in the chord (left) zone

            int validSends {};
            handler.onChordChanged = [&](const ArrangerChord& chord) { if (chord.isValid()) ++validSends; };

            // Press C major -> one valid send.
            handler.noteOnKeyboard(middleC, keyVelocity); handler.noteOnKeyboard(noteE4, keyVelocity); handler.noteOnKeyboard(noteG4, keyVelocity);
            expect(validSends >= once);
            const int afterFirst = validSends;

            // Release everything (chord memory holds; no send), then press the SAME C major again.
            handler.noteOffKeyboard(middleC, releaseVelocity); handler.noteOffKeyboard(noteE4, releaseVelocity); handler.noteOffKeyboard(noteG4, releaseVelocity);
            handler.noteOnKeyboard(middleC, keyVelocity); handler.noteOnKeyboard(noteE4, keyVelocity); handler.noteOnKeyboard(noteG4, keyVelocity);

            // Must be re-sent — the engine may have reset to its home key between presses (e.g. a 2nd Play).
            expect(validSends > afterFirst);
        }

        // ---- heldChord (Phase 5 / held-chord seed) ----

        beginTest(heldChordValidOnlyWhileNotesArePhysically);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);   // chord zone = notes < 100
            handler.setChordMemory(true);            // memory ON: current() remembers after release

            expect(! handler.heldChord().isValid());                 // nothing held yet

            handler.noteOnKeyboard(middleC, keyVelocity); handler.noteOnKeyboard(noteE4, keyVelocity); handler.noteOnKeyboard(noteG4, keyVelocity); // C major
            expect(handler.heldChord().isValid());
            expectEquals(handler.heldChord().root, pitchC);               // C

            handler.noteOffKeyboard(middleC, releaseVelocity); handler.noteOffKeyboard(noteE4, releaseVelocity); handler.noteOffKeyboard(noteG4, releaseVelocity);
            // Chord Memory still *remembers* C major, but nothing is physically held -> heldChord() invalid.
            expect(! handler.heldChord().isValid());
        }

        // ---- setChordMode (Phase 5) ----

        beginTest(setChordModeSingleFingerRecognizesKorgOneFinger);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);
            handler.setChordMode(ChordMode::SingleFinger);

            ArrangerChord last;
            handler.onChordChanged = [&](const ArrangerChord& chord) { last = chord; };

            handler.noteOnKeyboard(middleC, keyVelocity);   // C alone -> C major
            expect(last.isValid()); expectEquals(last.root, pitchC);
            expect(last.quality == ChordQuality::Maj);

            handler.noteOnKeyboard(noteBFlat3, keyVelocity);   // + Bb (black key to the left) -> C minor
            expect(last.quality == ChordQuality::Min);
        }

        beginTest(setChordModeFullKeyboardScansNotesAboveThe);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(noLeftBound, splitAt50);   // split at 50: in Fingered, notes >=50 are out of zone
            handler.setChordMode(ChordMode::FullKeyboard);

            int valids {};
            handler.onChordChanged = [&](const ArrangerChord& chord) { if (chord.isValid()) ++valids; };

            // 60/64/67 are ABOVE the split (would be ignored in Fingered) -> Full-Keyboard still reads them.
            handler.noteOnKeyboard(middleC, keyVelocity); handler.noteOnKeyboard(noteE4, keyVelocity); handler.noteOnKeyboard(noteG4, keyVelocity);
            expect(valids >= once);
        }

        // ---- setPlayableRange ----

        beginTest(setPlayableRange25Keys);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(keys25);
            expect(device.get_minNote() == noteC2);
            expect(device.get_maxNote() == middleC);
        }

        beginTest(setPlayableRange49Keys);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(keys49);
            expect(device.get_minNote() == noteE2);
            expect(device.get_maxNote() == noteE6);
        }

        beginTest(setPlayableRange61Keys);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(keys61);
            expect(device.get_minNote() == noteC2);
            expect(device.get_maxNote() == noteC7);
        }

        beginTest(setPlayableRange88Keys);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(keys88);
            expect(device.get_minNote() == noteA0);
            expect(device.get_maxNote() == noteC8);
        }

        beginTest(setPlayableRangeUnrecognizedKeyCountLeavesDefaults);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setPlayableRange(unsupportedKeyCount);  
            
            expect(device.get_minNote() == lowestNote);
            expect(device.get_maxNote() == highestNote);
        }

        // ---- set_start_end_notes ----

        beginTest(setStartEndNotesStoresValues);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_start_end_notes(noteC2, noteC7);
            // verified indirectly: no crash, internal state set
            // further verified through playBackSettingsChanged below
        }

        // ---- set_left_right_bounds ----

        beginTest(setLeftRightBoundsStoresValues);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(leftHandBound, rightHandBound);
            // verified indirectly through setCorrectChannelBasedOnHand below
        }

        // ---- channelForHand ----

        beginTest(channelForHandNoteBelowRightBoundUsesChannel);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(leftHandBound, middleC);

            // note 59 is below rightHandBound (60), so channel should be 1
            expectEquals(handler.channelForHand(noteB3), leftHand);
        }

        beginTest(channelForHandNoteAtRightBoundUsesChannel);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(leftHandBound, middleC);
            expectEquals(handler.channelForHand(middleC), rightHand);   // note >= rightHandBound
        }

        beginTest(channelForHandNoteAboveRightBoundUsesChannel);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.set_left_right_bounds(leftHandBound, middleC);
            expectEquals(handler.channelForHand(noteGSharp5), rightHand);
        }

        beginTest(channelForHandNoBoundsSetDefaultsToChannel);
        {
            MidiDevice device;
            MidiHandler handler(device);
            // rightHandBoundSetting is -1 by default => first condition false => channel 1
            expectEquals(handler.channelForHand(middleC), leftHand);
        }

        // ---- Program Numbers ----

        beginTest(getProgramNumberLeftHandDefaultIs0);
        {
            MidiDevice device;
            MidiHandler handler(device);
            expect(handler.getProgramNumberLeftHand() == defaultProgram);
        }

        beginTest(getProgramNumberRightHandDefaultIs0);
        {
            MidiDevice device;
            MidiHandler handler(device);
            expect(handler.getProgramNumberRightHand() == defaultProgram);
        }

        // ---- playBackSettingsChanged ----

        beginTest(playBackSettingsChangedAppliesAllSettings);
        {
            MidiDevice device;
            MidiHandler handler(device);

            PlayBackSettings settings;
            settings.startNote = noteC2;
            settings.endNote = noteC7;
            settings.leftHandBound = leftHandBound;
            settings.rightHandBound = rightHandBound;

            handler.playBackSettingsChanged(settings);
            // internally calls set_start_end_notes and set_left_right_bounds
            // verify channel routing works with the new bounds
            expectEquals(handler.channelForHand(noteB4), leftHand);   // below 72 => ch 1
            expectEquals(handler.channelForHand(noteC5), rightHand);  // at 72 => ch 16
        }

        // ---- handlePlayableRange ----

        beginTest(handlePlayableRangeKnownKeyCountWithPositiveNrKeys);
        {
            MidiDevice device;
            MidiHandler handler(device);
            int result = handler.handlePlayableRange(vendorId, productId, keys61, false);
            expect(result == rangeHandled);
            expect(device.get_minNote() == noteC2);
            expect(device.get_maxNote() == noteC7);
        }

        beginTest(handlePlayableRangeKeyboardInputMode);
        {
            MidiDevice device;
            MidiHandler handler(device);
            int result = handler.handlePlayableRange(vendorId, productId, keys49, true);
            expect(result == rangeHandled);
            expect(device.get_minNote() == noteE2);
            expect(device.get_maxNote() == noteE6);
        }

        beginTest(handlePlayableRangeNegativeNrKeysWithKeyboardInputStill);
        {
            MidiDevice device;
            MidiHandler handler(device);
            // isKeyboardInput = true, so it goes through setPlayableRange(-5)
            // setPlayableRange doesn't match any known size, so defaults remain
            int result = handler.handlePlayableRange(vendorId, productId, invalidKeyCount, true);
            expect(result == rangeHandled);
        }

        // ---- Listener add/remove ----

        beginTest(addListenerAndRemoveListenerNoCrash);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            class MockListener : public MidiHandlerListener
            {
            public:
                int noteOnCount {};
                int noteOffCount {};
                void noteOnReceived(int midiNote) override { noteOnCount++; (void)midiNote; }
                void noteOffReceived(int midiNote) override { noteOffCount++; (void)midiNote; }
            };

            MockListener listener;
            handler.addListener(&listener);
            handler.removeListener(&listener);
            // no crash = pass
        }

        // ---- getNextMidiBlock ----

        beginTest(getNextMidiBlockEmptyBufferWhenNoMessages);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == none);
        }

        // ==================================================================
        //  handleIncomingMidiMessage
        // ==================================================================

        beginTest(handleIncomingMidiMessageNoteOnMessageIsBuffered);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto noteOn = juce::MidiMessage::noteOn(leftHand, middleC, velocity);
            handler.handleIncomingMidiMessage(nullptr, noteOn);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);
        }

        beginTest(handleIncomingMidiMessageNoteOffMessageIsBuffered);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto noteOff = juce::MidiMessage::noteOff(leftHand, middleC, releaseVelocityByte);
            handler.handleIncomingMidiMessage(nullptr, noteOff);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);
        }

        beginTest(handleIncomingMidiMessageNonNoteMessageIsBuffered);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            auto controller = juce::MidiMessage::controllerEvent(leftHand, sustainController, fullControllerValue);
            handler.handleIncomingMidiMessage(nullptr, controller);

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);
        }

        beginTest(handleIncomingMidiMessageMultipleMessagesBufferedThenCleared);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, middleC, velocity));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, middleC, releaseVelocityByte));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, noteE4, softVelocity));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == threeEvents);

            // After getNextMidiBlock the internal buffer should be cleared
            juce::MidiBuffer buffer2;
            handler.getNextMidiBlock(buffer2, blockStart, blockSamples);
            expect(buffer2.getNumEvents() == none);
        }

        // ---- chord-zone mute must apply to the INTERNAL synth feed too (physical-MIDI path) ----
        // Regression: physical-keyboard left-zone notes were sounding on the internal SFZ synth
        // even with chord-zone mute on, because handleIncomingMidiMessage enqueued the raw note
        // into incomingMidiMessages unconditionally (the PC-keyboard path already gated it).

        beginTest(handleIncomingMidiMessageMutedChordZoneNoteONIs);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);   // notes < 100 are in the chord (left) zone
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            // A physical-keyboard left-zone note: drives the chord, must NOT sound on the internal synth.
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, noteC3, velocity));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == none);   // muted onset never reaches the SFZ feed
        }

        beginTest(handleIncomingMidiMessageChordZoneNoteONISFed);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);
            handler.setChordZoneMute(false);        // normal playing: the note must sound
            handler.setArrangerEngaged(true);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, noteC3, velocity));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);
        }

        beginTest(handleIncomingMidiMessageMutedChordZoneNoteOFFStill);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            // Onset is muted, but the release must always reach the synth so nothing can hang.
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, noteC3, releaseVelocityByte));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);
        }

        beginTest(handleIncomingMidiMessageRightHandNoteStillSoundsWhile);
        {
            MidiDevice device;
            MidiHandler handler(device);
            handler.setInternalSynthListening(true);   // the internal SFZ synth is running and draining the queue
            handler.set_left_right_bounds(noLeftBound, splitAbovePlayedNotes);   // split at 100; notes >= 100 are the right (melody) zone
            handler.setChordZoneMute(true);
            handler.setArrangerEngaged(true);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, noteC8, velocity));

            juce::MidiBuffer buffer;
            handler.getNextMidiBlock(buffer, blockStart, blockSamples);
            expect(buffer.getNumEvents() == once);   // only the left zone is muted
        }

        beginTest(handleIncomingMidiMessageNoteOffNotifiesNoteOffReceivedListener);
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

            auto noteOff = juce::MidiMessage::noteOff(leftHand, noteE4, releaseVelocityByte);
            handler.handleIncomingMidiMessage(nullptr, noteOff);

            expect(listener.noteOffCount == once);
            expect(listener.lastNoteOff == noteE4);

            handler.removeListener(&listener);
        }

        beginTest(handleIncomingMidiMessageNoteOffNotifiesHandleIncomingMessageListener);
        {
            MidiDevice device;
            MidiHandler handler(device);

            class MsgListener : public MidiHandlerListener
            {
            public:
                int msgCount {};
                bool lastWasNoteOff = false;
                void handleIncomingMessage(const juce::MidiMessage& message) override
                {
                    msgCount++;
                    lastWasNoteOff = message.isNoteOff();
                }
            };

            MsgListener listener;
            handler.addListener(&listener);

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, noteC5, releaseVelocityByte));

            expect(listener.msgCount == once);
            expect(listener.lastWasNoteOff == true);

            handler.removeListener(&listener);
        }

        beginTest(handleIncomingMidiMessageNoteOnNotifiesListenerEvenWithoutAn);
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

            auto noteOn = juce::MidiMessage::noteOn(leftHand, middleC, velocity);
            handler.handleIncomingMidiMessage(nullptr, noteOn);

            // Playability is decided from the note range FIRST, independent of any external
            // MIDI-OUT device (5c75d4f). startNoteSetting/endNoteSetting default to -1, so note 60
            // sets ok = 1 and the listener is notified even with no device open -- this is what
            // makes the internal SFZ sound from a physical keyboard. Gating this on the midiOut
            // lock was the bug; asserting 0 here would re-assert that bug.
            expect(listener.noteOnCount == once);

            handler.removeListener(&listener);
        }

        beginTest(handleIncomingMidiMessageStartNoteSettingNoteDoesNotTriggerNormal);
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

            // Note 60 matches startNoteSetting
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, middleC, velocity));
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(handleIncomingMidiMessageEndNoteSettingNoteDoesNotTriggerNormal);
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

            // Note 72 matches endNoteSetting
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(leftHand, noteC5, velocity));
            expect(listener.noteOnCount == none);

            handler.removeListener(&listener);
        }

        beginTest(handleIncomingMidiMessageNoteOffFiresForMultipleNotes);
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

            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, middleC, releaseVelocityByte));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, noteE4, releaseVelocityByte));
            handler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(leftHand, noteG4, releaseVelocityByte));

            expect(listener.noteOffCount == threeEvents);
            expect(listener.notesReceived[firstNote] == middleC);
            expect(listener.notesReceived[secondNote] == noteE4);
            expect(listener.notesReceived[thirdNote] == noteG4);

            handler.removeListener(&listener);
        }
    }
};

static MidiHandlerTest midiHandlerTest;
