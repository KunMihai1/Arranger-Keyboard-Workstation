
#pragma once

#include <JuceHeader.h>
#include "styleSettingsEntry.h"
#include "MidiDevicesDB.h"
#include "MidiHandlerAbstractSubject.h"
#include "InstrumentHandler.h"
#include "DisplayListener.h"
#include "Arranger/ChordDetector.h"
#include <atomic>
#include <functional>

/** Facts of the MIDI protocol itself, shared by every MIDI class. */
namespace MidiFacts
{
	constexpr int lowestNote  = 0;
	constexpr int highestNote = 127;
	constexpr int noteCount   = 128;
	constexpr int lowestControllerValue  = 0;
	constexpr int highestControllerValue = 127;
	constexpr int lowestChannel  = 1;
	constexpr int highestChannel = 16;
}

/** Which device list a MidiDevice call works on. */
namespace DeviceListChoice
{
	constexpr int midiInput   = 0;
	constexpr int midiOutput  = 1;
	constexpr int audioOutput = 2;
}

/** The MIDI channels the two hands play on (the keyboard split sends the right hand to its own channel). */
namespace HandChannels
{
	constexpr int leftHand  = 1;
	constexpr int rightHand = 16;
}

/** What MidiHandler::handlePlayableRange did with the device's key count. */
namespace PlayableRangeResult
{
	constexpr int askedForKeyCount = -1;   // unknown device: the UI asks the user, the range is set later
	constexpr int rangeApplied     = 1;
}

namespace DeviceNames
{
	inline const juce::String pcKeyboard = "PC Keyboard";   // the computer keyboard, listed as a MIDI input
	inline const juce::String noDevicesFound = "No devices found!";   // the only entry of an empty device list
}

/** MIDI controller (CC) numbers of the effects the app sends, one per SoundSettings field. */
namespace EffectControllers
{
	constexpr int vibrato     = 1;    // modulation wheel
	constexpr int volume      = 7;
	constexpr int pan         = 10;
	constexpr int expression  = 11;
	constexpr int sustain     = 64;
	constexpr int resonance   = 71;
	constexpr int release     = 72;
	constexpr int attack      = 73;
	constexpr int brightness  = 74;
	constexpr int decay       = 75;
	constexpr int filterTrack = 76;
	constexpr int distortion  = 80;
	constexpr int reverb      = 91;
	constexpr int tremolo     = 92;
	constexpr int chorus      = 93;
	constexpr int delay       = 94;
	constexpr int randomMod   = 95;
}

/** Which hand a program change is for (setProgramNumber / setProgarmNumber, matched ignoring case). */
namespace HandChoiceNames
{
	inline const juce::String left  = "left";
	inline const juce::String right = "right";
}

namespace MidiHandlerSettings
{
	constexpr int notSet = -1;   // a note setting or bound the user has not chosen, or a CC never sent
}

class MidiDevice {
public:

	/** @brief Default constructor*/
	MidiDevice();

	/** @brief Destructor */
	~MidiDevice();

	/** 
	 * @brief Populates a vector with the current available devices for MIDI input
	 * @param devices Vector to be populated with input devices
	 */

	std::vector<std::pair<juce::String, juce::String>> getAvailableInputDevicesNameIdentifier();

	void getAvailableDevicesMidiIN(std::vector<std::string>& devices);

	/**
	 * @brief Populates a vector with the current available devices for MIDI output
	 * @param devices Vector to be populated with output devices
	 */
	void getAvailableDevicesMidiOUT(std::vector<std::string>& devices);

	void getAvailableAudioDevicesOUT(std::vector<std::string>& devices);

	/**
	 * @brief Gets a device name, based on its index in the list of devices
	 * @param Index the index of the device 
	 * @param choice Parameter to filter out if it's input or output (a DeviceListChoice)
	 * @return the name of the device as a string
	 */
	juce::String getDeviceNameBasedOnIndex(int Index, int choice = DeviceListChoice::midiInput);

	/**
	 * @brief Gets a device identifier, based on its index in the list of devices
	 * @param Index the index of the device
	 * @param choice Parameter to filter out if it's input or output (a DeviceListChoice)
	 * @return the identifier of the device as a string
	 */
	juce::String getDeviceIdentifierBasedOnIndex(int Index, int choice = DeviceListChoice::midiInput);

