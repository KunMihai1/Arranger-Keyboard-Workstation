/*
  ==============================================================================

    MidiRecordPlayer.cpp
    Created: 25 May 2025 1:21:59am
    Author:  Kisuke

  ==============================================================================
*/

#include "MidiRecordPlayer.h"

namespace RecordPlayerValues
{
    constexpr int    recordedFirstHand   = 14;   // recordings move the hands to their own channels
    constexpr int    recordedSecondHand  = 15;
    constexpr double secondsPerMillisecond = 0.001;
    constexpr int    playbackTimerMs     = 1;
    constexpr size_t tooFewEventsToUse   = 1;    // a recording needs more than one event to play or save
    constexpr double secondsPerMinute    = 60.0;
    constexpr int    microsecondsPerMinute = 60000000;
    constexpr double fileStartTick       = 0;
    const juce::String nothingToSave       = "No recorded events to save!";
    const juce::String cannotOpenForSaving = "Failed to open file for saving!";
    const juce::String cannotParseMidi     = "Failed to parse MIDI data.";
    const juce::String fileDoesNotExist    = "Selected file does not exist.";
    const juce::String cannotOpenForReading = "Failed to open file for reading.";
}

using namespace RecordPlayerValues;

MidiRecordPlayer::MidiRecordPlayer(): programLeftHand{}, programRightHand{}
{

}

MidiRecordPlayer::MidiRecordPlayer(std::weak_ptr<juce::MidiOutput> out): midiOutputDevice{out}, programLeftHand{}, programRightHand{}
{
}

void MidiRecordPlayer::startRecording()
{
    allEventsPlayed.clear();
    allEventsPlayedFile.clear();
    applyPresetFunction();
    recordStartTime = juce::Time::getMillisecondCounterHiRes() * secondsPerMillisecond;
    isRecording = true;
}

bool MidiRecordPlayer::stopRecording()
{
    if (isRecording == false) //looks unecessary but it's actually necessary
        return false;

    isRecording = false;
    return true;
}

bool MidiRecordPlayer::startPlayBack()
{
    if (allEventsPlayed.size() <= tooFewEventsToUse)
    {
        notifyFunction();
        return false;
    }
    if (isPlaying)
        stopPlayBack();
    else if (isPlayingFile)
        stopRecordingFilePlaying();

    playBackStartTime = juce::Time::getMillisecondCounterHiRes() * secondsPerMillisecond;
    isPlaying = true;
    nextEventIndex = {};
    startTimer(playbackTimerMs);
    return true;
}

void MidiRecordPlayer::stopPlayBack()
{
    isPlaying = false;
    notifyFunction();
    if (auto midiOutShared = midiOutputDevice.lock())
    {
        for (int channel = MidiFacts::lowestChannel; channel <= MidiFacts::highestChannel; ++channel)
            midiOutShared->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
    }

    stopTimer();
}

void MidiRecordPlayer::startRecordingFilePlaying()
{
    if (isPlayingFile)
    {
        stopRecordingFilePlaying();
    }
    else if (isPlaying)
        stopPlayBack();
    isPlayingFile = true;
    nextEventFileIndex = {};
    playBackStartTime = juce::Time::getMillisecondCounterHiRes() * secondsPerMillisecond;
    startTimer(playbackTimerMs);
}

void MidiRecordPlayer::stopRecordingFilePlaying()
{
    isPlayingFile = false;
    if (auto midiOutShared = midiOutputDevice.lock())
    {
        for (int channel = MidiFacts::lowestChannel; channel <= MidiFacts::highestChannel; ++channel)
        {
            midiOutShared->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
        }
    }

    stopTimer();
}

void MidiRecordPlayer::handleIncomingMessage(const juce::MidiMessage& message)
{
    if (!isRecording)
        return;

    juce::MidiMessage newMessage = remapChannel(message);

    double now = juce::Time::getMillisecondCounterHiRes() * secondsPerMillisecond;
    RecordedEvent recordedEvent{ newMessage,now - recordStartTime };

    const juce::SpinLock::ScopedLockType eventsLock(lock);
    allEventsPlayed.push_back(recordedEvent);
}

