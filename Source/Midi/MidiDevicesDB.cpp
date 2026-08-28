/*
  ==============================================================================

    midiDevicesDB.cpp
    Created: 26 Mar 2025 2:01:51am
    Author:  Kisuke

  ==============================================================================
*/

#include "midiDevicesDB.h"
#include "AppInfo.h"

MidiDevicesDataBase::MidiDevicesDataBase()
{
	jsonEnsureExistance();
	loadJsonFile();
}

MidiDevicesDataBase::MidiDevicesDataBase(IFileSystem& fs): fileSystem{&fs}
{
}

MidiDevicesDataBase::~MidiDevicesDataBase()
{
}

void MidiDevicesDataBase::populateInitialDevices()
{
	addDeviceJson("07cf", "6803", "CT s-300", 61);
	addDeviceJson("0667", "1310", "Korg PA100", 61);
	addDeviceJson("0667", "1311", "Korg PA200", 61);
	addDeviceJson("0667", "1312", "Korg PA300", 61);
	addDeviceJson("0667", "1313", "Korg PA4X", 61);
	addDeviceJson("0667", "1314", "Korg PA4X", 76);
	addDeviceJson("0667", "1315", "Korg PA4X", 88);
	addDeviceJson("0667", "1316", "Korg PA5X", 61);
	addDeviceJson("0667", "1317", "Korg PA5X", 76);
	addDeviceJson("0667", "1318", "Korg PA5X", 88);
	addDeviceJson("0582", "7010", "Roland FP-30", 88);
	addDeviceJson("0582", "7011", "Roland FP-60", 88);
	addDeviceJson("0582", "10B0", "Roland A-49", 49);
	addDeviceJson("0582", "10b1", "Roland A-61", 61);
	addDeviceJson("0582", "10b2", "Roland A-88", 88);
	addDeviceJson("0582", "10f0", "Roland RD-2000", 88);
	addDeviceJson("0582", "1000", "Roland Juno-DS61", 61);
	addDeviceJson("0582", "1001", "Roland Juno-DS88", 88);
	addDeviceJson("0582", "7060", "Roland FA-06", 61);
	addDeviceJson("0582", "7070", "Roland FA-08", 88);
	addDeviceJson("0582", "1110", "Roland GO:KEYS", 61);
	addDeviceJson("0582", "1120", "Roland GO:PIANO", 61);
	addDeviceJson("09c4", "0100", "Casio Privia PX-160", 88);
	addDeviceJson("09c4", "0101", "Casio Privia PX-860", 88);
	addDeviceJson("09c4", "0111", "Casio CTK-3500", 61);
	addDeviceJson("09c4", "0122", "Casio WK-8000", 88);
	addDeviceJson("09c4", "0131", "Casio LK-280", 61);
	addDeviceJson("0763", "0001", "M-Audio Keystation 88 Mk3", 88);
	addDeviceJson("0763", "1010", "M-Audio Oxygen 61 Mk5", 61);
	addDeviceJson("1235", "1000", "Novation Launchkey 49 Mk3", 49);
}

void MidiDevicesDataBase::jsonEnsureExistance()
{
	juce::File jsonFile = getJsonFile();
	if (!jsonFile.existsAsFile())
	{
		jsonData = juce::var(new juce::DynamicObject());
		populateInitialDevices();
		saveJsonFile();
	}
}

void MidiDevicesDataBase::loadJsonFile()
{
	if (fileSystem) // mock file system provided
	{
		juce::String jsonString = fileSystem->readFile(fileSystem->getJsonFile());
		jsonData = juce::JSON::parse(jsonString);
	}
	else
	{
		juce::File jsonFile = getJsonFile();
		if (!jsonFile.existsAsFile())
			return;
		juce::String jsonString = jsonFile.loadFileAsString();
		jsonData = juce::JSON::parse(jsonString);
		if (!jsonData.isObject())
			jsonData = juce::var{}; 
	}
}

