#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiDevicesDB.h"

namespace MidiDeviceValues
{
	constexpr int noDeviceIndex = 0;
	constexpr int defaultHandLevel = 50;            // volume/reverb/... asked for a channel that is not a hand
	constexpr bool defaultSustainToggle = true;
	constexpr int inputReopenPauseMs = 50;           // let the driver release the old input first
	constexpr int noInputChannels  = 0;
	constexpr int stereoOutputChannels = 2;

	// USB identifiers look like "...vid_07cf&pid_6803...": a tag followed by four hex digits.
	const juce::String productIdTag = "pid_";
	const juce::String vendorIdTag  = "vid_";
	constexpr int usbIdTagLength = 4;
	constexpr int usbIdEnd       = 8;               // tag + four hex digits
	constexpr int tagNotFound    = -1;
	constexpr int noteSpanIncludesBothEnds = 1;   // keys from lowest to highest note, both counted

	const juce::String noMidiInputDevicesFound   = "No MIDI INPUT devices found!";
	const juce::String noMidiOutputDevicesFound  = "No MIDI OUTPUT devices found!";
	const juce::String noAudioOutputDevicesFound = "No AUDIO OUTPUT devices found!";
	const juce::String deviceTypeSeparator       = ": ";
}

using namespace MidiDeviceValues;


MidiDevice::MidiDevice() : currentDeviceIDin{ noDeviceIndex }, currentDeviceIDout{ noDeviceIndex }, currentDeviceIDAudioOUT{ noDeviceIndex }, identifier {}, minNote{ MidiFacts::lowestNote }, maxNote{ MidiFacts::highestNote } {
	this->currentDevicesIN.push_back(DeviceNames::pcKeyboard.toStdString());
	styleSettings = std::make_unique<StyleSettings>();
}

MidiDevice::~MidiDevice()
{
	audioDeviceManager.closeAudioDevice();

	this->CachedDevicesIN.clear();
	this->currentDevicesIN.clear();
	this->CachedDevicesOUT.clear();
	this->currentDevicesOUT.clear();
	this->CachedDevicesAudioOUT.clear();
	this->currentDevicesAudioOUT.clear();
	this->audioDeviceTypes.clear();
	if (this -> currentDeviceUSEDin)
	{
		this->currentDeviceUSEDin->stop();
		currentDeviceUSEDin.reset();
	}
	if (this->currentDeviceUSEDout)
		this->currentDeviceUSEDout.reset();
}

std::vector<std::pair<juce::String, juce::String>> MidiDevice::getAvailableInputDevicesNameIdentifier()
{
	std::vector<std::pair<juce::String, juce::String>> vec;
	refreshDeviceListNew(vec);
	if (vec.empty())
	{
		vec.clear();
		vec.push_back({ DeviceNames::noDevicesFound, {} });
	}
	return vec;
}

void MidiDevice::getAvailableDevicesMidiIN(std::vector<std::string>& devices)
{
	refreshDeviceList();
	devices = currentDevicesIN;
	if (devices.empty())
	{
		devices.clear();
		devices.push_back(noMidiInputDevicesFound.toStdString());
	}
}

void MidiDevice::getAvailableDevicesMidiOUT(std::vector<std::string>& devices) {
	refreshDeviceList(DeviceListChoice::midiOutput);
	devices = currentDevicesOUT;
	if (devices.empty())
	{
		devices.clear();
		devices.push_back(noMidiOutputDevicesFound.toStdString());
	}
}

void MidiDevice::getAvailableAudioDevicesOUT(std::vector<std::string>& devices)
{
	refreshDeviceList(DeviceListChoice::audioOutput);
	devices = currentDevicesAudioOUT;
	if (devices.empty())
	{
		devices.clear();
		devices.push_back(noAudioOutputDevicesFound.toStdString());
	}
}



