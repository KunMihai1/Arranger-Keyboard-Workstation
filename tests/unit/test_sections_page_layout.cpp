#include <JuceHeader.h>
#include "UI/Sections/SectionsPageLayout.h"

/*  The Sections page (board PageSections): the configurations list and the editor were two full-window overlays;
    they become one page with three columns. */

namespace SectionsPageLayoutTestValues
{
    const juce::String testName     = "SectionsPageLayout";
    const juce::String testCategory = "Unit";

    const juce::String threeColumns       = "configurations 170 wide, inspector 206 wide, the lanes between, 14 apart";
    const juce::String leftColumnStacks   = "heading, list (five rows), buttons, then Add section and its 2-column grid, in order";
    const juce::String padsUnderLanes     = "the jump pads sit under the lanes, across the centre";
    const juce::String scalesUniformly    = "at twice the scale every part is twice the board size";

    constexpr int   contentWidth  = 1152;   // 1200 board px minus the page padding
    constexpr int   contentHeight = 297;
    constexpr int   lanesHeight   = 206;    // ruler + 8 lanes + scroll bar
    constexpr float boardScale    = 1.0f;
    constexpr float doubleScale   = 2.0f;
    constexpr int   doubled       = 2;
    constexpr int   onePixel      = 1;
    constexpr int   leftWidth     = 170;
    constexpr int   rightWidth    = 206;
    constexpr int   gap           = 14;
    constexpr int   listHeight    = 111;    // 5 rows of 21 + 3 + 3
    constexpr int   firstAdd      = 0;
    constexpr int   secondAdd     = 1;
    constexpr int   thirdAdd      = 2;
    constexpr int   lastAdd       = 4;
}

class SectionsPageLayoutTest : public juce::UnitTest
{
public:
    SectionsPageLayoutTest() : juce::UnitTest (SectionsPageLayoutTestValues::testName, SectionsPageLayoutTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionsPageLayoutTestValues;
        const juce::Rectangle<int> content (contentWidth, contentHeight);
        const auto rects = sectionsPageLayout (content, boardScale, lanesHeight);

        beginTest (threeColumns);
        expectEquals (rects.leftColumn.getX(), content.getX());
        expectEquals (rects.leftColumn.getWidth(), leftWidth);
        expectEquals (rects.rightColumn.getRight(), content.getRight());
        expectEquals (rects.rightColumn.getWidth(), rightWidth);
        expectEquals (rects.centre.getX() - rects.leftColumn.getRight(), gap);
        expectEquals (rects.rightColumn.getX() - rects.centre.getRight(), gap);
        expectEquals (rects.centre.getHeight(), lanesHeight);

        beginTest (leftColumnStacks);
        expectEquals (rects.configList.getHeight(), listHeight);
        expect (rects.configHeading.getBottom() <= rects.configList.getY());
        expect (rects.configList.getBottom() <= rects.configButtons.getY());
        expect (rects.configButtons.getBottom() < rects.addHeading.getY());
        expect (rects.addHeading.getBottom() <= rects.addButtons[firstAdd].getY());
        expectEquals (rects.addButtons[firstAdd].getY(), rects.addButtons[secondAdd].getY());          // two per row
        expect (rects.addButtons[thirdAdd].getY() > rects.addButtons[firstAdd].getBottom());
        expectEquals (rects.addButtons[firstAdd].getX(), rects.leftColumn.getX());
        expectEquals (rects.addButtons[secondAdd].getRight(), rects.leftColumn.getRight());
        for (const auto& button : rects.addButtons)
            expect (rects.leftColumn.contains (button));
        expect (rects.addButtons[lastAdd].getBottom() <= content.getBottom());

        beginTest (padsUnderLanes);
        expect (rects.pads.getY() >= rects.centre.getBottom());
        expectEquals (rects.pads.getX(), rects.centre.getX());
        expectEquals (rects.pads.getWidth(), rects.centre.getWidth());

        beginTest (scalesUniformly);
        {
            const auto atTwo = sectionsPageLayout ({ contentWidth * doubled, contentHeight * doubled }, doubleScale, lanesHeight * doubled);
            expectWithinAbsoluteError (atTwo.leftColumn.getWidth(), rects.leftColumn.getWidth() * doubled, onePixel);
            expectWithinAbsoluteError (atTwo.configList.getHeight(), rects.configList.getHeight() * doubled, onePixel);
            expectWithinAbsoluteError (atTwo.addButtons[lastAdd].getY(), rects.addButtons[lastAdd].getY() * doubled, onePixel);
            expectWithinAbsoluteError (atTwo.rightColumn.getX(), rects.rightColumn.getX() * doubled, onePixel);
        }
    }
};
static SectionsPageLayoutTest sectionsPageLayoutTest;
