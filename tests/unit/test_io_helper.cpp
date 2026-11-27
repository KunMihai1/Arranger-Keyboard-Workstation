#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "IOHelper.h"
#include "PersistedKeys.h"

namespace TrackIOHelperTestValues
{
    const juce::String testName     = "TrackIOHelper";
    const juce::String testCategory = "Unit";

    const juce::String foundPercussionReturnsTrueForChannel10Notes = "foundPercussion - returns true for channel 10 notes";
    const juce::String foundPercussionReturnsFalseForNonChannel10 = "foundPercussion - returns false for non-channel-10 notes";
    const juce::String foundPercussionReturnsFalseForEmptySequence = "foundPercussion - returns false for empty sequence";
    const juce::String foundPercussionMixedChannelsOneIsChannel10 = "foundPercussion - mixed channels, one is channel 10";
    const juce::String getOriginalBpmFromFileReturns120ForEmptyMIDIFile = "getOriginalBpmFromFile - returns 120 for empty MIDI file";
    const juce::String getOriginalBpmFromFileReadsTempoFromFirstTrack = "getOriginalBpmFromFile - reads tempo from first track";
    const juce::String getOriginalBpmFromFileReturns120IfNoTempoEvent = "getOriginalBpmFromFile - returns 120 if no tempo event";
    const juce::String convertTicksToSecondsConvertsCorrectlyAt120BPM = "convertTicksToSeconds - converts correctly at 120 BPM";
    const juce::String convertTicksToSecondsTick0StaysAt0 = "convertTicksToSeconds - tick 0 stays at 0";
    const juce::String applyChangesToASequenceNotePairAppliesNoteAndVelocityChanges = "applyChangesToASequence (NotePair) - applies note and velocity changes";
    const juce::String applyChangesToASequenceNotePairShiftsTimestampByDelta = "applyChangesToASequence (NotePair) - shifts timestamp by delta";
    const juce::String applyChangesToASequenceNotePairClampsTo0WhenShifted = "applyChangesToASequence (NotePair) - clamps to 0 when shifted before start";
    const juce::String applyChangesToASequenceNotePairOutOfRangeRowIs = "applyChangesToASequence (NotePair) - out of range row is ignored";
    const juce::String applyChangesToASequenceAppliesNoteNumberAndVelocityChanges = "applyChangesToASequence - applies note number and velocity changes";
    const juce::String applyChangesToASequenceOutOfRangeRowIsIgnored = "applyChangesToASequence - out of range row is ignored";
    const juce::String extractNotePairEventsExtractsMatchedNoteOnOffPairs = "extractNotePairEvents - extracts matched note on/off pairs";
    const juce::String extractNotePairEventsEmptySequenceGivesEmptyResult = "extractNotePairEvents - empty sequence gives empty result";
    const juce::String saveToFileAndLoadFromFileBasicRoundtrip = "saveToFile and loadFromFile - basic roundtrip";
    const juce::String saveToFileAndLoadFromFileWithNonEmptyChanges = "saveToFile and loadFromFile - with non empty changes map";
    const juce::String saveToFileAndLoadFromFileEmptyFolderSurvivesSave = "saveToFile and loadFromFile - empty folder survives save-load";
    const juce::String loadFromFileNonExistentFileDoesNothing = "loadFromFile - non-existent file does nothing";
    const juce::String saveToFileAndLoadFromFileMultipleFoldersWithTracks = "saveToFile and loadFromFile - multiple folders with tracks";
    const juce::String saveToFileAndLoadFromFileTracksFromTwoFilesInOneFolder = "saveToFile and loadFromFile - one folder, tracks from two files, each reloads from its own file";
    const juce::String loadFromFileFirstFileGoneKeepsFolderAndOtherTracks = "loadFromFile - the folder's first file is gone: the folder and the other file's tracks stay";
    const juce::String loadFromFileOldFolderPathStillLoads = "loadFromFile - a track without its own file path loads from the folder's file";