void MidiRecordPlayer::timerCallback()
{
    if (!isPlaying && !isPlayingFile)
    {
        return;
    }
    double elapsedTime = juce::Time::getMillisecondCounterHiRes() * secondsPerMillisecond- playBackStartTime;

    if (isPlaying)
    {
        while (true)
        {
            RecordedEvent dueEvent;
            {
                const juce::SpinLock::ScopedLockType eventsLock(lock);
                if (nextEventIndex >= allEventsPlayed.size() ||
                    allEventsPlayed[nextEventIndex].timeFromStart > elapsedTime)
                    break;

                dueEvent = allEventsPlayed[nextEventIndex];
                nextEventIndex++;
            }

            if (auto midiOutShared = midiOutputDevice.lock())
                midiOutShared->sendMessageNow(dueEvent.message);

            if (onSfzMessage)
                onSfzMessage(dueEvent.message);
        }

        if (nextEventIndex >= allEventsPlayed.size())
            stopPlayBack();
    }

    else if (isPlayingFile)
    {
        while (true)
        {
            RecordedEvent dueEvent;
            {
                const juce::SpinLock::ScopedLockType eventsLock(lock);
                if (nextEventFileIndex >= allEventsPlayedFile.size() ||
                    allEventsPlayedFile[nextEventFileIndex].timeFromStart > elapsedTime)
                    break;

                dueEvent = allEventsPlayedFile[nextEventFileIndex];
                nextEventFileIndex++;
            }

            if (auto midiOutShared = midiOutputDevice.lock())
                midiOutShared->sendMessageNow(dueEvent.message);

            if (onSfzMessage)
                onSfzMessage(dueEvent.message);
        }

        if (nextEventFileIndex >= allEventsPlayedFile.size())
            stopRecordingFilePlaying();
    }
}

void MidiRecordPlayer::setOutputDevice(std::weak_ptr<juce::MidiOutput> outputDev)
{
    midiOutputDevice = outputDev;
}

void MidiRecordPlayer::setProgarmNumber(int newProgram, const juce::String& choice)
{
    if (choice.toLowerCase() == HandChoiceNames::left)
        programLeftHand = newProgram;
    else if (choice.toLowerCase() == HandChoiceNames::right)
        programRightHand = newProgram;
}

void MidiRecordPlayer::setInitialProgram(int value)
{
    initialProgram = value;
}

void MidiRecordPlayer::setReverb(int value, int channel)
{
    if (channel == RecordingDefaults::firstHandChannel)
        reverbFirst = value;
    else if (channel == HandChannels::rightHand)
        reverbSecond = value;
}

void MidiRecordPlayer::setVolume(int value, int channel)
{
    if (channel == RecordingDefaults::firstHandChannel)
        volumeFirst = value;
    else if (channel == HandChannels::rightHand)
        volumeSecond = value;
}

bool MidiRecordPlayer::getIsRecording()
{
    return isRecording == true;
}

bool MidiRecordPlayer::getIsPlaying()
{
    return isPlaying == true;
}

bool MidiRecordPlayer::writeRecordingToStream(juce::OutputStream& outputStream, juce::String& errorMsg, double tempo)
{
    if (allEventsPlayed.size() <= tooFewEventsToUse)
    {
        errorMsg = nothingToSave;
        return false;
    }

    juce::MidiFile midiFile;
    const int ticksPerQuarterNote = 960;
    midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);

    juce::MidiMessageSequence sequence;

    const double secondsPerBeat = secondsPerMinute / tempo;
    const double ticksPerSecond = ticksPerQuarterNote / secondsPerBeat;
    const int microsecondsPerQuarterNote = static_cast<int>(microsecondsPerMinute / tempo);

    juce::MidiMessageSequence tempoSequence;
    tempoSequence.addEvent(juce::MidiMessage::tempoMetaEvent(microsecondsPerQuarterNote), fileStartTick);

    midiFile.addTrack(tempoSequence);

    for (const auto& event : allEventsPlayed)
    {
        int tickPosition = static_cast<int>(event.timeFromStart * ticksPerSecond);
        sequence.addEvent(event.message, tickPosition);
    }

    midiFile.addTrack(sequence);

    midiFile.writeTo(outputStream);
    return true;
}

