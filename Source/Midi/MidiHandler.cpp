#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiDevicesDB.h"

namespace MidiNoteRange
{
	constexpr int lowestNote  = 0;
	constexpr int highestNote = 127;
	constexpr int noteCount   = 128;
	constexpr int lowestControllerValue  = 0;
	constexpr int highestControllerValue = 127;
}

namespace MidiHandlerValues
{
	constexpr float silentVelocity = 0.0f;
	constexpr float fullVelocity   = 1.0f;
	constexpr int pitchWheelCentre = 0x2000;   // no bend
	constexpr int presetSettleMs   = 5;
	const juce::String leftHandChoice  = "left";
	const juce::String rightHandChoice = "right";
}

/** Note span of each known keyboard size (lowest and highest MIDI note it sends). */
namespace KeyboardSizes
{
	struct KeyboardSpan { int keyCount; int lowestNote; int highestNote; };
	constexpr KeyboardSpan spans[] = {
		{ 25, 36, 60 },
		{ 49, 40, 88 },
		{ 61, 36, 96 },
		{ 88, 21, 108 },
	};
}

namespace InternalSynthQueue
{
	constexpr int queuedAtBlockStart = 0;   // every queued message plays at the start of the next block
	constexpr int noTimeShift = 0;
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

	if (message.isNoteOn())
	{
		int note = message.getNoteNumber();

		const int channel = channelForHand(note);
		int transposedNote = juce::jlimit(MidiNoteRange::lowestNote, MidiNoteRange::highestNote, note + transposeValue);

		if (note != this->startNoteSetting && note != this->endNoteSetting)
			feedChordNote(note, true);

		// Korg-style chord-zone mute: recognition above still runs, but we don't SOUND this onset when
		// muting. Only note-ONs are gated -- note-offs always pass, so a note can never get stuck.
		const bool muteChordZone = muteChordZoneNote(note);

		float velocity = message.getFloatVelocity();

		float volume = this->midiDevice.getVolume(message.getChannel());
		//DBG("VOLUME="+ juce::String(volume));
		float scaledVelocity = juce::jlimit(MidiHandlerValues::silentVelocity, MidiHandlerValues::fullVelocity, velocity * volume);

		juce::uint8 velocityByte = juce::MidiMessage::floatValueToMidiByte(scaledVelocity);

		bool isPlayable = false;

		// Decide playability from the note range FIRST -- independent of any external MIDI-OUT device.
		// (The internal SFZ works with no MIDI-OUT open; gating this on the device lock is what made
		//  a physical keyboard show/sound nothing on internal SFZ. Mirrors noteOnKeyboard.)
		if (note != this->startNoteSetting && note != this->endNoteSetting)
		{
			isPlayable = true;
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

		// External MIDI-OUT is optional: only forward when a device is actually open.
		if (auto midiOut = midiDevice.getDeviceOUT().lock())
		{
			if (isPlayable)
			{
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+9, velocityByte));
				midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(channel, MidiHandlerValues::pitchWheelCentre));
				if (! muteChordZone)
					midiOut->sendMessageNow(juce::MidiMessage::noteOn(channel, transposedNote, velocityByte));
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+10, velocityByte));
			}
		}

		if (isPlayable)
		{
			listeners.call(&MidiHandlerListener::noteOnReceived, note);
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 91, 80));
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 74, 100));
			if (! muteChordZone)
			{
				listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOn(channel, note, velocityByte));

				// Feed the internal SFZ synth on the hand-split channel (transposed), matching noteOnKeyboard.
				// midiMutex is already held for this whole function, so add directly (no re-lock).
				queueForInternalSynth(juce::MidiMessage::noteOn(channel, transposedNote, velocityByte));
			}
		}

	}

	if (message.isNoteOff())
	{
		int note = message.getNoteNumber();
		const int channel = channelForHand(note);
		int transposedNote = juce::jlimit(MidiNoteRange::lowestNote, MidiNoteRange::highestNote, note + transposeValue);

		if (note != this->startNoteSetting && note != this->endNoteSetting)
			feedChordNote(note, false);

		juce::uint8 velocityByte = juce::MidiMessage::floatValueToMidiByte(message.getFloatVelocity());

		if (auto midiOut = midiDevice.getDeviceOUT().lock())
		{
			midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, transposedNote, velocityByte));
		}

		listeners.call(&MidiHandlerListener::noteOffReceived, note);
		listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOff(channel, note, velocityByte));

		// Match the hand-split channel + transpose used for note-on so the internal synth releases it.
		queueForInternalSynth(juce::MidiMessage::noteOff(channel, transposedNote, velocityByte));
	}

	// Notes are fed to the internal synth on the correct hand-split channel above; pass everything
	// else (CC, pitch bend, aftertouch, ...) through raw so the synth still receives it.
	if (! message.isNoteOnOrOff())
		queueForInternalSynth(processedMessage);
}

