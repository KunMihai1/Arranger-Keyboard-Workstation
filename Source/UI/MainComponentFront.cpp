#include "MainComponent.h"

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
    addChildComponent (*homePage);
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
