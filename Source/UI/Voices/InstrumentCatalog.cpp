#include "InstrumentCatalog.h"

namespace InstrumentCatalogData
{
    // The old instrument tree's groups and General MIDI programs (0-based), in its order.
    const std::vector<CatalogGroup> catalogue = {
        { "Pianos", {
            { "Acoustic Grand Piano", 0 },
            { "Electric Piano", 4 } } },
        { "Basses", {
            { "Acoustic Bass", 32 },
            { "Electric Bass (finger)", 33 },
            { "Electric Bass (pick)", 34 },
            { "Fretless Bass", 35 },
            { "Slap Bass 1", 36 },
            { "Slap Bass 2", 37 },
            { "Synth Bass 1", 38 },
            { "Synth Bass 2", 39 } } },
        { "Guitars", {
            { "Nylon Acoustic Guitar", 24 },
            { "Steel Acoustic Guitar", 25 },
            { "Jazz Electric Guitar", 26 },
            { "Clean Electric Guitar", 27 } } },
        { "WoodWinds", {
            { "Flute", 73 },
            { "Clarinet", 71 },
            { "Oboe", 68 },
            { "Bassoon", 70 } } },
        { "Brass", {
            { "Trumpet", 56 },
            { "Trombone", 57 },
            { "French Horn", 59 } } },
        { "Strings", {
            { "Violin", 40 },
            { "Viola", 41 },
            { "Cello", 42 },
            { "Contrabass", 43 } } },
        { "Reeds", {
            { "Accordion", 21 },
            { "Harmonica", 22 },
            { "Bandoneon", 23 } } },
        { "Organs", {
            { "Hammond Organ", 16 },
            { "Percussive Organ", 17 },
            { "Rock Organ", 18 },
            { "Church Organ", 19 },
            { "Reed Organ", 20 } } }
    };
}

namespace InstrumentCatalog
{
    const std::vector<CatalogGroup>& groups()
    {
        return InstrumentCatalogData::catalogue;
    }

    std::vector<CatalogInstrument> search (const juce::String& text)
    {
        std::vector<CatalogInstrument> matches;
        const auto query = text.trim();
        if (query.isEmpty())
            return matches;
        for (const auto& group : groups())
            for (const auto& instrument : group.instruments)
                if (instrument.name.containsIgnoreCase (query))
                    matches.push_back (instrument);
        return matches;
    }

    std::optional<size_t> groupOf (int program)
    {
        const auto& all = groups();
        for (size_t groupIndex {}; groupIndex < all.size(); ++groupIndex)
            for (const auto& instrument : all[groupIndex].instruments)
                if (instrument.program == program)
                    return groupIndex;
        return {};
    }
}
