#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiHandler.h"

namespace MidiDeviceHwTestValues
{
    const juce::String testName     = "MidiDevice_HW";
    const juce::String testCategory = "Integration";

    const juce::String hwGetAvailableDevicesMidiINFindsAtLeastOneDevice = "[HW] getAvailableDevicesMidiIN - finds at least one device";
    const juce::String hwGetAvailableDevicesMidiOUTFindsAtLeastOneDevice = "[HW] getAvailableDevicesMidiOUT - finds at least one device";
    const juce::String hwGetDeviceNameBasedOnIndexReturnsNonEmptyNameFor = "[HW] getDeviceNameBasedOnIndex - returns non-empty name for valid input device";
    const juce::String hwGetDeviceNameBasedOnIndexReturnsNonEmptyNameFor2 = "[HW] getDeviceNameBasedOnIndex - returns non-empty name for valid output device";
    const juce::String hwGetDeviceIdentifierBasedOnIndexReturnsNonEmptyIdentifier = "[HW] getDeviceIdentifierBasedOnIndex - returns non-empty identifier";
    const juce::String hwDeviceOpenINOpensARealDeviceSuccessfully = "[HW] deviceOpenIN - opens a real device successfully";
    const juce::String hwDeviceOpenINOpensARealDeviceSuccessfully2 = "[HW] deviceOpenIN - opens a real device successfully with 2 consecutive calls";
    const juce::String hwDeviceCloseINClosingAlreadyClosedDeviceIs = "[HW] deviceCloseIN - closing already closed device is safe";
    const juce::String hwDeviceOpenOUTOpensARealOutputDevice = "[HW] deviceOpenOUT - opens a real output device successfully";
    const juce::String hwDeviceCloseOUTClosingAlreadyClosedDeviceIs = "[HW] deviceCloseOUT - closing already closed device is safe";
    const juce::String hwDeviceOpenINReopenAfterCloseWorks = "[HW] deviceOpenIN - reopen after close works";
    const juce::String hwChangeVolumeInstrumentSendsVolumeCCToOutput = "[HW] changeVolumeInstrument - sends volume CC to output device";
    const juce::String hwChangeReverbInstrumentSendsReverbCCToOutput = "[HW] changeReverbInstrument - sends reverb CC to output device";
    const juce::String hwGetAvailableInputDevicesNameIdentifierReturnsNameIdentifierPairs = "[HW] getAvailableInputDevicesNameIdentifier - returns name+identifier pairs";

    const juce::String skippedNoInput  = "SKIPPED - no MIDI input device connected";
    const juce::String skippedNoOutput = "SKIPPED - no MIDI output device connected";
    constexpr int noDevice = -1;
    constexpr size_t oneDevice = 1;
    constexpr int firstPair = 0;
    constexpr int leftHand = 1;
    constexpr int rightHand = 16;
    constexpr int volumeController = 7;
    constexpr int reverbController = 91;
    constexpr int leftVolume = 75;
    constexpr int rightVolume = 50;
    constexpr int leftReverb = 60;
    constexpr int rightReverb = 30;
}

using namespace MidiDeviceHwTestValues;

// ==================================================================
// HARDWARE-DEPENDENT TESTS (Integration)
// These tests require a physical MIDI device to be connected.
// They skip automatically if no device is found.
// ==================================================================

class MidiDeviceHWTest : public juce::UnitTest
{
public:
    MidiDeviceHWTest() : juce::UnitTest (MidiDeviceHwTestValues::testName, MidiDeviceHwTestValues::testCategory) {}

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
        int inputDeviceIndex = noDevice;
        for (int deviceIndex {}; deviceIndex < (int)inputDevices.size(); ++deviceIndex)
        {
            juce::String name(inputDevices[deviceIndex]);
            if (name != DeviceNames::pcKeyboard && name != DeviceNames::noDevicesFound)
            {
                hasInputDevice = true;
                inputDeviceIndex = deviceIndex;
                break;
            }
        }

        std::vector<std::string> outputDevices;
        hwDevice.getAvailableDevicesMidiOUT(outputDevices);

