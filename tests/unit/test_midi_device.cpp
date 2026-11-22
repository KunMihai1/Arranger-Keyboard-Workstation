#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiHandler.h"

namespace MidiDeviceTestValues
{
    const juce::String testName     = "MidiDevice";
    const juce::String testCategory = "Unit";

    const juce::String defaultVolumeAndReverb   = "Constructor - default volume and reverb";
    const juce::String defaultNoteRange         = "Constructor - default note range";
    const juce::String notOpenByDefault         = "Constructor - device not open by default";
    const juce::String defaultDeviceIndices     = "Constructor - default device indices";
    const juce::String minNoteRoundTrip         = "set_minNote and get_minNote";
    const juce::String maxNoteRoundTrip         = "set_maxNote and get_maxNote";
    const juce::String keysFullRange            = "getNrKeysAfterInitialized - full range";
    const juce::String keysCustomRange          = "getNrKeysAfterInitialized - custom range";
    const juce::String volumeLeftHand           = "setVolume / getVolume - channel 1";
    const juce::String volumeRightHand          = "setVolume / getVolume - channel 16";
    const juce::String volumeUnknownChannel     = "getVolume - unknown channel returns 50.0";
    const juce::String reverbLeftHand           = "setReverb / getReverb - channel 1";
    const juce::String reverbRightHand          = "setReverb / getReverb - channel 16";
    const juce::String reverbUnknownChannel     = "getReverb - unknown channel returns 50.0";
    const juce::String toneRoundTrip            = "brightness/expression/chorus/resonance - round-trip per channel, independent hands";
    const juce::String toneUnknownChannel       = "brightness/expression/chorus/resonance - unknown channel returns 50";
    const juce::String sustainRoundTrip         = "sustain toggle - round-trip per channel, unknown channel defaults true";
    const juce::String envelopeRoundTrip        = "envelope/mod accessors (attack/decay/release/vibrato/delay) - round-trip + default 0";
    const juce::String settersIgnoreOtherChannels = "setters ignore channels other than 1 and 16 (no first/second-hand change)";
    const juce::String deviceInRoundTrip        = "setDeviceIN / getDeviceIndexIN";
    const juce::String deviceOutRoundTrip       = "setDeviceOUT / getDeviceIndexOUT";
    const juce::String vendorIdRoundTrip        = "setVID / getVID";
    const juce::String productIdRoundTrip       = "setPID / getPID";
    const juce::String nameRoundTrip            = "setDeviceName / getName";
    const juce::String extractPidValid          = "extractPID - valid identifier";
    const juce::String extractPidUsb            = "extractPID - standard USB identifier";
    const juce::String extractPidMissing        = "extractPID - no pid_ in identifier";
    const juce::String extractPidEmpty          = "extractPID - empty string";
    const juce::String extractVidValid          = "extractVID - valid identifier";
    const juce::String extractVidMissing        = "extractVID - no vid_ in identifier";
    const juce::String extractVidEmpty          = "extractVID - empty string";
    const juce::String openInNegative           = "deviceOpenIN - negative index returns false";
    const juce::String openInOutOfRange         = "deviceOpenIN - out of range index returns false";
    const juce::String openOutNegative          = "deviceOpenOUT - negative index returns false";
    const juce::String openOutOutOfRange        = "deviceOpenOUT - out of range index returns false";
    const juce::String sendCcWithoutOutput      = "sendMidiCC - with no output device open does nothing";

    const juce::String noOutputOpen = "no output should be open";

    using HandChannels::leftHand;
    using HandChannels::rightHand;
    constexpr int otherChannel      = 5;    // neither hand
    constexpr int unknownChannel    = 7;

    constexpr int defaultLevel      = 64;   // volume and reverb start at the controller centre
    constexpr int unknownLevel      = 50;   // what an unknown channel reports
    constexpr int unknownEnvelope   = 0;
    constexpr int lowestNote        = 0;
    constexpr int highestNote       = 127;
    constexpr int allKeys           = 128;
    constexpr int noteC2            = 36;
    constexpr int noteC7            = 96;
    constexpr int noteC8            = 108;
    constexpr int keysC2ToC7        = 61;
    constexpr int firstDevice       = 0;
    constexpr int inputDevice       = 3;
    constexpr int outputDevice      = 5;
    constexpr int negativeIndex     = -1;
    constexpr int outOfRangeIndex   = 999;