	/**
	 * @brief Opens a MIDI input device
	 * @param DeviceIndex Index of the MIDI input device to open
	 * @param CallBack Pointer to a callback object that will receive MIDI messages
	 * @return true if the device was successfully opened, false otherwise
	 */

	bool deviceOpenIN(int DeviceIndex, juce::MidiInputCallback* CallBack);

	/**
	 * @brief Opens a MIDI output device
	 * @param DeviceIndex Index of the MIDI output device to open
	 * @return true if the device was successfully opened, false otherwise
	 */
	bool deviceOpenOUT(int DeviceIndex);

	bool deviceOpenAudioOUT(int DeviceIndex);

	/**
	 * @brief Checks if a MIDI input device is currently open
	 * @return true if a device is open, false otherwise
	 */
	bool isOpenIN() const;

	/**
	 * @brief Checks if a MIDI output device is currently open
	 * @return true if a device is open, false otherwise
	 */
	bool isOpenOUT() const;

	bool isOpenAudioOUT() const;

	/** @brief Closes the currently open MIDI input device */
	void deviceCloseIN();

	/** @brief Closes the currently open MIDI output device */
	void deviceCloseOUT();

	void deviceCloseAudioOUT();

	/**
	 * @brief Gets the index of the currently selected MIDI input device
	 * @return the index of the MIDI input device
	 */
	int getDeviceIndexIN() const;

	/**
	 * @brief Sets the index of the MIDI input device to use
	 * @param index Index of the device to set as active
	 */
	void setDeviceIN(const int index);

	/**
	 * @brief Gets the index of the currently selected MIDI output device
	 * @return the index of the MIDI output device
	 */
	int getDeviceIndexOUT() const;

	/**
	 * @brief Sets the index of the MIDI output device to use
	 * @param index Index of the device to set as active
	 */
	void setDeviceOUT(const int index);

	void setDeviceAudioOUT(const int index);

	int getDeviceIndexAudioOUT() const;

	juce::AudioDeviceManager& getAudioDeviceManager();

	/**
	 * @brief Gets the currently active MIDI input device as a weak pointer
	 * @return weak_ptr to the active MIDI input device
	 */
	std::weak_ptr<juce::MidiInput> getDeviceIN() const;

	/**
	 * @brief Gets the currently active MIDI output device as a weak pointer
	 * @return weak_ptr to the active MIDI output device
	 */
	std::weak_ptr<juce::MidiOutput> getDeviceOUT() const;

	/**
	 * @brief Identifier of the currently-open MIDI OUTPUT device, read live from the device.
	 * @return the output device's identifier, or an empty string if no output is open.
	 *
	 * Used to detect (against MidiOutput::getAvailableDevices()) when the output was unplugged.
	 */
	juce::String getOutputIdentifier() const
	{
		return currentDeviceUSEDout != nullptr ? currentDeviceUSEDout->getDeviceInfo().identifier
		                                       : juce::String();
	}

	/**
	 * @brief Sets the volume of the instrument
	 * @param vValue Volume value 
	 */
	void setVolume(const int vValue, int channel = HandChannels::leftHand);

	/**
	 * @brief Gets the current volume of the instrument
	 * @return current volume value
	 */
	int getVolume(int channel) const;

	/**
	 * @brief Sets the reverb amount of the instrument
	 * @param rValue Reverb value (typically 0.0 - 1.0)
	 */
	void setReverb(const int rValue, int channel = HandChannels::leftHand) ;

	/**
	 * @brief Gets the current reverb amount of the instrument
	 * @return current reverb value
	 */
	int getReverb(int channel) const;


	void setBrightness(const int rValue, int channel = HandChannels::leftHand);

	int getBrightness(int channel) const;

	void setChorus(const int rValue, int channel = HandChannels::leftHand);

	int getChorus(int channel) const;

	void setExpression(const int rValue, int channel = HandChannels::leftHand);

	int getExpression(int channel) const;

	void setResonance(const int rValue, int channel = HandChannels::leftHand);

	int getResonance(int channel) const;

	void setSustainToggle(const bool rValue, int channel = HandChannels::leftHand);

	bool getSustainToggle(int channel) const;

	// Envelope & Modulation
	void setAttack(const int rValue, int channel = HandChannels::leftHand);
	int getAttack(int channel) const;

	void setDecay(const int rValue, int channel = HandChannels::leftHand);
	int getDecay(int channel) const;