juce::String MidiDevice::getDeviceNameBasedOnIndex(int Index, int choice)
{
	if (choice == DeviceListChoice::midiInput)
	{
		if (Index >= int {} && Index < this->currentDevicesIN.size())
			return juce::String(this->currentDevicesIN[Index]);
	}
	else if (choice == DeviceListChoice::midiOutput)
	{
		if (Index >= int {} && Index < this->currentDevicesOUT.size())
			return juce::String(this->currentDevicesOUT[Index]);
	}
	return juce::String();
}

juce::String MidiDevice::getDeviceIdentifierBasedOnIndex(int Index, int choice)
{
	if (choice == DeviceListChoice::midiInput)
	{
		if ((Index >= int {} && Index < this->currentDevicesIN.size()))
		{
			for (int deviceIndex {}; deviceIndex < this->CachedDevicesIN.size(); deviceIndex++)
			{
				if (deviceIndex == Index)
					return juce::String(this->CachedDevicesIN[deviceIndex].identifier);
			}
		}
	}
	else if (choice == DeviceListChoice::midiOutput)
	{
		if ((Index >= int {} && Index < this->currentDevicesOUT.size()))
		{
			for (int deviceIndex {}; deviceIndex < this->CachedDevicesOUT.size(); deviceIndex++)
			{
				if (deviceIndex == Index)
					return juce::String(this->CachedDevicesOUT[deviceIndex].identifier);
			}
		}
	}
	return juce::String();
}


void MidiDevice::refreshDeviceList(int choice)
{
	if (choice == DeviceListChoice::midiInput)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiInput::getAvailableDevices();
		if (newDevices != this->CachedDevicesIN)
		{
			this->CachedDevicesIN = newDevices;
			this->currentDevicesIN.clear();
			for (int deviceIndex {}; deviceIndex < newDevices.size(); deviceIndex++)
			{
				if (newDevices[deviceIndex].name.isNotEmpty())
					this->currentDevicesIN.push_back(newDevices[deviceIndex].name.toStdString());
			}
			this->devicesChange = true;
			this->currentDevicesIN.push_back(DeviceNames::pcKeyboard.toStdString());
			this->deviceCheckedForUpdateAtLeastOnce = false;
		}
		else
			this->devicesChange = false;

	}
	else if (choice == DeviceListChoice::midiOutput)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiOutput::getAvailableDevices();
		if (newDevices != this->CachedDevicesOUT)
		{
			this->CachedDevicesOUT = newDevices;
			this->currentDevicesOUT.clear();
			for (int deviceIndex {}; deviceIndex < newDevices.size(); deviceIndex++)
			{
				if (newDevices[deviceIndex].name.isNotEmpty())
					this->currentDevicesOUT.push_back(newDevices[deviceIndex].name.toStdString());
			}
			this->devicesChange = true;
		}
		else
			this->devicesChange = false;
	}
	else if (choice == DeviceListChoice::audioOutput)
	{
		if (!audioDeviceTypesInitialized)
		{
			audioDeviceManager.createAudioDeviceTypes(audioDeviceTypes);
			audioDeviceTypesInitialized = true;
		}

		std::vector<std::string> newDeviceNames;
		std::vector<AudioOutputDeviceInfo> newDevices;

		for (auto* deviceType : audioDeviceTypes)
		{
			if (deviceType == nullptr)
				continue;

			deviceType->scanForDevices();

			const auto typeName = deviceType->getTypeName();
			const auto outputNames = deviceType->getDeviceNames(false);

			for (const auto& outputName : outputNames)
			{
				if (outputName.isEmpty())
					continue;

				newDevices.push_back({ typeName, outputName });
				newDeviceNames.push_back((typeName + deviceTypeSeparator + outputName).toStdString());
			}
		}

		if (newDevices != this->CachedDevicesAudioOUT)
		{
			this->CachedDevicesAudioOUT = newDevices;
			this->currentDevicesAudioOUT = newDeviceNames;
			this->devicesChange = true;
		}
		else
			this->devicesChange = false;

	}
}

