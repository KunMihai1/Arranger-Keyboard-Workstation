#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiDevicesDB.h"


MidiDevice::MidiDevice() : currentDeviceIDin{ 0 }, currentDeviceIDout{ 0 }, currentDeviceIDAudioOUT{ 0 }, identifier { "" }, minNote{ 0 }, maxNote{ 127 } {
	this->currentDevicesIN.push_back("PC Keyboard");
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
		vec.push_back({ "No devices found!",""});
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
		devices.push_back("No MIDI INPUT devices found!");
	}
}

void MidiDevice::getAvailableDevicesMidiOUT(std::vector<std::string>& devices) {
	refreshDeviceList(1);
	devices = currentDevicesOUT;
	if (devices.empty())
	{
		devices.clear();
		devices.push_back("No MIDI OUTPUT devices found!");
	}
}

void MidiDevice::getAvailableAudioDevicesOUT(std::vector<std::string>& devices)
{
	refreshDeviceList(2);
	devices = currentDevicesAudioOUT;
	if (devices.empty())
	{
		devices.clear();
		devices.push_back("No AUDIO OUTPUT devices found!");
	}
}



juce::String MidiDevice::getDeviceNameBasedOnIndex(int Index, int choice)
{
	if (choice == 0)
	{
		if (Index >= 0 && Index < this->currentDevicesIN.size())
			return juce::String(this->currentDevicesIN[Index]);
	}
	else if (choice == 1)
	{
		if (Index >= 0 && Index < this->currentDevicesOUT.size())
			return juce::String(this->currentDevicesOUT[Index]);
	}
	return juce::String();
}

juce::String MidiDevice::getDeviceIdentifierBasedOnIndex(int Index, int choice)
{
	if (choice == 0)
	{
		if ((Index >= 0 && Index < this->currentDevicesIN.size()))
		{
			for (int i = 0; i < this->CachedDevicesIN.size(); i++)
			{
				if (i == Index)
					return juce::String(this->CachedDevicesIN[i].identifier);
			}
		}
	}
	else if (choice == 1)
	{
		if ((Index >= 0 && Index < this->currentDevicesOUT.size()))
		{
			for (int i = 0; i < this->CachedDevicesOUT.size(); i++)
			{
				if (i == Index)
					return juce::String(this->CachedDevicesOUT[i].identifier);
			}
		}
	}
	return juce::String();
}


