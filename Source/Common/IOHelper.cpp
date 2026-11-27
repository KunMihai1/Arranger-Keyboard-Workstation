/*
  ==============================================================================

    TrackIOHelper.cpp
    Created: 17 Jul 2025 3:29:02pm
    Author:  Kisuke

  ==============================================================================
*/

#include "IOHelper.h"

#include "PersistedKeys.h"

namespace MidiFileFacts
{
    constexpr int    drumChannel                = 10;
    constexpr int    tempoTrackIndex            = 0;     // a type-1 MIDI file keeps its tempo in track 0
    constexpr double defaultTempoBpm            = 120.0;
    constexpr double secondsPerMinute           = 60.0;
    constexpr double noTempo                    = 0.0;
    constexpr int    defaultTicksPerQuarterNote = 960;
    constexpr int    noTicksPerQuarterNote      = 0;
}

namespace SequenceEditing
{
    constexpr int    firstRow             = 0;
    constexpr double sequenceStartSeconds = 0.0;
    constexpr double noDuration           = 0.0;
}

namespace
{
    juce::String getDeviceSettingsKey(const juce::String& VID, const juce::String& PID)
    {
        juce::String deviceKey = VID + DeviceSettingsKeys::vidPidSeparator + PID;
        if (deviceKey == DeviceSettingsKeys::noDeviceKey) deviceKey = DeviceSettingsKeys::defaultDeviceKey;
        return deviceKey;
    }

    juce::String getStyleDeviceSettingsPrefix(const juce::String& styleID, const juce::String& VID, const juce::String& PID)
    {
        return styleID + DeviceSettingsKeys::prefixSeparator + getDeviceSettingsKey(VID, PID) + DeviceSettingsKeys::prefixSeparator;
    }

    void saveIfFileBacked(juce::PropertySet* properties)
    {
        if (auto* propertiesFile = dynamic_cast<juce::PropertiesFile*>(properties))
            propertiesFile->saveIfNeeded();
    }
}

juce::File IOHelper::getFolder(const juce::String& name)
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(name);

}

juce::File IOHelper::getAppDataFolder()
{
    return getFolder(AppInfo::appName);
}

juce::File IOHelper::getFile(const juce::String& name)
{
    auto appDataFolder = getAppDataFolder();

     return appDataFolder.getChildFile(name);
}

juce::File IOHelper::getArrangerStylesFolder()
{
    return getFile(AppDataNames::arrangerStylesFolder);   // <appName>/ArrangerStyles
}