void MidiDevice::refreshDeviceListNew(std::vector<std::pair<juce::String, juce::String>>& vec, int choice)
{
	if (choice == DeviceListChoice::midiInput)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiInput::getAvailableDevices();
		vec.clear();
		for (int deviceIndex {}; deviceIndex < newDevices.size(); deviceIndex++)
		{
			if (newDevices[deviceIndex].name.isNotEmpty())
				vec.push_back({ newDevices[deviceIndex].name,newDevices[deviceIndex].identifier });
		}

	}
}

void MidiDevice::set_minNote(const int minNoteReceived)
{
	this->minNote = minNoteReceived;
}

void MidiDevice::set_maxNote(const int maxNoteReceived)
{
	this->maxNote = maxNoteReceived;
}

juce::String MidiDevice::extractPID(const juce::String& identifierReceived)
{
	int pidStart = identifierReceived.indexOf(productIdTag);
	if (pidStart != tagNotFound)
		return identifierReceived.substring(pidStart + usbIdTagLength, pidStart + usbIdEnd);
	return juce::String();
}

juce::String MidiDevice::extractVID(const juce::String& identifierReceived)
{
	int vidStart = identifierReceived.indexOf(vendorIdTag);
	if (vidStart != tagNotFound)
		return identifierReceived.substring(vidStart + usbIdTagLength, vidStart + usbIdEnd);
	return juce::String();
}

void MidiDevice::setVID(const juce::String& VID)
{
	this->VID = VID;
}

void MidiDevice::setPID(const juce::String& PID)
{
	this->PID = PID;
}

void MidiDevice::setDeviceName(const juce::String& name)
{
	this->name = name;
}

juce::String MidiDevice::getVID()
{
	return this->VID;
}

juce::String MidiDevice::getPID()
{
	return this->PID;
}

juce::String MidiDevice::getName()
{
	return this->name;
}

int MidiDevice::getNrInputActualDevices()
{
	return juce::MidiInput::getAvailableDevices().size();
}

int MidiDevice::getNrKeysAfterInitialized()
{
	return maxNote - minNote + noteSpanIncludesBothEnds;
}

bool MidiDevice::deviceOpenIN(int DeviceIndex, juce::MidiInputCallback* CallBack)
{
	if (DeviceIndex < int {} || DeviceIndex >= this->currentDevicesIN.size())
	{
		this->isdeviceOpenIN = false;
		return false;
	}

	if (currentDeviceUSEDin != nullptr)
	{
		currentDeviceUSEDin->stop();
		currentDeviceUSEDin.reset();  
		juce::Thread::sleep(inputReopenPauseMs);      
	}

	juce::String identifierReceived = getDeviceIdentifierBasedOnIndex(DeviceIndex);
	auto uniqueIn = juce::MidiInput::openDevice(identifierReceived, CallBack);
	currentDeviceUSEDin = std::shared_ptr<juce::MidiInput>(std::move(uniqueIn));
	
	if (currentDeviceUSEDin != nullptr)
	{
		currentDeviceUSEDin->start();
		this->identifier = identifierReceived;
		isdeviceOpenIN = true;
		return true;
	}
	this->isdeviceOpenIN = false;
	return false;
}

bool MidiDevice::deviceOpenOUT(int DeviceIndex) {
	if (DeviceIndex < int {} || DeviceIndex >= this->currentDevicesOUT.size())
	{
		this->isdeviceOpenOUT = false;
		return false;
	}
	juce::String identifierReceived = getDeviceIdentifierBasedOnIndex(DeviceIndex, DeviceListChoice::midiOutput);
	auto uniqueOut = juce::MidiOutput::openDevice(identifierReceived);
	currentDeviceUSEDout = std::shared_ptr<juce::MidiOutput>(std::move(uniqueOut));
	if (currentDeviceUSEDout != nullptr)
	{
		this->isdeviceOpenOUT = true;
		return true;
	}
	this->isdeviceOpenOUT = false;
	return false;
}

