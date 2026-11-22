#include <juce_core/juce_core.h>
#include "addDeviceWindow.h"

namespace AddDeviceWindowTestValues
{
    const juce::String testName     = "AddDeviceWindow";
    const juce::String testCategory = "Unit";

    const juce::String validInputCallsBack = "Callback is called with valid input";
    const juce::String invalidInputSkipped = "Callback not called with invalid input";

    const juce::String vendorId     = "VID123";
    const juce::String productId    = "PID456";
    constexpr int      testingMode  = 1;      // no message boxes in tests
    const juce::String deviceName   = "MyTestDevice";
    const juce::String validName    = "ValidName";
    const juce::String keyCountText = "61";
    constexpr int      keyCount     = 61;
    const juce::String zeroKeysText = "0";

    const juce::String expectCallback      = "Expected onAddDevice callback to be called";
    const juce::String expectNameMatches   = "Expected device name to match input";
    const juce::String expectKeysMatch     = "Expected number of keys to match input";
    const juce::String noCallbackEmptyName = "Callback should not be called with empty name";
    const juce::String noCallbackZeroKeys  = "Callback should not be called with zero keys";
}

class AddDeviceWindowTest : public juce::UnitTest
{
public:
    AddDeviceWindowTest() : juce::UnitTest(AddDeviceWindowTestValues::testName, AddDeviceWindowTestValues::testCategory) {}

    void runTest() override
    {
        using namespace AddDeviceWindowTestValues;

        beginTest(validInputCallsBack);

        AddDeviceWindow window(vendorId, productId, testingMode);

        bool callbackCalled = false;
        juce::String addedName;
        int addedKeys {};

        // Assign a lambda to capture callback arguments
        window.onAddDevice = [&](const juce::String& name, int keys)
        {
            callbackCalled = true;
            addedName = name;
            addedKeys = keys;
        };

        window.getNameEditor().setText(deviceName);
        window.getKeysEditor().setText(keyCountText);

        if (window.getAddButton().onClick)
            window.getAddButton().onClick();

        expect(callbackCalled, expectCallback);
        expect(addedName == deviceName, expectNameMatches);
        expect(addedKeys == keyCount, expectKeysMatch);

        beginTest(invalidInputSkipped);

        callbackCalled = false;

        window.getNameEditor().setText({});
        window.getKeysEditor().setText(keyCountText);
        if (window.getAddButton().onClick)
            window.getAddButton().onClick();
        expect(!callbackCalled, noCallbackEmptyName);

        window.getNameEditor().setText(validName);
        window.getKeysEditor().setText(zeroKeysText);
        if (window.getAddButton().onClick)
            window.getAddButton().onClick();
        expect(!callbackCalled, noCallbackZeroKeys);
    }
};


static AddDeviceWindowTest addDeviceWindowTest;