	void setRelease(const int rValue, int channel = HandChannels::leftHand);
	int getRelease(int channel) const;

	void setVibrato(const int rValue, int channel = HandChannels::leftHand);
	int getVibrato(int channel) const;

	// Effects & Pan

	void setDelay(const int rValue, int channel = HandChannels::leftHand);
	int getDelay(int channel) const;

	void setPan(const int rValue, int channel = HandChannels::leftHand);
	int getPan(int channel) const;

	// Additional effects
	void setDistortion(const int rValue, int channel = HandChannels::leftHand);
	int getDistortion(int channel) const;

	void setFilterTrack(const int rValue, int channel = HandChannels::leftHand);
	int getFilterTrack(int channel) const;

	void setTremolo(const int rValue, int channel = HandChannels::leftHand);
	int getTremolo(int channel) const;

	void setRandomMod(const int rValue, int channel = HandChannels::leftHand);
	int getRandomMod(int channel) const;

	/**
	 * @brief Gets the minimum MIDI note allowed
	 * @return minimum MIDI note
	 */
	int get_minNote() const;

	/**
	 * @brief Gets the maximum MIDI note allowed
	 * @return maximum MIDI note
	 */
	int get_maxNote() const;

	/**
	 * @brief Sets the minimum MIDI note allowed
	 * @param minNoteReceived Minimum MIDI note to set
	 */
	void set_minNote(const int minNoteReceived);

	/**
	 * @brief Sets the maximum MIDI note allowed
	 * @param maxNoteReceived Maximum MIDI note to set
	 */
	void set_maxNote(int maxNoteReceived);

	void sendMidiCC(int channel, int ccNumber, int value);

	/**
	 * @brief Gets the MIDI device identifier string
	 * @return identifier string of the current device
	 */
	const juce::String& get_identifier() const;

	/**
	 * @brief Extracts the PID (Product ID) from a device identifier string
	 * @param identifierReceived Identifier string to parse
	 * @return PID string
	 */
	juce::String extractPID(const juce::String& identifierReceived);

	/**
	 * @brief Extracts the VID (Vendor ID) from a device identifier string
	 * @param indentifierReceived Identifier string to parse
	 * @return VID string
	 */
	juce::String extractVID(const juce::String& indentifierReceived);

	void setVID(const juce::String& VID);
	
	void setPID(const juce::String& PID);

	void setDeviceName(const juce::String& name);

	juce::String getVID();

	juce::String getPID();

	juce::String getName();

	int getNrInputActualDevices();

	int getNrKeysAfterInitialized();

private:
	friend class MidiHandler;

	struct AudioOutputDeviceInfo
	{
		juce::String typeName;
		juce::String deviceName;

		bool operator==(const AudioOutputDeviceInfo& other) const
		{
			return typeName == other.typeName && deviceName == other.deviceName;
		}
	};

	void refreshDeviceList(int choice = DeviceListChoice::midiInput);

	void refreshDeviceListNew(std::vector<std::pair<juce::String,juce::String>>& vec, int choice = DeviceListChoice::midiInput);

	std::vector<std::string> currentDevicesIN;
	juce::Array<juce::MidiDeviceInfo> CachedDevicesIN;


	std::vector<std::string> currentDevicesOUT;
	juce::Array<juce::MidiDeviceInfo> CachedDevicesOUT;

	std::vector<std::string> currentDevicesAudioOUT;
	std::vector<AudioOutputDeviceInfo> CachedDevicesAudioOUT;
	juce::AudioDeviceManager audioDeviceManager;
	juce::OwnedArray<juce::AudioIODeviceType> audioDeviceTypes;
	bool audioDeviceTypesInitialized = false;

	int currentDeviceIDin;
	int currentDeviceIDout;
	int currentDeviceIDAudioOUT;


	std::shared_ptr<juce::MidiInput> currentDeviceUSEDin = nullptr;

	std::shared_ptr<juce::MidiOutput> currentDeviceUSEDout=nullptr;

	bool devicesChange = false;
	bool isdeviceOpenIN = false;
	bool isdeviceOpenOUT = false;
	bool isdeviceOpenAudioOUT = false;

	bool deviceCheckedForUpdateAtLeastOnce = false;
	juce::String identifier;

	std::unique_ptr<StyleSettings> styleSettings=nullptr;

	int minNote, maxNote;

	juce::String VID, PID, name;
};

