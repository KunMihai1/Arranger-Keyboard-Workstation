#include <JuceHeader.h>
#include "Display.h"
#include "StylesListComponent.h"
#include "CurrentStyleComponent.h"
#include "PersistedKeys.h"

/*  The Style page (board StageRevised openPage=styles, scale 1): content area (28, 180, 1144 x 296);
    styles column 230 wide, gap 24, tracks area 890 wide from x 282. Inside the styles column: heading
    12 high, the LCD list at +20 (two 21 px rows + padding), Add / Rename / Delete 8 below it (41.41, 61.31,
    58.92 wide, gaps 6). */

namespace StylePageTestValues
{
    const juce::String testName     = "StylePage";
    const juce::String testCategory = "Unit";

    const juce::String layoutMatchesBoard = "the Style page splits into the styles column and the tracks area like the board";
    const juce::String listMatchesBoard   = "the styles list and its buttons sit where the board puts them";
    const juce::String nothingOpen        = "with no style open, Rename and Delete are disabled";
    const juce::String defaultProtected   = "DEFAULT Style cannot be renamed or deleted; another style can";
    const juce::String openMarked         = "the open style is the marked row";
    const juce::String soloThroughCombo   = "All tracks / Solo go through the old play-mode choice";

    const juce::String styleOne = "Style 1";

    constexpr float referenceScale = 1.0f;
    constexpr int contentX      = 28;
    constexpr int contentY      = 180;
    constexpr int contentWidth  = 1144;
    constexpr int contentHeight = 296;
    constexpr int columnWidth   = 230;
    constexpr int tracksX       = 282;
    constexpr int tracksWidth   = 890;

    // The board's list is 52 high only because Chrome drew the open row's arrow with a taller fallback
    // font (that row is 23, the other 21). The rows are 21 each: 2 x 21 + 2 x 4 padding = 50.
    constexpr int listTop       = 20;
    constexpr int listHeight    = 50;
    constexpr int buttonsTop    = 78;
    constexpr int buttonHeight  = 22;
    constexpr int addWidth      = 41;
    constexpr int renameX       = 47;
    constexpr int renameWidth   = 61;
    constexpr int deleteX       = 115;
    constexpr int deleteWidth   = 59;
    constexpr int tolerance     = 1;
    constexpr int leftEdge      = 0;

    constexpr int panelWidth  = 890;
    constexpr int panelHeight = 296;
}

class StylePageTest : public juce::UnitTest
{
public:
    StylePageTest() : juce::UnitTest (StylePageTestValues::testName, StylePageTestValues::testCategory) {}

    void expectNear (juce::Rectangle<int> actual, juce::Rectangle<int> expected)
    {
        using namespace StylePageTestValues;
        expectWithinAbsoluteError (actual.getX(), expected.getX(), tolerance);
        expectWithinAbsoluteError (actual.getY(), expected.getY(), tolerance);
        expectWithinAbsoluteError (actual.getWidth(), expected.getWidth(), tolerance);
        expectWithinAbsoluteError (actual.getHeight(), expected.getHeight(), tolerance);
    }

    static std::unique_ptr<StylesListComponent> makeList()
    {
        using namespace StylePageTestValues;
        auto list = std::make_unique<StylesListComponent> (std::vector<juce::String> { StyleNames::defaultStyle, styleOne },
                                                           [] (const juce::String&) {});
        list->setScale (referenceScale);
        list->setSize (columnWidth, contentHeight);
        return list;
    }

    void runTest() override
    {
        using namespace StylePageTestValues;

        beginTest (layoutMatchesBoard);
        {
            const auto rects = stylePageLayout ({ contentX, contentY, contentWidth, contentHeight }, referenceScale);
            expectNear (rects.stylesColumn, { contentX, contentY, columnWidth, contentHeight });
            expectNear (rects.tracksArea,   { tracksX,  contentY, tracksWidth, contentHeight });
        }

        beginTest (listMatchesBoard);
        {
            auto list = makeList();
            expectNear (list->getListBounds(), { leftEdge, listTop, columnWidth, listHeight });
            expectNear (list->getAddButton().getBounds(),    { leftEdge, buttonsTop, addWidth,    buttonHeight });
            expectNear (list->getRenameButton().getBounds(), { renameX, buttonsTop, renameWidth, buttonHeight });
            expectNear (list->getDeleteButton().getBounds(), { deleteX, buttonsTop, deleteWidth, buttonHeight });
        }

        beginTest (nothingOpen);
        {
            auto list = makeList();
            expect (list->getAddButton().isEnabled());
            expect (! list->getRenameButton().isEnabled());
            expect (! list->getDeleteButton().isEnabled());
        }

        beginTest (defaultProtected);
        {
            auto list = makeList();
            list->setOpenStyle (StyleNames::defaultStyle);
            expect (! list->getRenameButton().isEnabled());
            expect (! list->getDeleteButton().isEnabled());
            list->setOpenStyle (styleOne);
            expect (list->getRenameButton().isEnabled());
            expect (list->getDeleteButton().isEnabled());
        }

        beginTest (openMarked);
        {
            auto list = makeList();
            list->setOpenStyle (styleOne);
            expectEquals (list->getOpenStyle(), styleOne);
        }

        beginTest (soloThroughCombo);
        {
            std::unordered_map<juce::Uuid, TrackEntry*> trackEntries;
            CurrentStyleComponent panel (styleOne, trackEntries, {}, {});
            panel.setSize (panelWidth, panelHeight);
            expect (! panel.isPlayModeSolo());
            panel.setPlayModeSolo (true);
            expect (panel.isPlayModeSolo());
            panel.setPlayModeSolo (false);
            expect (! panel.isPlayModeSolo());
        }
    }
};
static StylePageTest stylePageTest;
