#include <JuceHeader.h>
#include "CustomToolTip.h"
#include "Common/AppColours.h"

namespace StageTooltipTestValues
{
    const juce::String testName     = "StageTooltip";
    const juce::String testCategory = "Unit";

    const juce::String fitsText     = "a tooltip is as wide as its whole text plus padding (a long track name is not cut)";
    const juce::String stageLook    = "a tooltip is the page colour with a rule-line border and silkscreen text";
    const juce::String lookAndFeel  = "the app's tooltip look sizes JUCE tooltips the same way";
    const juce::String followsScale = "a tooltip grows with the stage scale (the same look on every resolution)";

    const juce::String longName = "Electric Piano 2 (Lead Layer)";
    constexpr int   borderColumn = 0;
    constexpr int   insideColumn = 2;
    constexpr int   halves       = 2;
    constexpr int   colourSlack  = 12;
    constexpr float stageScale   = 2.0f;
}

class StageTooltipTest : public juce::UnitTest
{
public:
    StageTooltipTest() : juce::UnitTest (StageTooltipTestValues::testName, StageTooltipTestValues::testCategory) {}

    static bool near (juce::Colour drawn, juce::Colour expected)
    {
        using namespace StageTooltipTestValues;
        return std::abs (drawn.getRed() - expected.getRed()) <= colourSlack
            && std::abs (drawn.getGreen() - expected.getGreen()) <= colourSlack
            && std::abs (drawn.getBlue() - expected.getBlue()) <= colourSlack;
    }

    void runTest() override
    {
        using namespace StageTooltipTestValues;

        beginTest (fitsText);
        CustomToolTip tip (longName);
        const auto textWidth = juce::GlyphArrangement::getStringWidth (StageTooltip::font(), longName);
        expect (static_cast<float> (tip.getWidth()) >= textWidth + static_cast<float> (StageTooltip::padding * halves));

        beginTest (stageLook);
        juce::Image image (juce::Image::ARGB, tip.getWidth(), tip.getHeight(), true, juce::SoftwareImageType());
        {
            juce::Graphics g (image);
            tip.paintEntireComponent (g, false);
        }
        const auto middleRow = tip.getHeight() / halves;
        expect (near (image.getPixelAt (borderColumn, middleRow), StagePalette::ruleLine));
        expect (near (image.getPixelAt (insideColumn, middleRow), StagePalette::pageTop)
             || near (image.getPixelAt (insideColumn, middleRow), StagePalette::pageBottom));

        beginTest (lookAndFeel);
        StageTooltipLookAndFeel look;
        const auto bounds = look.getTooltipBounds (longName, {}, { int {}, int {}, tip.getWidth() * halves * halves, tip.getHeight() * halves * halves });
        expectEquals (bounds.getWidth(), tip.getWidth());
        expectEquals (bounds.getHeight(), tip.getHeight());

        beginTest (followsScale);
        CustomToolTip scaledTip (longName, stageScale);
        const auto scaledText = juce::GlyphArrangement::getStringWidth (StageTooltip::font (stageScale), longName);
        expect (static_cast<float> (scaledTip.getWidth()) >= scaledText + static_cast<float> (StageTooltip::padding * halves) * stageScale);
        expect (scaledTip.getHeight() > tip.getHeight());
    }
};

static StageTooltipTest stageTooltipTest;
