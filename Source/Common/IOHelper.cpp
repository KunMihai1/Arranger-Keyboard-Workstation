/*
  ==============================================================================

    TrackIOHelper.cpp
    Created: 17 Jul 2025 3:29:02pm
    Author:  Kisuke

  ==============================================================================
*/

#include "IOHelper.h"

namespace
{
    juce::String getDeviceSettingsKey(const juce::String& VID, const juce::String& PID)
    {
        juce::String deviceKey = VID + "_" + PID;
        if (deviceKey == "_") deviceKey = "default";
        return deviceKey;
    }

    juce::String getStyleDeviceSettingsPrefix(const juce::String& styleID, const juce::String& VID, const juce::String& PID)
    {
        return styleID + "." + getDeviceSettingsKey(VID, PID) + ".";
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
    return getFile("ArrangerStyles");   // <appName>/ArrangerStyles
}

void TrackIOHelper::saveToFile(const juce::File& file, const std::unordered_map<juce::String, std::deque<TrackEntry>>& groupedTracks)
{
    juce::Array<juce::var> foldersArray;

    for (auto& [folderName, tracks] : groupedTracks)
    {
        // Create DynamicObject and wrap once in var
        auto* folderObj = new juce::DynamicObject{};
        juce::var folderVar(folderObj);

        folderObj->setProperty("folderName", folderName);

        if (!tracks.empty())
            folderObj->setProperty("filePath", tracks[0].file.getFullPathName());
        else
            folderObj->setProperty("filePath", juce::String());

        juce::Array<juce::var> trackArray;

        for (auto& tr : tracks)
        {
            auto* trackObj = new juce::DynamicObject{};
            juce::var trackVar(trackObj);

            trackObj->setProperty("trackIndex", tr.trackIndex);
            trackObj->setProperty("displayName", tr.displayName);
            trackObj->setProperty("uuid", tr.uuid.toString());

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

                        changeObj->setProperty("oldBPMchange", change.oldBPMchange);
                        changeObj->setProperty("oldNumber", change.oldNumber);
                        changeObj->setProperty("oldTimeStamp", change.oldTimeStamp);
                        changeObj->setProperty("oldVelocity", change.oldVelocity);

                        changeObj->setProperty("newBPMchange", change.newBPMchange);
                        changeObj->setProperty("newNumber", change.newNumber);
                        changeObj->setProperty("newTimeStamp", change.newTimeStamp);
                        changeObj->setProperty("newVelocity", change.newVelocity);

                        styleChangesObj->setProperty(juce::String(row), changeVar);
                    }

                    if (styleChangesObj->getProperties().size() > 0)
                        stylesObj->setProperty(styleName, styleChangesVar);
                }

                if (stylesObj->getProperties().size() > 0)
                    trackObj->setProperty("Styles", stylesVar);
            }

