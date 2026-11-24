#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"

namespace
{
    namespace Text
    {
        const juce::String unnamedTrack   = "<Unnamed>";
        const juce::String styleExtension = ".style";
        const juce::String notesTitle     = "Notes";
        const juce::String trackWord      = "Track ";
        const juce::String notesWord      = " notes";
        const char* const  middleDotUtf8  = " \xc2\xb7 ";   // " · " (U+00B7)
    }

    constexpr int    firstSlotNumber       = 1;      // the strips are numbered 1..8
    constexpr int    notesWindowWidth      = 350;
    constexpr int    notesWindowHeight     = 340;
    constexpr double noTempo               = 0.0;     // a BPM at or below this is unset
    constexpr int    channelVolumeCC       = 7;
    // Volume and program are sent just before the first note so the track starts with them.
    constexpr double earliestSetupTime     = 0.01;    // seconds
    constexpr double setupLeadTime         = 0.005;
    constexpr double volumeLeadTime        = 0.004;
    constexpr double programLeadTime       = 0.002;
}

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
            track->setNameLabel(StyleTrackJsonKeys::emptySlot);
            track->setUUID(juce::Uuid{ StyleTrackJsonKeys::noTrackId });
            track->setTypeOfTrack(StyleTrackJsonKeys::emptySlot);
            track->setChannel(MultipleTrackPlayer::firstTrackChannel);
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
                track->setNameLabel(StyleTrackJsonKeys::emptySlot);
                track->setUUID(juce::Uuid{ StyleTrackJsonKeys::noTrackId });
                track->setTypeOfTrack(StyleTrackJsonKeys::emptySlot);
                track->setChannel(MultipleTrackPlayer::firstTrackChannel);
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
    juce::String displayName = Text::unnamedTrack;
    double originalBPMfromFile = ArrangerStyleDefaults::tempoBpm;

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

    container->setSize(notesWindowWidth, notesWindowHeight);

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

    if (onShowNotes)
    {
        // "Notes · Bass" / "Track 2 · Finger Bass · 48 notes"
        int slot {};
        juce::String instrument;
        for (int trackIndex {}; trackIndex < allTracks.size(); ++trackIndex)
            if (allTracks[trackIndex]->getUsedID() == uuid && allTracks[trackIndex]->getChannel() == channel)
            {
                slot = trackIndex + firstSlotNumber;
                instrument = allTracks[trackIndex]->instrumentCaption();
                break;
            }
        const auto dot = juce::String::fromUTF8(Text::middleDotUtf8);
        const auto title = Text::notesTitle + dot + displayName;
        const auto subtitle = Text::trackWord + juce::String(slot) + dot + instrument + dot + juce::String(container->getNumNotes()) + Text::notesWord;
        onShowNotes(std::move(container), title, subtitle);
        return;
    }

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
    if (userBPM <= noTempo)
        userBPM = ArrangerStyleDefaults::tempoBpm;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    int channelCounter {};

    for (const auto& trackPtr : allTracks)
    {
        auto& tr = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (tr == nullptr)
            continue;

        int targetChannel = (tr->type == TrackType::Percussion) ? MultipleTrackPlayer::drumChannel : (channelCounter + MultipleTrackPlayer::firstTrackChannel);
        if (tr->type != TrackType::Percussion)
            ++channelCounter;

        double originalBPM = (tr->originalBPM > noTempo) ? tr->originalBPM : ArrangerStyleDefaults::tempoBpm;
        double ratio = originalBPM / userBPM;

        juce::MidiMessageSequence scaledSequence;

        for (int eventIndex {}; eventIndex < tr->originalSequenceTicks.getNumEvents(); ++eventIndex)
        {
            const auto& event = tr->originalSequenceTicks.getEventPointer(eventIndex)->message;
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

        double firstNoteOnTime = earliestSetupTime;
        for (int eventIndex {}; eventIndex < toApply.getNumEvents(); ++eventIndex)
        {
            const auto& msg = toApply.getEventPointer(eventIndex)->message;
            if (msg.isNoteOn())
            {
                firstNoteOnTime = std::max(earliestSetupTime, msg.getTimeStamp() - setupLeadTime);
                break;
            }
        }

        if (tr->volumeAssociated != TrackEntryDefaults::notSet)
        {
            toApply.addEvent(
                juce::MidiMessage::controllerEvent(targetChannel, channelVolumeCC, (int)tr->volumeAssociated)
                .withTimeStamp(firstNoteOnTime - volumeLeadTime));
        }

        if (targetChannel != MultipleTrackPlayer::drumChannel && tr->instrumentAssociated != TrackEntryDefaults::notSet)
        {
            toApply.addEvent(
                juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
                .withTimeStamp(firstNoteOnTime - programLeadTime));
        }
    }
}