void MidiDevicesDataBase::saveJsonFile()
{
	if (fileSystem) // test mode
	{
		DBG("in testing mode");
		fileSystem->writeFile(fileSystem->getJsonFile(), juce::JSON::toString(jsonData));
	}
	else {
		juce::File jsonFile = getJsonFile();
		jsonFile.replaceWithText(juce::JSON::toString(jsonData));
	}
}

void MidiDevicesDataBase::addDeviceJson(const juce::String& vid, const juce::String& pid, const juce::String& name, int numKeys)
{
	if (!jsonData.isObject())
	{
		jsonData = juce::var(new juce::DynamicObject());
	}

	juce::DynamicObject* rootObject = jsonData.getDynamicObject();

	if (!rootObject)
	{
		return;
	}

	juce::String key = vid + pid;

	if (key.isEmpty())
		return;
	if (rootObject->hasProperty(key))
		return;

	auto* newDevice = new juce::DynamicObject();
	newDevice->setProperty("name", name);
	newDevice->setProperty("keys", numKeys);

	rootObject->setProperty(key, juce::var(newDevice));

	
	saveJsonFile();
}

void MidiDevicesDataBase::updateDeviceJson(const juce::String& vid, const juce::String pid, const juce::String name, int numKeys)
{
	if (!jsonData.isObject())
		return;

	auto* rootObject = jsonData.getDynamicObject();
	if (!rootObject)
		return ;

	juce::String key = vid + pid;
	if (key.isEmpty())
		return ;

	if (!rootObject->hasProperty(key))
		return ;	

	auto deviceVar = rootObject->getProperty(key);
	if (!deviceVar.isObject())
		return ;

	auto* deviceObj = deviceVar.getDynamicObject();
	if (!deviceObj)
		return ;

	if(!name.isEmpty())
		deviceObj->setProperty("name", name);
	deviceObj->setProperty("keys", numKeys);

	saveJsonFile();
}

bool MidiDevicesDataBase::deviceExists(const juce::String& VID, const juce::String& PID)
{
	if (VID.isEmpty() || PID.isEmpty()) 
		return false;

	if (!jsonData.isObject())
		return false;

	auto* rootObject = jsonData.getDynamicObject();
	if (!rootObject)
		return false;

	juce::String key = VID + PID;
	if (key.isEmpty())
		return false;

	if (!rootObject->hasProperty(key))
		return false;

	return rootObject->hasProperty(key);
}

int MidiDevicesDataBase::getNrKeysPidVid(const juce::String& vid, const juce::String& pid)
{
	if (!jsonData.isObject())
		return -1;

	auto* rootObject = jsonData.getDynamicObject();
	if (!rootObject)
		return -1;

	juce::String key = vid + pid;
	if (key.isEmpty())
		return -1;

	if (!rootObject->hasProperty(key))
		return -1;

	auto deviceVar = rootObject->getProperty(key);
	if (!deviceVar.isObject())
		return -2;

	auto* deviceObj = deviceVar.getDynamicObject();
	if (!deviceObj)
		return -1;

	return static_cast<int>(deviceObj->getProperty("keys"));
}

juce::String MidiDevicesDataBase::getDeviceName(const juce::String& vid, const juce::String& pid)
{
	if (!jsonData.isObject())
		return "";

	auto* rootObject = jsonData.getDynamicObject();
	if (!rootObject)
		return "";

	juce::String key = vid + pid;
	if (key.isEmpty())
		return "";

	if (!rootObject->hasProperty(key))
		return "";

	auto deviceVar = rootObject->getProperty(key);
	if (!deviceVar.isObject())
		return "";

	auto* deviceObj = deviceVar.getDynamicObject();
	if (!deviceObj)
		return "";
	
	return deviceObj->getProperty("name").toString();
}

juce::File MidiDevicesDataBase::getAppDataFolder()
{
	juce::File appDataFolder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile(AppInfo::appName);

	if (!appDataFolder.exists())
		appDataFolder.createDirectory();

	return appDataFolder;
}

juce::File MidiDevicesDataBase::getJsonFile()
{
	return getAppDataFolder().getChildFile("myDevices.json");
}