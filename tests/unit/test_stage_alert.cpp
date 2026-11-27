#include <JuceHeader.h>
#include "UI/Stage/StageDialog.h"
#include "UI/Stage/StageAlert.h"

/*  The app's notices and questions are StageDialogs (StageAlert) instead of juce::AlertWindow: a notice has one OK
    button that Esc also presses, a choice has a second action, one dialog shows at a time, and the GL note layer is
    told to step aside while one covers the scene. */

namespace StageAlertTestValues
{
    const juce::String testName     = "StageAlert";
    const juce::String testCategory = "Unit";

    const juce::String noticeOneButton = "a notice shows only OK; Enter, Esc and OK all close it and report once";
    const juce::String choiceButtons   = "a choice has Cancel, the alternative and confirm, right to left; each reports only its own action";
    const juce::String oneAtATime      = "a new alert replaces the one showing, also from inside that one's OK";
    const juce::String coverHooks      = "the scene is covered while an alert shows and uncovered when it closes";
    const juce::String closeAllClears  = "closeAll hides everything and uncovers";
    const juce::String coversMainScene = "an anchor inside the main scene (a page) still covers the whole scene; one in another window stays there";

    const juce::String title       = "Save recording";
    const juce::String message     = "There is nothing recorded to save.";
    const juce::String okText      = "OK";
    const juce::String changeAll   = "Change All";
    const juce::String changeSome  = "Change selected";
    const juce::String nextTitle   = "Saved";

    constexpr int   hostWidth  = 1200;
    constexpr int   hostHeight = 800;
    constexpr float boardScale = 1.0f;
    constexpr int   once       = 1;
    constexpr int   never      = 0;
}

using namespace StageAlertTestValues;

class StageAlertTest : public juce::UnitTest
{
public:
    StageAlertTest() : juce::UnitTest (StageAlertTestValues::testName, StageAlertTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (noticeOneButton);
        for (const auto& press : { juce::KeyPress (juce::KeyPress::returnKey), juce::KeyPress (juce::KeyPress::escapeKey) })
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int reports {};
            StageDialogOptions options { title, message, okText };
            options.hasCancel = false;
            StageDialog notice (options, [&] (const juce::String&) { ++reports; });
            notice.showOver (host, boardScale);
            expect (! notice.getCancelButton().isVisible());
            expect (notice.getConfirmButton().isVisible());
            expect (notice.keyPressed (press));
            expectEquals (reports, once);
            expect (! notice.isVisible());
        }

        beginTest (choiceButtons);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int confirms {}, alternatives {};
            StageDialogOptions options { title, message, changeAll };
            options.alternativeText = changeSome;
            StageDialog choice (options, [&] (const juce::String&) { ++confirms; });
            choice.onAlternative = [&] { ++alternatives; };
            choice.showOver (host, boardScale);

            SegButton* alternative = nullptr;
            for (auto* child : choice.getChildren())
                if (auto* button = dynamic_cast<SegButton*> (child))
                    if (button->getButtonText() == changeSome)
                        alternative = button;
            expect (alternative != nullptr && alternative->isVisible());
            if (alternative == nullptr)
                return;
            expect (choice.getCancelButton().getRight() <= alternative->getX());
            expect (alternative->getRight() <= choice.getConfirmButton().getX());

            alternative->onClick();
            expectEquals (alternatives, once);
            expectEquals (confirms, never);
            expect (! choice.isVisible());
        }

        beginTest (oneAtATime);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int secondOks {};
            StageAlert::showMessage (title, message, &host, [&host, &secondOks]
            {
                StageAlert::showMessage (nextTitle, message, &host, [&secondOks] { ++secondOks; });
            });
            auto* first = dynamic_cast<StageDialog*> (StageAlert::current());
            expect (first != nullptr);
            if (first != nullptr)
                first->getConfirmButton().onClick();   // its OK opens the next notice from inside the callback

            auto* second = dynamic_cast<StageDialog*> (StageAlert::current());
            expect (second != nullptr && second != first);
            if (second != nullptr)
                second->getConfirmButton().onClick();
            expectEquals (secondOks, once);
            expect (StageAlert::current() == nullptr);
            StageAlert::closeAll();
        }

        beginTest (coverHooks);
        {
            juce::Component host;
            host.setSize (hostWidth, hostHeight);
            int covers {}, uncovers {};
            StageAlert::setCoverHooks ([&] { ++covers; }, [&] { ++uncovers; });
            StageAlert::showMessage (title, message, &host);
            StageAlert::showMessage (nextTitle, message, &host);   // replacing does not cover twice
            expectEquals (covers, once);
            expectEquals (uncovers, never);
            if (auto* dialog = dynamic_cast<StageDialog*> (StageAlert::current()))
                dialog->getConfirmButton().onClick();
            expectEquals (uncovers, once);

            beginTest (coversMainScene);
            {
                juce::Component mainScene, page, otherWindow;
                mainScene.setSize (hostWidth, hostHeight);
                otherWindow.setSize (hostWidth, hostHeight);
                mainScene.addAndMakeVisible (page);
                StageAlert::setMainHost (&mainScene);
                StageAlert::showMessage (title, message, &page);
                expect (StageAlert::current() != nullptr && StageAlert::current()->getParentComponent() == &mainScene);
                StageAlert::showMessage (title, message, &otherWindow);
                expect (StageAlert::current() != nullptr && StageAlert::current()->getParentComponent() == &otherWindow);
                StageAlert::closeAll();
                StageAlert::setCoverHooks ([&] { ++covers; }, [&] { ++uncovers; });
                covers = once;
                uncovers = once;
            }

            beginTest (closeAllClears);
            StageAlert::showQuestion (title, message, okText, {}, &host);
            expectEquals (covers, once + once);
            StageAlert::closeAll();
            expectEquals (uncovers, once + once);
            expect (StageAlert::current() == nullptr);
        }
    }
};

static StageAlertTest stageAlertTest;
