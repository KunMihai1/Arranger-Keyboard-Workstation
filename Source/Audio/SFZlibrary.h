/*
  ==============================================================================

    SFZlibrary.h
    Created: 29 May 2026 4:13:50pm
    Author:  Oricum

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct SFZLibraryEntry
{
    juce::String id;
    juce::String name;
    juce::String sfzPath;

    juce::File getFile() const { return juce::File(sfzPath); }
};

using SFZInstrumentMap = std::unordered_map<int, juce::String>;
using SFZStyleMappings = std::unordered_map<juce::String, SFZInstrumentMap>;

struct SFZLibraryData
{
    std::vector<SFZLibraryEntry> library;
    SFZStyleMappings styleMappings;
};

class SFZLibraryManager
{
public:
    void load(const juce::File& file);
    void save(const juce::File& file) const;

    const SFZLibraryData& getData() const;
    const std::vector<SFZLibraryEntry>& getEntries() const;

    juce::String addFile(const juce::File& sfzFile);
    void removeEntry(const juce::String& entryId);

    void assignToStyleInstrument(const juce::String& styleId, int instrumentNumber, const juce::String& entryId);
    void clearStyleInstrumentAssignment(const juce::String& styleId, int instrumentNumber);
    void importMappingsFromStyle(const juce::String& sourceStyleId, const juce::String& targetStyleId);

    const SFZLibraryEntry* getEntryById(const juce::String& entryId) const;
    juce::File getSfzForStyleInstrument(const juce::String& styleId, int instrumentNumber) const;

private:
    SFZLibraryData data;
};

/** @brief The General MIDI instrument names, indexed by instrument (program) number 0-127.

    Instrument numbers are the keys SFZ mappings are stored under, so this table is the
    shared vocabulary for anything that has to name a mapping to the user. */
const juce::StringArray& getGMInstrumentNames();

/** @brief Returns the General MIDI name for an instrument number.
    @param instrumentNumber Program number 0-127.
    @return The instrument name, or "Instrument <n>" when the number is out of range. */
juce::String getGMInstrumentName(int instrumentNumber);