void TrackIOHelper::saveToFile(const juce::File& file, const std::unordered_map<juce::String, std::deque<TrackEntry>>& groupedTracks)
{
    juce::Array<juce::var> foldersArray;

    for (auto& [folderName, tracks] : groupedTracks)
    {
        // Create DynamicObject and wrap once in var
        auto* folderObj = new juce::DynamicObject{};
        juce::var folderVar(folderObj);

        folderObj->setProperty(TrackJsonKeys::folderName, folderName);

        if (!tracks.empty())
            folderObj->setProperty(TrackJsonKeys::filePath, tracks.front().file.getFullPathName());
        else
            folderObj->setProperty(TrackJsonKeys::filePath, juce::String());

        juce::Array<juce::var> trackArray;

        for (auto& tr : tracks)
        {
            auto* trackObj = new juce::DynamicObject{};
            juce::var trackVar(trackObj);

            trackObj->setProperty(TrackJsonKeys::trackIndex, tr.trackIndex);
            trackObj->setProperty(TrackJsonKeys::displayName, tr.displayName);
            trackObj->setProperty(TrackJsonKeys::uuid, tr.uuid.toString());

            if (!tr.styleChangesMap.empty())
            {
                auto* stylesObj = new juce::DynamicObject();
                juce::var stylesVar(stylesObj);

                for (const auto& [styleName, changeMap] : tr.styleChangesMap)
                {
                    auto* styleChangesObj = new juce::DynamicObject();
                    juce::var styleChangesVar(styleChangesObj);

                    for (const auto& [row, change] : changeMap)
                    {

                        auto* changeObj = new juce::DynamicObject();
                        juce::var changeVar(changeObj);

                        changeObj->setProperty(NoteChangeJsonKeys::oldBpmChange, change.oldBPMchange);
                        changeObj->setProperty(NoteChangeJsonKeys::oldNumber, change.oldNumber);
                        changeObj->setProperty(NoteChangeJsonKeys::oldTimeStamp, change.oldTimeStamp);
                        changeObj->setProperty(NoteChangeJsonKeys::oldVelocity, change.oldVelocity);

                        changeObj->setProperty(NoteChangeJsonKeys::newBpmChange, change.newBPMchange);
                        changeObj->setProperty(NoteChangeJsonKeys::newNumber, change.newNumber);
                        changeObj->setProperty(NoteChangeJsonKeys::newTimeStamp, change.newTimeStamp);
                        changeObj->setProperty(NoteChangeJsonKeys::newVelocity, change.newVelocity);

                        styleChangesObj->setProperty(juce::String(row), changeVar);
                    }

                    if (! styleChangesObj->getProperties().isEmpty())
                        stylesObj->setProperty(styleName, styleChangesVar);
                }

                if (! stylesObj->getProperties().isEmpty())
                    trackObj->setProperty(TrackJsonKeys::styles, stylesVar);
            }

            trackArray.add(trackVar);
        }

        folderObj->setProperty(TrackJsonKeys::tracks, trackArray);
        foldersArray.add(folderVar);
    }

    juce::var jsonVar(foldersArray);
    juce::String jsonString = juce::JSON::toString(jsonVar);
    file.replaceWithText(jsonString);
}

