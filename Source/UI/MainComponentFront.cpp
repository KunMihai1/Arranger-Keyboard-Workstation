#include "MainComponent.h"
#include "../Backend/RememberedEmail.h"
#include "Stage/StageDialog.h"

// Log in and Home on the instrument (boards StageLogin / StageHome, owner decision 2026-11-23; split from
// MainComponentStage.cpp, ADR-003). The panel shows with its screen asleep ("Welcome" / "Ready", lamps dark) and its
// controls inactive; the page area holds the login form or Home's three big keys; the keys are drawn, not played
// (the playing keyboard and its note layer are made when Play opens the devices).

namespace
{
    namespace FrontText
    {
        const juce::String signInMode = "SIGN IN";
        const juce::String homeMode   = "HOME";
        const juce::String welcome    = "Welcome";
        const juce::String ready      = "Ready";
    }
}

void MainComponent::frontScreensInit()
{
    homePage = std::make_unique<StageHomePage>();
    homePage->onPlay     = [this] { if (playButton.onClick) playButton.onClick(); };   // as the old Play on the picture
    homePage->onSettings = [this] { openStagePage (StagePage::settings); refreshStage(); };
    homePage->onExit     = []
    {
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    };
    homePage->onLogOut   = [this] { logOut(); };
    addChildComponent (*homePage);
    if (loginWindow != nullptr)
        loginWindow->setRememberedEmail (RememberedEmail::load (propertiesFile));
    addChildComponent (frontKeys);
    showFrontScreen (FrontScreen::login);
}

void MainComponent::showFrontScreen (FrontScreen screen)
{
    frontScreen = screen;
    const bool front = screen != FrontScreen::none;
    if (loginWindow != nullptr)
        loginWindow->setVisible (screen == FrontScreen::login);
    if (homePage != nullptr)
        homePage->setVisible (screen == FrontScreen::home);
    frontKeys.setVisible (front);
    if (stagePanel != nullptr)
    {
        stagePanel->setVisible (true);
        stagePanel->setEnabled (! front);   // its controls work once Play has opened the devices
    }
    layoutFrontScreens();
    refreshStage();
    if (screen == FrontScreen::home && isShowing())
        grabKeyboardFocus();   // the login field that had it is hidden now: Esc must still open the menu
}

void MainComponent::layoutFrontScreens()
{
    if (stagePanel == nullptr)
        return;
    if (loginWindow != nullptr)
    {
        loginWindow->setScale (stageRegions.scale);
        loginWindow->setBounds (stageRegions.noteArea);
    }
    if (homePage != nullptr)
    {
        homePage->setScale (stageRegions.scale);
        homePage->setBounds (stageRegions.noteArea);
    }
    frontKeys.setBounds (stageRegions.keys);
}

bool MainComponent::frontScreenContent (StageScreen::Content& content) const
{
    if (frontScreen == FrontScreen::none)
        return false;
    content.asleep    = true;
    if (pageRouter.open() == StagePage::none)   // a page opened from here (Settings) names itself
        content.mode  = frontScreen == FrontScreen::login ? FrontText::signInMode : FrontText::homeMode;
    content.styleName = frontScreen == FrontScreen::login ? FrontText::welcome : FrontText::ready;
    content.variation = {};
    content.chord     = {};
    return true;
}

void MainComponent::signedIn (const juce::String& email)
{
    if (email.isNotEmpty())   // the email only, never the password (spec part 1 §10)
    {
        RememberedEmail::save (propertiesFile, email);
        if (propertiesFile != nullptr)
            propertiesFile->saveIfNeeded();
    }
    homePage->setSignedInAs (email);
    showFrontScreen (FrontScreen::home);
}

void MainComponent::logOut()
{
    // Ends the session on this computer; the email stays remembered for the next log in.
    client->setAccessToken ({});
    client->setUserId ({});
    homePage->setSignedInAs ({});
    if (loginWindow != nullptr)
        loginWindow->setRememberedEmail (RememberedEmail::load (propertiesFile));
    showFrontScreen (FrontScreen::login);
}

void MainComponent::forgetRememberedEmail()
{
    RememberedEmail::forget (propertiesFile);
    if (propertiesFile != nullptr)
        propertiesFile->saveIfNeeded();
}

void MainComponent::alertsInit()
{
    // The GL note layer draws over ordinary components: hide it while an alert covers the scene, and bring it back
    // only if the alert was what hid it (a page or the Esc menu keep their own hold).
    StageAlert::setMainHost (this);
    StageAlert::setCoverHooks ([this]
    {
        // Remember what was typing - unless the focus is still in the alert that just closed (a chain of alerts):
        // then the first alert's choice stands.
        auto* focused = juce::Component::getCurrentlyFocusedComponent();
        if (! StageDialog::isInsideDialog (focused))
            focusBeforeAlert = focused;
        noteLayerHiddenForAlert = false;
        keepNoteLayerUnderAlert();
    },
    [this]
    {
        // Back only if nothing else wants it hidden now (a page, the Esc menu or the loading overlay keep their hold).
        const bool sceneFree = pageRouter.open() == StagePage::none && (overlayWindow == nullptr || ! overlayWindow->isVisible())
                            && ! loadingOverlayActive;
        if (noteLayerHiddenForAlert && noteLayer != nullptr && sceneFree)
        {
            noteLayer->setHeldHidden (false);
            noteLayer->setVisible (true);
        }
        noteLayerHiddenForAlert = false;
        // Back to what was typing before (e.g. the login field after "Login failed"), else here: Esc keeps working.
        // After the key or click that closed the alert has finished: JUCE moves the focus off the hidden dialog
        // itself, after this hook.
        const juce::Component::SafePointer<MainComponent> safeMain (this);
        juce::MessageManager::callAsync ([safeThis = safeMain]
        {
            if (safeThis == nullptr || StageAlert::current() != nullptr)
                return;   // gone, or the next alert has the keys
            if (safeThis->focusBeforeAlert != nullptr && safeThis->focusBeforeAlert->isShowing())
                safeThis->focusBeforeAlert->grabKeyboardFocus();
            else if (safeThis->isShowing())
                safeThis->grabKeyboardFocus();
        });
    });
}

void MainComponent::keepNoteLayerUnderAlert()
{
    // Called when an alert opens, and wherever the note layer is shown again (Play, a page or menu closing).
    if (StageAlert::current() != nullptr && noteLayer != nullptr && noteLayer->isVisible())
    {
        noteLayerHiddenForAlert = true;
        noteLayer->setHeldHidden (true);
        noteLayer->setVisible (false);
    }
}
