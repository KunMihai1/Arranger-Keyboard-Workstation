#include <juce_core/juce_core.h>
#include "midiDevicesDB.h"
#include "FileSystemInterface.h"

namespace MidiDevicesDbTestValues
{
    const juce::String testName     = "MidiDevicesDataBase";
    const juce::String testCategory = "Unit";

    const juce::String addDeviceAndQueryItSaveJson = "Add device and query it + save json and expect it to be there";
    const juce::String duplicateDeviceIsIgnored = "Duplicate device is ignored";
    const juce::String updateDevice = "Update device";
    const juce::String updateWithEmptyNameKeepsOldName = "Update with empty name keeps old name";
    const juce::String queryNonExistentDevices = "Query non-existent devices";
    const juce::String addMultipleDevices = "Add multiple devices";
    const juce::String edgeCasesEmptyVIDOrPID = "Edge cases: empty VID or PID";
    const juce::String saveAndLoadJSONInMemory = "Save and load JSON in memory";

    const juce::String mockFile = "mock.json";
    const juce::String ignoredName = "Should Not Overwrite";
    const juce::String updatedName = "Updated Device";
    const juce::String thirdName = "Yet Another Device";
    const juce::String secondName = "Another Device";
    const juce::String deviceName = "Test Device";
    const juce::String emptyVidName = "Empty VID";
    const juce::String emptyPidName = "Empty PID";
    const juce::String reloadedLog = "it has2:";
    const juce::String writtenLog = "it has:";
    const juce::String vendorId = "1234";
    const juce::String productId = "0001";
    const juce::String unknownVendor = "9999";
    const juce::String unknownProduct = "0000";
    const juce::String secondVendor = "abcd";
    const juce::String secondProduct = "0010";
    const juce::String thirdProduct = "0020";
    const juce::String emptyId;   // no id at all
    constexpr int keys49 = 49;
    constexpr int keys61 = 61;
    constexpr int keys76 = 76;
    constexpr int keys88 = 88;
    constexpr int notFound = -1;
}

using namespace MidiDevicesDbTestValues;

class MockFileSystem : public IFileSystem
{
public:
    juce::File getJsonFile() override {
        return juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile(mockFile);
    }
    void writeFile(const juce::File&, const juce::String& text) override { lastWritten = text; }
    juce::String readFile(const juce::File&) override { return lastWritten; }

    juce::String lastWritten;
};

class TestableMidiDevicesDB : public MidiDevicesDataBase
{
public:
    TestableMidiDevicesDB(IFileSystem& fs): MidiDevicesDataBase(fs)
    {
        fileSystem = &fs;

        if (!jsonData.isObject())
            jsonData = juce::var(new juce::DynamicObject());
    }

    void saveJsonFile() override
    {
        if (fileSystem)
            fileSystem->writeFile(fileSystem->getJsonFile(), juce::JSON::toString(jsonData));
        else
            MidiDevicesDataBase::saveJsonFile();
    }

    void loadJsonFile() override
    {
        if (fileSystem)
        {
            juce::String jsonString = fileSystem->readFile(fileSystem->getJsonFile());
            jsonData = juce::JSON::parse(jsonString);
        }
        else
            MidiDevicesDataBase::loadJsonFile();

        if (!jsonData.isObject())
            jsonData = juce::var(new juce::DynamicObject());
    }

private:
    IFileSystem* fileSystem = nullptr;
};

class MidiDevicesDBUnitTest : public juce::UnitTest
{
public:
    MidiDevicesDBUnitTest() : juce::UnitTest (MidiDevicesDbTestValues::testName, MidiDevicesDbTestValues::testCategory) {}

    void runTest() override
    {
        MockFileSystem mockFS;
        TestableMidiDevicesDB db(mockFS);

        beginTest(addDeviceAndQueryItSaveJson);
        {
            db.addDeviceJson(vendorId, productId, deviceName, keys61);

            expect(db.deviceExists(vendorId, productId) == true);
            expect(db.getNrKeysPidVid(vendorId, productId) == keys61);
            expect(db.getDeviceName(vendorId, productId) == deviceName);

            db.saveJsonFile();
            expect(mockFS.lastWritten.contains(deviceName));
        }

        beginTest(duplicateDeviceIsIgnored);
        {
            db.addDeviceJson(vendorId, productId, ignoredName, keys88);
            expect(db.getDeviceName(vendorId, productId) == deviceName);
            expect(db.getNrKeysPidVid(vendorId, productId) == keys61);
        }


        beginTest(updateDevice);
        {
            db.updateDeviceJson(vendorId, productId, updatedName, keys76);
            expect(db.getDeviceName(vendorId, productId) == updatedName);
            expect(db.getNrKeysPidVid(vendorId, productId) == keys76);
        }

        beginTest(updateWithEmptyNameKeepsOldName);
        {
            db.updateDeviceJson(vendorId, productId, emptyId, keys88);
            expect(db.getDeviceName(vendorId, productId) == updatedName);
            expect(db.getNrKeysPidVid(vendorId, productId) == keys88);
        }

        beginTest(queryNonExistentDevices);
        {
            expect(db.deviceExists(unknownVendor, unknownProduct) == false);
            expect(db.getNrKeysPidVid(unknownVendor, unknownProduct) == notFound);
            expect(db.getDeviceName(unknownVendor, unknownProduct) == emptyId);
        }

        beginTest(addMultipleDevices);
        {
            db.addDeviceJson(secondVendor, secondProduct, secondName, keys49);
            db.addDeviceJson(secondVendor, thirdProduct, thirdName, keys61);

            expect(db.deviceExists(secondVendor, secondProduct) == true);
            expect(db.deviceExists(secondVendor, thirdProduct) == true);
            expect(db.getNrKeysPidVid(secondVendor, secondProduct) == keys49);
            expect(db.getNrKeysPidVid(secondVendor, thirdProduct) == keys61);
        }

        beginTest(edgeCasesEmptyVIDOrPID);
        {
            db.addDeviceJson(emptyId, productId, emptyVidName, keys61);
            db.addDeviceJson(vendorId, emptyId, emptyPidName, keys61);

            expect(db.deviceExists(emptyId, productId) == false);
            expect(db.deviceExists(vendorId, emptyId) == false);
        }

        beginTest(saveAndLoadJSONInMemory);
        {
            db.saveJsonFile();
            DBG(writtenLog + mockFS.lastWritten);
            expect(mockFS.lastWritten.contains(updatedName));

            // Simulate new db and load from mock
            TestableMidiDevicesDB db2(mockFS);
            db2.loadJsonFile();

            DBG(reloadedLog + mockFS.lastWritten);
            expect(db2.deviceExists(vendorId, productId) == true);
            expect(db2.getDeviceName(vendorId, productId) == updatedName);
            expect(db2.deviceExists(secondVendor, secondProduct) == true);
            expect(db2.getDeviceName(secondVendor, secondProduct) == secondName);
        }
    }
};


static MidiDevicesDBUnitTest midiDevicesDBUnitTest;