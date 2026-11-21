#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiHandler.h"

class MidiDeviceTest : public juce::UnitTest
{
public:
    MidiDeviceTest() : juce::UnitTest("MidiDevice", "Unit") {}

    void runTest() override
    {
        // ---- Constructor Defaults ----

        beginTest("Constructor - default volume and reverb");
        {
            MidiDevice device;
            expect(device.getVolume(1) == 64);
            expect(device.getVolume(16) == 64);
            expect(device.getReverb(1) == 64);
            expect(device.getReverb(16) == 64);
        }

        beginTest("Constructor - default note range");
        {
            MidiDevice device;
            expect(device.get_minNote() == 0);
            expect(device.get_maxNote() == 127);
        }

        beginTest("Constructor - device not open by default");
        {
            MidiDevice device;
            expect(device.isOpenIN() == false);
            expect(device.isOpenOUT() == false);
        }

        beginTest("Constructor - default device indices");
        {
            MidiDevice device;
            expect(device.getDeviceIndexIN() == 0);
            expect(device.getDeviceIndexOUT() == 0);
        }

        // ---- min/max Note ----

        beginTest("set_minNote and get_minNote");
        {
            MidiDevice device;
            device.set_minNote(36);
            expect(device.get_minNote() == 36);
            device.set_minNote(0);
            expect(device.get_minNote() == 0);
        }

        beginTest("set_maxNote and get_maxNote");
        {
            MidiDevice device;
            device.set_maxNote(96);
            expect(device.get_maxNote() == 96);
            device.set_maxNote(108);
            expect(device.get_maxNote() == 108);
        }

        beginTest("getNrKeysAfterInitialized - full range");
        {
            MidiDevice device;
            // default: min=0, max=127 => 128 keys
            expect(device.getNrKeysAfterInitialized() == 128);
        }

        beginTest("getNrKeysAfterInitialized - custom range");
        {
            MidiDevice device;
            device.set_minNote(36);
            device.set_maxNote(96);
            expect(device.getNrKeysAfterInitialized() == 61);
        }

        // ---- Volume per channel ----

        beginTest("setVolume / getVolume - channel 1");
        {
            MidiDevice device;
            device.setVolume(75, 1);
            expect(device.getVolume(1) == 75);
            expect(device.getVolume(16) == 64);  // channel 16 unchanged
        }

        beginTest("setVolume / getVolume - channel 16");
        {
            MidiDevice device;
            device.setVolume(30, 16);
            expect(device.getVolume(16) == 30);
            expect(device.getVolume(1) == 64);  // channel 1 unchanged
        }

        beginTest("getVolume - unknown channel returns 50.0");
        {
            MidiDevice device;
            expect(device.getVolume(5) == 50);
        }

        // ---- Reverb per channel ----

        beginTest("setReverb / getReverb - channel 1");
        {
            MidiDevice device;
            device.setReverb(80, 1);
            expect(device.getReverb(1) == 80);
            expect(device.getReverb(16) == 64);
        }

        beginTest("setReverb / getReverb - channel 16");
        {
            MidiDevice device;
            device.setReverb(25, 16);
            expect(device.getReverb(16) == 25);
            expect(device.getReverb(1) == 64);
        }

        beginTest("getReverb - unknown channel returns 50.0");
        {
            MidiDevice device;
            expect(device.getReverb(7) == 50);
        }

        // ---- Per-hand CC / sound-shaping accessors (channel 1 = first hand, 16 = second) ----

        beginTest("brightness/expression/chorus/resonance - round-trip per channel, independent hands");
        {
            MidiDevice d;
            d.setBrightness(20, 1);  d.setBrightness(80, 16);
            d.setExpression(33, 1);  d.setExpression(99, 16);
            d.setChorus(10, 1);      d.setChorus(70, 16);
            d.setResonance(5, 1);    d.setResonance(60, 16);

            expectEquals(d.getBrightness(1), 20);  expectEquals(d.getBrightness(16), 80);
            expectEquals(d.getExpression(1), 33);  expectEquals(d.getExpression(16), 99);
            expectEquals(d.getChorus(1), 10);      expectEquals(d.getChorus(16), 70);
            expectEquals(d.getResonance(1), 5);    expectEquals(d.getResonance(16), 60);
        }

        beginTest("brightness/expression/chorus/resonance - unknown channel returns 50");
        {
            MidiDevice d;
            expectEquals(d.getBrightness(7), 50);
            expectEquals(d.getExpression(7), 50);
            expectEquals(d.getChorus(7), 50);
            expectEquals(d.getResonance(7), 50);
        }

        beginTest("sustain toggle - round-trip per channel, unknown channel defaults true");
        {
            MidiDevice d;
            d.setSustainToggle(false, 1);
            d.setSustainToggle(true, 16);
            expect(d.getSustainToggle(1) == false);
            expect(d.getSustainToggle(16) == true);
            expect(d.getSustainToggle(7) == true);   // unknown -> default
        }

        beginTest("envelope/mod accessors (attack/decay/release/vibrato/delay) - round-trip + default 0");
        {
            MidiDevice d;
            d.setAttack(15, 1);   d.setAttack(45, 16);
            d.setDecay(25, 1);    d.setDecay(55, 16);
            d.setRelease(35, 1);  d.setRelease(65, 16);
            d.setVibrato(12, 1);  d.setVibrato(72, 16);
            d.setDelay(18, 1);    d.setDelay(88, 16);

            expectEquals(d.getAttack(1), 15);   expectEquals(d.getAttack(16), 45);
            expectEquals(d.getDecay(1), 25);    expectEquals(d.getDecay(16), 55);
            expectEquals(d.getRelease(1), 35);  expectEquals(d.getRelease(16), 65);
            expectEquals(d.getVibrato(1), 12);  expectEquals(d.getVibrato(16), 72);
            expectEquals(d.getDelay(1), 18);    expectEquals(d.getDelay(16), 88);

            expectEquals(d.getAttack(7), 0);
            expectEquals(d.getDecay(7), 0);
            expectEquals(d.getRelease(7), 0);
            expectEquals(d.getVibrato(7), 0);
            expectEquals(d.getDelay(7), 0);
        }

        beginTest("setters ignore channels other than 1 and 16 (no first/second-hand change)");
        {
            MidiDevice d;
            const int b1 = d.getBrightness(1), b16 = d.getBrightness(16);
            d.setBrightness(123, 5);   // channel 5 is neither hand -> ignored
            expectEquals(d.getBrightness(1), b1);
            expectEquals(d.getBrightness(16), b16);
        }

        // ---- Device Index ----

        beginTest("setDeviceIN / getDeviceIndexIN");
        {
            MidiDevice device;
            device.setDeviceIN(3);
            expect(device.getDeviceIndexIN() == 3);
        }

        beginTest("setDeviceOUT / getDeviceIndexOUT");
        {
            MidiDevice device;
            device.setDeviceOUT(5);
            expect(device.getDeviceIndexOUT() == 5);
        }

        // ---- VID / PID / Name ----

        beginTest("setVID / getVID");
        {
            MidiDevice device;
            device.setVID("07cf");
            expect(device.getVID() == "07cf");
        }

        beginTest("setPID / getPID");
        {
            MidiDevice device;
            device.setPID("6803");
            expect(device.getPID() == "6803");
        }

        beginTest("setDeviceName / getName");
        {
            MidiDevice device;
            device.setDeviceName("CT s-300");
            expect(device.getName() == "CT s-300");
        }

        // ---- extractPID ----

        beginTest("extractPID - valid identifier");
        {
            MidiDevice device;
            juce::String identifier = "\\\\?\\SWD#MMDEVAPI#MIDII_VID&pid_6803&SomethingElse";
            expect(device.extractPID("some_prefix_pid_6803_suffix") == "6803");
        }

        beginTest("extractPID - standard USB identifier");
        {
            MidiDevice device;
            expect(device.extractPID("usb_vid_07cf&pid_ABCD&rev_0100") == "ABCD");
        }

        beginTest("extractPID - no pid_ in identifier");
        {
            MidiDevice device;
            expect(device.extractPID("no_product_id_here") == "");
        }

        beginTest("extractPID - empty string");
        {
            MidiDevice device;
            expect(device.extractPID("") == "");
        }

        // ---- extractVID ----

        beginTest("extractVID - valid identifier");
        {
            MidiDevice device;
            expect(device.extractVID("usb_vid_07cf&pid_6803") == "07cf");
        }

        beginTest("extractVID - no vid_ in identifier");
        {
            MidiDevice device;
            expect(device.extractVID("no_vendor_id_here") == "");
        }

        beginTest("extractVID - empty string");
        {
            MidiDevice device;
            expect(device.extractVID("") == "");
        }

        // ---- deviceOpenIN invalid index ----

        beginTest("deviceOpenIN - negative index returns false");
        {
            MidiDevice device;
            expect(device.deviceOpenIN(-1, nullptr) == false);
            expect(device.isOpenIN() == false);
        }

        beginTest("deviceOpenIN - out of range index returns false");
        {
            MidiDevice device;
            expect(device.deviceOpenIN(999, nullptr) == false);
            expect(device.isOpenIN() == false);
        }

        // ---- deviceOpenOUT invalid index ----

        beginTest("deviceOpenOUT - negative index returns false");
        {
            MidiDevice device;
            expect(device.deviceOpenOUT(-1) == false);
            expect(device.isOpenOUT() == false);
        }

        beginTest("deviceOpenOUT - out of range index returns false");
        {
            MidiDevice device;
            expect(device.deviceOpenOUT(999) == false);
            expect(device.isOpenOUT() == false);
        }

        // Crashed the CI runner (0xC0000005): an output that is listed but fails to open leaves no
        // device, and sendMidiCC dereferenced it.
        beginTest("sendMidiCC - with no output device open does nothing");
        {
            constexpr int firstHandChannel   = 1;
            constexpr int volumeController   = 7;
            constexpr int halfVolume         = 64;
            const juce::String noOutputOpen  = "no output should be open";
            MidiDevice device;
            device.sendMidiCC(firstHandChannel, volumeController, halfVolume);   // must not crash
            expect(device.isOpenOUT() == false, noOutputOpen);
        }
    }
};

static MidiDeviceTest midiDeviceTest;