        bool hasOutputDevice = false;
        int outputDeviceIndex = noDevice;
        for (int deviceIndex {}; deviceIndex < (int)outputDevices.size(); ++deviceIndex)
        {
            juce::String name(outputDevices[deviceIndex]);
            if (name != DeviceNames::noDevicesFound)
            {
                hasOutputDevice = true;
                outputDeviceIndex = deviceIndex;
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

        beginTest(hwGetAvailableDevicesMidiINFindsAtLeastOneDevice);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
            else
            {
                expect(inputDevices.size() > oneDevice);  // at least one real device + "PC Keyboard"
            }
        }

        beginTest(hwGetAvailableDevicesMidiOUTFindsAtLeastOneDevice);
        {
            if (!hasOutputDevice) { logMessage(skippedNoOutput); }
            else
            {
                expect(outputDevices.size() >= oneDevice);
            }
        }

        beginTest(hwGetDeviceNameBasedOnIndexReturnsNonEmptyNameFor);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiIN(devs);
                juce::String name = dev.getDeviceNameBasedOnIndex(inputDeviceIndex);
                expect(name.isNotEmpty());
            }
        }

        beginTest(hwGetDeviceNameBasedOnIndexReturnsNonEmptyNameFor2);
        {
            if (!hasOutputDevice) { logMessage(skippedNoOutput); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                juce::String name = dev.getDeviceNameBasedOnIndex(outputDeviceIndex,DeviceListChoice::midiOutput);
                expect(name.isNotEmpty());
            }
        }

        beginTest(hwGetDeviceIdentifierBasedOnIndexReturnsNonEmptyIdentifier);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
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

        beginTest(hwDeviceOpenINOpensARealDeviceSuccessfully);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
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

        beginTest(hwDeviceOpenINOpensARealDeviceSuccessfully2);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
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

        beginTest(hwDeviceCloseINClosingAlreadyClosedDeviceIs);
        {
            MidiDevice dev;
            dev.deviceCloseIN();  // should not crash
            expect(dev.isOpenIN() == false);
        }

        // ---- Open/Close OUTPUT ----

        beginTest(hwDeviceOpenOUTOpensARealOutputDevice);
        {
            if (!hasOutputDevice) { logMessage(skippedNoOutput); }
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

        beginTest(hwDeviceCloseOUTClosingAlreadyClosedDeviceIs);
        {
            MidiDevice dev;
            dev.deviceCloseOUT();  // should not crash
            expect(dev.isOpenOUT() == false);
        }

        // ---- Reopen after close ----

        beginTest(hwDeviceOpenINReopenAfterCloseWorks);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
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

        beginTest(hwChangeVolumeInstrumentSendsVolumeCCToOutput);
        {
            if (!hasOutputDevice) { logMessage(skippedNoOutput); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                dev.deviceOpenOUT(outputDeviceIndex);

                dev.setVolume(leftVolume, leftHand);
                dev.sendMidiCC(leftHand,volumeController,dev.getVolume(leftHand));  // should not crash, sends CC7

                dev.setVolume(rightVolume, rightHand);
                dev.sendMidiCC(rightHand,volumeController,dev.getVolume(rightHand));  // sends CC7 on channel 16

                dev.deviceCloseOUT();
            }
        }

        beginTest(hwChangeReverbInstrumentSendsReverbCCToOutput);
        {
            if (!hasOutputDevice) { logMessage(skippedNoOutput); }
            else
            {
                MidiDevice dev;
                std::vector<std::string> devs;
                dev.getAvailableDevicesMidiOUT(devs);
                dev.deviceOpenOUT(outputDeviceIndex);

                dev.setReverb(leftReverb, leftHand);
                dev.sendMidiCC(leftHand,reverbController,dev.getReverb(leftHand));  // should not crash, sends CC91

                dev.setReverb(rightReverb, rightHand);
                dev.sendMidiCC(rightHand,reverbController,dev.getReverb(rightHand));  // sends CC91 on channel 16

                dev.deviceCloseOUT();
            }
        }

        // ---- getAvailableInputDevicesNameIdentifier ----

        beginTest(hwGetAvailableInputDevicesNameIdentifierReturnsNameIdentifierPairs);
        {
            if (!hasInputDevice) { logMessage(skippedNoInput); }
            else
            {
                MidiDevice dev;
                auto pairs = dev.getAvailableInputDevicesNameIdentifier();
                expect(pairs.size() >= oneDevice);
                // First real device should have non-empty name and identifier
                expect(pairs[firstPair].first.isNotEmpty());
                expect(pairs[firstPair].second.isNotEmpty());
            }
        }
    }
};

static MidiDeviceHWTest midiDeviceHWTest;
