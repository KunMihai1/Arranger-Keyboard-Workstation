#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "IOHelper.h"


class TrackIOHelperTest : public juce::UnitTest
{
public:
    TrackIOHelperTest() : juce::UnitTest("TrackIOHelper", "Unit") {}

    void runTest() override
    {

        beginTest("foundPercussion - returns true for channel 10 notes");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(10, 36, (juce::uint8)100), 0.0);
            seq.addEvent(juce::MidiMessage::noteOff(10, 36), 0.5);
            expect(TrackIOHelper::foundPercussion(&seq) == true);
        }

        beginTest("foundPercussion - returns false for non-channel-10 notes");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 0.5);
            expect(TrackIOHelper::foundPercussion(&seq) == false);
        }

        beginTest("foundPercussion - returns false for empty sequence");
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
        beginTest("foundPercussion - mixed channels, one is channel 10");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 0.5);
            seq.addEvent(juce::MidiMessage::noteOn(10, 42, (juce::uint8)80), 1.0);
            seq.addEvent(juce::MidiMessage::noteOff(10, 42), 1.5);
            expect(TrackIOHelper::foundPercussion(&seq) == true);
        }

        



        beginTest("getOriginalBpmFromFile - returns 120 for empty MIDI file");
        {
            juce::MidiFile midiFile;
            expect(TrackIOHelper::getOriginalBpmFromFile(midiFile) == 120.0);
        }

        beginTest("getOriginalBpmFromFile - reads tempo from first track");
        {
            juce::MidiFile midiFile;
            juce::MidiMessageSequence tempoTrack;

            // 140 BPM = 60/140 seconds per quarter note
            double spqn = 60.0 / 140.0;
            auto tempoMsg = juce::MidiMessage::tempoMetaEvent(
                static_cast<int>(spqn * 1000000.0)  // microseconds per quarter note
            );
            tempoMsg.setTimeStamp(0.0);
            tempoTrack.addEvent(tempoMsg);

            midiFile.addTrack(tempoTrack);
            double bpm = TrackIOHelper::getOriginalBpmFromFile(midiFile);
            
            //0.5 tolerance
            expect(std::abs(bpm - 140.0) < 0.5);
        }

        beginTest("getOriginalBpmFromFile - returns 120 if no tempo event");
        {
            juce::MidiFile midiFile;
            juce::MidiMessageSequence track;
            track.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
            midiFile.addTrack(track);
            expect(TrackIOHelper::getOriginalBpmFromFile(midiFile) == 120.0);
        }

        

        beginTest("convertTicksToSeconds - converts correctly at 120 BPM");
        {
            juce::MidiFile midiFile;
            midiFile.setTicksPerQuarterNote(960);

            juce::MidiMessageSequence track;
            // A note at tick 960 = 1 quarter note = 0.5 seconds at 120 BPM
            auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            noteOn.setTimeStamp(960.0);
            track.addEvent(noteOn);
            midiFile.addTrack(track);

            TrackIOHelper::convertTicksToSeconds(midiFile, 120.0);

            auto* seq = midiFile.getTrack(0);
            double convertedTime = seq->getEventPointer(0)->message.getTimeStamp();
            // At 120 BPM, 960 ticks = 0.5 seconds
            expect(std::abs(convertedTime - 0.5) < 0.001);
        }

        beginTest("convertTicksToSeconds - tick 0 stays at 0");
        {
            juce::MidiFile midiFile;
            midiFile.setTicksPerQuarterNote(480);

            juce::MidiMessageSequence track;
            auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            noteOn.setTimeStamp(0.0);
            track.addEvent(noteOn);
            midiFile.addTrack(track);

            TrackIOHelper::convertTicksToSeconds(midiFile, 120.0);

            auto* seq = midiFile.getTrack(0);
            expect(seq->getEventPointer(0)->message.getTimeStamp() == 0.0);
        }

        beginTest("applyChangesToASequence (NotePair) - applies note and velocity changes");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 1.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 2.0);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = 1.0;
            info.newTimeStamp = 1.0; 
            info.newNumber = 67;
            info.newVelocity = 110;
            changes[0] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[0].noteOn->getNoteNumber() == 67);
            expect(pairs[0].noteOn->getVelocity() == 110);
            expect(pairs[0].noteOff->getNoteNumber() == 67);
        }
        beginTest("applyChangesToASequence (NotePair) - shifts timestamp by delta");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 5.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 6.0);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = 5.0;
            info.newTimeStamp = 3.0;  
            info.newNumber = 60;
            info.newVelocity = 100;
            changes[0] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(std::abs(pairs[0].noteOn->getTimeStamp() - 3.0) < 0.001);
            expect(std::abs(pairs[0].noteOff->getTimeStamp() - 4.0) < 0.001); 
        }
        beginTest("applyChangesToASequence (NotePair) - clamps to 0 when shifted before start");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 1.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 2.0);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldTimeStamp = 1.0;
            info.newTimeStamp = 5.0;  

            info.oldTimeStamp = 1.0;
            info.newTimeStamp = 3.0;  

            info.oldTimeStamp = 5.0;
            info.newTimeStamp = 0.0;
            info.newNumber = 60;
            info.newVelocity = 100;
            changes[0] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[0].noteOn->getTimeStamp() == 0.0);        
            expect(std::abs(pairs[0].noteOff->getTimeStamp() - 1.0) < 0.001); 
        }
        beginTest("applyChangesToASequence (NotePair) - out of range row is ignored");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 1.0);
            seq.updateMatchedPairs();
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.newNumber = 72;
            info.newVelocity = 50;
            info.oldTimeStamp = 0.0;
            info.newTimeStamp = 0.0;
            changes[99] = info;
            TrackIOHelper::applyChangesToASequence(pairs, changes);
            expect(pairs[0].noteOn->getNoteNumber() == 60);  
        }

        beginTest("applyChangesToASequence - applies note number and velocity changes");
        {
            juce::MidiMessageSequence seq;
            auto on = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            on.setTimeStamp(1.0);
            auto off = juce::MidiMessage::noteOff(1, 60);
            off.setTimeStamp(2.0);
            seq.addEvent(on);
            seq.addEvent(off);
            seq.updateMatchedPairs();

            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.oldNumber = 60;
            info.oldTimeStamp = 1.0;
            info.oldVelocity = 100;
            info.newNumber = 64;        
            info.newTimeStamp = 1.0;    
            info.newVelocity = 80;
            changes[0] = info;

            TrackIOHelper::applyChangesToASequence(seq, changes);

            
            bool found = false;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                auto& msg = seq.getEventPointer(i)->message;
                if (msg.isNoteOn())
                {
                    expect(msg.getNoteNumber() == 64);
                    expect(msg.getVelocity() == 80);
                    found = true;
                    break;
                }
            }
            expect(found);
        }

        beginTest("applyChangesToASequence - out of range row is ignored");
        {
            juce::MidiMessageSequence seq;
            auto on = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
            on.setTimeStamp(0.0);
            auto off = juce::MidiMessage::noteOff(1, 60);
            off.setTimeStamp(1.0);
            seq.addEvent(on);
            seq.addEvent(off);
            seq.updateMatchedPairs();

            std::unordered_map<int, MidiChangeInfo> changes;
            MidiChangeInfo info;
            info.newNumber = 64;
            info.newVelocity = 80;
            info.oldTimeStamp = 0.0;
            info.newTimeStamp = 0.0;
            changes[99] = info;  

            TrackIOHelper::applyChangesToASequence(seq, changes);

            expect(seq.getEventPointer(0)->message.getNoteNumber() == 60);
        }

        

        beginTest("extractNotePairEvents - extracts matched note on/off pairs");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 60), 1.0);
            seq.addEvent(juce::MidiMessage::noteOn(1, 64, (juce::uint8)90), 1.0);
            seq.addEvent(juce::MidiMessage::noteOff(1, 64), 2.0);
            seq.updateMatchedPairs();

            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);

            expect((int)pairs.size() == 2);
            expect(pairs[0].noteOn->getNoteNumber() == 60);
            expect(pairs[0].noteOff->getNoteNumber() == 60);
            expect(pairs[1].noteOn->getNoteNumber() == 64);
            expect(pairs[1].noteOff->getNoteNumber() == 64);
        }

        beginTest("extractNotePairEvents - empty sequence gives empty result");
        {
            juce::MidiMessageSequence seq;
            std::vector<TrackIOHelper::NotePair> pairs;
            TrackIOHelper::extractNotePairEvents(seq, pairs);
            expect(pairs.empty());
        }

        

        beginTest("saveToFile and loadFromFile - basic roundtrip");
        {
            
            auto tempMidiFile = juce::File::createTempFile(".mid");
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(480);

                juce::MidiMessageSequence track;
                track.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
                track.addEvent(juce::MidiMessage::noteOff(1, 60), 480.0);
                track.addEvent(juce::MidiMessage::noteOn(1, 64, (juce::uint8)90), 480.0);
                track.addEvent(juce::MidiMessage::noteOff(1, 64), 960.0);

                midiFile.addTrack(track);

                juce::FileOutputStream outStream(tempMidiFile);
                if (outStream.openedOk())
                    midiFile.writeTo(outStream);
            }

            
            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry entry;
            entry.file = tempMidiFile;
            entry.trackIndex = 0;
            entry.displayName = "Piano";
            entry.folderName = "MyFolder";
            entry.uuid = TrackEntry::generateUUID();

            groupedTracks["MyFolder"].push_back(entry);

            
            auto tempJsonFile = juce::File::createTempFile(".json");
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            
            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            
            expect((int)loadedKeys.size() == 1);
            expect(loadedKeys[0] == "MyFolder");
            expect(loadedTracks.count("MyFolder") == 1);
            expect((int)loadedTracks["MyFolder"].size() == 1);

            auto& loadedEntry = loadedTracks["MyFolder"][0];
            expect(loadedEntry.displayName == "Piano");
            expect(loadedEntry.trackIndex == 0);
            expect(loadedEntry.folderName == "MyFolder");

            tempJsonFile.deleteFile();
            tempMidiFile.deleteFile();
        }

        beginTest("saveToFile and loadFromFile - with non empty changes map");
        {

            auto tempMidiFile = juce::File::createTempFile(".mid");
            {
                juce::MidiFile midiFile;
                midiFile.setTicksPerQuarterNote(480);

                juce::MidiMessageSequence track;
                track.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
                track.addEvent(juce::MidiMessage::noteOff(1, 60), 480.0);
                track.addEvent(juce::MidiMessage::noteOn(1, 64, (juce::uint8)90), 480.0);
                track.addEvent(juce::MidiMessage::noteOff(1, 64), 960.0);

                midiFile.addTrack(track);

                juce::FileOutputStream outStream(tempMidiFile);
                if (outStream.openedOk())
                    midiFile.writeTo(outStream);
            }


            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry entry;
            entry.file = tempMidiFile;
            entry.trackIndex = 0;
            entry.displayName = "Piano";
            entry.folderName = "MyFolder";
            entry.uuid = TrackEntry::generateUUID();

            std::unordered_map<juce::String, std::unordered_map<int, MidiChangeInfo>> changesMap;

            MidiChangeInfo newChange;

            newChange.oldVelocity = 80;
            newChange.oldNumber = 63;
            newChange.oldTimeStamp = 20.5;

            newChange.oldBPMchange = 120.0;

            newChange.newBPMchange = 150.0;
            newChange.newTimeStamp = 20.5;
            newChange.newNumber = 63;
            newChange.newVelocity = 90;

            std::unordered_map<int, MidiChangeInfo> innerMap;
            innerMap[1] = newChange;

            changesMap["Style1"] = innerMap;
            entry.styleChangesMap = changesMap;

            groupedTracks["MyFolder"].push_back(entry);


            auto tempJsonFile = juce::File::createTempFile(".json");
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);


            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);



            expect((int)loadedKeys.size() == 1);
            expect(loadedKeys[0] == "MyFolder");
            expect(loadedTracks.count("MyFolder") == 1);
            expect((int)loadedTracks["MyFolder"].size() == 1);

            auto& loadedEntry = loadedTracks["MyFolder"][0];
            expect(loadedEntry.displayName == "Piano");
            expect(loadedEntry.trackIndex == 0);
            expect(loadedEntry.folderName == "MyFolder");

            expect(loadedEntry.styleChangesMap.count("Style1") == 1);
            expect(loadedEntry.styleChangesMap["Style1"].count(1) == 1);


            auto& loadedChange = loadedEntry.styleChangesMap["Style1"][1];
            expect(loadedChange.oldVelocity == 80);
            expect(loadedChange.oldNumber == 63);
            expect(std::abs(loadedChange.oldTimeStamp - 20.5) < 0.001);
            expect(std::abs(loadedChange.oldBPMchange - 120.0) < 0.001);
            expect(std::abs(loadedChange.newBPMchange - 150.0) < 0.001);
            expect(std::abs(loadedChange.newTimeStamp - 20.5) < 0.001);
            expect(loadedChange.newNumber == 63);
            expect(loadedChange.newVelocity == 90);

            tempJsonFile.deleteFile();
            tempMidiFile.deleteFile();
        }

        beginTest("saveToFile and loadFromFile - empty folder survives save-load");
        {
            auto tempJsonFile = juce::File::createTempFile(".json");

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;
            groupedTracks["EmptyFolder"] = {};  // folder with no tracks

            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedKeys.size() == 1);
            expect(loadedKeys[0] == "EmptyFolder");
            expect(loadedTracks["EmptyFolder"].empty());

            tempJsonFile.deleteFile();
        }

        beginTest("loadFromFile - non-existent file does nothing");
        {
            juce::File nonExistent("C:\\this_track_file_does_not_exist.json");
            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;

            TrackIOHelper::loadFromFile(nonExistent, loadedTracks, loadedKeys);

            expect(loadedTracks.empty());
            expect(loadedKeys.empty());
        }

        beginTest("saveToFile and loadFromFile - multiple folders with tracks");
        {
            //create two temp MIDI files
            auto tempMidi1 = juce::File::createTempFile(".mid");
            auto tempMidi2 = juce::File::createTempFile(".mid");
            {
                juce::MidiFile mf;
                mf.setTicksPerQuarterNote(480);
                juce::MidiMessageSequence t;
                t.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);
                t.addEvent(juce::MidiMessage::noteOff(1, 60), 480.0);
                mf.addTrack(t);

                juce::FileOutputStream os1(tempMidi1);
                if (os1.openedOk()) mf.writeTo(os1);
            }
            {
                juce::MidiFile mf;
                mf.setTicksPerQuarterNote(480);
                juce::MidiMessageSequence t;
                t.addEvent(juce::MidiMessage::noteOn(10, 36, (juce::uint8)80), 0.0);
                t.addEvent(juce::MidiMessage::noteOff(10, 36), 240.0);
                mf.addTrack(t);

                juce::FileOutputStream os2(tempMidi2);
                if (os2.openedOk()) mf.writeTo(os2);
            }

            std::unordered_map<juce::String, std::deque<TrackEntry>> groupedTracks;

            TrackEntry e1;
            e1.file = tempMidi1;
            e1.trackIndex = 0;
            e1.displayName = "Melody";
            e1.folderName = "Songs";
            groupedTracks["Songs"].push_back(e1);

            TrackEntry e2;
            e2.file = tempMidi2;
            e2.trackIndex = 0;
            e2.displayName = "Drums";
            e2.folderName = "Beats";
            groupedTracks["Beats"].push_back(e2);

            auto tempJsonFile = juce::File::createTempFile(".json");
            TrackIOHelper::saveToFile(tempJsonFile, groupedTracks);

            std::unordered_map<juce::String, std::deque<TrackEntry>> loadedTracks;
            std::vector<juce::String> loadedKeys;
            TrackIOHelper::loadFromFile(tempJsonFile, loadedTracks, loadedKeys);

            expect((int)loadedKeys.size() == 2);
            expect(loadedTracks.count("Songs") == 1);
            expect(loadedTracks.count("Beats") == 1);
            expect(loadedTracks["Songs"][0].displayName == "Melody");
            expect(loadedTracks["Beats"][0].displayName == "Drums");

            //the drums track should be detected as percussion (channel 10)
            expect(loadedTracks["Beats"][0].type == TrackType::Percussion);

            tempJsonFile.deleteFile();
            tempMidi1.deleteFile();
            tempMidi2.deleteFile();
        }
    }
};

static TrackIOHelperTest trackIOHelperTest;
