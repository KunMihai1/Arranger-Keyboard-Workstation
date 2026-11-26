#pragma once
#include <JuceHeader.h>
#include <optional>

/** The Voices page's instruments: the groups and General MIDI programs the old instrument tree offered
    (it was MainComponent::buildTree), and a search across every group. Pure data, no UI. */
struct CatalogInstrument
{
    juce::String name;
    int program {};   ///< General MIDI program, 0-based
};

struct CatalogGroup
{
    juce::String name;
    std::vector<CatalogInstrument> instruments;
};

namespace InstrumentCatalog
{
    const std::vector<CatalogGroup>& groups();

    /** Instruments whose name contains the text (case-insensitive), in catalogue order; blank text matches none. */
    std::vector<CatalogInstrument> search (const juce::String& text);

    /** The index of the group holding this program, if the catalogue has it. */
    std::optional<size_t> groupOf (int program);
}