void TrackIOHelper::loadFromFile(const juce::File& fileParam, std::unordered_map<juce::String, std::deque<TrackEntry>>& groupedTracks, std::vector<juce::String>& groupedTrackKeys)
{
    if (!fileParam.existsAsFile())
        return;

    juce::String jsonString = fileParam.loadFileAsString();
    juce::var jsonVar = juce::JSON::parse(jsonString);

    if (!jsonVar.isArray())
        return;

    groupedTrackKeys.clear();
    groupedTracks.clear();

    juce::Array<juce::var>* foldersArray = jsonVar.getArray();

    for (auto& item : *foldersArray)
    {
        auto* folderObj = item.getDynamicObject();
        if (folderObj == nullptr)
            continue;

        juce::String filePath = folderObj->getProperty(TrackJsonKeys::filePath).toString();
        juce::String folderName = folderObj->getProperty(TrackJsonKeys::folderName).toString();

        if (filePath.isEmpty())
        {
            // Empty folder, add folder name and an empty vector of tracks
            groupedTrackKeys.push_back(folderName);
            groupedTracks[folderName] = {};
            continue; // move on to next folder
        }

        juce::File file{ filePath };

        if (!file.existsAsFile())
            continue;

        groupedTrackKeys.push_back(folderName);
        groupedTracks[folderName] = {};

        juce::var tracksVar = folderObj->getProperty(TrackJsonKeys::tracks);

        if (!tracksVar.isArray())
            continue;

        juce::Array<juce::var>* trackArray = tracksVar.getArray();

        juce::FileInputStream inputStream(file);
        if (!inputStream.openedOk())
            continue;

        juce::MidiFile midiFile;
        if (!midiFile.readFrom(inputStream))
            continue;


        double originalBpm = getOriginalBpmFromFile(midiFile);

        convertTicksToSeconds(midiFile, originalBpm);

        for (auto& trackItem : *trackArray)
        {
            auto* trackObj = trackItem.getDynamicObject();
            if (trackObj == nullptr)
                continue;

            int trackIndex = (int)trackObj->getProperty(TrackJsonKeys::trackIndex);
            juce::String displayName = trackObj->getProperty(TrackJsonKeys::displayName).toString();

            if (trackIndex >= midiFile.getNumTracks())
                continue;

            auto* sequence = midiFile.getTrack(trackIndex);
            if (sequence == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty(TrackJsonKeys::uuid).toString();

            TrackEntry tr;
            tr.file = file;
            tr.trackIndex = trackIndex;
            tr.displayName = displayName;
            tr.originalSequenceTicks = *sequence;
            tr.sequence = *sequence;
            tr.sequence.updateMatchedPairs();
            tr.originalBPM = originalBpm;
            tr.folderName = folderName;

            if (foundPercussion(sequence))
                tr.type = TrackType::Percussion;
            else tr.type = TrackType::Melodic;

            if (uuidString.isNotEmpty())
                tr.uuid = juce::Uuid(uuidString);
            else tr.uuid = TrackEntry::generateUUID();


            auto* stylesDynamicObj = trackObj->getProperty(TrackJsonKeys::styles).getDynamicObject();
            if (stylesDynamicObj != nullptr)
            {
                for (int styleIndex {}; styleIndex < stylesDynamicObj->getProperties().size(); ++styleIndex)
                {
                    juce::Identifier styleKey = stylesDynamicObj->getProperties().getName(styleIndex);
                    juce::var styleVar = stylesDynamicObj->getProperties().getValueAt(styleIndex);

                    auto* styleChangesObj = styleVar.getDynamicObject();
                    if (styleChangesObj == nullptr)
                        continue;

                    std::unordered_map<int, MidiChangeInfo> styleChanges;

                    for (int changeIndex {}; changeIndex < styleChangesObj->getProperties().size(); ++changeIndex)
                    {
                        juce::Identifier key = styleChangesObj->getProperties().getName(changeIndex);
                        juce::var changeVar = styleChangesObj->getProperties().getValueAt(changeIndex);

                        auto* changeObj = changeVar.getDynamicObject();
                        if (changeObj == nullptr)
                            continue;


                        MidiChangeInfo changeInfo;
                        changeInfo.oldBPMchange = (double)changeObj->getProperty(NoteChangeJsonKeys::oldBpmChange);
                        changeInfo.newBPMchange = (double)changeObj->getProperty(NoteChangeJsonKeys::newBpmChange);

                        changeInfo.oldNumber = (int)changeObj->getProperty(NoteChangeJsonKeys::oldNumber);
                        changeInfo.oldTimeStamp = (double)changeObj->getProperty(NoteChangeJsonKeys::oldTimeStamp);
                        changeInfo.oldVelocity = (int)changeObj->getProperty(NoteChangeJsonKeys::oldVelocity);
                        changeInfo.newNumber = (int)changeObj->getProperty(NoteChangeJsonKeys::newNumber);
                        changeInfo.newTimeStamp = (double)changeObj->getProperty(NoteChangeJsonKeys::newTimeStamp);
                        changeInfo.newVelocity = (int)changeObj->getProperty(NoteChangeJsonKeys::newVelocity);

                        int row = key.toString().getIntValue();
                        styleChanges[row] = changeInfo;
                    }

                    tr.styleChangesMap[styleKey.toString()] = styleChanges;
                }
            }

            groupedTracks[folderName].push_back(tr); 
        }
    }
}

bool TrackIOHelper::foundPercussion(const juce::MidiMessageSequence* sequence)
{
    bool perc = false;
    for (int eventIndex {}; eventIndex < sequence->getNumEvents(); ++eventIndex)
    {
        auto msg = sequence->getEventPointer(eventIndex)->message;
        if (msg.isNoteOnOrOff() && msg.getChannel() == MidiFileFacts::drumChannel)
        {
            perc = true;
            break;
        }
    }
    return perc;
}

double TrackIOHelper::getOriginalBpmFromFile(const juce::MidiFile& midiFile)
{
    using namespace MidiFileFacts;
    if (midiFile.getNumTracks() <= tempoTrackIndex)
        return defaultTempoBpm;

    auto* firstTrack = midiFile.getTrack(tempoTrackIndex);
    if (firstTrack == nullptr)
        return defaultTempoBpm;

    for (int eventIndex {}; eventIndex < firstTrack->getNumEvents(); ++eventIndex)
    {
        const auto& msg = firstTrack->getEventPointer(eventIndex)->message;
        if (msg.isTempoMetaEvent())
        {
            double bpm;
            if (msg.getTempoSecondsPerQuarterNote() > noTempo)
                bpm = secondsPerMinute / msg.getTempoSecondsPerQuarterNote();
            else bpm = defaultTempoBpm;
            return bpm;
        }
    }
    return defaultTempoBpm;
}

void TrackIOHelper::convertTicksToSeconds(juce::MidiFile& midiFile, double bpm)
{
    int tpqn = midiFile.getTimeFormat();
    if (tpqn <= MidiFileFacts::noTicksPerQuarterNote)
        tpqn = MidiFileFacts::defaultTicksPerQuarterNote;

    const double defaultTempoBPM = bpm; 
    const double secondsPerQuarterNote = MidiFileFacts::secondsPerMinute / defaultTempoBPM;
    const double secondsPerTick = secondsPerQuarterNote / double(tpqn);

    for (int trackIndex {}; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        if (auto* seq = midiFile.getTrack(trackIndex))
        {
            for (int eventIndex {}; eventIndex < seq->getNumEvents(); ++eventIndex)
            {
                auto& msg = seq->getEventPointer(eventIndex)->message;
                double ticks = msg.getTimeStamp();
                msg.setTimeStamp(ticks * secondsPerTick);
            }
        }
    }
}

void TrackIOHelper::applyChangesToASequence(juce::MidiMessageSequence& sequence, const std::unordered_map<int, MidiChangeInfo>& changesMap)
{
    sequence.updateMatchedPairs();

    for (const auto& [row, changeInfo] : changesMap)
    {
        if (row < SequenceEditing::firstRow || row >= sequence.getNumEvents())
            continue;

        auto* onEvent = sequence.getEventPointer(row);
        if (!onEvent)
            continue;

        auto& msgOn = onEvent->message;

        if (!msgOn.isNoteOn())
            continue;

        auto* offEvent = onEvent->noteOffObject;
        if (!offEvent)
            continue;

        auto& msgOff = offEvent->message;

        double onTime = msgOn.getTimeStamp();
        double offTime = msgOff.getTimeStamp();
        double duration = offTime - onTime;

        double delta = changeInfo.oldTimeStamp - changeInfo.newTimeStamp;
        double newTimeOn = onTime- delta;

        if (duration < SequenceEditing::noDuration)
            duration = SequenceEditing::noDuration;

        int channel = msgOn.getChannel();

        juce::MidiMessage newOn = juce::MidiMessage::noteOn(
            channel,
            changeInfo.newNumber,
            (juce::uint8)changeInfo.newVelocity
        );
        newOn.setTimeStamp(newTimeOn);

        juce::MidiMessage newOff = juce::MidiMessage::noteOff(
            channel,
            changeInfo.newNumber
        );
        newOff.setTimeStamp(newTimeOn + duration);

        msgOn = newOn;
        msgOff = newOff;
    }

    sequence.sort();
    sequence.updateMatchedPairs();
}

void TrackIOHelper::applyChangesToASequence(std::vector<NotePair>& pairs, const std::unordered_map<int, MidiChangeInfo>& changesMap)
{
    for (const auto& [row, changeInfo] : changesMap)
    {
        if (row < SequenceEditing::firstRow || row >= pairs.size())
            continue;

        auto& pair = pairs[row];
        auto* msgOn = pair.noteOn;
        auto* msgOff = pair.noteOff;

        if (!msgOn || !msgOff)
            continue;

        int channel = msgOn->getChannel();

        double oldOn = msgOn->getTimeStamp();
        double oldOff = msgOff->getTimeStamp();
        double duration = oldOff - oldOn;

        double delta = changeInfo.oldTimeStamp - changeInfo.newTimeStamp;
        double newTimeOn = oldOn - delta;
        bool shouldClamp = newTimeOn < SequenceEditing::sequenceStartSeconds;

        if (duration < SequenceEditing::noDuration)
            duration = SequenceEditing::noDuration; 

        juce::MidiMessage newOn = juce::MidiMessage::noteOn(
            channel, changeInfo.newNumber, (juce::uint8)changeInfo.newVelocity
        );
        if (shouldClamp)
            newOn.setTimeStamp(SequenceEditing::sequenceStartSeconds);
        else newOn.setTimeStamp(newTimeOn);

        juce::MidiMessage newOff = juce::MidiMessage::noteOff(
            channel, changeInfo.newNumber
        );
        if (shouldClamp)
            newOff.setTimeStamp(duration);
        else newOff.setTimeStamp(newTimeOn + duration);
        

        *msgOn = newOn;
        *msgOff = newOff;
    }
}

void TrackIOHelper::extractNotePairEvents(juce::MidiMessageSequence& sequence, std::vector<NotePair>& notesVector)
{
    for (int eventIndex {}; eventIndex < sequence.getNumEvents(); eventIndex++)
    {
        auto* event = sequence.getEventPointer(eventIndex);
        if (!event)
            continue;

        if (event->message.isNoteOn())
        {
            auto* off = event->noteOffObject;
            if (off)
            {
                notesVector.push_back({ &event->message,&off->message });
            }
        }
    }
}

void PlaybackSettingsIOHelper::savePlaybackSettings(juce::PropertySet* properties, const PlayBackSettings& settings, const juce::String& styleID)
{
    if (!properties) return;

    // Saved as given: the caller saves the home-octave values (PlaybackRange). Folding a PC keyboard's notes into
    // the middle-C octave here moved a split saved while shifted by an octave (A3 came back as A4; none as B3).
    juce::String prefix = getStyleDeviceSettingsPrefix(styleID, settings.VID, settings.PID);
    properties->setValue(prefix + PlaybackSettingsKeys::startNote, settings.startNote);
    properties->setValue(prefix + PlaybackSettingsKeys::endNote, settings.endNote);
    properties->setValue(prefix + PlaybackSettingsKeys::leftHandBound, settings.leftHandBound);
    properties->setValue(prefix + PlaybackSettingsKeys::rightHandBound, settings.rightHandBound);

    saveIfFileBacked(properties);
}

PlayBackSettings PlaybackSettingsIOHelper::loadPlaybackSettings(const juce::PropertySet* properties, const juce::String& VID, const juce::String& PID, const juce::String& styleID)
{
    using PlaybackSettingsKeys::notSet;
    PlayBackSettings settings{ notSet, notSet, notSet, notSet, juce::String(), juce::String() };

    if (properties)
    {
        juce::String prefix = getStyleDeviceSettingsPrefix(styleID, VID, PID);

        settings.startNote = properties->getIntValue(prefix + PlaybackSettingsKeys::startNote, settings.startNote);
        settings.endNote = properties->getIntValue(prefix + PlaybackSettingsKeys::endNote, settings.endNote);
        settings.leftHandBound = properties->getIntValue(prefix + PlaybackSettingsKeys::leftHandBound, settings.leftHandBound);
        settings.rightHandBound = properties->getIntValue(prefix + PlaybackSettingsKeys::rightHandBound, settings.rightHandBound);
    }

    settings.VID = VID;
    settings.PID = PID;

    return settings;
}

void SectionIOHelper::saveToFile(const juce::File& file, const std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>& map)
{
    auto* rootObj = new juce::DynamicObject();
    juce::var rootVar(rootObj);

    for (const auto& [styleID, sections] : map)
    {
        juce::Array<juce::var> sectionsArray;
        for (const auto& section : sections)
        {
            auto* sectionObj = new juce::DynamicObject();
            juce::var sectionVar(sectionObj);

            sectionObj->setProperty(SectionJsonKeys::id, section.second.id);
            sectionObj->setProperty(SectionJsonKeys::name, section.second.name);
            sectionObj->setProperty(SectionJsonKeys::startTimeSeconds, section.second.startTimeSeconds);
            sectionObj->setProperty(SectionJsonKeys::endTimeSeconds, section.second.endTimeSeconds);
            sectionObj->setProperty(SectionJsonKeys::startBar, section.second.startBar);
            sectionObj->setProperty(SectionJsonKeys::endBar, section.second.endBar);

            sectionsArray.add(sectionVar);
        }
        rootObj->setProperty(styleID, sectionsArray);
    }

    juce::String jsonString = juce::JSON::toString(rootVar);
    file.replaceWithText(jsonString);
}

void SectionIOHelper::loadFromFile(const juce::File& file, std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>& map)
{
    map.clear();

    if (!file.existsAsFile())
        return;

    juce::String jsonString = file.loadFileAsString();
    juce::var jsonVar = juce::JSON::parse(jsonString);

    if (!jsonVar.isObject())
        return;

    auto* rootObj = jsonVar.getDynamicObject();
    if (!rootObj)
        return;

    for (auto& stylePair : rootObj->getProperties())
    {
        juce::String styleID = stylePair.name.toString();
        juce::var sectionsVar = stylePair.value;

        if (!sectionsVar.isArray())
            return;

        juce::Array<juce::var>* sectionsArray = sectionsVar.getArray();
        if (!sectionsArray)
            return;

        std::unordered_map<juce::String,StyleSection> sectionsMap;

        for (auto& sectionsVar : *sectionsArray)
        {
            auto* sectionObj = sectionsVar.getDynamicObject();
            if (!sectionObj)
                continue;

            StyleSection section;
            
            section.id = sectionObj->getProperty(SectionJsonKeys::id).toString();
            section.name = sectionObj->getProperty(SectionJsonKeys::name).toString();
            section.startTimeSeconds = (double)sectionObj->getProperty(SectionJsonKeys::startTimeSeconds);
            section.endTimeSeconds = (double)sectionObj->getProperty(SectionJsonKeys::endTimeSeconds);
            section.startBar = (int)sectionObj->getProperty(SectionJsonKeys::startBar);
            section.endBar = (int)sectionObj->getProperty(SectionJsonKeys::endBar);

            sectionsMap[section.name] = section;
        }

        map[styleID] = std::move(sectionsMap);
    }
}

void EffectSettingsIOHelper::saveEffectsStyle(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const SoundSettings& settings)
{
    if (!properties) return;

    const juce::String handSuffix = (channel == DeviceSettingsKeys::firstHandChannel ? DeviceSettingsKeys::firstHandSuffix : DeviceSettingsKeys::secondHandSuffix);
    const juce::String prefix = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    properties->setValue(prefix + EffectSettingsKeys::reverb + handSuffix, settings.reverb);
    properties->setValue(prefix + EffectSettingsKeys::volume + handSuffix, settings.volume);

    properties->setValue(prefix + EffectSettingsKeys::brightness + handSuffix, settings.brightness);
    properties->setValue(prefix + EffectSettingsKeys::chorus + handSuffix, settings.chorus);
    properties->setValue(prefix + EffectSettingsKeys::expression + handSuffix, settings.expression);
    properties->setValue(prefix + EffectSettingsKeys::resonance + handSuffix, settings.resonance);
    properties->setValue(prefix + EffectSettingsKeys::sustainToggle + handSuffix, settings.sustainToggle ? EffectSettingsKeys::sustainOn : EffectSettingsKeys::sustainOff);

    properties->setValue(prefix + EffectSettingsKeys::attack + handSuffix, settings.attack);
    properties->setValue(prefix + EffectSettingsKeys::decay + handSuffix, settings.decay);
    properties->setValue(prefix + EffectSettingsKeys::release + handSuffix, settings.release);
    properties->setValue(prefix + EffectSettingsKeys::vibrato + handSuffix, settings.vibrato);

    properties->setValue(prefix + EffectSettingsKeys::delay + handSuffix, settings.delay);
    properties->setValue(prefix + EffectSettingsKeys::pan + handSuffix, settings.pan);

    properties->setValue(prefix + EffectSettingsKeys::distortion + handSuffix, settings.distortion);
    properties->setValue(prefix + EffectSettingsKeys::filterTrack + handSuffix, settings.filterTrack);
    properties->setValue(prefix + EffectSettingsKeys::tremolo + handSuffix, settings.tremolo);
    properties->setValue(prefix + EffectSettingsKeys::randomMod + handSuffix, settings.randomMod);

    saveIfFileBacked(properties);
}

void EffectSettingsIOHelper::saveSingleEffect(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const juce::String& key, int value)
{
    if (!properties) return;

    const juce::String handSuffix = (channel == DeviceSettingsKeys::firstHandChannel) ? DeviceSettingsKeys::firstHandSuffix : DeviceSettingsKeys::secondHandSuffix;
    const juce::String prefix = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    properties->setValue(prefix + key + handSuffix, value);
    saveIfFileBacked(properties);
}

SoundSettings EffectSettingsIOHelper::loadEffectsStyle(const juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel)
{
    SoundSettings settings;
    if (!properties) return settings;

    const juce::String handSuffix = (channel == DeviceSettingsKeys::firstHandChannel) ? DeviceSettingsKeys::firstHandSuffix : DeviceSettingsKeys::secondHandSuffix;
    const juce::String prefix = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    settings.reverb = properties->getIntValue(prefix + EffectSettingsKeys::reverb + handSuffix, settings.reverb);
    settings.volume = properties->getIntValue(prefix + EffectSettingsKeys::volume + handSuffix, settings.volume);

    settings.brightness = properties->getIntValue(prefix + EffectSettingsKeys::brightness + handSuffix, settings.brightness);
    settings.chorus = properties->getIntValue(prefix + EffectSettingsKeys::chorus + handSuffix, settings.chorus);
    settings.expression = properties->getIntValue(prefix + EffectSettingsKeys::expression + handSuffix, settings.expression);
    settings.resonance = properties->getIntValue(prefix + EffectSettingsKeys::resonance + handSuffix, settings.resonance);
    settings.sustainToggle = properties->getIntValue(prefix + EffectSettingsKeys::sustainToggle + handSuffix, EffectSettingsKeys::sustainOff) != EffectSettingsKeys::sustainOff;

    settings.attack = properties->getIntValue(prefix + EffectSettingsKeys::attack + handSuffix, settings.attack);
    settings.decay = properties->getIntValue(prefix + EffectSettingsKeys::decay + handSuffix, settings.decay);
    settings.release = properties->getIntValue(prefix + EffectSettingsKeys::release + handSuffix, settings.release);
    settings.vibrato = properties->getIntValue(prefix + EffectSettingsKeys::vibrato + handSuffix, settings.vibrato);

    settings.delay = properties->getIntValue(prefix + EffectSettingsKeys::delay + handSuffix, settings.delay);
    settings.pan = properties->getIntValue(prefix + EffectSettingsKeys::pan + handSuffix, settings.pan);

    settings.distortion = properties->getIntValue(prefix + EffectSettingsKeys::distortion + handSuffix, settings.distortion);
    settings.filterTrack = properties->getIntValue(prefix + EffectSettingsKeys::filterTrack + handSuffix, settings.filterTrack);
    settings.tremolo = properties->getIntValue(prefix + EffectSettingsKeys::tremolo + handSuffix, settings.tremolo);
    settings.randomMod = properties->getIntValue(prefix + EffectSettingsKeys::randomMod + handSuffix, settings.randomMod);

    return settings;
}

void SFZLibraryIOHelper::saveToFile(const juce::File& file, const SFZLibraryData& data)
{
    auto* rootObj = new juce::DynamicObject();
    juce::var rootVar(rootObj);

    juce::Array<juce::var> libraryArray;
    for (const auto& entry : data.library)
    {
        auto* entryObj = new juce::DynamicObject();
        entryObj->setProperty(SfzLibraryJsonKeys::id, entry.id);
        entryObj->setProperty(SfzLibraryJsonKeys::name, entry.name);
        entryObj->setProperty(SfzLibraryJsonKeys::sfzPath, entry.sfzPath);
        libraryArray.add(juce::var(entryObj));
    }
    rootObj->setProperty(SfzLibraryJsonKeys::library, libraryArray);

    auto* mappingsObj = new juce::DynamicObject();
    for (const auto& [styleId, instruments] : data.styleMappings)
    {
        auto* instObj = new juce::DynamicObject();
        for (const auto& [instNum, entryId] : instruments)
        {
            instObj->setProperty(juce::String(instNum), entryId);
        }
        mappingsObj->setProperty(styleId, juce::var(instObj));
    }
    rootObj->setProperty(SfzLibraryJsonKeys::styleMappings, juce::var(mappingsObj));

    juce::String jsonString = juce::JSON::toString(rootVar);
    file.getParentDirectory().createDirectory();
    file.replaceWithText(jsonString);
}

void SFZLibraryIOHelper::loadFromFile(const juce::File& file, SFZLibraryData& data)
{
    data.library.clear();
    data.styleMappings.clear();

    if (!file.existsAsFile())
        return;

    juce::String jsonString = file.loadFileAsString();
    juce::var jsonVar = juce::JSON::parse(jsonString);

    if (!jsonVar.isObject())
        return;

    auto* rootObj = jsonVar.getDynamicObject();
    if (!rootObj)
        return;

    if (rootObj->hasProperty(SfzLibraryJsonKeys::library))
    {
        auto libraryVar = rootObj->getProperty(SfzLibraryJsonKeys::library);
        if (libraryVar.isArray())
        {
            auto* libraryArray = libraryVar.getArray();
            for (const auto& entryVar : *libraryArray)
            {
                if (auto* entryObj = entryVar.getDynamicObject())
                {
                    SFZLibraryEntry entry;
                    entry.id = entryObj->getProperty(SfzLibraryJsonKeys::id).toString();
                    entry.name = entryObj->getProperty(SfzLibraryJsonKeys::name).toString();
                    entry.sfzPath = entryObj->getProperty(SfzLibraryJsonKeys::sfzPath).toString();
                    data.library.push_back(entry);
                }
            }
        }
    }

    if (rootObj->hasProperty(SfzLibraryJsonKeys::styleMappings))
    {
        auto mappingsVar = rootObj->getProperty(SfzLibraryJsonKeys::styleMappings);
        if (auto* mappingsObj = mappingsVar.getDynamicObject())
        {
            for (auto& stylePair : mappingsObj->getProperties())
            {
                juce::String styleId = stylePair.name.toString();
                if (auto* instObj = stylePair.value.getDynamicObject())
                {
                    for (auto& instPair : instObj->getProperties())
                    {
                        int instNum = instPair.name.toString().getIntValue();
                        juce::String entryId = instPair.value.toString();
                        data.styleMappings[styleId][instNum] = entryId;
                    }
                }
            }
        }
    }
}