bool MidiRecordPlayer::saveRecordingToFile(const juce::File& fileToSaveTo, juce::String& errorMsg, double tempo)
{
    if (allEventsPlayed.size() <= tooFewEventsToUse)
    {
        errorMsg = nothingToSave;
        return false;
    }

    juce::FileOutputStream outputStream(fileToSaveTo);
    if (!outputStream.openedOk())
    {
        errorMsg = cannotOpenForSaving;
        return false;
    }
    return writeRecordingToStream(outputStream, errorMsg, tempo);
}

bool MidiRecordPlayer::readRecordingFromStream(juce::InputStream& inputStream, juce::String& errorMsg)
{
    juce::MidiFile midiFile;
    if (!midiFile.readFrom(inputStream))
    {
        errorMsg = cannotParseMidi;
        return false;
    }

    midiFile.convertTimestampTicksToSeconds();
    allEventsPlayedFile.clear();
    const int numTracks = midiFile.getNumTracks();
    for (int trackIndex {}; trackIndex < numTracks; ++trackIndex)
    {
        auto* sequence = midiFile.getTrack(trackIndex);
        if (sequence == nullptr)
            continue;

        for (int eventIndex {}; eventIndex < sequence->getNumEvents(); ++eventIndex)
        {
            const auto& eventHolder = sequence->getEventPointer(eventIndex);
            if (eventHolder != nullptr)
            {
                const auto& msg = eventHolder->message;
                double timeFromStart = msg.getTimeStamp();
                allEventsPlayedFile.push_back(RecordedEvent{ msg, timeFromStart });
            }
        }
    }
    std::sort(allEventsPlayedFile.begin(), allEventsPlayedFile.end(),
        [](const RecordedEvent& earlier, const RecordedEvent& later)
        {
            return earlier.timeFromStart < later.timeFromStart;
        });

    return true;
}

bool MidiRecordPlayer::parseRecordingFromFile(const juce::File& fileToParse, juce::String& errorMsg)
{
    if (!fileToParse.existsAsFile())
    {
        errorMsg = fileDoesNotExist;
        return false;
    }

    juce::FileInputStream inputStream(fileToParse);
    if (!inputStream.openedOk())
    {
        errorMsg = cannotOpenForReading;
        return false;
    }

    return readRecordingFromStream(inputStream, errorMsg);
}

int MidiRecordPlayer::getSizeRecorded()
{
    return allEventsPlayed.size();
}

int MidiRecordPlayer::getProgramLeftHand()
{
    return programLeftHand;
}

int MidiRecordPlayer::getProgramRightHand()
{
    return programRightHand;
}

std::vector<RecordedEvent>& MidiRecordPlayer::getAllRecordedEvents()
{
    return this->allEventsPlayed;
}

juce::MidiMessage MidiRecordPlayer::remapChannel(const juce::MidiMessage& message)
{
    int newChannel = message.getChannel();

    if (message.getChannel() == RecordingDefaults::firstHandChannel)
        newChannel = recordedFirstHand;
    else if (message.getChannel() == HandChannels::rightHand)
        newChannel = recordedSecondHand;

    if (message.isNoteOn())
        return juce::MidiMessage::noteOn(newChannel, message.getNoteNumber(), message.getVelocity());

    if (message.isNoteOff())
        return juce::MidiMessage::noteOff(newChannel, message.getNoteNumber());

    if (message.isController())
        return juce::MidiMessage::controllerEvent(newChannel, message.getControllerNumber(), message.getControllerValue());

    if (message.isProgramChange())
        return juce::MidiMessage::programChange(newChannel, message.getProgramChangeNumber());

    if (message.isPitchWheel())
        return juce::MidiMessage::pitchWheel(newChannel, message.getPitchWheelValue());

    return message;
}