class MidiHandler :public juce::MidiInputCallback, public DisplayListener 
{
public:
	std::function<void()> onStartNoteSetting;
	std::function<void()> onEndNoteSetting;
	std::function<void(const juce::String& vid, const juce::String& pid, std::function<void(const juce::String&, int)>)> onAddCallBack;

	/** @brief Constructor with a MidiDevice as reference */
	MidiHandler(MidiDevice& device, InstrumentHandler* instrumentHandler=nullptr);

	/** @brief Destructor */
	~MidiHandler();

	/**
	 * @brief Callback that is called whenever a MIDI message is received
	 * @param source Pointer to the MIDI input source
	 * @param message The incoming MIDI message
	 */
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

	/**
	 * @brief Handles and determines the playable range for a given MIDI device
	 * @param vid Vendor ID of the device
	 * @param pid Product ID of the device
	 * @param isKeyboardInput Optional flag indicating if the input is from a keyboard (default false)
	 * @return integer representing the number of playable keys or range
	 */
	int handlePlayableRange(const juce::String& vid, const juce:: String& pid, int nrKeys, bool isKeyboardInput=false);

	/**
	 * @brief Adds a listener to receive MIDI events
	 * @param listener Pointer to a MidiHandlerListener to add
	 */
	void addListener(MidiHandlerListener* listener) { listeners.add(listener); }

	/**
	 * @brief Removes a listener from receiving MIDI events
	 * @param listener Pointer to a MidiHandlerListener to remove
	 */
	void removeListener(MidiHandlerListener* listener) { listeners.remove(listener); }

	/**
	 * @brief Fills the provided MIDI buffer with the next block of MIDI messages
	 * @param destBuffer Destination MIDI buffer to fill
	 * @param startSample Starting sample index
	 * @param numSamples Number of samples to process
	 */
	void getNextMidiBlock(juce::MidiBuffer& destBuffer, int startSample, int numSamples);

	/**
	 * @brief Sends a note-on message as if triggered from a keyboard
	 * @param note MIDI note number
	 * @param velocity Velocity value (0-127)
	 */
	void noteOnKeyboard(int note, juce::uint8 velocity);

	/**
	 * @brief Sends a note-off message as if triggered from a keyboard
	 * @param note MIDI note number
	 * @param velocity Velocity value (0-127)
	 */
	void noteOffKeyboard(int note, juce::uint8 velocity);

	/** @brief Turns off all currently active notes from the keyboard */
	void allOffKeyboard();

	void injectMidiMessage(const juce::MidiMessage& msg);

	/**
	 * @brief Whether the internal SFZ synth is running and taking MIDI from getNextMidiBlock.
	 * Messages are queued for it only while it is; switching either way drops whatever is pending,
	 * so an External MIDI session neither grows the queue nor replays it into the synth later.
	 * Called by AudioHandler when its audio device starts and stops.
	 */
	void setInternalSynthListening(bool isListening);
	void injectCC(int channel, int ccNumber, int value);

	/**
	 * @brief Sets the instrument program (patch) number
	 * @param toSetNumber Program number to set
	 * @param choice Optional string specifying a particular choice or bank
	 */
	void setProgramNumber(int toSetNumber, const juce::String& choice="");

	/** @brief Gets the current program number for the left-hand instrument */
	int getProgramNumberLeftHand();

	/** @brief Gets the current program number for the right-hand instrument */
	int getProgramNumberRightHand();

	/**
	 * @brief Sets the start and end notes of the instrument's playable range
	 * @param start MIDI note number for the start
	 * @param end MIDI note number for the end
	 */
	void set_start_end_notes(int start, int end);

	/**
	 * @brief Sets the left and right bounds for hand ranges
	 * @param left Left-hand bound MIDI note
	 * @param right Right-hand bound MIDI note
	 */
	void set_left_right_bounds(int left, int right);


	/**
	 * @brief Called when playback settings change
	 * @param settings New playback settings
	 */
	void playBackSettingsChanged(const PlayBackSettings& settings) override;

	/**
	 * @brief Called when playback transpose value changes
	 * @param transposeValue New transpose value
	 */
	void playBackSettingsTransposeChanged(int transposeValue) override;


	/**
	 * @brief Returns the MIDI channel for a note based on which hand zone it falls in.
	 * @param note MIDI note number to determine the channel for
	 * @return 16 for the right-hand zone, 1 otherwise. Pure: stores no state (per-note local).
	 */
	int channelForHand(int note) const;

