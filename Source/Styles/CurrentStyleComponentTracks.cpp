#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"


void CurrentStyleComponent::applyChangesForOneTrack(TrackEntry& track)
{

    auto it = track.styleChangesMap.find(styleID);
    if (it != track.styleChangesMap.end())
    {
        std::vector<TrackIOHelper::NotePair> toApply;
        TrackIOHelper::extractNotePairEvents(track.sequence, toApply);
        TrackIOHelper::applyChangesToASequence(toApply, it->second);

        track.sequence.sort();
        track.sequence.updateMatchedPairs();
    }
}

void CurrentStyleComponent::applyChangesForAllTracksCurrentStyle()
{
    for (auto& [uuid, track] : mapUuidToTrackEntry)
    {
        if (track)
            applyChangesForOneTrack(*track);
    }
}

void CurrentStyleComponent::removingTrack(const juce::Uuid& uuid)
{
    for (auto& track : allTracks)
    {
        if (track->getUsedID() == uuid)
        {
            track->setNameLabel("None");
            track->setUUID(juce::Uuid{ "noID" });
            track->setTypeOfTrack("None");
            track->setChannel(2);
        }
    }
}

void CurrentStyleComponent::removingTracks(const std::vector<juce::Uuid>& uuids)
{
    for (auto& track : allTracks)
    {
        auto trackUUID = track->getUsedID();
        for (auto& uuid : uuids)
        {
            if (trackUUID == uuid)
            {
                track->setNameLabel("None");
                track->setUUID(juce::Uuid{ "noID" });
                track->setTypeOfTrack("None");
                track->setChannel(2);
                break;
            }
        }
    }
}

void CurrentStyleComponent::renamingTrack(const juce::Uuid& uuid, const juce::String& newName)
{
    for (auto& track:allTracks)
    {
        if (track->getUsedID() == uuid)
            track->setNameLabel(newName);
    }
}

void CurrentStyleComponent::showingTheInformationNotesFromTrack(const juce::Uuid& uuid, int channel)
{
    juce::MidiMessageSequence* sequence = nullptr;
    std::unordered_map<int, MidiChangeInfo>* changeMap = nullptr;
    juce::String displayName = "<Unnamed>";
    double originalBPMfromFile = 120.0;

    auto it = mapUuidToTrackEntry.find(uuid);
    if (it != mapUuidToTrackEntry.end())
    {
        auto& track = it->second;
        if (track != nullptr)
        {
            sequence = &track->sequence;
            changeMap = &track->styleChangesMap[styleID];
            originalBPMfromFile = track->originalBPM;

            if (track->displayName.isNotEmpty())
                displayName = track->displayName;
        }
    }

    // Fallbacks if no data
    static juce::MidiMessageSequence emptySequence;
    static std::unordered_map<int, MidiChangeInfo> emptyMap;


    auto container = std::make_unique<TableContainer>(
        sequence ? *sequence : emptySequence,
        displayName,
        channel,
        changeMap ? *changeMap : emptyMap,
        isPlaying
        );

    trackPlayer->addSubjectTrackPlayerModifyListener(container.get());

    container->setSize(350, 340);

    container->updateToFile = [this]()
    {
        if (updateTrackFile)
            updateTrackFile();
    };

    container->addModelAsListenerToTrackPlayer(trackPlayer.get());
    container->removeModelFromListener = [this](TrackPlayerListener* listener)
    {
        if (trackPlayer)
            trackPlayer->removeSubjectTrackPlayerListener(listener);
    };

    container->removeContainerFromListeners = [this, container=container.get()]()
    {
        trackPlayer->removeSubjectTrackPlayerModifyListener(container);
    };

    container->getCurrentBPMstyle = [this]()
    {
        return this->currentTempo;
    };

    container->isPlayerRunning = [this]()
    {
        return trackPlayer != nullptr && trackPlayer->isPlaying();
    };

    juce::CallOutBox::launchAsynchronously(
        std::move(container),
        getScreenBounds(),
        nullptr
    );

}

juce::OwnedArray<Track>& CurrentStyleComponent::getAllTracks()
{
    return allTracks;
}

MultipleTrackPlayer* CurrentStyleComponent::getTrackPlayer()
{
    return trackPlayer.get();
}

