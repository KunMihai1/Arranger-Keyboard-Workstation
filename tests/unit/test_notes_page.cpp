#include <JuceHeader.h>
#include "CustomTableContainer.h"
#include "MidiNotesTableModel.h"

/*  The notes page (board PageNotes): the buttons of "Whole track" and "Back to recorded" map to today's
    actions; the velocity bar is velocity x 0.9 board px (board barWidth); Cancel in the confirm dialog
    is a dismissal (backlog section 2: with no rows selected, Cancel got the id of "change selected" and
    the change ran). */

namespace NotesPageTestValues
{
    const juce::String testName     = "NotesPage";
    const juce::String testCategory = "Unit";

    const juce::String cancelIsDismissed  = "Cancel in the confirm dialog returns 'dismissed', whatever its position";
    const juce::String buttonsMapToActions = "each notes-page button runs today's action";
    const juce::String velocityBar        = "the velocity bar is velocity x 0.9 board px, scaled";

    const juce::String notesCaption      = "Notes";
    const juce::String timeStampsCaption = "Time stamps";
    const juce::String velocitiesCaption = "Velocities";
    const juce::String everythingCaption = "Everything";
    const char* const  shiftUtf8         = "Shift time stamps\xe2\x80\xa6";
    const char* const  setVelocitiesUtf8 = "Set velocities\xe2\x80\xa6";

    constexpr int firstButton  = 0;
    constexpr int secondButton = 1;
    constexpr int thirdButton  = 2;
    constexpr int fourthButton = 3;
    constexpr int resetCount   = 4;
    constexpr int modifyCount  = 2;
    constexpr int fullVelocity = 127;
    constexpr int boardVelocity = 70;
    constexpr int noVelocity    = 0;
    constexpr float boardBar    = 63.0f;
    constexpr float fullBar     = 114.3f;
    constexpr float noBar       = 0.0f;
    constexpr float referenceScale = 1.0f;
    constexpr float ownerScale     = 1.5364f;
    constexpr float tolerance      = 0.01f;
}

class NotesPageTest : public juce::UnitTest
{
public:
    NotesPageTest() : juce::UnitTest (NotesPageTestValues::testName, NotesPageTestValues::testCategory) {}

    void runTest() override
    {
        using namespace NotesPageTestValues;

        beginTest (cancelIsDismissed);
        {
            expectEquals (TableDialog::resultIdFor (TableDialog::cancel, secondButton), TableDialog::dismissed);
            expectEquals (TableDialog::resultIdFor (TableDialog::cancel, thirdButton),  TableDialog::dismissed);
            expectEquals (TableDialog::resultIdFor (TableDialog::changeAll, firstButton),       TableDialog::applyToAll);
            expectEquals (TableDialog::resultIdFor (TableDialog::changeSelected, secondButton), TableDialog::applyToSelected);
        }

        beginTest (buttonsMapToActions);
        {
            const auto reset = TableContainer::resetButtons();
            expectEquals (static_cast<int> (reset.size()), resetCount);
            if (static_cast<int> (reset.size()) == resetCount)
            {
                expect (reset[firstButton] == std::make_pair (notesCaption, ResetActionIds::notes));
                expect (reset[secondButton] == std::make_pair (timeStampsCaption, ResetActionIds::timeStamps));
                expect (reset[thirdButton] == std::make_pair (velocitiesCaption, ResetActionIds::velocities));
                expect (reset[fourthButton] == std::make_pair (everythingCaption, ResetActionIds::everything));
            }
            const auto modify = TableContainer::modifyButtons();
            expectEquals (static_cast<int> (modify.size()), modifyCount);
            if (static_cast<int> (modify.size()) == modifyCount)
            {
                expect (modify[firstButton] == std::make_pair (juce::String::fromUTF8 (shiftUtf8), ModifyActionIds::timeStamps));
                expect (modify[secondButton] == std::make_pair (juce::String::fromUTF8 (setVelocitiesUtf8), ModifyActionIds::velocities));
            }
        }

        beginTest (velocityBar);
        {
            expectWithinAbsoluteError (MidiNotesTableModel::velocityBarWidth (boardVelocity, referenceScale), boardBar, tolerance);
            expectWithinAbsoluteError (MidiNotesTableModel::velocityBarWidth (fullVelocity, referenceScale), fullBar, tolerance);
            expectWithinAbsoluteError (MidiNotesTableModel::velocityBarWidth (noVelocity, referenceScale), noBar, tolerance);
            expectWithinAbsoluteError (MidiNotesTableModel::velocityBarWidth (fullVelocity, ownerScale), fullBar * ownerScale, tolerance);
        }
    }
};
static NotesPageTest notesPageTest;