    constexpr int leftVolume  = 75;
    constexpr int rightVolume = 30;
    constexpr int leftReverb  = 80;
    constexpr int rightReverb = 25;

    constexpr int leftBrightness  = 20;
    constexpr int rightBrightness = 80;
    constexpr int leftExpression  = 33;
    constexpr int rightExpression = 99;
    constexpr int leftChorus      = 10;
    constexpr int rightChorus     = 70;
    constexpr int leftResonance   = 5;
    constexpr int rightResonance  = 60;
    constexpr int ignoredBrightness = 123;

    constexpr int leftAttack   = 15;
    constexpr int rightAttack  = 45;
    constexpr int leftDecay    = 25;
    constexpr int rightDecay   = 55;
    constexpr int leftRelease  = 35;
    constexpr int rightRelease = 65;
    constexpr int leftVibrato  = 12;
    constexpr int rightVibrato = 72;
    constexpr int leftDelay    = 18;
    constexpr int rightDelay   = 88;

    const juce::String vendorId       = "07cf";
    const juce::String productId      = "6803";
    const juce::String deviceName     = "CT s-300";
    const juce::String windowsIdentifier = "\\\\?\\SWD#MMDEVAPI#MIDII_VID&pid_6803&SomethingElse";
    const juce::String prefixedPid    = "some_prefix_pid_6803_suffix";
    const juce::String usbIdentifier  = "usb_vid_07cf&pid_ABCD&rev_0100";
    const juce::String usbProductId   = "ABCD";
    const juce::String noProductId    = "no_product_id_here";
    const juce::String vendorAndProduct = "usb_vid_07cf&pid_6803";
    const juce::String noVendorId     = "no_vendor_id_here";

    constexpr int volumeController = 7;
    constexpr int halfVolume       = 64;
}

using namespace MidiDeviceTestValues;

class MidiDeviceTest : public juce::UnitTest
{
public:
    MidiDeviceTest() : juce::UnitTest(MidiDeviceTestValues::testName, MidiDeviceTestValues::testCategory) {}

