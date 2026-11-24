#include <JuceHeader.h>
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageMetrics.h"
#include "Common/AppColours.h"

/*  Board values measured on the StageRevised render at scale 1 (openPage=styles), see the Style page
    test plan: "Sections…" 78.13 x 22, "All tracks" (with LED) 92.2 x 22, "Styles" heading rule
    75.84 .. 258 at y 185.5 for a heading row at (28, 180, 230, 12). */

namespace StagePageWidgetsTestValues
{
    const juce::String testName     = "StagePageWidgets";
    const juce::String testCategory = "Unit";

    const juce::String segSizes       = "a seg button is as big as the board's .seg: 22 high, text + padding wide";
    const juce::String ledSegSize     = "a seg button with an LED adds the LED and its gap";
    const juce::String litLed         = "a seg button's lit LED is drawn red";
    const juce::String faderCap       = "the fader cap runs from 116 (value 0) to 0 (value 127) like the board's capTop";
    const juce::String faderRange     = "the fader is a vertical 0-127 slider that does not jump to the click";
    const juce::String faderFollows   = "the fader cap follows the hand: dragged 58 px up at scale 1 it moves 58 px (half its travel)";
    constexpr float dragDistance = 58.0f;   // half of the 116 px cap travel
    constexpr double halfTravelValue = 63.5;
    constexpr double stepTolerance   = 1.0;
    constexpr int   faderBoxWidth  = 40;
    constexpr int   faderBoxHeight = 132;
    constexpr float grabX = 20.0f;
    constexpr float grabY = 124.0f;
    constexpr float noPressure = 0.0f;
    constexpr float noAngle    = 0.0f;
    constexpr int   singleClick = 1;
    const juce::String lcdRowInverts  = "a selected LCD row is dark with light text; an unselected one the reverse";
    const juce::String headingRule    = "a section heading's rule starts after the title and fills to the right edge";
    const juce::String headingDimText = "a section heading's dim text sits at the right edge, the rule ends before it";

    const char* const  sectionsUtf8 = "Sections\xe2\x80\xa6";   // "Sections" + U+2026 ellipsis
    const char* const  tracksUtf8   = "Tracks \xc2\xb7 Style 1";   // U+00B7 middle dot
    const juce::String sectionsText = juce::String::fromUTF8 (sectionsUtf8);
    const juce::String allTracks    = "All tracks";
    const juce::String stylesTitle  = "Styles";
    const juce::String tracksTitle  = juce::String::fromUTF8 (tracksUtf8);
    const juce::String dimText      = "Click an instrument to change it";
    const juce::String rowText      = "Style 1";
    const juce::String noText;

    constexpr float boardSectionsWidth  = 78.13f;
    constexpr float boardAllTracksWidth = 92.2f;
    constexpr int   boardSegHeight      = 22;
    constexpr float widthTolerance      = 1.0f;
    constexpr float exact               = 0.01f;

    constexpr double valueTop    = 127.0;
    constexpr double valueMiddle = 64.0;
    constexpr double valueBottom = 0.0;
    constexpr float  capAtTop    = 0.0f;
    constexpr float  capAtMiddle = 57.54f;   // 116 - 116 x 64 / 127
    constexpr float  capAtBottom = 116.0f;

    constexpr float referenceScale = 1.0f;
    constexpr int headingX = 28;
    constexpr int headingY = 180;
    constexpr int headingWidth = 230;
    constexpr int headingHeight = 12;
    constexpr float boardRuleLeft  = 75.84f;
    constexpr float boardRuleRight = 258.0f;
    constexpr float boardRuleY     = 186.0f;   // the 1 px rule at 185.5: its centre
    constexpr int tracksX = 282;
    constexpr int tracksWidth = 890;
    constexpr float boardTracksRuleLeft  = 379.84f;
    constexpr float boardTracksRuleRight = 983.75f;   // 10 px before the dim text at 993.75
    constexpr float positionTolerance    = 1.0f;

    constexpr int rowWidth = 230;
    constexpr int rowHeight = 21;
    constexpr int sampleX  = 2;   // inside the row, clear of the text
    constexpr int sampleY = 10;
    constexpr int ledButtonWidth = 100;
    constexpr int redDominance   = 2;   // red at least twice green and blue
}

class StagePageWidgetsTest : public juce::UnitTest
{
public:
    StagePageWidgetsTest() : juce::UnitTest (StagePageWidgetsTestValues::testName, StagePageWidgetsTestValues::testCategory) {}

    static bool isReddish (juce::Colour colour)
    {
        return colour.getRed() > colour.getGreen() * StagePageWidgetsTestValues::redDominance
            && colour.getRed() > colour.getBlue() * StagePageWidgetsTestValues::redDominance;
    }