//if this function is used, there's an issue with the sync of the whole tracks
void CurrentStyleComponent::applyBPMchangeForOne(double userBPM, const juce::Uuid& uuid)
{

    if (userBPM <= noTempo)
        userBPM = ArrangerStyleDefaults::tempoBpm;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    auto it = mapUuidToTrackEntry.find(uuid);
    if (it == mapUuidToTrackEntry.end() || it->second == nullptr)
        return;

    auto& tr = it->second;


    int channelCounter {};
    int targetChannel = (tr->type == TrackType::Percussion) ? MultipleTrackPlayer::drumChannel : MultipleTrackPlayer::firstTrackChannel;
    for (const auto& trackPtr : allTracks)
    {
        if (trackPtr->getUsedID() == uuid)
            break;

        auto& other = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (other && other->type != TrackType::Percussion)
            ++channelCounter;
    }
    if (tr->type != TrackType::Percussion)
        targetChannel = channelCounter + MultipleTrackPlayer::firstTrackChannel;

    double originalBPM = (tr->originalBPM > noTempo) ? tr->originalBPM : ArrangerStyleDefaults::tempoBpm;
    double ratio = originalBPM / userBPM;

    juce::MidiMessageSequence scaledSequence;

    for (int eventIndex {}; eventIndex < tr->originalSequenceTicks.getNumEvents(); ++eventIndex)
    {
        const auto& event = tr->originalSequenceTicks.getEventPointer(eventIndex)->message;
        double scaledTime = event.getTimeStamp() * ratio;

        juce::MidiMessage newMsg = event;
        newMsg.setTimeStamp(scaledTime);
        newMsg.setChannel(targetChannel);
        scaledSequence.addEvent(newMsg);
    }

    scaledSequence.sort();
    scaledSequence.updateMatchedPairs();

    double firstNoteOnTime = earliestSetupTime;
    for (int eventIndex {}; eventIndex < scaledSequence.getNumEvents(); ++eventIndex)
    {
        const auto& msg = scaledSequence.getEventPointer(eventIndex)->message;
        if (msg.isNoteOn())
        {
            firstNoteOnTime = std::max(earliestSetupTime, msg.getTimeStamp() - setupLeadTime);
            break;
        }
    }

    if (tr->volumeAssociated != TrackEntryDefaults::notSet)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::controllerEvent(targetChannel, channelVolumeCC, (int)tr->volumeAssociated)
            .withTimeStamp(firstNoteOnTime - volumeLeadTime));
    }

    if (targetChannel != MultipleTrackPlayer::drumChannel && tr->instrumentAssociated != TrackEntryDefaults::notSet)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
            .withTimeStamp(firstNoteOnTime - programLeadTime));
    }

    tr->sequence = scaledSequence;
}