std::vector<CurrentStyleComponent::TrackChannelInstrument> CurrentStyleComponent::getTrackChannelInstruments() const
{
    std::vector<TrackChannelInstrument> result;
    for (const auto* track : allTracks)
        result.push_back({ track->getChannel(), track->getInstrumentNumber() });
    return result;
}

void CurrentStyleComponent::syncPercussionTracksVolumeChange(double newVolume)
{
    for (auto& track : allTracks)
    {
        if (track->getTypeOfTrack() == TrackTypeConversion::toString(TrackType::Percussion))
        {
            if (track->getVolume() != newVolume)
                track->setVolumeSlider(newVolume);
        }
    }
}

void CurrentStyleComponent::applyBPMchangeBeforePlayback(double userBPM, bool applyStyleChanges)
{
    if (userBPM <= 0.0)
        userBPM = 120.0;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    int channelCounter = 0;

    for (const auto& trackPtr : allTracks)
    {
        auto& tr = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (tr == nullptr)
            continue;

        int targetChannel = (tr->type == TrackType::Percussion) ? 10 : (channelCounter + 2);
        if (tr->type != TrackType::Percussion)
            ++channelCounter;

        double originalBPM = (tr->originalBPM > 0.0) ? tr->originalBPM : 120.0;
        double ratio = originalBPM / userBPM;

        juce::MidiMessageSequence scaledSequence;

        for (int i = 0; i < tr->originalSequenceTicks.getNumEvents(); ++i)
        {
            const auto& event = tr->originalSequenceTicks.getEventPointer(i)->message;
            double scaledTime = event.getTimeStamp() * ratio;

            juce::MidiMessage newMsg = event;
            newMsg.setTimeStamp(scaledTime);
            newMsg.setChannel(targetChannel);

            scaledSequence.addEvent(newMsg);
        }

        scaledSequence.sort();
        scaledSequence.updateMatchedPairs();


        tr->sequence = scaledSequence;
        if (applyStyleChanges)
        {
            applyChangesForOneTrack(*tr);
        }

        auto& toApply = tr->sequence;

        double firstNoteOnTime = 0.01;
        for (int i = 0; i < toApply.getNumEvents(); ++i)
        {
            const auto& msg = toApply.getEventPointer(i)->message;
            if (msg.isNoteOn())
            {
                firstNoteOnTime = std::max(0.01, msg.getTimeStamp() - 0.005);
                break;
            }
        }

        if (tr->volumeAssociated != -1)
        {
            toApply.addEvent(
                juce::MidiMessage::controllerEvent(targetChannel, 7, (int)tr->volumeAssociated)
                .withTimeStamp(firstNoteOnTime - 0.004));
        }

        if (targetChannel != 10 && tr->instrumentAssociated != -1)
        {
            toApply.addEvent(
                juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
                .withTimeStamp(firstNoteOnTime - 0.002));
        }
    }
}

//if this function is used, there's an issue with the sync of the whole tracks
void CurrentStyleComponent::applyBPMchangeForOne(double userBPM, const juce::Uuid& uuid)
{

    if (userBPM <= 0.0)
        userBPM = 120.0;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    auto it = mapUuidToTrackEntry.find(uuid);
    if (it == mapUuidToTrackEntry.end() || it->second == nullptr)
        return;

    auto& tr = it->second;


    int channelCounter = 0;
    int targetChannel = (tr->type == TrackType::Percussion) ? 10 : 2;
    for (const auto& trackPtr : allTracks)
    {
        if (trackPtr->getUsedID() == uuid)
            break;

        auto& other = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (other && other->type != TrackType::Percussion)
            ++channelCounter;
    }
    if (tr->type != TrackType::Percussion)
        targetChannel = channelCounter + 2;

    double originalBPM = (tr->originalBPM > 0.0) ? tr->originalBPM : 120.0;
    double ratio = originalBPM / userBPM;

    juce::MidiMessageSequence scaledSequence;

    for (int i = 0; i < tr->originalSequenceTicks.getNumEvents(); ++i)
    {
        const auto& event = tr->originalSequenceTicks.getEventPointer(i)->message;
        double scaledTime = event.getTimeStamp() * ratio;

        juce::MidiMessage newMsg = event;
        newMsg.setTimeStamp(scaledTime);
        newMsg.setChannel(targetChannel);
        scaledSequence.addEvent(newMsg);
    }

    scaledSequence.sort();
    scaledSequence.updateMatchedPairs();

    double firstNoteOnTime = 0.01;
    for (int i = 0; i < scaledSequence.getNumEvents(); ++i)
    {
        const auto& msg = scaledSequence.getEventPointer(i)->message;
        if (msg.isNoteOn())
        {
            firstNoteOnTime = std::max(0.01, msg.getTimeStamp() - 0.005);
            break;
        }
    }

    if (tr->volumeAssociated != -1)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::controllerEvent(targetChannel, 7, (int)tr->volumeAssociated)
            .withTimeStamp(firstNoteOnTime - 0.004));
    }

    if (targetChannel != 10 && tr->instrumentAssociated != -1)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
            .withTimeStamp(firstNoteOnTime - 0.002));
    }

    tr->sequence = scaledSequence;
}

