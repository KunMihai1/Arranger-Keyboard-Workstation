#include <JuceHeader.h>
#include "UI/Voices/InstrumentCatalog.h"

/*  The Voices page's instrument data (it was MainComponent::buildTree): 8 groups, 33 General MIDI programs, a search
    across every group. */

namespace InstrumentCatalogTestValues
{
    const juce::String testName     = "InstrumentCatalog";
    const juce::String testCategory = "Unit";

    const juce::String groupsAsBefore   = "the groups and programs are the instrument picker's, in its order";
    const juce::String searchAllGroups  = "the search matches names in every group, ignoring case, in catalogue order";
    const juce::String emptySearch      = "an empty or blank search matches nothing";
    const juce::String groupOfProgram   = "a program's group is found; a program outside the catalogue has none";

    const juce::String pianos          = "Pianos";
    const juce::String organs          = "Organs";
    const juce::String firstInstrument = "Acoustic Grand Piano";
    const juce::String bassQuery       = "BaSs";
    const juce::String organQuery      = "organ";
    const juce::String firstBass       = "Acoustic Bass";
    const juce::String bassoon         = "Bassoon";
    const juce::String contrabass      = "Contrabass";
    const juce::String blank           = "   ";

    constexpr size_t groupCount       = 8;
    constexpr size_t instrumentCount  = 33;
    constexpr size_t bassMatches      = 10;   // 8 basses, Bassoon, Contrabass
    constexpr size_t organMatches     = 5;
    constexpr size_t firstGroup       = 0;
    constexpr size_t lastGroup        = 7;
    constexpr size_t guitarsGroup     = 2;
    constexpr size_t firstMatch       = 0;
    constexpr int    grandPiano       = 0;
    constexpr int    nylonGuitar      = 24;
    constexpr int    outsideCatalogue = 127;   // Gunshot
}

class InstrumentCatalogTest : public juce::UnitTest
{
public:
    InstrumentCatalogTest() : juce::UnitTest (InstrumentCatalogTestValues::testName, InstrumentCatalogTestValues::testCategory) {}

    void runTest() override
    {
        using namespace InstrumentCatalogTestValues;

        beginTest (groupsAsBefore);
        {
            const auto& groups = InstrumentCatalog::groups();
            expectEquals (groups.size(), groupCount);
            size_t total {};
            for (const auto& group : groups)
                total += group.instruments.size();
            expectEquals (total, instrumentCount);
            expectEquals (groups[firstGroup].name, pianos);
            expectEquals (groups[lastGroup].name, organs);
            expectEquals (groups[firstGroup].instruments[firstMatch].name, firstInstrument);
            expectEquals (groups[firstGroup].instruments[firstMatch].program, grandPiano);
        }

        beginTest (searchAllGroups);
        {
            const auto basses = InstrumentCatalog::search (bassQuery);
            expectEquals (basses.size(), bassMatches);
            expectEquals (basses[firstMatch].name, firstBass);
            bool hasBassoon {}, hasContrabass {};
            for (const auto& instrument : basses)
            {
                hasBassoon    = hasBassoon || instrument.name == bassoon;
                hasContrabass = hasContrabass || instrument.name == contrabass;
            }
            expect (hasBassoon && hasContrabass);
            expectEquals (InstrumentCatalog::search (organQuery).size(), organMatches);
        }

        beginTest (emptySearch);
        {
            expect (InstrumentCatalog::search ({}).empty());
            expect (InstrumentCatalog::search (blank).empty());
        }

        beginTest (groupOfProgram);
        {
            expect (InstrumentCatalog::groupOf (nylonGuitar) == std::optional<size_t> (guitarsGroup));
            expect (InstrumentCatalog::groupOf (grandPiano) == std::optional<size_t> (firstGroup));
            expect (! InstrumentCatalog::groupOf (outsideCatalogue).has_value());
        }
    }
};
static InstrumentCatalogTest instrumentCatalogTest;