bool MidiDevice::deviceOpenAudioOUT(int DeviceIndex)
{
	refreshDeviceList(DeviceListChoice::audioOutput);

	if (DeviceIndex < int {} || DeviceIndex >= this->CachedDevicesAudioOUT.size())
	{
		this->isdeviceOpenAudioOUT = false;
		return false;
	}

	const auto& selectedDevice = this->CachedDevicesAudioOUT[DeviceIndex];

	deviceCloseAudioOUT();

	audioDeviceManager.setCurrentAudioDeviceType(selectedDevice.typeName, true);

	juce::AudioDeviceManager::AudioDeviceSetup setup;
	audioDeviceManager.getAudioDeviceSetup(setup);

	setup.inputDeviceName = {};
	setup.outputDeviceName = selectedDevice.deviceName;
	setup.useDefaultInputChannels = false;
	setup.useDefaultOutputChannels = true;

	const auto result = audioDeviceManager.initialise(noInputChannels, stereoOutputChannels, nullptr, false, {}, &setup);

	if (result.isNotEmpty() || audioDeviceManager.getCurrentAudioDevice() == nullptr)
	{
		this->isdeviceOpenAudioOUT = false;
		return false;
	}

	this->currentDeviceIDAudioOUT = DeviceIndex;
	this->isdeviceOpenAudioOUT = true;
	return true;
}

bool MidiDevice::isOpenIN() const
{
	return this->isdeviceOpenIN;
}

bool MidiDevice::isOpenOUT() const
{
	return this->isdeviceOpenOUT;
}

bool MidiDevice::isOpenAudioOUT() const
{
	return this->isdeviceOpenAudioOUT;
}

void MidiDevice::deviceCloseIN()
{
	if (this->currentDeviceUSEDin != nullptr)
	{
		this->currentDeviceUSEDin->stop();
		this->currentDeviceUSEDin = nullptr;
		this->isdeviceOpenIN = false;
	}
}

void MidiDevice::deviceCloseOUT()
{
	if (this->currentDeviceUSEDout != nullptr)
	{

		this->currentDeviceUSEDout = nullptr;
		this->isdeviceOpenOUT = false;
	}
}

void MidiDevice::deviceCloseAudioOUT()
{
	audioDeviceManager.closeAudioDevice();
	isdeviceOpenAudioOUT = false;
}

int MidiDevice::getDeviceIndexIN() const
{
	return this->currentDeviceIDin;
}

void MidiDevice::setDeviceIN(const int index) {
	this->currentDeviceIDin = index;
}

int MidiDevice::getDeviceIndexOUT() const {
	return this->currentDeviceIDout;
}

void MidiDevice::setDeviceOUT(const int index) {
	this->currentDeviceIDout = index;
}

void MidiDevice::setDeviceAudioOUT(const int index)
{
	this->currentDeviceIDAudioOUT = index;
}

int MidiDevice::getDeviceIndexAudioOUT() const
{
	return this->currentDeviceIDAudioOUT;
}

juce::AudioDeviceManager& MidiDevice::getAudioDeviceManager()
{
	return this->audioDeviceManager;
}

std::weak_ptr<juce::MidiInput> MidiDevice::getDeviceIN() const
{
	return this->currentDeviceUSEDin;
}

std::weak_ptr<juce::MidiOutput> MidiDevice::getDeviceOUT() const
{
	return this->currentDeviceUSEDout;
}

void MidiDevice::setVolume(const int vValue, int channel) 
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.volume = vValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.volume = vValue;
}

int MidiDevice::getVolume(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.volume;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.volume;

	return defaultHandLevel;
}

void MidiDevice::setReverb(const int rValue, int channel) 
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.reverb = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.reverb = rValue;
}

int MidiDevice::getReverb(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.reverb;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.reverb;

	return defaultHandLevel;
}

void MidiDevice::setBrightness(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.brightness = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.brightness = rValue;
}

int MidiDevice::getBrightness(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.brightness;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.brightness;

	return defaultHandLevel;
}

void MidiDevice::setExpression(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.expression = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.expression = rValue;
}

