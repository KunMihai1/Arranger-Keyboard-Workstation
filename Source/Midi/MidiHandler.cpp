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

	juce::MidiMessage msg = juce::MidiMessage::controllerEvent(channel, ccNumber, midiValue);
	currentDeviceUSEDout->sendMessageNow(msg);
}

const juce::String& MidiDevice::get_identifier() const
{
	return this->identifier;
}

MidiHandler::MidiHandler(MidiDevice& device, InstrumentHandler* instrumentH) : midiDevice{ device }, instrumentHandler{instrumentH}
{
}

MidiHandler::~MidiHandler()
{
}

void MidiHandler::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
	const juce::ScopedLock lock(midiMutex);

	juce::MidiMessage processedMessage = message;

	// When a chord-zone onset is muted (Korg-style), it must not reach the internal SFZ synth either.
	// Only note-ONs are suppressed; note-offs/CC always pass so a note can never get stuck.
	bool suppressInternalFeed = false;

	if (message.isNoteOn())
	{
		int note = message.getNoteNumber();

		const int channel = channelForHand(note);
		int transposedNote = juce::jlimit(0, 127, note + transposeValue);

		if (note != this->startNoteSetting && note != this->endNoteSetting)
			feedChordNote(note, true);

		// Korg-style chord-zone mute: recognition above still runs, but we don't SOUND this onset when
		// muting. Only note-ONs are gated -- note-offs always pass, so a note can never get stuck.
		const bool muteChordZone = muteChordZoneNote(note);
		suppressInternalFeed = muteChordZone;   // keep the muted onset off the internal SFZ feed too

		float velocity = message.getFloatVelocity();

		float volume = this->midiDevice.getVolume(message.getChannel());
		//DBG("VOLUME="+ juce::String(volume));
		float scaledVelocity = juce::jlimit(0.0f, 1.0f, velocity * volume);

		juce::uint8 velocityByte = juce::MidiMessage::floatValueToMidiByte(scaledVelocity);

		int ok = 0;

		if (auto midiOut = midiDevice.getDeviceOUT().lock())
		{
			if (note != this->startNoteSetting && note != this->endNoteSetting)
			{
				ok = 1;
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+9, velocityByte));
				midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(channel, 0x2000));
				if (! muteChordZone)
					midiOut->sendMessageNow(juce::MidiMessage::noteOn(channel, transposedNote, velocityByte));
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+10, velocityByte));
			}
			else if (note == this->startNoteSetting)
			{
				if (onStartNoteSetting)
					onStartNoteSetting();
			}
			else if (note == this->endNoteSetting)
			{
				if (onEndNoteSetting)
					onEndNoteSetting();
			}
		}

		if (ok)
		{
			listeners.call(&MidiHandlerListener::noteOnReceived, note);
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 91, 80));
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 74, 100));
			if (! muteChordZone)
				listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOn(channel, note, velocityByte));
		}

	}

	if (message.isNoteOff())
	{
		int note = message.getNoteNumber();
		const int channel = channelForHand(note);
		int transposedNote = juce::jlimit(0, 127, note + transposeValue);

		if (note != this->startNoteSetting && note != this->endNoteSetting)
			feedChordNote(note, false);

		juce::uint8 velocityByte = juce::MidiMessage::floatValueToMidiByte(message.getFloatVelocity());

		if (auto midiOut = midiDevice.getDeviceOUT().lock())
		{
			midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, transposedNote, velocityByte));
		}

		listeners.call(&MidiHandlerListener::noteOffReceived, note);
		listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOff(channel, note, velocityByte));
	}

	if (! suppressInternalFeed)
		incomingMidiMessages.addEvent(processedMessage, 0);
}

void MidiHandler::getNextMidiBlock(juce::MidiBuffer& destBuffer, int startSample, int numSamples) {
	//DBG("getNextMidiBlock called");
	const juce::ScopedLock lock(midiMutex);
	destBuffer.addEvents(incomingMidiMessages, startSample, numSamples, 0);
	incomingMidiMessages.clear();
	//DBG("getNextMidiBlock: " << destBuffer.getNumEvents());

}

void MidiHandler::noteOnKeyboard(int note, juce::uint8 velocity) {
	int ok = 0;
	const int channel = channelForHand(note);
	int transposedNote = juce::jlimit(0, 127, note + transposeValue);

	if (note != this->startNoteSetting && note != this->endNoteSetting)
		feedChordNote(note, true);

	const bool muteChordZone = muteChordZoneNote(note);   // Korg-style: drive the chord, don't sound the keys

	if (note != this->startNoteSetting && note!=this->endNoteSetting)
	{
		ok = 1;
	}
	else if(note==this->startNoteSetting)
	{
		if(onStartNoteSetting)
			onStartNoteSetting();
	}
	else if (note == this->endNoteSetting)
	{
		if (onEndNoteSetting)
			onEndNoteSetting();
	}

	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		if (ok)
		{
			midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(channel, 0x2000));
			if (! muteChordZone)
				midiOut->sendMessageNow(juce::MidiMessage::noteOn(channel, transposedNote, velocity));
		}
	}

	if (ok)
	{
		listeners.call(&MidiHandlerListener::noteOnReceived, note);
		if (! muteChordZone)
		{
			listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOn(channel, note, velocity));

			const juce::ScopedLock lock(midiMutex);
			incomingMidiMessages.addEvent(juce::MidiMessage::noteOn(channel, transposedNote, velocity), 0);
		}
	}
}

