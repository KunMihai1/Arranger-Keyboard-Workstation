#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiHandler.h"

// ==================================================================
// HARDWARE-DEPENDENT TESTS (Integration)
// These tests require a physical MIDI device to be connected.
// They skip automatically if no device is found.
// ==================================================================

class MidiDeviceHWTest : public juce::UnitTest
{
public:
    MidiDeviceHWTest() : juce::UnitTest("MidiDevice_HW", "Integration") {}

    void runTest() override
    {
        // Helper: dummy callback for opening input devices
        struct DummyCallback : public juce::MidiInputCallback {
            void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override {}
        };

        // Detect available devices once for all hardware tests
        MidiDevice hwDevice;
        std::vector<std::string> inputDevices;
        hwDevice.getAvailableDevicesMidiIN(inputDevices);

        // Check if we have a real device (not just "PC Keyboard" or "No devices found!")
        bool hasInputDevice = false;
        int inputDeviceIndex = -1;
        for (int i = 0; i < (int)inputDevices.size(); ++i)
        {
            juce::String name(inputDevices[i]);
            if (name != "PC Keyboard" && name != "No devices found!")
            {
                hasInputDevice = true;
                inputDeviceIndex = i;
                break;
            }
        }

        std::vector<std::string> outputDevices;
        hwDevice.getAvailableDevicesMidiOUT(outputDevices);

        bool hasOutputDevice = false;
        int outputDeviceIndex = -1;
        for (int i = 0; i < (int)outputDevices.size(); ++i)
        {
            juce::String name(outputDevices[i]);
            if (name != "No devices found!")
            {
                hasOutputDevice = true;
                outputDeviceIndex = i;
                break;
            }
        }

        // A listed output that can't be opened (CI runners list the Windows GS synth but have no MIDI
        // service) is not usable hardware: treat it like "no device" so these tests skip.
        if (hasOutputDevice)
        {
            const juce::String listedButUnopenableMessage = "SKIPPED - MIDI output is listed but cannot be opened";
            MidiDevice probe;
            std::vector<std::string> probeDevices;
            probe.getAvailableDevicesMidiOUT(probeDevices);
            if (probe.deviceOpenOUT(outputDeviceIndex))
            {
                probe.deviceCloseOUT();
            }
            else
            {
                logMessage(listedButUnopenableMessage);
                hasOutputDevice = false;
            }
        }

        // ---- Device Discovery ----

        beginTest("[HW] getAvailableDevicesMidiIN - finds at least one device");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                expect(inputDevices.size() > 1);  // at least one real device + "PC Keyboard"
            }
        }

        beginTest("[HW] getAvailableDevicesMidiOUT - finds at least one device");
        {
            if (!hasOutputDevice) { logMessage("SKIPPED - no MIDI output device connected"); }
            else
            {
                expect(outputDevices.size() >= 1);
            }
        }

        beginTest("[HW] getDeviceNameBasedOnIndex - returns non-empty name for valid input device");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);
                juce::String name = dev.getDeviceNameBasedOnIndex(inputDeviceIndex);
                expect(name.isNotEmpty());
            }
        }

        beginTest("[HW] getDeviceNameBasedOnIndex - returns non-empty name for valid out0put device");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                juce::String name = dev.getDeviceNameBasedOnIndex(inputDeviceIndex,1);
                expect(name.isNotEmpty());
            }
        }

        beginTest("[HW] getDeviceIdentifierBasedOnIndex - returns non-empty identifier");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);
                juce::String id = dev.getDeviceIdentifierBasedOnIndex(inputDeviceIndex);
                expect(id.isNotEmpty());
            }
        }

        // ---- Open/Close INPUT ----

        beginTest("[HW] deviceOpenIN - opens a real device successfully");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);

                DummyCallback callback;
                bool opened = dev.deviceOpenIN(inputDeviceIndex, &callback);
                expect(opened == true);
                expect(dev.isOpenIN() == true);
                expect(dev.get_identifier().isNotEmpty());

                dev.deviceCloseIN();
                expect(dev.isOpenIN() == false);
            }
        }

        beginTest("[HW] deviceOpenIN - opens a real device successfully with 2 consecutive calls");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);

                DummyCallback callback;
                bool opened = dev.deviceOpenIN(inputDeviceIndex, &callback);
                expect(opened == true);
                expect(dev.isOpenIN() == true);
                expect(dev.get_identifier().isNotEmpty());

                bool openedS = dev.deviceOpenIN(inputDeviceIndex, &callback);
                expect(openedS == true);
                expect(dev.isOpenIN() == true);
                expect(dev.get_identifier().isNotEmpty());

                dev.deviceCloseIN();
                expect(dev.isOpenIN() == false);
            }
        }

        beginTest("[HW] deviceCloseIN - closing already closed device is safe");
        {
            MidiDevice dev;
            dev.deviceCloseIN();  // should not crash
            expect(dev.isOpenIN() == false);
        }

        // ---- Open/Close OUTPUT ----

        beginTest("[HW] deviceOpenOUT - opens a real output device successfully");
        {
            if (!hasOutputDevice) { logMessage("SKIPPED - no MIDI output device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);

                bool opened = dev.deviceOpenOUT(outputDeviceIndex);
                expect(opened == true);
                expect(dev.isOpenOUT() == true);

                dev.deviceCloseOUT();
                expect(dev.isOpenOUT() == false);
            }
        }

        beginTest("[HW] deviceCloseOUT - closing already closed device is safe");
        {
            MidiDevice dev;
            dev.deviceCloseOUT();  // should not crash
            expect(dev.isOpenOUT() == false);
        }

        // ---- Reopen after close ----

        beginTest("[HW] deviceOpenIN - reopen after close works");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);

                DummyCallback callback;

                // First open
                expect(dev.deviceOpenIN(inputDeviceIndex, &callback) == true);
                expect(dev.isOpenIN() == true);

                // Close
                dev.deviceCloseIN();
                expect(dev.isOpenIN() == false);

                // Reopen
                dev.getAvailableDevicesMidiIN(devs);
                expect(dev.deviceOpenIN(inputDeviceIndex, &callback) == true);
                expect(dev.isOpenIN() == true);

                dev.deviceCloseIN();
            }
        }

        // ---- changeVolumeInstrument / changeReverbInstrument ----

        beginTest("[HW] changeVolumeInstrument - sends volume CC to output device");
        {
            if (!hasOutputDevice) { logMessage("SKIPPED - no MIDI output device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                dev.deviceOpenOUT(outputDeviceIndex);

                dev.setVolume(75, 1);
                dev.sendMidiCC(1,7,dev.getVolume(1));  // should not crash, sends CC7

                dev.setVolume(50, 16);
                dev.sendMidiCC(16,7,dev.getVolume(16));  // sends CC7 on channel 16

                dev.deviceCloseOUT();
            }
        }

        beginTest("[HW] changeReverbInstrument - sends reverb CC to output device");
        {
            if (!hasOutputDevice) { logMessage("SKIPPED - no MIDI output device connected"); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                dev.deviceOpenOUT(outputDeviceIndex);

                dev.setReverb(60, 1);
                dev.sendMidiCC(1,91,dev.getReverb(1));  // should not crash, sends CC91

                dev.setReverb(30, 16);
                dev.sendMidiCC(16,91,dev.getReverb(16));  // sends CC91 on channel 16

                dev.deviceCloseOUT();
            }
        }

        // ---- getAvailableInputDevicesNameIdentifier ----

        beginTest("[HW] getAvailableInputDevicesNameIdentifier - returns name+identifier pairs");
        {
            if (!hasInputDevice) { logMessage("SKIPPED - no MIDI input device connected"); }
            else
            {
                MidiDevice dev;
                auto pairs = dev.getAvailableInputDevicesNameIdentifier();
                expect((int)pairs.size() >= 1);
                // First real device should have non-empty name and identifier
                expect(pairs[0].first.isNotEmpty());
                expect(pairs[0].second.isNotEmpty());
            }
        }
    }
};

static MidiDeviceHWTest midiDeviceHWTest;
