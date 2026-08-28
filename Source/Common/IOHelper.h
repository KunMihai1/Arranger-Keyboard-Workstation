/*
  ==============================================================================

    TrackIOHelper.h
    Created: 17 Jul 2025 3:29:02pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AppInfo.h"
#include "styleSettingsEntry.h"
#include "TrackEntry.h"
#include "StyleSection.h"
#include "PlayBackSettingsCustomComponent.h"
#include "SFZlibrary.h"

/**
 * @class IOHelper
 * @brief Provides utility functions for file and folder access.
 */
class IOHelper
{
public:
    /**
     * @brief Returns a folder in the user's application data directory.
     * @param name Name of the folder
     * @return juce::File object representing the folder
     */
    static juce::File getFolder(const juce::String& name);

    /**
     * @brief Returns the application's own data folder (%APPDATA%/AppInfo::appName).
     *        Prefer this over getFolder(...) with a literal name.
     */
    static juce::File getAppDataFolder();

    /**
     * @brief Returns a file in the application's data folder.
     * @param name Name of the file
     * @return juce::File object representing the file
     */
    static juce::File getFile(const juce::String& name);

    /**
     * @brief Returns the folder holding saved arranger *.style configurations,
     *        nested inside the app's data folder (<appName>/ArrangerStyles).
     */
    static juce::File getArrangerStylesFolder();

private:
};

/**
 * @class TrackIOHelper
 * @brief Handles loading, saving, and modifying MIDI track data.
 */
class TrackIOHelper
{
public:
    struct NotePair {
        juce::MidiMessage* noteOn;
        juce::MidiMessage* noteOff;
    };

    /**
     * @brief Saves grouped tracks to a file in JSON format.
     * @param file File to save the data
     * @param groupedTracks Map of folder names to track entries
     */
    static void saveToFile(const juce::File& file, const std::unordered_map<juce::String, std::deque<TrackEntry>>& groupedTracks);

    /**
     * @brief Loads grouped tracks from a file.
     * @param file File to load from
     * @param groupedTracks Output map of folder names to track entries
     * @param groupedTrackKeys Output vector of folder names in order
     */
    static void loadFromFile(const juce::File& file,
        std::unordered_map<juce::String, std::deque<TrackEntry>>& groupedTracks,
        std::vector<juce::String>& groupedTrackKeys);

    /**
     * @brief Checks whether a MIDI sequence contains percussion notes.
     * @param sequence Pointer to the MIDI sequence
     * @return true if the sequence contains percussion (channel 10), false otherwise
     */
    static bool foundPercussion(const juce::MidiMessageSequence* sequence);

    /**
     * @brief Extracts the original BPM from a MIDI file.
     * @param midiFile MIDI file to inspect
     * @return Original BPM if found, defaults to 120.0 if not
     */
    static double getOriginalBpmFromFile(const juce::MidiFile& midiFile);

    /**
     * @brief Converts all MIDI events in a file from ticks to seconds.
     * @param midiFile MIDI file to modify
     * @param bpm BPM value to calculate seconds per tick
     */
    static void convertTicksToSeconds(juce::MidiFile& midiFile, double bpm);

    /**
     * @brief Applies changes from a map to a MIDI sequence.
     * @param sequence MIDI sequence to modify
     * @param changesMap Map of row indices to MidiChangeInfo
     */
    static void applyChangesToASequence(juce::MidiMessageSequence& sequence, const std::unordered_map<int, MidiChangeInfo>& changesMap);

    static void applyChangesToASequence(std::vector<NotePair>& events, const std::unordered_map<int, MidiChangeInfo>& changesMap);

    static void extractNotePairEvents(juce::MidiMessageSequence& sequence, std::vector<NotePair>& noteOnVector);

private:
    
};

/**
 * @class PlaybackSettingsIOHelper
 * @brief Handles saving and loading playback settings for a device.
 */
class PlaybackSettingsIOHelper
{
public:
    /**
     * @brief Saves playback settings to a file.
     * @param properties Pointer to the properties storage
     * @param settings Settings to save
     * @param lowest Lowest value
     * @param highest Highest value
     * @param styleID Style ID
     */
    static void savePlaybackSettings(juce::PropertySet* properties, const PlayBackSettings& settings, int lowest, int highest, const juce::String& styleID);

    /**
     * @brief Loads playback settings from a file.
     * @param properties Pointer to the properties storage
     * @param VID Device VID
     * @param PID Device PID
     * @param styleID Style ID
     * @return Loaded PlayBackSettings object
     */
    static PlayBackSettings loadPlaybackSettings(const juce::PropertySet* properties, const juce::String& VID, const juce::String& PID, const juce::String& styleID);

private:
};

class SectionIOHelper
{
public:
    static void saveToFile(const juce::File& file, const std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>& map);
    
    static void loadFromFile(const juce::File& file, std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>& map);

private:

};


class EffectSettingsIOHelper
{
public:
    static void saveEffectsStyle(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const SoundSettings& s);

    static void saveSingleEffect(juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel, const juce::String& key, int value);

    static SoundSettings loadEffectsStyle(const juce::PropertySet* properties, const juce::String& styleID, const juce::String& VID, const juce::String& PID, int channel);
};

class SFZLibraryIOHelper
{
public:
    static void saveToFile(const juce::File& file, const SFZLibraryData& data);
    static void loadFromFile(const juce::File& file, SFZLibraryData& data);
};