int MidiDevice::getExpression(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.expression;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.expression;

	return defaultHandLevel;
}

void MidiDevice::setChorus(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.chorus = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.chorus = rValue;
}

int MidiDevice::getChorus(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.chorus;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.chorus;

	return defaultHandLevel;
}

void MidiDevice::setResonance(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.resonance = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.resonance = rValue;
}

int MidiDevice::getResonance(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.resonance;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.resonance;

	return defaultHandLevel;
}

void MidiDevice::setSustainToggle(const bool rValue, int channel)
{
	if (channel == HandChannels::leftHand)
		this->styleSettings->firstHand.sustainToggle = rValue;
	else if (channel == HandChannels::rightHand)
		this->styleSettings->secondHand.sustainToggle = rValue;
}

bool MidiDevice::getSustainToggle(int channel) const
{
	if (channel == HandChannels::leftHand)
		return this->styleSettings->firstHand.sustainToggle;
	else if (channel == HandChannels::rightHand)
		return this->styleSettings->secondHand.sustainToggle;

	return defaultSustainToggle;
}


void MidiDevice::setAttack(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.attack = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.attack = rValue;
}

int MidiDevice::getAttack(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.attack;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.attack;
	return {};
}


void MidiDevice::setDecay(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.decay = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.decay = rValue;
}

int MidiDevice::getDecay(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.decay;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.decay;
	return {};
}


void MidiDevice::setRelease(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.release = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.release = rValue;
}

int MidiDevice::getRelease(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.release;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.release;
	return {};
}


void MidiDevice::setVibrato(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.vibrato = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.vibrato = rValue;
}

int MidiDevice::getVibrato(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.vibrato;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.vibrato;
	return {};
}



void MidiDevice::setDelay(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.delay = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.delay = rValue;
}

int MidiDevice::getDelay(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.delay;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.delay;
	return {};
}


void MidiDevice::setPan(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.pan = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.pan = rValue;
}

int MidiDevice::getPan(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.pan;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.pan;
	return {};
}


void MidiDevice::setDistortion(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.distortion = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.distortion = rValue;
}

int MidiDevice::getDistortion(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.distortion;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.distortion;
	return {};
}


void MidiDevice::setFilterTrack(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.filterTrack = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.filterTrack = rValue;
}

int MidiDevice::getFilterTrack(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.filterTrack;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.filterTrack;
	return {};
}


void MidiDevice::setTremolo(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.tremolo = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.tremolo = rValue;
}

int MidiDevice::getTremolo(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.tremolo;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.tremolo;
	return {};
}


void MidiDevice::setRandomMod(const int rValue, int channel)
{
	if (channel == HandChannels::leftHand) styleSettings->firstHand.randomMod = rValue;
	else if (channel == HandChannels::rightHand) styleSettings->secondHand.randomMod = rValue;
}

int MidiDevice::getRandomMod(int channel) const
{
	if (channel == HandChannels::leftHand) return styleSettings->firstHand.randomMod;
	else if (channel == HandChannels::rightHand) return styleSettings->secondHand.randomMod;
	return {};
}



int MidiDevice::get_minNote() const
{
	return this->minNote;
}

int MidiDevice::get_maxNote() const
{
	return this->maxNote;
}

void MidiDevice::sendMidiCC(int channel, int ccNumber, int value)
{
	juce::uint8 midiValue = static_cast<juce::uint8>(
		juce::jlimit(MidiFacts::lowestControllerValue, MidiFacts::highestControllerValue, value)
		);

	if (channel < MidiFacts::lowestChannel || channel > MidiFacts::highestChannel)
		return;

	// Callers check isOpenOUT() first; a failed open leaves no device, and that must not crash.
	if (currentDeviceUSEDout == nullptr)
		return;

	juce::MidiMessage msg = juce::MidiMessage::controllerEvent(channel, ccNumber, midiValue);
	currentDeviceUSEDout->sendMessageNow(msg);
}

const juce::String& MidiDevice::get_identifier() const
{
	return this->identifier;
}
