#include <JuceHeader.h>
#include "UI/Stage/StageFonts.h"

namespace StageFontsTestValues
{
    const juce::String testName     = "StageFonts";
    const juce::String testCategory = "Unit";

    const juce::String silkscreenIsBarlow = "silkscreen labels use the embedded Barlow Semi Condensed";
    const juce::String lcdIsShareTechMono = "screen text uses the embedded Share Tech Mono";
    const juce::String heightIsKept       = "the requested height is kept";

    // DirectWrite reports Barlow as family "Barlow" + style "Semi Condensed SemiBold",
    // so the checks read the family and the style together.
    const juce::String barlowFamily    = "Barlow";
    const juce::String semiCondensed   = "Semi Condensed";
    const juce::String semiBoldWeight  = "SemiBold";
    const juce::String mediumWeight    = "Medium";
    const juce::String shareTechFamily = "Share Tech Mono";
    const juce::String nameSeparator   = " ";

    constexpr float labelHeight  = 10.0f;
    constexpr float screenHeight = 28.0f;
    constexpr float tolerance    = 0.01f;
    constexpr bool  semiBold     = true;
    constexpr bool  medium       = false;
}

class StageFontsTest : public juce::UnitTest
{
    static juce::String familyAndStyle (const juce::Font& font)
    {
        const auto typeface = font.getTypefacePtr();
        return typeface->getName() + StageFontsTestValues::nameSeparator + typeface->getStyle();
    }

    void expectBarlow (const juce::Font& font, const juce::String& weight)
    {
        using namespace StageFontsTestValues;
        const auto fullName = familyAndStyle (font);
        expect (fullName.startsWith (barlowFamily), fullName);
        expect (fullName.contains (semiCondensed), fullName);
        expect (fullName.contains (weight), fullName);
    }

public:
    StageFontsTest() : juce::UnitTest (StageFontsTestValues::testName, StageFontsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageFontsTestValues;

        beginTest (silkscreenIsBarlow);
        {
            expectBarlow (StageFonts::silkscreen (labelHeight, semiBold), semiBoldWeight);
            expectBarlow (StageFonts::silkscreen (labelHeight, medium), mediumWeight);
        }

        beginTest (lcdIsShareTechMono);
        {
            const auto lcdName = familyAndStyle (StageFonts::lcd (screenHeight));
            expect (lcdName.startsWith (shareTechFamily), lcdName);
        }

        beginTest (heightIsKept);
        {
            expectWithinAbsoluteError (StageFonts::lcd (screenHeight).getHeight(), screenHeight, tolerance);
        }
    }
};
static StageFontsTest stageFontsTest;