void MidiHandler::noteOffKeyboard(int note, juce::uint8 velocity) {
	const int channel = channelForHand(note);
	int transposedNote = juce::jlimit(0, 127, note + transposeValue);

	if (note != this->startNoteSetting && note != this->endNoteSetting)
		feedChordNote(note, false);

	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, transposedNote,velocity));

	}
	listeners.call(&MidiHandlerListener::noteOffReceived,note);
	listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOff(channel, note));

	const juce::ScopedLock lock(midiMutex);
	incomingMidiMessages.addEvent(juce::MidiMessage::noteOff(channel, transposedNote, velocity), 0);
} 

void MidiHandler::allOffKeyboard()
{
	for (int i = 0; i < 128; i++)
	{
		if (midiDevice.currentDeviceUSEDout == nullptr) return;

		midiDevice.currentDeviceUSEDout->sendMessageNow(juce::MidiMessage::noteOff(1, i));
		midiDevice.currentDeviceUSEDout->sendMessageNow(juce::MidiMessage::noteOff(16, i));
	}
}

void MidiHandler::setProgramNumber(int toSetNumber, const juce::String& choice) {
	if (choice.toLowerCase() == "left")
		this->programNumberLeftHand = toSetNumber;
	else if (choice.toLowerCase() == "right")
		this->programNumberRightHand = toSetNumber;

	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		if (instrumentHandler)
		{
			const auto& preset = instrumentHandler->getPreset(toSetNumber);

			applyInstrumentPreset(toSetNumber, preset, choice);
			juce::Thread::sleep(5);
		}
	}
}

int MidiHandler::getProgramNumberLeftHand()
{
	return programNumberLeftHand;
}

int MidiHandler::getProgramNumberRightHand()
{
	return programNumberRightHand;
}

void MidiHandler::set_start_end_notes(int start, int end)
{
	this->startNoteSetting = start;
	this->endNoteSetting = end;
}

void MidiHandler::set_left_right_bounds(int left, int right)
{
	this->leftHandBoundSetting = left;
	this->rightHandBoundSetting = right;
}

void MidiHandler::playBackSettingsChanged(const PlayBackSettings& settings)
{
	set_start_end_notes(settings.startNote, settings.endNote);
	set_left_right_bounds(settings.leftHandBound, settings.rightHandBound);
}

void MidiHandler::playBackSettingsTransposeChanged(int transposeValue)
{
	this->transposeValue = transposeValue;
	allOffKeyboard();
}

int MidiHandler::channelForHand(int note) const
{
	return (rightHandBoundSetting != -1 && note >= rightHandBoundSetting) ? 16 : 1;
}

void MidiHandler::setChordMode(ChordMode mode)
{
	chordMode.store(mode);
	chordDetector.setMode(mode);
}

bool MidiHandler::inChordZone(int note) const
{
	if (chordMode.load() == ChordMode::FullKeyboard)
		return true;
	// Split-zone modes (Fingered, Single-Finger): the left-hand zone (below the split). With no
	// split set, the whole keyboard is the left zone (matches channelForHand semantics).
	return (rightHandBoundSetting == -1) ? true : note < rightHandBoundSetting;
}

void MidiHandler::feedChordNote(int note, bool isOn)
{
	if (! inChordZone(note))
		return;

	if (isOn) chordDetector.noteOn(note);
	else      chordDetector.noteOff(note);

	ArrangerChord c = chordDetector.current();

	// Chord Memory: when the zone empties to "no chord", hold the last chord (memory on) instead of
	// telling the engine to revert. With memory off, push the invalid chord so it reverts to the key.
	if (! c.isValid() && chordMemory)
		return;

	// Always forward the current chord. We deliberately do NOT skip "same as last time": the engine
	// resets its chord to the style's home key on every Start, so re-pressing the same chord after a
	// 2nd Play must reach it again. Re-applying an identical chord is cheap and harmless.
	if (onChordChanged)
		onChordChanged(c);
}

int MidiHandler::handlePlayableRange(const juce::String& vid, const juce::String& pid, int nrKeys, bool isKeyboardInput)
{
	
	int isK = isKeyboardInput;
	if (nrKeys<0 && !isKeyboardInput)
	{
		if (onAddCallBack)
		{
			onAddCallBack(vid, pid, [this, vid, pid](const juce::String& name, int keys)
				{
					setPlayableRange(keys);
				});
		}
		return -1;
	}
	else {
		setPlayableRange(nrKeys);
		return 1;
	}
}


void MidiHandler::applyInstrumentPreset(int programNumber, std::vector<std::pair<int, int>> ccValues, const juce::String& choice)
{
	int channel = 1;
	if (choice.toLowerCase() == "right")
		channel = 16;
	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		for (int i = 0; i < 128; ++i)
			midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, i));

		midiOut->sendMessageNow(juce::MidiMessage::programChange(channel, programNumber));
		listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::programChange(channel, programNumber));

	}
}

void MidiHandler::injectMidiMessage(const juce::MidiMessage& msg)
{
	const juce::ScopedLock lock(midiMutex);
	incomingMidiMessages.addEvent(msg, 0);
}

void MidiHandler::injectCC(int channel, int ccNumber, int value)
{
	injectMidiMessage(juce::MidiMessage::controllerEvent(channel, ccNumber, juce::jlimit(0, 127, value)));
}

void MidiHandler::setPlayableRange(int nrKeys)
{
	if (nrKeys == 25)
	{
		midiDevice.set_minNote(36);
		midiDevice.set_maxNote(60);
	}
	else if (nrKeys == 49)
	{
		midiDevice.set_minNote(40);
		midiDevice.set_maxNote(88);
	}
	else if (nrKeys == 61)
	{
		midiDevice.set_minNote(36);
		midiDevice.set_maxNote(96);
	}
	else if (nrKeys == 88)
	{
		midiDevice.set_minNote(21);
		midiDevice.set_maxNote(108);
	}
}