            trackArray.add(trackVar);
        }

        folderObj->setProperty("Tracks", trackArray);
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

        juce::String filePath = folderObj->getProperty("filePath").toString();
        juce::String folderName = folderObj->getProperty("folderName").toString();

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

        juce::var tracksVar = folderObj->getProperty("Tracks");

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

            int trackIndex = (int)trackObj->getProperty("trackIndex");
            juce::String displayName = trackObj->getProperty("displayName").toString();

            if (trackIndex >= midiFile.getNumTracks())
                continue;

            auto* sequence = midiFile.getTrack(trackIndex);
            if (sequence == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty("uuid").toString();

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


            auto* stylesDynamicObj = trackObj->getProperty("Styles").getDynamicObject();
            if (stylesDynamicObj != nullptr)
            {
                for (int s = 0; s < stylesDynamicObj->getProperties().size(); ++s)
                {
                    juce::Identifier styleKey = stylesDynamicObj->getProperties().getName(s);
                    juce::var styleVar = stylesDynamicObj->getProperties().getValueAt(s);

                    auto* styleChangesObj = styleVar.getDynamicObject();
                    if (styleChangesObj == nullptr)
                        continue;

                    std::unordered_map<int, MidiChangeInfo> styleChanges;

                    for (int i = 0; i < styleChangesObj->getProperties().size(); ++i)
                    {
                        juce::Identifier key = styleChangesObj->getProperties().getName(i);
                        juce::var changeVar = styleChangesObj->getProperties().getValueAt(i);

                        auto* changeObj = changeVar.getDynamicObject();
                        if (changeObj == nullptr)
                            continue;


                        MidiChangeInfo changeInfo;
                        changeInfo.oldBPMchange = (double)changeObj->getProperty("oldBPMchange");
                        changeInfo.newBPMchange = (double)changeObj->getProperty("newBPMchange");

                        changeInfo.oldNumber = (int)changeObj->getProperty("oldNumber");
                        changeInfo.oldTimeStamp = (double)changeObj->getProperty("oldTimeStamp");
                        changeInfo.oldVelocity = (int)changeObj->getProperty("oldVelocity");
                        changeInfo.newNumber = (int)changeObj->getProperty("newNumber");
                        changeInfo.newTimeStamp = (double)changeObj->getProperty("newTimeStamp");
                        changeInfo.newVelocity = (int)changeObj->getProperty("newVelocity");

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
    for (int i = 0; i < sequence->getNumEvents(); ++i)
    {
        auto msg = sequence->getEventPointer(i)->message;
        if (msg.isNoteOnOrOff() && msg.getChannel() == 10)
        {
            perc = true;
            break;
        }
    }
    return perc;
}

double TrackIOHelper::getOriginalBpmFromFile(const juce::MidiFile& midiFile)
{
    if (midiFile.getNumTracks() == 0)
        return 120.0;

    auto* firstTrack = midiFile.getTrack(0);
    if (firstTrack == nullptr)
        return 120.0;

    for (int i = 0; i < firstTrack->getNumEvents(); ++i)
    {
        const auto& msg = firstTrack->getEventPointer(i)->message;
        if (msg.isTempoMetaEvent())
        {
            double bpm;
            if (msg.getTempoSecondsPerQuarterNote() > 0.0)
                bpm = 60.0 / msg.getTempoSecondsPerQuarterNote();
            else bpm = 120.0;
            return bpm;
        }
    }
    return 120.0;
}

void TrackIOHelper::convertTicksToSeconds(juce::MidiFile& midiFile, double bpm)
{
    int tpqn = midiFile.getTimeFormat();
    if (tpqn <= 0)
        tpqn = 960;

    const double defaultTempoBPM = bpm; 
    const double secondsPerQuarterNote = 60.0 / defaultTempoBPM;
    const double secondsPerTick = secondsPerQuarterNote / double(tpqn);

    for (int t = 0; t < midiFile.getNumTracks(); ++t)
    {
        if (auto* seq = midiFile.getTrack(t))
        {
            for (int e = 0; e < seq->getNumEvents(); ++e)
            {
                auto& msg = seq->getEventPointer(e)->message;
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
        if (row < 0 || row >= sequence.getNumEvents())
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

        if (duration < 0)
            duration = 0.0;

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
        if (row < 0 || row >= pairs.size())
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
        bool shouldClamp = newTimeOn < 0;

        if (duration < 0)
            duration = 0; 

        juce::MidiMessage newOn = juce::MidiMessage::noteOn(
            channel, changeInfo.newNumber, (juce::uint8)changeInfo.newVelocity
        );
        if (shouldClamp)
            newOn.setTimeStamp(0.0);
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
    for (int i = 0; i < sequence.getNumEvents(); i++)
    {
        auto* event = sequence.getEventPointer(i);
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

void PlaybackSettingsIOHelper::savePlaybackSettings(juce::PropertySet* properties, const PlayBackSettings& settings, int lowest, int highest, const juce::String& styleID)
{
    if (!properties) return;

    int baseStart = 60;
    bool keyboardInputCase = false;
    if (highest - lowest < 49)
        keyboardInputCase = true;

    juce::String prefix = getStyleDeviceSettingsPrefix(styleID, settings.VID, settings.PID);

    if (keyboardInputCase && lowest != 60)
    {
        properties->setValue(prefix + "startNote", baseStart + settings.startNote % 12);
        properties->setValue(prefix + "endNote", baseStart + settings.endNote % 12);
        properties->setValue(prefix + "leftHandBound", baseStart + settings.leftHandBound % 12);
        properties->setValue(prefix + "rightHandBound", baseStart + settings.rightHandBound % 12);
    }
    else {
        properties->setValue(prefix + "startNote", settings.startNote);
        properties->setValue(prefix + "endNote", settings.endNote);
        properties->setValue(prefix + "leftHandBound", settings.leftHandBound);
        properties->setValue(prefix + "rightHandBound", settings.rightHandBound);
    }

    saveIfFileBacked(properties);
}

PlayBackSettings PlaybackSettingsIOHelper::loadPlaybackSettings(const juce::PropertySet* properties, const juce::String& VID, const juce::String& PID, const juce::String& styleID)
{
    PlayBackSettings settings{ -1,-1,-1,-1, "", "" };

    if (properties)
    {
        juce::String prefix = getStyleDeviceSettingsPrefix(styleID, VID, PID);

        settings.startNote = properties->getIntValue(prefix + "startNote", settings.startNote);
        settings.endNote = properties->getIntValue(prefix + "endNote", settings.endNote);
        settings.leftHandBound = properties->getIntValue(prefix + "leftHandBound", settings.leftHandBound);
        settings.rightHandBound = properties->getIntValue(prefix + "rightHandBound", settings.rightHandBound);
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

            sectionObj->setProperty("id", section.second.id);
            sectionObj->setProperty("name", section.second.name);
            sectionObj->setProperty("startTimeSeconds", section.second.startTimeSeconds);
            sectionObj->setProperty("endTimeSeconds", section.second.endTimeSeconds);
            sectionObj->setProperty("startBar", section.second.startBar);
            sectionObj->setProperty("endBar", section.second.endBar);

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
            
            section.id = sectionObj->getProperty("id").toString();
            section.name = sectionObj->getProperty("name").toString();
            section.startTimeSeconds = (double)sectionObj->getProperty("startTimeSeconds");
            section.endTimeSeconds = (double)sectionObj->getProperty("endTimeSeconds");
            section.startBar = (int)sectionObj->getProperty("startBar");
            section.endBar = (int)sectionObj->getProperty("endBar");

            sectionsMap[section.name] = section;
        }

        map[styleID] = std::move(sectionsMap);
    }
}

void EffectSettingsIOHelper::saveEffectsStyle(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const SoundSettings& s)
{
    if (!properties) return;

    const juce::String ch = (channel == 1 ? "First" : "Second");
    const juce::String p = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    properties->setValue(p + "reverb" + ch, s.reverb);
    properties->setValue(p + "volume" + ch, s.volume);

    properties->setValue(p + "brightness" + ch, s.brightness);
    properties->setValue(p + "chorus" + ch, s.chorus);
    properties->setValue(p + "expression" + ch, s.expression);
    properties->setValue(p + "resonance" + ch, s.resonance);
    properties->setValue(p + "sustainToggle" + ch, s.sustainToggle ? 1 : 0);

    properties->setValue(p + "attack" + ch, s.attack);
    properties->setValue(p + "decay" + ch, s.decay);
    properties->setValue(p + "release" + ch, s.release);
    properties->setValue(p + "vibrato" + ch, s.vibrato);

    properties->setValue(p + "delay" + ch, s.delay);
    properties->setValue(p + "pan" + ch, s.pan);

    properties->setValue(p + "distortion" + ch, s.distortion);
    properties->setValue(p + "filterTrack" + ch, s.filterTrack);
    properties->setValue(p + "tremolo" + ch, s.tremolo);
    properties->setValue(p + "randomMod" + ch, s.randomMod);

    saveIfFileBacked(properties);
}

void EffectSettingsIOHelper::saveSingleEffect(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const juce::String& key, int value)
{
    if (!properties) return;

    const juce::String ch = (channel == 1) ? "First" : "Second";
    const juce::String p = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    properties->setValue(p + key + ch, value);
    saveIfFileBacked(properties);
}

SoundSettings EffectSettingsIOHelper::loadEffectsStyle(const juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel)
{
    SoundSettings s;
    if (!properties) return s;

    const juce::String suffix = (channel == 1) ? "First" : "Second";
    const juce::String p = getStyleDeviceSettingsPrefix(styleID, VID, PID);

    s.reverb = properties->getIntValue(p + "reverb" + suffix, s.reverb);
    s.volume = properties->getIntValue(p + "volume" + suffix, s.volume);

    s.brightness = properties->getIntValue(p + "brightness" + suffix, s.brightness);
    s.chorus = properties->getIntValue(p + "chorus" + suffix, s.chorus);
    s.expression = properties->getIntValue(p + "expression" + suffix, s.expression);
    s.resonance = properties->getIntValue(p + "resonance" + suffix, s.resonance);
    s.sustainToggle = properties->getIntValue(p + "sustainToggle" + suffix, 0) != 0;

    s.attack = properties->getIntValue(p + "attack" + suffix, s.attack);
    s.decay = properties->getIntValue(p + "decay" + suffix, s.decay);
    s.release = properties->getIntValue(p + "release" + suffix, s.release);
    s.vibrato = properties->getIntValue(p + "vibrato" + suffix, s.vibrato);

    s.delay = properties->getIntValue(p + "delay" + suffix, s.delay);
    s.pan = properties->getIntValue(p + "pan" + suffix, s.pan);

    s.distortion = properties->getIntValue(p + "distortion" + suffix, s.distortion);
    s.filterTrack = properties->getIntValue(p + "filterTrack" + suffix, s.filterTrack);
    s.tremolo = properties->getIntValue(p + "tremolo" + suffix, s.tremolo);
    s.randomMod = properties->getIntValue(p + "randomMod" + suffix, s.randomMod);

    return s;
}

void SFZLibraryIOHelper::saveToFile(const juce::File& file, const SFZLibraryData& data)
{
    auto* rootObj = new juce::DynamicObject();
    juce::var rootVar(rootObj);

    juce::Array<juce::var> libraryArray;
    for (const auto& entry : data.library)
    {
        auto* entryObj = new juce::DynamicObject();
        entryObj->setProperty("id", entry.id);
        entryObj->setProperty("name", entry.name);
        entryObj->setProperty("sfzPath", entry.sfzPath);
        libraryArray.add(juce::var(entryObj));
    }
    rootObj->setProperty("library", libraryArray);

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
    rootObj->setProperty("styleMappings", juce::var(mappingsObj));

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

    if (rootObj->hasProperty("library"))
    {
        auto libraryVar = rootObj->getProperty("library");
        if (libraryVar.isArray())
        {
            auto* libraryArray = libraryVar.getArray();
            for (const auto& entryVar : *libraryArray)
            {
                if (auto* entryObj = entryVar.getDynamicObject())
                {
                    SFZLibraryEntry entry;
                    entry.id = entryObj->getProperty("id").toString();
                    entry.name = entryObj->getProperty("name").toString();
                    entry.sfzPath = entryObj->getProperty("sfzPath").toString();
                    data.library.push_back(entry);
                }
            }
        }
    }

    if (rootObj->hasProperty("styleMappings"))
    {
        auto mappingsVar = rootObj->getProperty("styleMappings");
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