    constexpr int         melodicChannel = 1;
    constexpr int         drumChannel    = 10;
    constexpr int         kickNote       = 36;
    constexpr int         closedHiHat    = 42;
    constexpr int         middleC        = 60;
    constexpr int         noteEFlat4     = 63;
    constexpr int         noteE4         = 64;
    constexpr int         noteG4         = 67;
    constexpr int         noteC5         = 72;
    constexpr juce::uint8 velocity       = 100;
    constexpr juce::uint8 softerVelocity = 90;
    constexpr juce::uint8 softVelocity   = 80;
    constexpr int keyVelocity       = 100;
    constexpr int louderVelocity    = 110;
    constexpr int softerKeyVelocity = 90;
    constexpr int softKeyVelocity   = 80;
    constexpr int quietKeyVelocity  = 50;

    constexpr double atStart            = 0.0;
    constexpr double halfSecond         = 0.5;
    constexpr double oneSecond          = 1.0;
    constexpr double oneAndAHalfSeconds = 1.5;
    constexpr double twoSeconds         = 2.0;
    constexpr double threeSeconds       = 3.0;
    constexpr double fourSeconds        = 4.0;
    constexpr double fiveSeconds        = 5.0;
    constexpr double sixSeconds         = 6.0;
    constexpr double changeTime         = 20.5;
    constexpr double timeTolerance      = 0.001;

    constexpr int    standardResolution = 480;    // ticks per quarter note
    constexpr int    fineResolution     = 960;
    constexpr double eighthNoteTicks    = 240.0;  // at 480 ticks per quarter note
    constexpr double quarterNoteTicks   = 480.0;
    constexpr double halfNoteTicks      = 960.0;  // also one quarter note at 960 ticks

    constexpr double defaultTempo        = 120.0;
    constexpr double fileTempo           = 140.0;
    constexpr double changedTempo        = 150.0;
    constexpr double tempoTolerance      = 0.5;
    constexpr double secondsPerMinute    = 60.0;
    constexpr double microsecondsPerSecond = 1000000.0;

    constexpr int    firstTrack   = 0;
    constexpr int    firstEvent   = 0;
    constexpr int    firstRow     = 0;
    constexpr int    changedRow   = 1;
    constexpr int    missingRow   = 99;
    constexpr size_t firstPair    = 0;
    constexpr size_t secondPair   = 1;
    constexpr int    twoPairs     = 2;
    constexpr size_t firstKey     = 0;
    constexpr size_t firstEntry   = 0;
    constexpr size_t secondEntry  = 1;
    constexpr int    twoTracks    = 2;
    constexpr int    oneFolder    = 1;
    constexpr int    twoFolders   = 2;
    constexpr int    oneTrack     = 1;
    constexpr size_t present      = 1;

    const juce::String midiExtension   = ".mid";
    const juce::String jsonExtension   = ".json";
    const juce::String pianoName       = "Piano";
    const juce::String folderName      = "MyFolder";
    const juce::String styleName       = "Style1";
    const juce::String emptyFolderName = "EmptyFolder";
    const juce::String melodyName      = "Melody";
    const juce::String songsFolder     = "Songs";
    const juce::String drumsName       = "Drums";
    const juce::String beatsFolder     = "Beats";
    const juce::String missingTrackFile = "C:\\this_track_file_does_not_exist.json";
}

using namespace TrackIOHelperTestValues;


class TrackIOHelperTest : public juce::UnitTest
{
public:
    TrackIOHelperTest() : juce::UnitTest (TrackIOHelperTestValues::testName, TrackIOHelperTestValues::testCategory) {}

