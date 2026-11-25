#include <JuceHeader.h>
#include "StylesListComponent.h"
#include "Track.h"

namespace StyleListDialogsTestValues
{
    const juce::String testName     = "StylesListComponent (in-app dialogs)";
    const juce::String testCategory = "Unit";

    const juce::String addOpensDialog  = "Add opens an in-app name dialog; OK adds the style and tells the screen";
    const juce::String duplicateRefused = "a name already in the list is refused with a screen message";
    const juce::String renameHoldsName = "Rename opens the dialog holding the open style's name";
    const juce::String stripDialogs    = "a strip's Rename holds its name; Delete asks, then empties the slot";
    const juce::String bassTrack       = "Bass";
    const juce::String emptySlot       = "None";

    const juce::String styleOne  = "Style 1";
    const juce::String styleTwo  = "Style 2";
    const juce::String jazz      = "Jazz";
    constexpr int listWidth  = 300;
    constexpr int listHeight = 400;
}

class StyleListDialogsTest : public juce::UnitTest
{
public:
    StyleListDialogsTest() : juce::UnitTest (StyleListDialogsTestValues::testName, StyleListDialogsTestValues::testCategory) {}

    static void typeAndConfirm (StageDialog& dialog, const juce::String& text)
    {
        for (auto* child : dialog.getChildren())
            if (auto* input = dynamic_cast<StageLcdInput*> (child))
                input->setText (text, false);
        dialog.confirm();
    }

    void runTest() override
    {
        using namespace StyleListDialogsTestValues;

        beginTest (addOpensDialog);
        {
            juce::Component page;
            StylesListComponent list ({ styleOne, styleTwo }, [] (const juce::String&) {});
            list.setSize (listWidth, listHeight);
            page.addAndMakeVisible (list);
            juce::String added, info;
            list.onStyleAdd = [&added] (const juce::String& name) { added = name; };
            list.onInfo = [&info] (const juce::String& text) { info = text; };

            list.getAddButton().onClick();
            auto* dialog = list.getOpenDialog();
            expect (dialog != nullptr && dialog->isVisible() && dialog->hasInput());
            if (dialog != nullptr)
                typeAndConfirm (*dialog, jazz);
            expectEquals (added, jazz);
            expect (info.contains (jazz));
            expect (list.getOpenDialog() == nullptr);

            beginTest (duplicateRefused);
            added = {};
            info = {};
            list.getAddButton().onClick();
            if (auto* again = list.getOpenDialog())
                typeAndConfirm (*again, styleTwo);
            expect (added.isEmpty());
            expect (info.isNotEmpty());
        }

        beginTest (renameHoldsName);
        {
            juce::Component page;
            StylesListComponent list ({ styleOne, styleTwo }, [] (const juce::String&) {});
            list.setSize (listWidth, listHeight);
            page.addAndMakeVisible (list);
            list.setOpenStyle (styleTwo);
            list.getRenameButton().onClick();
            auto* dialog = list.getOpenDialog();
            expect (dialog != nullptr && dialog->hasInput());
            if (dialog != nullptr)
                expectEquals (dialog->getText(), styleTwo);
        }

        beginTest (stripDialogs);
        {
            juce::Component page;
            page.setSize (listWidth, listHeight);
            Track strip;
            page.addAndMakeVisible (strip);
            strip.setNameLabel (bassTrack);
            strip.renameOneTrack();
            auto* rename = strip.getOpenDialog();
            expect (rename != nullptr && rename->hasInput());
            if (rename != nullptr)
            {
                expectEquals (rename->getText(), bassTrack);
                rename->cancel();
            }

            strip.deleteOneTrack();
            auto* remove = strip.getOpenDialog();
            expect (remove != nullptr && ! remove->hasInput());
            expectEquals (strip.getName(), bassTrack);   // nothing happens before the answer
            if (remove != nullptr)
                remove->confirm();
            expectEquals (strip.getName(), emptySlot);
        }
    }
};

static StyleListDialogsTest styleListDialogsTest;