	// --- Phase 4: arranger chord recognition -------------------------------------------------
	/** Fired (on the MIDI input thread) when the recognized left-hand/whole-keyboard chord changes.
	    Wire this to ArrangerEngine::setActiveChord. */
	std::function<void(const ArrangerChord&)> onChordChanged;

	/** The chord PHYSICALLY held in the chord zone right now, for seeding the arranger at Start.
	    Returns an invalid chord when nothing is held, so a chord only *remembered* by Chord Memory
	    (from an earlier, released press) does NOT seed Start — that case keeps the home key. */
	ArrangerChord heldChord() const
	{
	    return chordDetector.hasHeldNotes() ? chordDetector.current() : ArrangerChord{};
	}

	/** Choose the chord-recognition mode (Fingered / Single-Finger / Full-Keyboard). */
	void setChordMode(ChordMode mode);
	/** When true (default), the last recognized chord is held after the chord zone empties; when
	    false, emptying the zone reverts the accompaniment to the recorded key. */
	void setChordMemory(bool shouldHold) { chordMemory = shouldHold; }

	/** Chord-zone mute (Korg-style): when on AND the arranger is engaged, the chord-recognition keys
	    drive the accompaniment WITHOUT sounding their raw notes -- so a one-finger cluster (e.g. C+B+Bb)
	    is never heard, only the clean backing. Has no effect when the arranger is off (normal playing). */
	void setChordZoneMute(bool shouldMute) { chordZoneMute = shouldMute; }
	/** Tell the handler whether Arranger mode is currently on; gates chord-zone muting so plain piano
	    playing (arranger off) always sounds. */
	void setArrangerEngaged(bool engaged)  { arrangerEngaged = engaged; }


	void setPlayableRange(int nrKeys);

private:
	//void sendCCifChanged(int ccNumber, int value, int& lastSentValue);

	/**
	 * @brief Applies an instrument preset, including program number and CC values
	 * @param programNumber Program number of the instrument
	 * @param ccValues Vector of CC number/value pairs
	 * @param choice Optional string specifying a particular bank or option
	 */
	void applyInstrumentPreset(int programNumber, std::vector<std::pair<int, int>> ccValues, const juce::String& choice="");

	juce::ListenerList<MidiHandlerListener> listeners;

	MidiDevice& midiDevice;
	InstrumentHandler* instrumentHandler=nullptr;

	bool receivedValidNote = false;
	juce::MidiBuffer incomingMidiMessages;
	juce::CriticalSection midiMutex;
	std::atomic<bool> internalSynthListening { false };
	void queueForInternalSynth(const juce::MidiMessage& message);
	int programNumberLeftHand {};
	int programNumberRightHand {};
	int startNoteSetting = MidiHandlerSettings::notSet;
	int endNoteSetting = MidiHandlerSettings::notSet;
	int leftHandBoundSetting = MidiHandlerSettings::notSet;
	int rightHandBoundSetting = MidiHandlerSettings::notSet;
	int transposeValue {};

	// Phase 4: chord recognition fed from the live input. The detector lives on the input thread;
	// only the resulting ArrangerChord crosses to the engine (via onChordChanged).
	std::atomic<ChordMode> chordMode { ChordMode::Fingered };
	ChordDetector chordDetector;
	bool          chordMemory = true;
	bool          chordZoneMute = false;    // silence the chord-recognition keys when the arranger is engaged
	bool          arrangerEngaged = false;  // true while Arranger mode is on; gates chordZoneMute
	bool inChordZone (int note) const;
	/** True when this note's onset should be muted: chord-zone-mute on, arranger engaged, note in the zone. */
	bool muteChordZoneNote (int note) const { return chordZoneMute && arrangerEngaged && inChordZone (note); }
	void feedChordNote (int note, bool isOn);

	int lastCC91 = MidiHandlerSettings::notSet; //reverb
	int lastCC74 = MidiHandlerSettings::notSet;  //brightness
	int lastCC11 = MidiHandlerSettings::notSet; //expression
	int lastCC93 = MidiHandlerSettings::notSet; //chours
	int lastCC71 = MidiHandlerSettings::notSet; //resonance
	int lastCC64 = MidiHandlerSettings::notSet; //sustain pedal;


};