    void runTest() override
    {

        beginTest(foundPercussionReturnsTrueForChannel10Notes);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(drumChannel, kickNote, velocity), atStart);
            seq.addEvent(juce::MidiMessage::noteOff(drumChannel, kickNote), halfSecond);
            expect(TrackIOHelper::foundPercussion(&seq) == true);
        }

        beginTest(foundPercussionReturnsFalseForNonChannel10);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), halfSecond);
            expect(TrackIOHelper::foundPercussion(&seq) == false);
        }

        beginTest(foundPercussionReturnsFalseForEmptySequence);
        {
            juce::MidiMessageSequence seq;
            expect(TrackIOHelper::foundPercussion(&seq) == false);
        }


        //assumption is here that a midi file has been correctly been edited(this part isn't for my app to control)
        //in any situation possible, if a single track which should have all the notes on the same channel
        //has mixed channels, then i can't make neither assumption and correctly take a decision if it's
        //a percussion sequence of events or a normal one
        //check can be added so that when a user wants to add his midi file, a check for correctness of tracks
        //can be made
        beginTest(foundPercussionMixedChannelsOneIsChannel10);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), halfSecond);
            seq.addEvent(juce::MidiMessage::noteOn(drumChannel, closedHiHat, softVelocity), oneSecond);
            seq.addEvent(juce::MidiMessage::noteOff(drumChannel, closedHiHat), oneAndAHalfSeconds);
            expect(TrackIOHelper::foundPercussion(&seq) == true);
        }

        



        beginTest(getOriginalBpmFromFileReturns120ForEmptyMIDIFile);
        {
            juce::MidiFile midiFile;
            expect(TrackIOHelper::getOriginalBpmFromFile(midiFile) == defaultTempo);
        }

        beginTest(getOriginalBpmFromFileReadsTempoFromFirstTrack);
        {
            juce::MidiFile midiFile;
            juce::MidiMessageSequence tempoTrack;

            // 140 BPM = 60/140 seconds per quarter note
            double spqn = secondsPerMinute / fileTempo;
            auto tempoMsg = juce::MidiMessage::tempoMetaEvent(
                static_cast<int>(spqn * microsecondsPerSecond)  // microseconds per quarter note
            );
            tempoMsg.setTimeStamp(atStart);
            tempoTrack.addEvent(tempoMsg);

            midiFile.addTrack(tempoTrack);
            double bpm = TrackIOHelper::getOriginalBpmFromFile(midiFile);
            
            //0.5 tolerance
            expect(std::abs(bpm - fileTempo) < tempoTolerance);
        }

        beginTest(getOriginalBpmFromFileReturns120IfNoTempoEvent);
        {
            juce::MidiFile midiFile;
            juce::MidiMessageSequence track;
            track.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
            midiFile.addTrack(track);
            expect(TrackIOHelper::getOriginalBpmFromFile(midiFile) == defaultTempo);
        }

        

        beginTest(convertTicksToSecondsConvertsCorrectlyAt120BPM);
        {
            juce::MidiFile midiFile;
            midiFile.setTicksPerQuarterNote(fineResolution);

            juce::MidiMessageSequence track;
            // A note at tick 960 = 1 quarter note = 0.5 seconds at 120 BPM
            auto noteOn = juce::MidiMessage::noteOn(melodicChannel, middleC, velocity);
            noteOn.setTimeStamp(halfNoteTicks);
            track.addEvent(noteOn);
            midiFile.addTrack(track);

            TrackIOHelper::convertTicksToSeconds(midiFile, defaultTempo);

            auto* seq = midiFile.getTrack(firstTrack);
            double convertedTime = seq->getEventPointer(firstEvent)->message.getTimeStamp();
            // At 120 BPM, 960 ticks = 0.5 seconds
            expect(std::abs(convertedTime - halfSecond) < timeTolerance);
        }

        beginTest(convertTicksToSecondsTick0StaysAt0);
        {
            juce::MidiFile midiFile;
            midiFile.setTicksPerQuarterNote(standardResolution);

            juce::MidiMessageSequence track;
            auto noteOn = juce::MidiMessage::noteOn(melodicChannel, middleC, velocity);
            noteOn.setTimeStamp(atStart);
            track.addEvent(noteOn);
            midiFile.addTrack(track);

            TrackIOHelper::convertTicksToSeconds(midiFile, defaultTempo);

            auto* seq = midiFile.getTrack(firstTrack);
            expect(seq->getEventPointer(firstEvent)->message.getTimeStamp() == atStart);
        }

        beginTest(applyChangesToASequenceNotePairAppliesNoteAndVelocityChanges);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), oneSecond);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), twoSeconds);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = oneSecond;
            info.newTimeStamp = oneSecond; 
            info.newNumber = noteG4;
            info.newVelocity = louderVelocity;
            changes[firstRow] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[firstPair].noteOn->getNoteNumber() == noteG4);
            expect(pairs[firstPair].noteOn->getVelocity() == louderVelocity);
            expect(pairs[firstPair].noteOff->getNoteNumber() == noteG4);
        }
        beginTest(applyChangesToASequenceNotePairShiftsTimestampByDelta);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), fiveSeconds);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), sixSeconds);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = fiveSeconds;
            info.newTimeStamp = threeSeconds;  
            info.newNumber = middleC;
            info.newVelocity = keyVelocity;
            changes[firstRow] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(std::abs(pairs[firstPair].noteOn->getTimeStamp() - threeSeconds) < timeTolerance);
            expect(std::abs(pairs[firstPair].noteOff->getTimeStamp() - fourSeconds) < timeTolerance); 
        }
        beginTest(applyChangesToASequenceNotePairClampsTo0WhenShifted);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), oneSecond);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), twoSeconds);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = oneSecond;
            info.newTimeStamp = fiveSeconds;  

            info.oldTimeStamp = oneSecond;
            info.newTimeStamp = threeSeconds;  

            info.oldTimeStamp = fiveSeconds;
            info.newTimeStamp = atStart;
            info.newNumber = middleC;
            info.newVelocity = keyVelocity;
            changes[firstRow] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[firstPair].noteOn->getTimeStamp() == atStart);        
            expect(std::abs(pairs[firstPair].noteOff->getTimeStamp() - oneSecond) < timeTolerance); 
        }
        beginTest(applyChangesToASequenceNotePairOutOfRangeRowIs);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), oneSecond);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.newNumber = noteC5;
            info.newVelocity = quietKeyVelocity;
            info.oldTimeStamp = atStart;
            info.newTimeStamp = atStart;
            changes[missingRow] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[firstPair].noteOn->getNoteNumber() == middleC);  
        }

        beginTest(applyChangesToASequenceAppliesNoteNumberAndVelocityChanges);
        {
            juce::MidiMessageSequence seq;
            auto on = juce::MidiMessage::noteOn(melodicChannel, middleC, velocity);
            on.setTimeStamp(oneSecond);
            auto off = juce::MidiMessage::noteOff(melodicChannel, middleC);
            off.setTimeStamp(twoSeconds);
            seq.addEvent(on);
            seq.addEvent(off);
            seq.updateMatchedPairs();

            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldNumber = middleC;
            info.oldTimeStamp = oneSecond;
            info.oldVelocity = keyVelocity;
            info.newNumber = noteE4;        
            info.newTimeStamp = oneSecond;    
            info.newVelocity = softKeyVelocity;
            changes[firstRow] = info;

            TrackIOHelper::applyChangesToASequence(seq, changes);

            
            bool found = false;
            for (int eventIndex {}; eventIndex < seq.getNumEvents(); ++eventIndex)
            {
                auto& msg = seq.getEventPointer(eventIndex)->message;
                if (msg.isNoteOn())
                {
                    expect(msg.getNoteNumber() == noteE4);
                    expect(msg.getVelocity() == softKeyVelocity);
                    found = true;
                    break;
                }
            }
            expect(found);
        }

        beginTest(applyChangesToASequenceOutOfRangeRowIsIgnored);
        {
            juce::MidiMessageSequence seq;
            auto on = juce::MidiMessage::noteOn(melodicChannel, middleC, velocity);
            on.setTimeStamp(atStart);
            auto off = juce::MidiMessage::noteOff(melodicChannel, middleC);
            off.setTimeStamp(oneSecond);
            seq.addEvent(on);
            seq.addEvent(off);
            seq.updateMatchedPairs();

            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.newNumber = noteE4;
            info.newVelocity = softKeyVelocity;
            info.oldTimeStamp = atStart;
            info.newTimeStamp = atStart;
            changes[missingRow] = info;  

            TrackIOHelper::applyChangesToASequence(seq, changes);

            expect(seq.getEventPointer(firstEvent)->message.getNoteNumber() == middleC);
        }

        

        beginTest(extractNotePairEventsExtractsMatchedNoteOnOffPairs);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), oneSecond);
            seq.addEvent(juce::MidiMessage::noteOn(melodicChannel, noteE4, softerVelocity), oneSecond);
            seq.addEvent(juce::MidiMessage::noteOff(melodicChannel, noteE4), twoSeconds);
            seq.updateMatchedPairs();

            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);

            expect((int)pairs.size() == twoPairs);
            expect(pairs[firstPair].noteOn->getNoteNumber() == middleC);
            expect(pairs[firstPair].noteOff->getNoteNumber() == middleC);
            expect(pairs[secondPair].noteOn->getNoteNumber() == noteE4);
            expect(pairs[secondPair].noteOff->getNoteNumber() == noteE4);
        }

        beginTest(extractNotePairEventsEmptySequenceGivesEmptyResult);
        {
            juce::MidiMessageSequence seq;
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            expect(pairs.empty());
        }

        

        beginTest(saveToFileAndLoadFromFileBasicRoundtrip);
        {
            
            auto tempMidiFile = juce::File::createTempFile(midiExtension);
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(standardResolution);

                juce::MidiMessageSequence track;
                track.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
                track.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), quarterNoteTicks);
                track.addEvent(juce::MidiMessage::noteOn(melodicChannel, noteE4, softerVelocity), quarterNoteTicks);
                track.addEvent(juce::MidiMessage::noteOff(melodicChannel, noteE4), halfNoteTicks);

                midiFile.addTrack(track);

                juce::FileOutputStream outStream(tempMidiFile);
                if (outStream.openedOk())
                    midiFile.writeTo(outStream);
            }

            
            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry entry;
            entry.file = tempMidiFile;
            entry.trackIndex = firstTrack;
            entry.displayName = pianoName;
            entry.folderName = folderName;
            entry.uuid = TrackEntry::generateUUID();

            groupedTracks[folderName].push_back(entry);

            
            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            
            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            
            expect((int)loadedKeys.size() == oneFolder);
            expect(loadedKeys[firstKey] == folderName);
            expect(loadedTracks.count(folderName) == present);
            expect((int)loadedTracks[folderName].size() == oneTrack);

            auto& loadedEntry = loadedTracks[folderName][firstEntry];
            expect(loadedEntry.displayName == pianoName);
            expect(loadedEntry.trackIndex == firstTrack);
            expect(loadedEntry.folderName == folderName);

            tempJsonFile.deleteFile();
            tempMidiFile.deleteFile();
        }

        beginTest(saveToFileAndLoadFromFileWithNonEmptyChanges);
        {

            auto tempMidiFile = juce::File::createTempFile(midiExtension);
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(standardResolution);

                juce::MidiMessageSequence track;
                track.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
                track.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), quarterNoteTicks);
                track.addEvent(juce::MidiMessage::noteOn(melodicChannel, noteE4, softerVelocity), quarterNoteTicks);
                track.addEvent(juce::MidiMessage::noteOff(melodicChannel, noteE4), halfNoteTicks);

                midiFile.addTrack(track);

                juce::FileOutputStream outStream(tempMidiFile);
                if (outStream.openedOk())
                    midiFile.writeTo(outStream);
            }


            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry entry;
            entry.file = tempMidiFile;
            entry.trackIndex = firstTrack;
            entry.displayName = pianoName;
            entry.folderName = folderName;
            entry.uuid = TrackEntry::generateUUID();

            std::unordered_map<juce::String, std::unordered_map<int, MidiChangeInfo>> changesMap;

            MidiChangeInfo newChange;

            newChange.oldVelocity = softKeyVelocity;
            newChange.oldNumber = noteEFlat4;
            newChange.oldTimeStamp = changeTime;

            newChange.oldBPMchange = defaultTempo;

            newChange.newBPMchange = changedTempo;
            newChange.newTimeStamp = changeTime;
            newChange.newNumber = noteEFlat4;
            newChange.newVelocity = softerKeyVelocity;

            std::unordered_map<int, MidiChangeInfo> innerMap;
            innerMap[changedRow] = newChange;

            changesMap[styleName] = innerMap;
            entry.styleChangesMap = changesMap;

            groupedTracks[folderName].push_back(entry);


            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);


            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);



            expect((int)loadedKeys.size() == oneFolder);
            expect(loadedKeys[firstKey] == folderName);
            expect(loadedTracks.count(folderName) == present);
            expect((int)loadedTracks[folderName].size() == oneTrack);

            auto& loadedEntry = loadedTracks[folderName][firstEntry];
            expect(loadedEntry.displayName == pianoName);
            expect(loadedEntry.trackIndex == firstTrack);
            expect(loadedEntry.folderName == folderName);

            expect(loadedEntry.styleChangesMap.count(styleName) == present);
            expect(loadedEntry.styleChangesMap[styleName].count(changedRow) == present);


            auto& loadedChange = loadedEntry.styleChangesMap[styleName][changedRow];
            expect(loadedChange.oldVelocity == softKeyVelocity);
            expect(loadedChange.oldNumber == noteEFlat4);
            expect(std::abs(loadedChange.oldTimeStamp - changeTime) < timeTolerance);
            expect(std::abs(loadedChange.oldBPMchange - defaultTempo) < timeTolerance);
            expect(std::abs(loadedChange.newBPMchange - changedTempo) < timeTolerance);
            expect(std::abs(loadedChange.newTimeStamp - changeTime) < timeTolerance);
            expect(loadedChange.newNumber == noteEFlat4);
            expect(loadedChange.newVelocity == softerKeyVelocity);

            tempJsonFile.deleteFile();
            tempMidiFile.deleteFile();
        }

        beginTest(saveToFileAndLoadFromFileEmptyFolderSurvivesSave);
        {
            auto tempJsonFile = juce::File::createTempFile(jsonExtension);

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;
            groupedTracks[emptyFolderName] = {};  // folder with no tracks

            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedKeys.size() == oneFolder);
            expect(loadedKeys[firstKey] == emptyFolderName);
            expect(loadedTracks[emptyFolderName].empty());

            tempJsonFile.deleteFile();
        }

        beginTest(loadFromFileNonExistentFileDoesNothing);
        {
            juce::File nonExistent(missingTrackFile);
            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;

            TrackIOHelper::loadFromFile(nonExistent, loadedTracks, loadedKeys);

            expect(loadedTracks.empty());
            expect(loadedKeys.empty());
        }

        beginTest(saveToFileAndLoadFromFileMultipleFoldersWithTracks);
        {
            //create two temp MIDI files
            auto tempMidi1 = juce::File::createTempFile(midiExtension);
            auto tempMidi2 = juce::File::createTempFile(midiExtension);
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(standardResolution);
                juce::MidiMessageSequence sequence;
                sequence.addEvent(juce::MidiMessage::noteOn(melodicChannel, middleC, velocity), atStart);
                sequence.addEvent(juce::MidiMessage::noteOff(melodicChannel, middleC), quarterNoteTicks);
                midiFile.addTrack(sequence);

                juce::FileOutputStream melodyStream(tempMidi1);
                if (melodyStream.openedOk()) midiFile.writeTo(melodyStream);
            }
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(standardResolution);
                juce::MidiMessageSequence sequence;
                sequence.addEvent(juce::MidiMessage::noteOn(drumChannel, kickNote, softVelocity), atStart);
                sequence.addEvent(juce::MidiMessage::noteOff(drumChannel, kickNote), eighthNoteTicks);
                midiFile.addTrack(sequence);

                juce::FileOutputStream drumsStream(tempMidi2);
                if (drumsStream.openedOk()) midiFile.writeTo(drumsStream);
            }

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry melodyEntry;
            melodyEntry.file = tempMidi1;
            melodyEntry.trackIndex = firstTrack;
            melodyEntry.displayName = melodyName;
            melodyEntry.folderName = songsFolder;
            groupedTracks[songsFolder].push_back(melodyEntry);

            TrackEntry drumsEntry;
            drumsEntry.file = tempMidi2;
            drumsEntry.trackIndex = firstTrack;
            drumsEntry.displayName = drumsName;
            drumsEntry.folderName = beatsFolder;
            groupedTracks[beatsFolder].push_back(drumsEntry);

            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedKeys.size() == twoFolders);
            expect(loadedTracks.count(songsFolder) == present);
            expect(loadedTracks.count(beatsFolder) == present);
            expect(loadedTracks[songsFolder][firstEntry].displayName == melodyName);
            expect(loadedTracks[beatsFolder][firstEntry].displayName == drumsName);

            //the drums track should be detected as percussion (channel 10)
            expect(loadedTracks[beatsFolder][firstEntry].type == TrackType::Percussion);

            tempJsonFile.deleteFile();
            tempMidi1.deleteFile();
            tempMidi2.deleteFile();
        }

        beginTest(saveToFileAndLoadFromFileTracksFromTwoFilesInOneFolder);
        {
            auto melodyMidi = writeOneTrackMidiFile(melodicChannel, middleC);
            auto drumsMidi  = writeOneTrackMidiFile(drumChannel, kickNote);

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;
            groupedTracks[folderName].push_back(makeEntry(melodyMidi, melodyName));
            groupedTracks[folderName].push_back(makeEntry(drumsMidi, drumsName));

            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedTracks[folderName].size() == twoTracks);
            auto& loadedDrums = loadedTracks[folderName][secondEntry];
            expect(loadedDrums.file == drumsMidi, loadedDrums.file.getFullPathName());
            expect(loadedDrums.type == TrackType::Percussion);
            expect(loadedDrums.sequence.getEventPointer(firstEvent)->message.getNoteNumber() == kickNote);

            tempJsonFile.deleteFile();
            melodyMidi.deleteFile();
            drumsMidi.deleteFile();
        }

        beginTest(loadFromFileFirstFileGoneKeepsFolderAndOtherTracks);
        {
            auto melodyMidi = writeOneTrackMidiFile(melodicChannel, middleC);
            auto drumsMidi  = writeOneTrackMidiFile(drumChannel, kickNote);

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;
            groupedTracks[folderName].push_back(makeEntry(melodyMidi, melodyName));
            groupedTracks[folderName].push_back(makeEntry(drumsMidi, drumsName));

            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);
            melodyMidi.deleteFile();

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedKeys.size() == oneFolder);
            expect((int)loadedTracks[folderName].size() == oneTrack);
            expect(loadedTracks[folderName][firstEntry].file == drumsMidi);

            tempJsonFile.deleteFile();
            drumsMidi.deleteFile();
        }

        beginTest(loadFromFileOldFolderPathStillLoads);
        {
            auto melodyMidi = writeOneTrackMidiFile(melodicChannel, middleC);

            auto* trackObj = new juce::DynamicObject{};
            juce::var trackVar(trackObj);
            trackObj->setProperty(TrackJsonKeys::trackIndex, firstTrack);
            trackObj->setProperty(TrackJsonKeys::displayName, melodyName);

            auto* folderObj = new juce::DynamicObject{};
            juce::var folderVar(folderObj);
            folderObj->setProperty(TrackJsonKeys::folderName, folderName);
            folderObj->setProperty(TrackJsonKeys::filePath, melodyMidi.getFullPathName());
            folderObj->setProperty(TrackJsonKeys::tracks, juce::Array<juce::var> { trackVar });

            auto tempJsonFile = juce::File::createTempFile(jsonExtension);
            tempJsonFile.replaceWithText(juce::JSON::toString(juce::Array<juce::var> { folderVar }));

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedTracks[folderName].size() == oneTrack);
            expect(loadedTracks[folderName][firstEntry].file == melodyMidi);

            tempJsonFile.deleteFile();
            melodyMidi.deleteFile();
        }
    }

private:
    static juce::File writeOneTrackMidiFile(int channel, int noteNumber)
    {
        auto midiFileOnDisk = juce::File::createTempFile(midiExtension);
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(standardResolution);
        juce::MidiMessageSequence sequence;
        sequence.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, velocity), atStart);
        sequence.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), quarterNoteTicks);
        midiFile.addTrack(sequence);

        juce::FileOutputStream outStream(midiFileOnDisk);
        if (outStream.openedOk())
            midiFile.writeTo(outStream);
        return midiFileOnDisk;
    }

    static TrackEntry makeEntry(const juce::File& midiFileOnDisk, const juce::String& displayName)
    {
        TrackEntry entry;
        entry.file = midiFileOnDisk;
        entry.trackIndex = firstTrack;
        entry.displayName = displayName;
        entry.folderName = folderName;
        entry.uuid = TrackEntry::generateUUID();
        return entry;
    }
};

static TrackIOHelperTest trackIOHelperTest;