void MidiDevice::refreshDeviceList(int choice)
{
	if (choice == 0)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiInput::getAvailableDevices();
		if (newDevices != this->CachedDevicesIN)
		{
			this->CachedDevicesIN = newDevices;
			this->currentDevicesIN.clear();
			for (int i = 0; i < newDevices.size(); i++)
			{
				if (newDevices[i].name.isNotEmpty())
					this->currentDevicesIN.push_back(newDevices[i].name.toStdString());
			}
			this->devicesChange = true;
			this->currentDevicesIN.push_back("PC Keyboard");
			this->deviceCheckedForUpdateAtLeastOnce = false;
		}
		else
			this->devicesChange = false;

	}
	else if (choice == 1)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiOutput::getAvailableDevices();
		if (newDevices != this->CachedDevicesOUT)
		{
			this->CachedDevicesOUT = newDevices;
			this->currentDevicesOUT.clear();
			for (int i = 0; i < newDevices.size(); i++)
			{
				if (newDevices[i].name.isNotEmpty())
					this->currentDevicesOUT.push_back(newDevices[i].name.toStdString());
			}
			this->devicesChange = true;
		}
		else
			this->devicesChange = false;
	}
	else if (choice == 2)
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
				newDeviceNames.push_back((typeName + ": " + outputName).toStdString());
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
	if (choice == 0)
	{
		juce::Array<juce::MidiDeviceInfo> newDevices = juce::MidiInput::getAvailableDevices();
		vec.clear();
		for (int i = 0; i < newDevices.size(); i++)
		{
			if (newDevices[i].name.isNotEmpty())
				vec.push_back({ newDevices[i].name,newDevices[i].identifier });
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
	int pidStart = identifierReceived.indexOf("pid_");
	if (pidStart != -1)
		return identifierReceived.substring(pidStart + 4, pidStart + 8);
	return juce::String();
}

juce::String MidiDevice::extractVID(const juce::String& identifierReceived)
{
	int vidStart = identifierReceived.indexOf("vid_");
	if (vidStart != -1)
		return identifierReceived.substring(vidStart + 4, vidStart + 8);
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
	return maxNote - minNote+1;
}

bool MidiDevice::deviceOpenIN(int DeviceIndex, juce::MidiInputCallback* CallBack)
{
	if (DeviceIndex < 0 || DeviceIndex >= this->currentDevicesIN.size())
	{
		this->isdeviceOpenIN = false;
		return false;
	}

	if (currentDeviceUSEDin != nullptr)
	{
		currentDeviceUSEDin->stop();
		currentDeviceUSEDin.reset();  
		juce::Thread::sleep(50);      
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
	if (DeviceIndex < 0 || DeviceIndex >= this->currentDevicesOUT.size())
	{
		this->isdeviceOpenOUT = false;
		return false;
	}
	juce::String identifierReceived = getDeviceIdentifierBasedOnIndex(DeviceIndex, 1);
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
	refreshDeviceList(2);

	if (DeviceIndex < 0 || DeviceIndex >= this->CachedDevicesAudioOUT.size())
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

	const auto result = audioDeviceManager.initialise(0, 2, nullptr, false, {}, &setup);

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
	if (channel == 1)
		this->styleSettings->firstHand.volume = vValue;
	else if (channel == 16)
		this->styleSettings->secondHand.volume = vValue;
}

int MidiDevice::getVolume(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.volume;
	else if (channel == 16)
		return this->styleSettings->secondHand.volume;

	return 50.0;
}

void MidiDevice::setReverb(const int rValue, int channel) 
{
	if (channel == 1)
		this->styleSettings->firstHand.reverb = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.reverb = rValue;
}

int MidiDevice::getReverb(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.reverb;
	else if (channel == 16)
		return this->styleSettings->secondHand.reverb;

	return 50.0;
}

void MidiDevice::setBrightness(const int rValue, int channel)
{
	if (channel == 1)
		this->styleSettings->firstHand.brightness = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.brightness = rValue;
}

int MidiDevice::getBrightness(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.brightness;
	else if (channel == 16)
		return this->styleSettings->secondHand.brightness;

	return 50.0;
}

void MidiDevice::setExpression(const int rValue, int channel)
{
	if (channel == 1)
		this->styleSettings->firstHand.expression = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.expression = rValue;
}

int MidiDevice::getExpression(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.expression;
	else if (channel == 16)
		return this->styleSettings->secondHand.expression;

	return 50.0;
}

void MidiDevice::setChorus(const int rValue, int channel)
{
	if (channel == 1)
		this->styleSettings->firstHand.chorus = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.chorus = rValue;
}

int MidiDevice::getChorus(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.chorus;
	else if (channel == 16)
		return this->styleSettings->secondHand.chorus;

	return 50.0;
}

void MidiDevice::setResonance(const int rValue, int channel)
{
	if (channel == 1)
		this->styleSettings->firstHand.resonance = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.resonance = rValue;
}

int MidiDevice::getResonance(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.resonance;
	else if (channel == 16)
		return this->styleSettings->secondHand.resonance;

	return 50.0;
}

void MidiDevice::setSustainToggle(const bool rValue, int channel)
{
	if (channel == 1)
		this->styleSettings->firstHand.sustainToggle = rValue;
	else if (channel == 16)
		this->styleSettings->secondHand.sustainToggle = rValue;
}

bool MidiDevice::getSustainToggle(int channel) const
{
	if (channel == 1)
		return this->styleSettings->firstHand.sustainToggle;
	else if (channel == 16)
		return this->styleSettings->secondHand.sustainToggle;

	return 1;
}


void MidiDevice::setAttack(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.attack = rValue;
	else if (channel == 16) styleSettings->secondHand.attack = rValue;
}

int MidiDevice::getAttack(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.attack;
	else if (channel == 16) return styleSettings->secondHand.attack;
	return 0;
}


void MidiDevice::setDecay(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.decay = rValue;
	else if (channel == 16) styleSettings->secondHand.decay = rValue;
}

int MidiDevice::getDecay(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.decay;
	else if (channel == 16) return styleSettings->secondHand.decay;
	return 0;
}


void MidiDevice::setRelease(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.release = rValue;
	else if (channel == 16) styleSettings->secondHand.release = rValue;
}

int MidiDevice::getRelease(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.release;
	else if (channel == 16) return styleSettings->secondHand.release;
	return 0;
}


void MidiDevice::setVibrato(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.vibrato = rValue;
	else if (channel == 16) styleSettings->secondHand.vibrato = rValue;
}

int MidiDevice::getVibrato(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.vibrato;
	else if (channel == 16) return styleSettings->secondHand.vibrato;
	return 0;
}



void MidiDevice::setDelay(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.delay = rValue;
	else if (channel == 16) styleSettings->secondHand.delay = rValue;
}

int MidiDevice::getDelay(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.delay;
	else if (channel == 16) return styleSettings->secondHand.delay;
	return 0;
}


void MidiDevice::setPan(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.pan = rValue;
	else if (channel == 16) styleSettings->secondHand.pan = rValue;
}

int MidiDevice::getPan(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.pan;
	else if (channel == 16) return styleSettings->secondHand.pan;
	return 0;
}


void MidiDevice::setDistortion(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.distortion = rValue;
	else if (channel == 16) styleSettings->secondHand.distortion = rValue;
}

int MidiDevice::getDistortion(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.distortion;
	else if (channel == 16) return styleSettings->secondHand.distortion;
	return 0;
}


void MidiDevice::setFilterTrack(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.filterTrack = rValue;
	else if (channel == 16) styleSettings->secondHand.filterTrack = rValue;
}

int MidiDevice::getFilterTrack(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.filterTrack;
	else if (channel == 16) return styleSettings->secondHand.filterTrack;
	return 0;
}


void MidiDevice::setTremolo(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.tremolo = rValue;
	else if (channel == 16) styleSettings->secondHand.tremolo = rValue;
}

int MidiDevice::getTremolo(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.tremolo;
	else if (channel == 16) return styleSettings->secondHand.tremolo;
	return 0;
}


void MidiDevice::setRandomMod(const int rValue, int channel)
{
	if (channel == 1) styleSettings->firstHand.randomMod = rValue;
	else if (channel == 16) styleSettings->secondHand.randomMod = rValue;
}

int MidiDevice::getRandomMod(int channel) const
{
	if (channel == 1) return styleSettings->firstHand.randomMod;
	else if (channel == 16) return styleSettings->secondHand.randomMod;
	return 0;
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
		juce::jlimit(0, 127, value)
		);

	if (channel < 1 || channel > 16)
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