juce::DynamicObject* CurrentStyleComponent::getJson() const
{
    auto* styleObj = new juce::DynamicObject{};

    styleObj->setProperty(AllStylesJsonKeys::name, name);
    styleObj->setProperty(AllStylesJsonKeys::bpm, currentTempo);
    styleObj->setProperty(AllStylesJsonKeys::styleId, styleID);

    if (hasActiveArrangerConfig && activeArrangerConfigName.isNotEmpty())
        styleObj->setProperty(AllStylesJsonKeys::arrangerConfig, activeArrangerConfigName);   // remembered selection

    juce::Array<juce::var> tracksArray;

    for (auto* track : allTracks)
    {
        tracksArray.add(juce::var(track->getJson()));
    }

    styleObj->setProperty(AllStylesJsonKeys::tracks, tracksArray);

    return styleObj;
}

void CurrentStyleComponent::loadJson(const juce::var& styleVar)
{
    if (!styleVar.isObject())
        return;


    auto* obj = styleVar.getDynamicObject();
    if (!obj)
        return;

    updateName(obj->getProperty(AllStylesJsonKeys::name).toString());
    setTempo(static_cast<double>(obj->getProperty(AllStylesJsonKeys::bpm)));
    setStyleID(obj->getProperty(AllStylesJsonKeys::styleId).toString());

    // Restore the remembered section configuration: selected (primed), not auto-playing.
    // Silent + graceful if the .style was since deleted/renamed (falls back to live tracks).
    {
        // Configuration selection is strictly per-style. This component is reused across
        // style tabs, so drop any config carried over from the previously-shown style
        // before restoring this style's own (if it has one).
        hasActiveArrangerConfig  = false;
        activeArrangerConfig     = {};
        activeArrangerConfigName = {};

        const auto cfgName = obj->getProperty(AllStylesJsonKeys::arrangerConfig).toString();
        if (cfgName.isNotEmpty())
        {
            auto file = IOHelper::getArrangerStylesFolder().getChildFile(cfgName + Text::styleExtension);
            ArrangerStyle savedStyle;
            if (buildConfigFromFile(file, savedStyle, false))
            {
                activeArrangerConfig     = savedStyle;
                hasActiveArrangerConfig  = true;
                activeArrangerConfigName = cfgName;
                if (arrangerEngine) { arrangerEngine->setStyle(savedStyle); arrangerEngine->setBpm(currentTempo); }
            }
        }

        rebuildPlaySettingsItems();
        arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String());
    }

    auto tracksVar = obj->getProperty(AllStylesJsonKeys::tracks);

    if (!tracksVar.isArray())
        return;

    auto* tracksArray = tracksVar.getArray();
    int current = MultipleTrackPlayer::firstTrackChannel;

    for (int trackIndex {}; trackIndex < allTracks.size() && trackIndex < tracksArray->size(); ++trackIndex)
    {
        auto trackVar = tracksArray->getReference(trackIndex);
        if (!trackVar.isObject())
            continue;

        auto* trackObj = trackVar.getDynamicObject();
        if (!trackObj)
            continue;

        auto name = trackObj->getProperty(StyleTrackJsonKeys::name).toString();
        double volume = static_cast<double>(trackObj->getProperty(StyleTrackJsonKeys::volume));
        int instrumentNumber = static_cast<int>(trackObj->getProperty(StyleTrackJsonKeys::instrumentNumber));

        auto uuidSTR = trackObj->getProperty(StyleTrackJsonKeys::uuid);

        juce::Uuid uuid{ uuidSTR };
        allTracks[trackIndex]->addListener(trackPlayer.get());
        auto type = trackObj->getProperty(StyleTrackJsonKeys::type);
        allTracks[trackIndex]->setTypeOfTrack(type);

        if (type == TrackTypeNames::percussion)
        {
            allTracks[trackIndex]->setChannel(MultipleTrackPlayer::drumChannel);
        }
        else
        {
            allTracks[trackIndex]->setChannel(current);
            current++;
        }
        allTracks[trackIndex]->setNameLabel(name);
        allTracks[trackIndex]->setVolumeSlider(volume);
        allTracks[trackIndex]->setVolumeLabel(juce::String(volume));
        allTracks[trackIndex]->setInstrumentNumber(instrumentNumber);
        allTracks[trackIndex]->setUUID(uuid);
    }
}