juce::DynamicObject* CurrentStyleComponent::getJson() const
{
    auto* styleObj = new juce::DynamicObject{};

    styleObj->setProperty("name", name);
    styleObj->setProperty("BPM", currentTempo);
    styleObj->setProperty("StyleID", styleID);

    if (hasActiveArrangerConfig && activeArrangerConfigName.isNotEmpty())
        styleObj->setProperty("arrangerConfig", activeArrangerConfigName);   // remembered selection

    juce::Array<juce::var> tracksArray;

    for (auto* track : allTracks)
    {
        tracksArray.add(juce::var(track->getJson()));
    }

    styleObj->setProperty("tracks", tracksArray);

    return styleObj;
}

void CurrentStyleComponent::loadJson(const juce::var& styleVar)
{
    if (!styleVar.isObject())
        return;


    auto* obj = styleVar.getDynamicObject();
    if (!obj)
        return;

    updateName(obj->getProperty("name").toString());
    setTempo(static_cast<double>(obj->getProperty("BPM")));
    setStyleID(obj->getProperty("StyleID").toString());

    // Restore the remembered section configuration: selected (primed), not auto-playing.
    // Silent + graceful if the .style was since deleted/renamed (falls back to live tracks).
    {
        // Configuration selection is strictly per-style. This component is reused across
        // style tabs, so drop any config carried over from the previously-shown style
        // before restoring this style's own (if it has one).
        hasActiveArrangerConfig  = false;
        activeArrangerConfig     = {};
        activeArrangerConfigName = {};

        const auto cfgName = obj->getProperty("arrangerConfig").toString();
        if (cfgName.isNotEmpty())
        {
            auto file = IOHelper::getArrangerStylesFolder().getChildFile(cfgName + ".style");
            ArrangerStyle s;
            if (buildConfigFromFile(file, s, false))
            {
                activeArrangerConfig     = s;
                hasActiveArrangerConfig  = true;
                activeArrangerConfigName = cfgName;
                if (arrangerEngine) { arrangerEngine->setStyle(s); arrangerEngine->setBpm(currentTempo); }
            }
        }

        rebuildPlaySettingsItems();
        arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String());
    }

    auto tracksVar = obj->getProperty("tracks");

    if (!tracksVar.isArray())
        return;

    auto* tracksArray = tracksVar.getArray();
    int current = 2;

    for (int i = 0; i < allTracks.size() && i < tracksArray->size(); ++i)
    {
        auto trackVar = tracksArray->getReference(i);
        if (!trackVar.isObject())
            continue;

        auto* trackObj = trackVar.getDynamicObject();
        if (!trackObj)
            continue;

        auto name = trackObj->getProperty("name").toString();
        double volume = static_cast<double>(trackObj->getProperty("volume"));
        int instrumentNumber = static_cast<int>(trackObj->getProperty("instrumentNumber"));

        auto uuidSTR = trackObj->getProperty("uuid");

        juce::Uuid uuid{ uuidSTR };
        allTracks[i]->addListener(trackPlayer.get());
        auto type = trackObj->getProperty("type");
        allTracks[i]->setTypeOfTrack(type);

        if (type == "percussion")
        {
            allTracks[i]->setChannel(10);
        }
        else
        {
            allTracks[i]->setChannel(current);
            current++;
        }
        allTracks[i]->setNameLabel(name);
        allTracks[i]->setVolumeSlider(volume);
        allTracks[i]->setVolumeLabel(juce::String(volume));
        allTracks[i]->setInstrumentNumber(instrumentNumber);
        allTracks[i]->setUUID(uuid);
    }
}
