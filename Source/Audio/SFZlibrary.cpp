/*
  ==============================================================================

    SFZlibrary.cpp
    Created: 29 May 2026 4:13:50pm
    Author:  Oricum

  ==============================================================================
*/

#include "SFZlibrary.h"
#include "IOHelper.h"

void SFZLibraryManager::load(const juce::File& file)
{
    SFZLibraryIOHelper::loadFromFile(file, data);
}

void SFZLibraryManager::save(const juce::File& file) const
{
    SFZLibraryIOHelper::saveToFile(file, data);
}

const SFZLibraryData& SFZLibraryManager::getData() const
{
    return data;
}

const std::vector<SFZLibraryEntry>& SFZLibraryManager::getEntries() const
{
    return data.library;
}

juce::String SFZLibraryManager::addFile(const juce::File& sfzFile)
{
    if (!sfzFile.existsAsFile())
        return {};

    const auto fullPath = sfzFile.getFullPathName();

    for (const auto& entry : data.library)
    {
        if (entry.sfzPath == fullPath)
            return entry.id;
    }

    SFZLibraryEntry entry;
    entry.id = juce::Uuid().toString();
    entry.name = sfzFile.getFileNameWithoutExtension();
    entry.sfzPath = fullPath;

    data.library.push_back(entry);
    return entry.id;
}

void SFZLibraryManager::removeEntry(const juce::String& entryId)
{
    data.library.erase(std::remove_if(data.library.begin(), data.library.end(),
                                      [&entryId](const SFZLibraryEntry& entry)
                                      {
                                          return entry.id == entryId;
                                      }),
                       data.library.end());

    for (auto styleIt = data.styleMappings.begin(); styleIt != data.styleMappings.end();)
    {
        auto& instrumentMap = styleIt->second;
        for (auto it = instrumentMap.begin(); it != instrumentMap.end();)
        {
            if (it->second == entryId)
                it = instrumentMap.erase(it);
            else
                ++it;
        }
        if (instrumentMap.empty())
            styleIt = data.styleMappings.erase(styleIt);
        else
            ++styleIt;
    }
}

void SFZLibraryManager::importMappingsFromStyle(const juce::String& sourceStyleId, const juce::String& targetStyleId)
{
    if (sourceStyleId == targetStyleId) return;

    const auto sourceIt = data.styleMappings.find(sourceStyleId);
    if (sourceIt == data.styleMappings.end()) return;

    auto& targetMap = data.styleMappings[targetStyleId];
    for (const auto& pair : sourceIt->second)
        targetMap[pair.first] = pair.second;
}

void SFZLibraryManager::assignToStyleInstrument(const juce::String& styleId, int instrumentNumber, const juce::String& entryId)
{
    if (styleId.isEmpty() || instrumentNumber < 0 || instrumentNumber > 127 || getEntryById(entryId) == nullptr)
        return;

    data.styleMappings[styleId][instrumentNumber] = entryId;
}

void SFZLibraryManager::clearStyleInstrumentAssignment(const juce::String& styleId, int instrumentNumber)
{
    auto styleIt = data.styleMappings.find(styleId);
    if (styleIt == data.styleMappings.end())
        return;

    styleIt->second.erase(instrumentNumber);

    if (styleIt->second.empty())
        data.styleMappings.erase(styleIt);
}

const SFZLibraryEntry* SFZLibraryManager::getEntryById(const juce::String& entryId) const
{
    for (const auto& entry : data.library)
    {
        if (entry.id == entryId)
            return &entry;
    }

    return nullptr;
}

juce::File SFZLibraryManager::getSfzForStyleInstrument(const juce::String& styleId, int instrumentNumber) const
{
    const auto styleIt = data.styleMappings.find(styleId);
    if (styleIt == data.styleMappings.end())
        return {};

    const auto instrumentIt = styleIt->second.find(instrumentNumber);
    if (instrumentIt == styleIt->second.end())
        return {};

    if (const auto* entry = getEntryById(instrumentIt->second))
        return entry->getFile();

    return {};
}

const juce::StringArray& getGMInstrumentNames()
{
    static const juce::StringArray names {
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano",
        "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavi",
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone",
        "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ",
        "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
        "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar harmonics",
        "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass",
        "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        "Violin", "Viola", "Cello", "Contrabass",
        "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2",
        "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet",
        "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax",
        "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute",
        "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)",
        "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)",
        "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
        "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
        "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
        "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
        "Sitar", "Banjo", "Shamisen", "Koto",
        "Kalimba", "Bag pipe", "Fiddle", "Shanai",
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock",
        "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet",
        "Telephone Ring", "Helicopter", "Applause", "Gunshot"
    };
    return names;
}

juce::String getGMInstrumentName(int instrumentNumber)
{
    const auto& names = getGMInstrumentNames();
    if (instrumentNumber >= 0 && instrumentNumber < names.size())
        return names[instrumentNumber];

    return "Instrument " + juce::String(instrumentNumber);
}
