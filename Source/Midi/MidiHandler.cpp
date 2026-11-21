#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiDevicesDB.h"

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
		int transposedNote = juce::jlimit(0, 127, note + transposeValue);

		if (note != this->startNoteSetting && note != this->endNoteSetting)
			feedChordNote(note, true);

		// Korg-style chord-zone mute: recognition above still runs, but we don't SOUND this onset when
		// muting. Only note-ONs are gated -- note-offs always pass, so a note can never get stuck.
		const bool muteChordZone = muteChordZoneNote(note);

		float velocity = message.getFloatVelocity();

		float volume = this->midiDevice.getVolume(message.getChannel());
		//DBG("VOLUME="+ juce::String(volume));
		float scaledVelocity = juce::jlimit(0.0f, 1.0f, velocity * volume);

		juce::uint8 velocityByte = juce::MidiMessage::floatValueToMidiByte(scaledVelocity);

		int ok = 0;

		// Decide playability from the note range FIRST -- independent of any external MIDI-OUT device.
		// (The internal SFZ works with no MIDI-OUT open; gating this on the device lock is what made
		//  a physical keyboard show/sound nothing on internal SFZ. Mirrors noteOnKeyboard.)
		if (note != this->startNoteSetting && note != this->endNoteSetting)
		{
			ok = 1;
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
			if (ok)
			{
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+9, velocityByte));
				midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(channel, 0x2000));
				if (! muteChordZone)
					midiOut->sendMessageNow(juce::MidiMessage::noteOn(channel, transposedNote, velocityByte));
				//midiOut->sendMessageNow(juce::MidiMessage::noteOn(2, note+10, velocityByte));
			}
		}

		if (ok)
		{
			listeners.call(&MidiHandlerListener::noteOnReceived, note);
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 91, 80));
			//listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::controllerEvent(1, 74, 100));
			if (! muteChordZone)
			{
				listeners.call(&MidiHandlerListener::handleIncomingMessage, juce::MidiMessage::noteOn(channel, note, velocityByte));

				// Feed the internal SFZ synth on the hand-split channel (transposed), matching noteOnKeyboard.
				// midiMutex is already held for this whole function, so add directly (no re-lock).
				incomingMidiMessages.addEvent(juce::MidiMessage::noteOn(channel, transposedNote, velocityByte), 0);
			}
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

		// Match the hand-split channel + transpose used for note-on so the internal synth releases it.
		incomingMidiMessages.addEvent(juce::MidiMessage::noteOff(channel, transposedNote, velocityByte), 0);
	}

	// Notes are fed to the internal synth on the correct hand-split channel above; pass everything
	// else (CC, pitch bend, aftertouch, ...) through raw so the synth still receives it.
	if (! message.isNoteOnOrOff())
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
