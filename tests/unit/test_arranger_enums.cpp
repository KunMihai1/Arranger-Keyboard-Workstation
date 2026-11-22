#include <juce_core/juce_core.h>
#include "Arranger/ArrangerEnums.h"

namespace ArrangerEnumsTestValues
{
    const juce::String testName     = "ArrangerEnums";
    const juce::String testCategory = "Arranger";

    const juce::String sectionTypeRoundTrip   = "section type round-trips through string";
    const juce::String afterCompleteRoundTrip = "afterComplete round-trips through string";
    const juce::String partTypeRoundTrip      = "partType round-trips through string";
    const juce::String unknownFallsBack       = "unknown strings fall back to safe defaults";

    const juce::String unknownText = "nope";
}

class ArrangerEnumsTest : public juce::UnitTest
{
public:
    ArrangerEnumsTest() : juce::UnitTest (ArrangerEnumsTestValues::testName, ArrangerEnumsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ArrangerEnumsTestValues;

        beginTest (sectionTypeRoundTrip);
        {
            for (auto type : { ArrangerSectionType::Intro, ArrangerSectionType::Variation,
                               ArrangerSectionType::Fill, ArrangerSectionType::Break,
                               ArrangerSectionType::Ending, ArrangerSectionType::CountIn })
                expect (ArrangerEnums::sectionTypeFromString (ArrangerEnums::toString (type)) == type);
        }

        beginTest (afterCompleteRoundTrip);
        {
            for (auto after : { ArrangerAfterComplete::Loop, ArrangerAfterComplete::FallThrough,
                                ArrangerAfterComplete::Stop })
                expect (ArrangerEnums::afterFromString (ArrangerEnums::toString (after)) == after);
        }

        beginTest (partTypeRoundTrip);
        {
            for (auto part : { ArrangerPartType::Drum, ArrangerPartType::Perc,
                               ArrangerPartType::Bass, ArrangerPartType::Acc })
                expect (ArrangerEnums::partFromString (ArrangerEnums::toString (part)) == part);
        }

        beginTest (unknownFallsBack);
        {
            expect (ArrangerEnums::sectionTypeFromString (unknownText) == ArrangerSectionType::Variation);
            expect (ArrangerEnums::afterFromString (unknownText)       == ArrangerAfterComplete::Loop);
            expect (ArrangerEnums::partFromString (unknownText)        == ArrangerPartType::Acc);
        }
    }
};
static ArrangerEnumsTest arrangerEnumsTest;
