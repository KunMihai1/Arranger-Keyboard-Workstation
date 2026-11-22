#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerModel.h"

namespace ArrangerModelTestValues
{
    const juce::String testName     = "ArrangerModel";
    const juce::String testCategory = "Arranger";

    const juce::String defaultsAndAssembly         = "defaults and assembly";
    const juce::String schemaAndAfterCompleteDefaults = "schema v2 and after-complete defaults";

    constexpr int          bassChannel    = 2;
    constexpr double       patternStart   = 0.0;
    constexpr int          middleC        = 60;
    constexpr juce::uint8  velocity       = 100;
    constexpr int          twoBars        = 2;
    constexpr int          oneItem        = 1;
    constexpr int          firstItem      = 0;
    constexpr int          schemaVersion2 = 2;
}

class ArrangerModelTest : public juce::UnitTest
{
public:
    ArrangerModelTest() : juce::UnitTest (ArrangerModelTestValues::testName, ArrangerModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerModelTestValues;

        beginTest (defaultsAndAssembly);
        {
            ArrangerStyle style;
            expect (style.schemaVersion == ARRANGER_SCHEMA_VERSION);
            expect (style.sections.empty());

            ArrangerTrack track;
            track.partType = ArrangerPartType::Bass;
            track.channel  = bassChannel;
            track.pattern.push_back ({ patternStart, juce::MidiMessage::noteOn (bassChannel, middleC, velocity) });

            ArrangerSection section;
            section.type = ArrangerSectionType::Variation;
            section.lengthBars = twoBars;
            section.tracks.push_back (track);

            style.sections.push_back (section);

            expectEquals ((int) style.sections.size(), oneItem);
            expectEquals (style.sections[firstItem].lengthBars, twoBars);
            expectEquals ((int) style.sections[firstItem].tracks[firstItem].pattern.size(), oneItem);
            expect (style.sections[firstItem].tracks[firstItem].partType == ArrangerPartType::Bass);
        }

        beginTest (schemaAndAfterCompleteDefaults);
        {
            expectEquals (ARRANGER_SCHEMA_VERSION, schemaVersion2);

            ArrangerSection section;
            expect (section.afterComplete == ArrangerAfterComplete::Loop); // sections loop unless told otherwise

            section.afterComplete = ArrangerAfterComplete::Stop;
            expect (section.afterComplete == ArrangerAfterComplete::Stop);
        }
    }
};

static ArrangerModelTest arrangerModelTest;
