#include <JuceHeader.h>
#include "UI/Voices/VoicesPageLayout.h"
#include "UI/Voices/VoicesInstrumentPanel.h"
#include "UI/PlayBackSettingsCustomComponent.h"
#include "Audio/SFZLibraryUI.h"
#include "Common/AppColours.h"

/*  The Voices page: its layout against the board (StageRevised openPage=voices, scale 1, page content at (28, 180),
    1144 wide) and at the 1920 x 991 window's scale; the instrument column's heading, tags and search; the transpose
    field's choices; what an SFZ library row says it plays. */

namespace VoicesPageTestValues
{
    const juce::String testName     = "VoicesPage";
    const juce::String testCategory = "Unit";

    const juce::String columnsAsBoard    = "the three columns sit where the board has them";
    const juce::String instrumentAsBoard = "instrument column: heading, search, group and instrument lists as the board";
    const juce::String rangeAsBoard      = "range column: labels and fields as the board";
    const juce::String sfzAsBoard        = "SFZ column: heading, list and buttons as the board";
    const juce::String scalesWithStage   = "at the window's scale every rect is the board's times the scale";
    const juce::String headingNamesHands = "the heading names the lit hands; the tags the hands playing a program";
    const juce::String openingGroupOf    = "the page opens on the edited program's group, else the right hand's, else the first";
    const juce::String searchAndGroups   = "a search lists matches from every group; clearing it or a group click shows the group";
    const juce::String searchUnmarksGroup  = "while a search shows, no group row is drawn selected";
    const juce::String clickReportsProgram = "a click on an instrument reports its program";
    const juce::String transposeChoices  = "the transpose field offers -12 ... +12 semitones, signed, ids round-tripping";
    const juce::String sfzRowAssignment  = "an SFZ row shows the instrument it plays in the style, or how many";

    const juce::String noHand     = "No hand";
    const juce::String leftHand   = "Left hand";
    const juce::String rightHand  = "Right hand";
    const juce::String bothHands  = "Both hands";
    const juce::String leftTag    = "L";
    const juce::String rightTag   = "R";
    const juce::String bothTags   = "L R";
    const juce::String bassQuery  = "bass";
    const juce::String noSemitones    = "0 semitones";
    const juce::String upThree        = "+3 semitones";
    const juce::String downTwelve     = "-12 semitones";
    const juce::String styleId        = "style-under-test";
    const juce::String otherStyleId   = "other-style";
    const juce::String entryId        = "entry-under-test";
    const juce::String otherEntryId   = "other-entry";
    const juce::String grandPianoName = "Acoustic Grand Piano";
    const juce::String twoInstruments = "2 instruments";

    constexpr int grandPiano       = 0;
    constexpr int nylonGuitar      = 24;
    constexpr int fingerBass       = 33;
    constexpr int outsideCatalogue = 127;
    constexpr int transposeUp      = 3;
    constexpr size_t pianosGroup   = 0;
    constexpr size_t bassesGroup   = 1;
    constexpr size_t guitarsGroup  = 2;
    constexpr size_t bassMatches   = 10;
    constexpr size_t pianoCount    = 2;
    constexpr size_t guitarCount   = 4;
    constexpr size_t firstShown    = 0;
    constexpr int rowEdgeInset     = 3;
    constexpr int halves           = 2;     // a row's middle     // a pixel inside the row's right end, clear of the group's name

    constexpr int contentX      = 28;
    constexpr int contentY      = 180;
    constexpr int contentWidth  = 1144;
    constexpr int contentHeight = 300;
    constexpr float boardScale  = 1.0f;
    constexpr float windowScale = 1.5364f;
    constexpr int roundingSlack = 1;
    const juce::String versus   = " vs ";

    const juce::Rectangle<int> content { contentX, contentY, contentWidth, contentHeight };

    const juce::Rectangle<int> instrumentColumn { 28, 180, 400, contentHeight };
    const juce::Rectangle<int> rangeColumn      { 456, 180, 250, contentHeight };
    const juce::Rectangle<int> sfzColumn        { 734, 180, 438, contentHeight };

    const juce::Rectangle<int> instrumentHeading { 28, 180, 400, 12 };
    const juce::Rectangle<int> search            { 28, 200, 400, 24 };
    const juce::Rectangle<int> groupList         { 28, 232, 140, 176 };   // eight rows (board: seven samples, 155)
    const juce::Rectangle<int> instrumentList    { 176, 232, 252, 176 };

