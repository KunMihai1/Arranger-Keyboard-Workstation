#include <JuceHeader.h>
#include "UI/Stage/StageDialog.h"

namespace StageDialogTestValues
{
    const juce::String testName     = "StageDialog";
    const juce::String testCategory = "Unit";

    const juce::String inputConfirms   = "Enter confirms an input dialog with its trimmed text and hides it";
    const juce::String escapeCancels   = "Esc cancels: nothing is reported, the dialog hides";
    const juce::String confirmDialog   = "a question dialog has no input and reports on its confirm button";
    const juce::String coversHost      = "the dialog covers the page that holds the anchor";
    const juce::String layoutCentred   = "the panel is 400 board px wide, centred, with the input and buttons inside it";
    const juce::String keepsFocus      = "a click on the dialog leaves the focus in its input, and the app knows the click was in a dialog";
    const juce::String followsHost     = "the dialog keeps covering its page when the window is resized";

    const juce::String renameTitle  = "Rename folder";
    const juce::String nameLabel    = "Folder name";
    const juce::String okText       = "OK";
    const juce::String removeTitle  = "Remove folders";
    const juce::String removeText   = "Remove";
    const juce::String question     = "Remove the selected folders and their tracks?";
    const juce::String oldName      = "Pop";
    const juce::String typedName    = "  Ballads  ";
    const juce::String trimmedName  = "Ballads";
    const juce::String noText;

    constexpr int   hostWidth      = 1000;
    constexpr int   hostHeight     = 800;
    constexpr int   anchorSize     = 100;
    constexpr float ownerScale     = 1.5f;
    constexpr float boardPanelWidth = 400.0f;
    constexpr float oneLine        = 18.0f;
    constexpr float tolerance      = 1.0f;
    constexpr int   halves         = 2;
    constexpr int   oneReport      = 1;
    constexpr int   grownWidth     = 1400;
    constexpr int   grownHeight    = 900;
}

class StageDialogTest : public juce::UnitTest
{
public:
    StageDialogTest() : juce::UnitTest (StageDialogTestValues::testName, StageDialogTestValues::testCategory) {}

    static StageLcdInput* inputOf (StageDialog& dialog)
    {
        for (auto* child : dialog.getChildren())
            if (auto* input = dynamic_cast<StageLcdInput*> (child))
                return input;
        return nullptr;
    }

    void runTest() override
    {
        using namespace StageDialogTestValues;

        beginTest (inputConfirms);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            juce::Component anchor;
            anchor.setSize (anchorSize, anchorSize);
            host.addAndMakeVisible (anchor);

            juce::String reported;
            int reports {};
            StageDialog dialog ({ renameTitle, nameLabel, okText, true, oldName },
                                [&] (const juce::String& text) { reported = text; ++reports; });
            dialog.showOver (anchor, ownerScale);

            auto* input = inputOf (dialog);
            expect (input != nullptr && input->isVisible());
            expectEquals (dialog.getText(), oldName);
            if (input != nullptr)
                input->setText (typedName, false);

            expect (dialog.keyPressed (juce::KeyPress (juce::KeyPress::returnKey)));
            expectEquals (reports, oneReport);
            expectEquals (reported, trimmedName);
            expect (! dialog.isVisible());

            beginTest (coversHost);
            expect (dialog.getParentComponent() == &host);
            expect (dialog.getBounds() == host.getLocalBounds());
        }

        beginTest (escapeCancels);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int reports {};
            StageDialog dialog ({ renameTitle, nameLabel, okText, true, oldName }, [&] (const juce::String&) { ++reports; });
            dialog.showOver (host, ownerScale);
            expect (dialog.isVisible());
            expect (dialog.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey)));
            expectEquals (reports, int {});
            expect (! dialog.isVisible());
        }

        beginTest (confirmDialog);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int reports {};
            juce::String reported = oldName;
            StageDialog dialog ({ removeTitle, question, removeText }, [&] (const juce::String& text) { reported = text; ++reports; });
            dialog.showOver (host, ownerScale);
            expect (! dialog.hasInput());
            auto* input = inputOf (dialog);
            expect (input == nullptr || ! input->isVisible());
            expectEquals (dialog.getConfirmButton().getButtonText(), removeText);
            dialog.getConfirmButton().onClick();
            expectEquals (reports, oneReport);
            expectEquals (reported, noText);
            expect (! dialog.isVisible());
        }

        beginTest (keepsFocus);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            StageDialog dialog ({ renameTitle, nameLabel, okText, true, oldName }, {});
            dialog.showOver (host, ownerScale);
            expect (! dialog.getMouseClickGrabsKeyboardFocus());
            expect (StageDialog::isInsideDialog (&dialog));
            expect (StageDialog::isInsideDialog (&dialog.getConfirmButton()));
            expect (! StageDialog::isInsideDialog (&host));
            expect (! StageDialog::isInsideDialog (nullptr));

            beginTest (followsHost);
            host.setSize (grownWidth, grownHeight);
            expect (dialog.getBounds() == host.getLocalBounds());
        }

        beginTest (layoutCentred);
        {
            const juce::Rectangle<int> host { hostWidth, hostHeight };
            const auto rects = stageDialogLayout (host, ownerScale, true, oneLine);
            expectWithinAbsoluteError (static_cast<float> (rects.panel.getWidth()), boardPanelWidth * ownerScale, tolerance);
            expectWithinAbsoluteError (static_cast<float> (rects.panel.getCentreX()), static_cast<float> (hostWidth / halves), tolerance);
            expectWithinAbsoluteError (static_cast<float> (rects.panel.getCentreY()), static_cast<float> (hostHeight / halves), tolerance);
            expect (rects.panel.contains (rects.heading));
            expect (rects.panel.contains (rects.input));
            expect (rects.panel.contains (rects.buttonRow));
            expect (rects.input.getY() >= rects.message.getBottom());
            expect (rects.buttonRow.getY() >= rects.input.getBottom());
        }
    }
};

static StageDialogTest stageDialogTest;
