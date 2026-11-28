/*
  ==============================================================================

    trackPlayer.cpp
    Created: 7 Jul 2025 9:02:50pm
    Author:  Kisuke

  ==============================================================================
*/

#include "TrackPlayer.h"

namespace
{
    constexpr int    lowestMidiChannel  = 1;
    constexpr int    highestMidiChannel = 16;
    constexpr int    channelVolumeCC    = 7;
    constexpr int    timerIntervalMs    = 10;
    constexpr double noTime             = 0.0;
    constexpr int    oneSequence        = 1;     // size() - oneSequence is the newest index
    constexpr int    halves             = 2;     // binary search midpoint
    constexpr int    oneEvent           = 1;
}

MultipleTrackPlayer::MultipleTrackPlayer(std::weak_ptr<juce::MidiOutput> out): outputDevice{out}
{
    clock.setTempo(defaultBpm);

}

void MultipleTrackPlayer::setTracks(const std::vector<TrackEntry>& newTracks)
{
    currentTracks = newTracks;
    tracks.clear();
    filteredSequences.clear();  // Clear old sequences first

    const double startupDelay = 0.05;  // 50 ms delay before starting playback

    int melodicCount {};

    for (auto& trackEntry : newTracks)
    {
        juce::MidiMessageSequence filteredSeq;
        int channel;
        if (trackEntry.type == TrackType::Percussion)
            channel = drumChannel;
        else
        {
            channel = melodicCount + firstTrackChannel;
            melodicCount++;
        }

        for (int eventIndex {}; eventIndex < trackEntry.sequence.getNumEvents(); ++eventIndex)
        {
            const auto& msg = trackEntry.sequence.getEventPointer(eventIndex)->message;
            if (msg.isNoteOn() || msg.isNoteOff())
            {
                auto modifiedMsg = msg;
                modifiedMsg.setChannel(channel);
                filteredSeq.addEvent(modifiedMsg);
            }
        }

        //apply small startup delay, not in the same for so it's safer glitch wise because of pairs note on-note off
        if (filteredSeq.getNumEvents() > int {})
        {
            filteredSeq.updateMatchedPairs();
            
            /*
            for (int e = 0; e < filteredSeq.getNumEvents(); ++e)
            {
                auto* event = filteredSeq.getEventPointer(e);
                juce::MidiMessage shiftedMsg = event->message;
                shiftedMsg.setTimeStamp(shiftedMsg.getTimeStamp() + startupDelay);
                filteredSeq.getEventPointer(e)->message = shiftedMsg;
            }
            */

            filteredSequences.push_back(filteredSeq);
            int newIndex = static_cast<int>(filteredSequences.size()) - oneSequence;
            tracks.push_back(TrackPlaybackData{ newIndex, {} });
            
        }
    }
}

void MultipleTrackPlayer::setCurrentBPM(double newBPM)
{
    clock.setTempo(newBPM);
}

void MultipleTrackPlayer::stop(bool shouldModify)
{
    stopTimer();

    if (onElapsedUpdate)
    {
        juce::MessageManager::callAsync([this, alive = std::weak_ptr<bool> (liveness)]() {
            if (alive.expired()) return;
            onElapsedUpdate(noTime);
            });
    }

    auto sharedPtrDev = outputDevice.lock();
    for (int channel = lowestMidiChannel; channel <= highestMidiChannel; ++channel)
    {
        auto msg = juce::MidiMessage::allNotesOff(channel);
        if (sharedPtrDev)
            sharedPtrDev->sendMessageNow(msg);
        if (onMidiMessage)
            onMidiMessage(msg);
    }

    if (shouldModify)
    {
        trackPlayerModifyObjectsSubjects.notifyChangeStatesOfObjects();
        if (onStopTriggerClickFromPlayer)
            onStopTriggerClickFromPlayer();
    }
}

void MultipleTrackPlayer::start()
{

    lastTickSeconds = static_cast<double>(juce::Time::getHighResolutionTicks()) / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
    clock.restart();   // the tracks were scaled for the tempo set now

    for (auto& track : tracks)
        track.nextEventIndex = {};


    trackPlayerModifyObjectsSubjects.notifyChangeStatesOfObjects();
    startTimer(timerIntervalMs);
}

void MultipleTrackPlayer::updatePlaybackSettings(int channel, int newVolume, int newInstrument)
{
    if (auto sharedPtrDev=outputDevice.lock())
    {
        if (newInstrument != TrackEntryDefaults::keepCurrent && channel != drumChannel)
            sharedPtrDev->sendMessageNow(juce::MidiMessage::programChange(channel, newInstrument));

        if (newVolume != TrackEntryDefaults::keepCurrent)
            sharedPtrDev->sendMessageNow(juce::MidiMessage::controllerEvent(channel, channelVolumeCC, newVolume));
    }
}