    const juce::Rectangle<int> rangeHeading   { 456, 180, 250, 12 };
    const juce::Rectangle<int> rangeLabel     { 456, 200, 250, 12 };
    const juce::Rectangle<int> startField     { 456, 220, 122, 24 };
    const juce::Rectangle<int> endField       { 584, 220, 122, 24 };
    const juce::Rectangle<int> leftLabel      { 456, 252, 250, 12 };
    const juce::Rectangle<int> leftField      { 456, 272, 250, 24 };
    const juce::Rectangle<int> rightLabel     { 456, 304, 250, 12 };
    const juce::Rectangle<int> rightField     { 456, 324, 250, 24 };
    const juce::Rectangle<int> transposeLabel { 456, 356, 250, 12 };
    const juce::Rectangle<int> transposeField { 456, 376, 250, 24 };

    const juce::Rectangle<int> sfzHeading   { 734, 180, 438, 12 };
    const juce::Rectangle<int> sfzList      { 734, 200, 438, 113 };
    const juce::Rectangle<int> sfzButtonRow { 734, 321, 438, 22 };
}

class VoicesPageTest : public juce::UnitTest
{
public:
    VoicesPageTest() : juce::UnitTest (VoicesPageTestValues::testName, VoicesPageTestValues::testCategory) {}