    void runTest() override
    {
        using namespace StagePageWidgetsTestValues;

        beginTest (segSizes);
        {
            SegButton button { sectionsText, SegButton::Style::seg };
            button.setScale (referenceScale);
            expectEquals (button.getPreferredHeight(), boardSegHeight);
            expectWithinAbsoluteError (static_cast<float> (button.getPreferredWidth()), boardSectionsWidth, widthTolerance);
        }

        beginTest (ledSegSize);
        {
            SegButton button { allTracks, SegButton::Style::seg, true };
            button.setScale (referenceScale);
            expectWithinAbsoluteError (static_cast<float> (button.getPreferredWidth()), boardAllTracksWidth, widthTolerance);
        }

        beginTest (litLed);
        {
            SegButton button { allTracks, SegButton::Style::seg, true };
            button.setScale (referenceScale);
            button.setSize (ledButtonWidth, boardSegHeight);
            button.setLedOn (true);
            juce::Image image (juce::Image::ARGB, button.getWidth(), button.getHeight(), true);
            {
                juce::Graphics g (image);
                button.paintEntireComponent (g, true);
            }
            const auto led = button.getLedBounds().getCentre().roundToInt();
            expect (isReddish (image.getPixelAt (led.x, led.y)));
        }

        beginTest (faderCap);
        {
            expectWithinAbsoluteError (StageFader::capTopFor (valueTop),    capAtTop,    exact);
            expectWithinAbsoluteError (StageFader::capTopFor (valueMiddle), capAtMiddle, exact);
            expectWithinAbsoluteError (StageFader::capTopFor (valueBottom), capAtBottom, exact);
        }

        beginTest (faderRange);
        {
            StageFader fader;
            expect (fader.getSliderStyle() == juce::Slider::LinearVertical);
            expectEquals (fader.getMinimum(), valueBottom);
            expectEquals (fader.getMaximum(), valueTop);
            expect (! fader.getSliderSnapsToMousePosition());
            expect (fader.getTextBoxPosition() == juce::Slider::NoTextBox);
        }

        beginTest (faderFollows);
        {
            StageFader fader;
            fader.setScale (referenceScale);
            fader.setSize (faderBoxWidth, faderBoxHeight);
            fader.setValue (valueBottom);
            const juce::Point<float> grab { grabX, grabY };
            const juce::Point<float> moved { grabX, grabY - dragDistance };
            const auto now = juce::Time::getCurrentTime();
            const juce::MouseEvent down { juce::Desktop::getInstance().getMainMouseSource(), grab, juce::ModifierKeys::leftButtonModifier,
                                          noPressure, noAngle, noAngle, noAngle, noAngle, &fader, &fader, now, grab, now, singleClick, false };
            const juce::MouseEvent drag { juce::Desktop::getInstance().getMainMouseSource(), moved, juce::ModifierKeys::leftButtonModifier,
                                          noPressure, noAngle, noAngle, noAngle, noAngle, &fader, &fader, now, grab, now, singleClick, true };
            fader.mouseDown (down);
            fader.mouseDrag (drag);
            fader.mouseUp (drag);
            expectWithinAbsoluteError (fader.getValue(), halfTravelValue, stepTolerance);
        }

        beginTest (lcdRowInverts);
        {
            for (const bool selected : { true, false })
            {
                juce::Image image (juce::Image::ARGB, rowWidth, rowHeight, true);
                {
                    juce::Graphics g (image);
                    StageLcd::paintRow (g, { rowWidth, rowHeight }, referenceScale, rowText, noText, selected);
                }
                const auto expected = selected ? StagePalette::lcdInk : StagePalette::lcd;
                expect (image.getPixelAt (sampleX, sampleY) == expected);
                expect (StageLcd::textColour (selected) == (selected ? StagePalette::lcd : StagePalette::lcdInk));
            }
        }

        beginTest (headingRule);
        {
            const auto rule = StageLcd::headingRule ({ headingX, headingY, headingWidth, headingHeight }, referenceScale, stylesTitle, noText);
            expectWithinAbsoluteError (rule.getX(),       boardRuleLeft,  positionTolerance);
            expectWithinAbsoluteError (rule.getRight(),   boardRuleRight, positionTolerance);
            expectWithinAbsoluteError (rule.getCentreY(), boardRuleY,     positionTolerance);
        }

        beginTest (headingDimText);
        {
            const auto rule = StageLcd::headingRule ({ tracksX, headingY, tracksWidth, headingHeight }, referenceScale, tracksTitle, dimText);
            expectWithinAbsoluteError (rule.getX(),     boardTracksRuleLeft,  positionTolerance);
            expectWithinAbsoluteError (rule.getRight(), boardTracksRuleRight, positionTolerance);
        }
    }
};
static StagePageWidgetsTest stagePageWidgetsTest;