    void runTest() override
    {
        // ---- Constructor Defaults ----

        beginTest(defaultVolumeAndReverb);
        {
            MidiDevice device;
            expect(device.getVolume(leftHand) == defaultLevel);
            expect(device.getVolume(rightHand) == defaultLevel);
            expect(device.getReverb(leftHand) == defaultLevel);
            expect(device.getReverb(rightHand) == defaultLevel);
        }

        beginTest(defaultNoteRange);
        {
            MidiDevice device;
            expect(device.get_minNote() == lowestNote);
            expect(device.get_maxNote() == highestNote);
        }

        beginTest(notOpenByDefault);
        {
            MidiDevice device;
            expect(device.isOpenIN() == false);
            expect(device.isOpenOUT() == false);
        }

        beginTest(defaultDeviceIndices);
        {
            MidiDevice device;
            expect(device.getDeviceIndexIN() == firstDevice);
            expect(device.getDeviceIndexOUT() == firstDevice);
        }

        // ---- min/max Note ----

        beginTest(minNoteRoundTrip);
        {
            MidiDevice device;
            device.set_minNote(noteC2);
            expect(device.get_minNote() == noteC2);
            device.set_minNote(lowestNote);
            expect(device.get_minNote() == lowestNote);
        }

        beginTest(maxNoteRoundTrip);
        {
            MidiDevice device;
            device.set_maxNote(noteC7);
            expect(device.get_maxNote() == noteC7);
            device.set_maxNote(noteC8);
            expect(device.get_maxNote() == noteC8);
        }

        beginTest(keysFullRange);
        {
            MidiDevice device;
            // default: min=0, max=127 => 128 keys
            expect(device.getNrKeysAfterInitialized() == allKeys);
        }

        beginTest(keysCustomRange);
        {
            MidiDevice device;
            device.set_minNote(noteC2);
            device.set_maxNote(noteC7);
            expect(device.getNrKeysAfterInitialized() == keysC2ToC7);
        }

        // ---- Volume per channel ----

        beginTest(volumeLeftHand);
        {
            MidiDevice device;
            device.setVolume(leftVolume, leftHand);
            expect(device.getVolume(leftHand) == leftVolume);
            expect(device.getVolume(rightHand) == defaultLevel);  // channel 16 unchanged
        }

        beginTest(volumeRightHand);
        {
            MidiDevice device;
            device.setVolume(rightVolume, rightHand);
            expect(device.getVolume(rightHand) == rightVolume);
            expect(device.getVolume(leftHand) == defaultLevel);  // channel 1 unchanged
        }

        beginTest(volumeUnknownChannel);
        {
            MidiDevice device;
            expect(device.getVolume(otherChannel) == unknownLevel);
        }

        // ---- Reverb per channel ----

        beginTest(reverbLeftHand);
        {
            MidiDevice device;
            device.setReverb(leftReverb, leftHand);
            expect(device.getReverb(leftHand) == leftReverb);
            expect(device.getReverb(rightHand) == defaultLevel);
        }

        beginTest(reverbRightHand);
        {
            MidiDevice device;
            device.setReverb(rightReverb, rightHand);
            expect(device.getReverb(rightHand) == rightReverb);
            expect(device.getReverb(leftHand) == defaultLevel);
        }

        beginTest(reverbUnknownChannel);
        {
            MidiDevice device;
            expect(device.getReverb(unknownChannel) == unknownLevel);
        }

        // ---- Per-hand CC / sound-shaping accessors (channel 1 = first hand, 16 = second) ----

        beginTest(toneRoundTrip);
        {
            MidiDevice device;
            device.setBrightness(leftBrightness, leftHand);  device.setBrightness(rightBrightness, rightHand);
            device.setExpression(leftExpression, leftHand);  device.setExpression(rightExpression, rightHand);
            device.setChorus(leftChorus, leftHand);          device.setChorus(rightChorus, rightHand);
            device.setResonance(leftResonance, leftHand);    device.setResonance(rightResonance, rightHand);

            expectEquals(device.getBrightness(leftHand), leftBrightness);  expectEquals(device.getBrightness(rightHand), rightBrightness);
            expectEquals(device.getExpression(leftHand), leftExpression);  expectEquals(device.getExpression(rightHand), rightExpression);
            expectEquals(device.getChorus(leftHand), leftChorus);          expectEquals(device.getChorus(rightHand), rightChorus);
            expectEquals(device.getResonance(leftHand), leftResonance);    expectEquals(device.getResonance(rightHand), rightResonance);
        }

        beginTest(toneUnknownChannel);
        {
            MidiDevice device;
            expectEquals(device.getBrightness(unknownChannel), unknownLevel);
            expectEquals(device.getExpression(unknownChannel), unknownLevel);
            expectEquals(device.getChorus(unknownChannel), unknownLevel);
            expectEquals(device.getResonance(unknownChannel), unknownLevel);
        }

        beginTest(sustainRoundTrip);
        {
            MidiDevice device;
            device.setSustainToggle(false, leftHand);
            device.setSustainToggle(true, rightHand);
            expect(device.getSustainToggle(leftHand) == false);
            expect(device.getSustainToggle(rightHand) == true);
            expect(device.getSustainToggle(unknownChannel) == true);   // unknown -> default
        }

        beginTest(envelopeRoundTrip);
        {
            MidiDevice device;
            device.setAttack(leftAttack, leftHand);     device.setAttack(rightAttack, rightHand);
            device.setDecay(leftDecay, leftHand);       device.setDecay(rightDecay, rightHand);
            device.setRelease(leftRelease, leftHand);   device.setRelease(rightRelease, rightHand);
            device.setVibrato(leftVibrato, leftHand);   device.setVibrato(rightVibrato, rightHand);
            device.setDelay(leftDelay, leftHand);       device.setDelay(rightDelay, rightHand);

            expectEquals(device.getAttack(leftHand), leftAttack);    expectEquals(device.getAttack(rightHand), rightAttack);
            expectEquals(device.getDecay(leftHand), leftDecay);      expectEquals(device.getDecay(rightHand), rightDecay);
            expectEquals(device.getRelease(leftHand), leftRelease);  expectEquals(device.getRelease(rightHand), rightRelease);
            expectEquals(device.getVibrato(leftHand), leftVibrato);  expectEquals(device.getVibrato(rightHand), rightVibrato);
            expectEquals(device.getDelay(leftHand), leftDelay);      expectEquals(device.getDelay(rightHand), rightDelay);

            expectEquals(device.getAttack(unknownChannel), unknownEnvelope);
            expectEquals(device.getDecay(unknownChannel), unknownEnvelope);
            expectEquals(device.getRelease(unknownChannel), unknownEnvelope);
            expectEquals(device.getVibrato(unknownChannel), unknownEnvelope);
            expectEquals(device.getDelay(unknownChannel), unknownEnvelope);
        }

        beginTest(settersIgnoreOtherChannels);
        {
            MidiDevice device;
            const int leftBefore = device.getBrightness(leftHand), rightBefore = device.getBrightness(rightHand);
            device.setBrightness(ignoredBrightness, otherChannel);   // channel 5 is neither hand -> ignored
            expectEquals(device.getBrightness(leftHand), leftBefore);
            expectEquals(device.getBrightness(rightHand), rightBefore);
        }

        // ---- Device Index ----

        beginTest(deviceInRoundTrip);
        {
            MidiDevice device;
            device.setDeviceIN(inputDevice);
            expect(device.getDeviceIndexIN() == inputDevice);
        }

        beginTest(deviceOutRoundTrip);
        {
            MidiDevice device;
            device.setDeviceOUT(outputDevice);
            expect(device.getDeviceIndexOUT() == outputDevice);
        }

        // ---- VID / PID / Name ----

        beginTest(vendorIdRoundTrip);
        {
            MidiDevice device;
            device.setVID(vendorId);
            expect(device.getVID() == vendorId);
        }

        beginTest(productIdRoundTrip);
        {
            MidiDevice device;
            device.setPID(productId);
            expect(device.getPID() == productId);
        }

        beginTest(nameRoundTrip);
        {
            MidiDevice device;
            device.setDeviceName(deviceName);
            expect(device.getName() == deviceName);
        }

        // ---- extractPID ----

        beginTest(extractPidValid);
        {
            MidiDevice device;
            juce::String identifier = windowsIdentifier;
            expect(device.extractPID(prefixedPid) == productId);
        }

        beginTest(extractPidUsb);
        {
            MidiDevice device;
            expect(device.extractPID(usbIdentifier) == usbProductId);
        }

        beginTest(extractPidMissing);
        {
            MidiDevice device;
            expect(device.extractPID(noProductId).isEmpty());
        }

        beginTest(extractPidEmpty);
        {
            MidiDevice device;
            expect(device.extractPID({}).isEmpty());
        }

        // ---- extractVID ----

        beginTest(extractVidValid);
        {
            MidiDevice device;
            expect(device.extractVID(vendorAndProduct) == vendorId);
        }

        beginTest(extractVidMissing);
        {
            MidiDevice device;
            expect(device.extractVID(noVendorId).isEmpty());
        }

        beginTest(extractVidEmpty);
        {
            MidiDevice device;
            expect(device.extractVID({}).isEmpty());
        }

        // ---- deviceOpenIN invalid index ----

        beginTest(openInNegative);
        {
            MidiDevice device;
            expect(device.deviceOpenIN(negativeIndex, nullptr) == false);
            expect(device.isOpenIN() == false);
        }

        beginTest(openInOutOfRange);
        {
            MidiDevice device;
            expect(device.deviceOpenIN(outOfRangeIndex, nullptr) == false);
            expect(device.isOpenIN() == false);
        }

        // ---- deviceOpenOUT invalid index ----

        beginTest(openOutNegative);
        {
            MidiDevice device;
            expect(device.deviceOpenOUT(negativeIndex) == false);
            expect(device.isOpenOUT() == false);
        }

        beginTest(openOutOutOfRange);
        {
            MidiDevice device;
            expect(device.deviceOpenOUT(outOfRangeIndex) == false);
            expect(device.isOpenOUT() == false);
        }

        // Crashed the CI runner (0xC0000005): an output that is listed but fails to open leaves no
        // device, and sendMidiCC dereferenced it.
        beginTest(sendCcWithoutOutput);
        {
            MidiDevice device;
            device.sendMidiCC(leftHand, volumeController, halfVolume);   // must not crash
            expect(device.isOpenOUT() == false, noOutputOpen);
        }
    }
};

static MidiDeviceTest midiDeviceTest;