void MidiHandler::getNextMidiBlock(juce::MidiBuffer& destBuffer, int startSample, int numSamples) {
	//DBG("getNextMidiBlock called");
	const juce::ScopedLock lock(midiMutex);
	destBuffer.addEvents(incomingMidiMessages, startSample, numSamples, InternalSynthQueue::noTimeShift);
	incomingMidiMessages.clear();
	//DBG("getNextMidiBlock: " << destBuffer.getNumEvents());

}

void MidiHandler::noteOnKeyboard(int note, juce::uint8 velocity) {
	bool isPlayable = false;
	const int channel = channelForHand(note);
	int transposedNote = juce::jlimit(MidiNoteRange::lowestNote, MidiNoteRange::highestNote, note + transposeValue);

	if (note != this->startNoteSetting && note != this->endNoteSetting)
		feedChordNote(note, true);

	const bool muteChordZone = muteChordZoneNote(note);   // Korg-style: drive the chord, don't sound the keys

	if (note != this->startNoteSetting && note!=this->endNoteSetting)
	{
		isPlayable = true;
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
		if (isPlayable)
		{
			midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(channel, MidiHandlerValues::pitchWheelCentre));
			if (! muteChordZone)
				midiOut->sendMessageNow(juce::MidiMessage::noteOn(channel, transposedNote, velocity));
		}
	}

	if (isPlayable)
	{
		listeners.call(&MidiHandlerListener::noteOnReceived, note);
		if (! muteChordZone)
		{
			listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOn(channel, note, velocity));

			const juce::ScopedLock lock(midiMutex);
			queueForInternalSynth(juce::MidiMessage::noteOn(channel, transposedNote, velocity));
		}
	}
}

void MidiHandler::noteOffKeyboard(int note, juce::uint8 velocity) {
	const int channel = channelForHand(note);
	int transposedNote = juce::jlimit(MidiNoteRange::lowestNote, MidiNoteRange::highestNote, note + transposeValue);

	if (note != this->startNoteSetting && note != this->endNoteSetting)
		feedChordNote(note, false);

	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, transposedNote,velocity));

	}
	listeners.call(&MidiHandlerListener::noteOffReceived,note);
	listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOff(channel, note));

	const juce::ScopedLock lock(midiMutex);
	queueForInternalSynth(juce::MidiMessage::noteOff(channel, transposedNote, velocity));
} 

void MidiHandler::allOffKeyboard()
{
	for (int noteNumber {}; noteNumber < MidiNoteRange::noteCount; noteNumber++)
	{
		if (midiDevice.currentDeviceUSEDout == nullptr) return;

		midiDevice.currentDeviceUSEDout->sendMessageNow(juce::MidiMessage::noteOff(HandChannels::leftHand, noteNumber));
		midiDevice.currentDeviceUSEDout->sendMessageNow(juce::MidiMessage::noteOff(HandChannels::rightHand, noteNumber));
	}
}