MultipleTrackPlayer::~MultipleTrackPlayer()
{
    // JUCE: a HighResolutionTimer must be stopped in the derived destructor; otherwise a tick can still run
    // (and post) while this player's members are being destroyed.
    stopTimer();
}

int MultipleTrackPlayer::findNextEventIndex(const juce::MidiMessageSequence& seq, double currentTime)
{
    int left {};
    int right = seq.getNumEvents() - oneEvent;
    int result = seq.getNumEvents();

    while (left <= right)
    {
        int mid = (left + right) / halves;
        if (seq.getEventPointer(mid)->message.getTimeStamp() >= currentTime)
        {
            result = mid;
            right = mid - oneEvent;
        }
        else
        {
            left = mid + oneEvent;
        }
    }

    return result;
}

void MultipleTrackPlayer::syncPlaybackSettings()
{
    int melodicCount {};
    for (int trackIndex {}; trackIndex < currentTracks.size(); trackIndex++)
    {
        auto& track = currentTracks[trackIndex];
        int channel;
        if (track.type == TrackType::Percussion)
            channel = drumChannel;
        else {
            channel = melodicCount + firstTrackChannel;
            melodicCount++;
        }
        int instrument = track.instrumentAssociated;
        int volume = static_cast<int>(track.volumeAssociated);
        updatePlaybackSettings(channel, volume, instrument);
    }
}

void MultipleTrackPlayer::setDeviceOutputTrackPlayer(std::weak_ptr<juce::MidiOutput> newOutput)
{
    this->outputDevice = newOutput;
}

void MultipleTrackPlayer::setTimeSignatureDenominator(int newDenominator)
{
    this->timeSignatureDenominator = newDenominator;
}

void MultipleTrackPlayer::setTimeSignatureNumerator(int newNumerator)
{
    this->timeSignatureNumerator = newNumerator;
}

void MultipleTrackPlayer::setLastSectionUsed(const StyleSection& section)
{
    this->lastStyleSectionUsed = section;
    sectionApplied = false;
}

void MultipleTrackPlayer::resetLastSectionUsed()
{
    this->lastStyleSectionUsed.reset();
}

void MultipleTrackPlayer::addSubjectTrackPlayerListener(TrackPlayerListener* listener)
{
    trackPlayerSubjects.addListener(listener);
}

void MultipleTrackPlayer::addSubjectTrackPlayerModifyListener(TrackPlayerListenerModifyStateObjects* listener)
{
    trackPlayerModifyObjectsSubjects.addListener(listener);
}

void MultipleTrackPlayer::removeSubjectTrackPlayerListener(TrackPlayerListener* listener)
{
    trackPlayerSubjects.removeListener(listener);
}

void MultipleTrackPlayer::removeSubjectTrackPlayerModifyListener(TrackPlayerListenerModifyStateObjects* listener)
{
    trackPlayerModifyObjectsSubjects.removeListener(listener);
}


void MultipleTrackPlayer::hiResTimerCallback()
{
    auto midiOut = outputDevice.lock();
    const bool hasMidiOut = midiOut != nullptr;
    const bool hasInjectCallback = bool(onMidiMessage);

    if (!hasMidiOut && !hasInjectCallback)
    {
        stop();
        return;
    }

    double now = juce::Time::getHighResolutionTicks() /
        static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
    const double elapsed = clock.advance(now - lastTickSeconds);   // sequence seconds, at the live tempo
    lastTickSeconds = now;

    const double beatsElapsed = clock.beats();

    juce::MessageManager::callAsync([this, alive = std::weak_ptr<bool> (liveness), beatsElapsed, elapsed]
        {
            if (alive.expired()) return;
            if (onElapsedUpdate)
                onElapsedUpdate(beatsElapsed);
            trackPlayerSubjects.notifyMidPlay(elapsed);
        });

    for (auto& track : tracks)
    {
        auto& sequence = filteredSequences[track.filteredSequenceIndex];
        while (track.nextEventIndex < sequence.getNumEvents())
        {
            const auto& midiEvent = sequence.getEventPointer(track.nextEventIndex)->message;
            double eventTime = midiEvent.getTimeStamp();

            if (eventTime <= elapsed)
            {
                if (hasMidiOut)
                    midiOut->sendMessageNow(midiEvent);
                if (hasInjectCallback)
                    onMidiMessage(midiEvent);
                track.nextEventIndex++;
            }
            else break;
        }
    }

    bool allDone = std::all_of(tracks.begin(), tracks.end(),
        [this](const TrackPlaybackData& trackData) {
            return trackData.nextEventIndex >= filteredSequences[trackData.filteredSequenceIndex].getNumEvents();
        });

    if (allDone)
        stop();
}

const std::vector<TrackEntry>& MultipleTrackPlayer::getCurrentTracks() const
{
    return currentTracks;
}