    void runTest() override
    {
        using namespace VoicesPageTestValues;
        const auto columns = voicesPageColumns (content, boardScale);

        beginTest (columnsAsBoard);
        {
            expect (columns.instrument == instrumentColumn, columns.instrument.toString());
            expect (columns.range == rangeColumn, columns.range.toString());
            expect (columns.sfz == sfzColumn, columns.sfz.toString());
        }

        beginTest (instrumentAsBoard);
        {
            const auto rects = instrumentColumnLayout (columns.instrument, boardScale);
            expect (rects.heading == instrumentHeading, rects.heading.toString());
            expect (rects.search == search, rects.search.toString());
            expect (rects.groupList == groupList, rects.groupList.toString());
            expect (rects.instrumentList == instrumentList, rects.instrumentList.toString());
        }

        beginTest (rangeAsBoard);
        {
            const auto rects = rangeColumnLayout (columns.range, boardScale);
            expect (rects.heading == rangeHeading, rects.heading.toString());
            expect (rects.rangeLabel == rangeLabel, rects.rangeLabel.toString());
            expect (rects.startField == startField, rects.startField.toString());
            expect (rects.endField == endField, rects.endField.toString());
            expect (rects.leftLabel == leftLabel, rects.leftLabel.toString());
            expect (rects.leftField == leftField, rects.leftField.toString());
            expect (rects.rightLabel == rightLabel, rects.rightLabel.toString());
            expect (rects.rightField == rightField, rects.rightField.toString());
            expect (rects.transposeLabel == transposeLabel, rects.transposeLabel.toString());
            expect (rects.transposeField == transposeField, rects.transposeField.toString());
        }

        beginTest (sfzAsBoard);
        {
            const auto rects = sfzColumnLayout (columns.sfz, boardScale);
            expect (rects.heading == sfzHeading, rects.heading.toString());
            expect (rects.list == sfzList, rects.list.toString());
            expect (rects.buttonRow == sfzButtonRow, rects.buttonRow.toString());
        }

        beginTest (scalesWithStage);
        {
            auto scaledRect = [] (juce::Rectangle<int> board)
            {
                return board.toFloat().transformedBy (juce::AffineTransform::scale (windowScale)).toNearestInt();
            };
            auto near = [this] (juce::Rectangle<int> actual, juce::Rectangle<int> expected)
            {
                expect (std::abs (actual.getX() - expected.getX()) <= roundingSlack
                        && std::abs (actual.getY() - expected.getY()) <= roundingSlack
                        && std::abs (actual.getRight() - expected.getRight()) <= roundingSlack
                        && std::abs (actual.getBottom() - expected.getBottom()) <= roundingSlack,
                        actual.toString() + versus + expected.toString());
            };
            const auto scaledColumns = voicesPageColumns (scaledRect (content), windowScale);
            near (instrumentColumnLayout (scaledColumns.instrument, windowScale).instrumentList, scaledRect (instrumentList));
            near (rangeColumnLayout (scaledColumns.range, windowScale).endField, scaledRect (endField));
            near (rangeColumnLayout (scaledColumns.range, windowScale).transposeField, scaledRect (transposeField));
            near (sfzColumnLayout (scaledColumns.sfz, windowScale).buttonRow, scaledRect (sfzButtonRow));
        }

        beginTest (headingNamesHands);
        {
            expectEquals (VoicesModel::editedHands ({}), noHand);
            expectEquals (VoicesModel::editedHands ({ true, false }), leftHand);
            expectEquals (VoicesModel::editedHands ({ false, true }), rightHand);
            expectEquals (VoicesModel::editedHands ({ true, true }), bothHands);
            const HandVoices hands { false, true, grandPiano, nylonGuitar };
            expectEquals (VoicesModel::handTags (grandPiano, hands), leftTag);
            expectEquals (VoicesModel::handTags (nylonGuitar, hands), rightTag);
            expect (VoicesModel::handTags (fingerBass, hands).isEmpty());
            expectEquals (VoicesModel::handTags (grandPiano, { false, false, grandPiano, grandPiano }), bothTags);
            expect (VoicesModel::editedProgram (hands) == std::optional<int> (nylonGuitar));
            expect (VoicesModel::editedProgram ({ true, false, grandPiano, nylonGuitar }) == std::optional<int> (grandPiano));
            expect (! VoicesModel::editedProgram ({ false, false, grandPiano, nylonGuitar }).has_value());
        }

        beginTest (openingGroupOf);
        {
            expectEquals (VoicesModel::openingGroup ({ false, true, grandPiano, nylonGuitar }), guitarsGroup);
            expectEquals (VoicesModel::openingGroup ({ true, false, fingerBass, nylonGuitar }), bassesGroup);
            expectEquals (VoicesModel::openingGroup ({ false, false, fingerBass, nylonGuitar }), guitarsGroup);
            expectEquals (VoicesModel::openingGroup ({ false, true, grandPiano, outsideCatalogue }), pianosGroup);
        }

        beginTest (searchAndGroups);
        {
            VoicesInstrumentPanel panel;
            panel.setHands ({ false, true, grandPiano, nylonGuitar });
            panel.showOpeningGroup();
            expect (panel.getSelectedGroup() == std::optional<size_t> (guitarsGroup));
            expectEquals (panel.getShownInstruments().size(), guitarCount);
            panel.setSearchText (bassQuery);
            expectEquals (panel.getShownInstruments().size(), bassMatches);
            expect (! panel.getSelectedGroup().has_value());
            panel.setSearchText ({});
            expectEquals (panel.getShownInstruments().size(), guitarCount);
            panel.setSearchText (bassQuery);
            panel.chooseGroup (pianosGroup);
            expect (panel.getSelectedGroup() == std::optional<size_t> (pianosGroup));
            expectEquals (panel.getShownInstruments().size(), pianoCount);
            expect (panel.getHeading().endsWith (rightHand));
        }

        beginTest (searchUnmarksGroup);
        {
            VoicesInstrumentPanel panel;
            panel.setBounds (instrumentColumn.withPosition (int {}, int {}));
            panel.setScale (boardScale);
            panel.setHands ({ false, true, grandPiano, nylonGuitar });
            panel.showOpeningGroup();
            const auto groupList = instrumentColumnLayout (panel.getLocalBounds(), boardScale).groupList;
            const auto guitarsRowY = groupList.getY() + StageMetrics::lcdListPadding
                                   + static_cast<int> (guitarsGroup) * StageMetrics::lcdRowHeight + StageMetrics::lcdRowHeight / halves;
            const auto rowEndX = groupList.getRight() - rowEdgeInset;
            auto groupRowFill = [&] { return panel.createComponentSnapshot (panel.getLocalBounds()).getPixelAt (rowEndX, guitarsRowY); };

            expect (groupRowFill() == StagePalette::lcdInk);   // the opening group is drawn selected
            panel.setSearchText (bassQuery);
            expect (groupRowFill() == StagePalette::lcd);      // matches from every group: no group drawn selected
            panel.setSearchText ({});
            expect (groupRowFill() == StagePalette::lcdInk);
        }

        beginTest (clickReportsProgram);
        {
            VoicesInstrumentPanel panel;
            panel.chooseGroup (guitarsGroup);
            int reported = outsideCatalogue;
            panel.onInstrumentChosen = [&reported] (int program) { reported = program; };
            panel.chooseShownInstrument (firstShown);
            expectEquals (reported, nylonGuitar);
        }

        beginTest (transposeChoices);
        {
            expectEquals (TransposeField::itemText (int {}), noSemitones);
            expectEquals (TransposeField::itemText (transposeUp), upThree);
            expectEquals (TransposeField::itemText (-TransposeField::maxSemitones), downTwelve);
            for (int semitones = -TransposeField::maxSemitones; semitones <= TransposeField::maxSemitones; ++semitones)
            {
                expect (TransposeField::itemId (semitones) > int {});   // a ComboBox id is never 0
                expectEquals (TransposeField::semitonesFor (TransposeField::itemId (semitones)), semitones);
            }
        }

        beginTest (sfzRowAssignment);
        {
            SFZLibraryData data;
            expect (sfzAssignmentText (data, styleId, entryId).isEmpty());
            data.styleMappings[styleId][grandPiano] = entryId;
            data.styleMappings[styleId][fingerBass] = otherEntryId;
            expectEquals (sfzAssignmentText (data, styleId, entryId), grandPianoName);
            expect (sfzAssignmentText (data, otherStyleId, entryId).isEmpty());
            data.styleMappings[styleId][nylonGuitar] = entryId;
            expectEquals (sfzAssignmentText (data, styleId, entryId), twoInstruments);
        }
    }
};
static VoicesPageTest voicesPageTest;