void MidiHandler::setProgramNumber(int toSetNumber, const juce::String& choice) {
	if (choice.toLowerCase() == MidiHandlerValues::leftHandChoice)
		this->programNumberLeftHand = toSetNumber;
	else if (choice.toLowerCase() == MidiHandlerValues::rightHandChoice)
		this->programNumberRightHand = toSetNumber;

	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		if (instrumentHandler)
		{
			const auto& preset = instrumentHandler->getPreset(toSetNumber);

			applyInstrumentPreset(toSetNumber, preset, choice);
			juce::Thread::sleep(MidiHandlerValues::presetSettleMs);
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
	return (rightHandBoundSetting != MidiHandlerSettings::notSet && note >= rightHandBoundSetting) ? HandChannels::rightHand : HandChannels::leftHand;
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
	return (rightHandBoundSetting == MidiHandlerSettings::notSet) ? true : note < rightHandBoundSetting;
}

void MidiHandler::feedChordNote(int note, bool isOn)
{
	if (! inChordZone(note))
		return;

	if (isOn) chordDetector.noteOn(note);
	else      chordDetector.noteOff(note);

	ArrangerChord currentChord = chordDetector.current();

	// Chord Memory: when the zone empties to "no chord", hold the last chord (memory on) instead of
	// telling the engine to revert. With memory off, push the invalid chord so it reverts to the key.
	if (! currentChord.isValid() && chordMemory)
		return;

	// Always forward the current chord. We deliberately do NOT skip "same as last time": the engine
	// resets its chord to the style's home key on every Start, so re-pressing the same chord after a
	// 2nd Play must reach it again. Re-applying an identical chord is cheap and harmless.
	if (onChordChanged)
		onChordChanged(currentChord);
}

int MidiHandler::handlePlayableRange(const juce::String& vid, const juce::String& pid, int nrKeys, bool isKeyboardInput)
{
	if (nrKeys < int {} && !isKeyboardInput)
	{
		if (onAddCallBack)
		{
			onAddCallBack(vid, pid, [this, vid, pid](const juce::String& name, int keys)
				{
					setPlayableRange(keys);
				});
		}
		return PlayableRangeResult::askedForKeyCount;
	}
	else {
		setPlayableRange(nrKeys);
		return PlayableRangeResult::rangeApplied;
	}
}


void MidiHandler::applyInstrumentPreset(int programNumber, std::vector<std::pair<int, int>> ccValues, const juce::String& choice)
{
	int channel = HandChannels::leftHand;
	if (choice.toLowerCase() == MidiHandlerValues::rightHandChoice)
		channel = HandChannels::rightHand;
	if (auto midiOut = midiDevice.getDeviceOUT().lock())
	{
		for (int noteNumber {}; noteNumber < MidiNoteRange::noteCount; ++noteNumber)
			midiOut->sendMessageNow(juce::MidiMessage::noteOff(channel, noteNumber));

		midiOut->sendMessageNow(juce::MidiMessage::programChange(channel, programNumber));
		listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::programChange(channel, programNumber));

	}
}

void MidiHandler::injectMidiMessage(const juce::MidiMessage& msg)
{
	queueForInternalSynth(msg);
}

void MidiHandler::queueForInternalSynth(const juce::MidiMessage& message)
{
	const juce::ScopedLock lock(midiMutex);
	if (internalSynthListening.load())
		incomingMidiMessages.addEvent(message, InternalSynthQueue::queuedAtBlockStart);
}

void MidiHandler::setInternalSynthListening(bool isListening)
{
	const juce::ScopedLock lock(midiMutex);
	incomingMidiMessages.clear();
	internalSynthListening.store(isListening);
}

void MidiHandler::injectCC(int channel, int ccNumber, int value)
{
	injectMidiMessage(juce::MidiMessage::controllerEvent(channel, ccNumber, juce::jlimit(MidiNoteRange::lowestControllerValue, MidiNoteRange::highestControllerValue, value)));
}

void MidiHandler::setPlayableRange(int nrKeys)
{
	for (const auto& span : KeyboardSizes::spans)
	{
		if (nrKeys == span.keyCount)
		{
			midiDevice.set_minNote(span.lowestNote);
			midiDevice.set_maxNote(span.highestNote);
			return;
		}
	}
}
