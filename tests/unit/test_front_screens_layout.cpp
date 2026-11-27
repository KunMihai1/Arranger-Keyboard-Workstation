#include <JuceHeader.h>
#include "UI/Stage/FrontScreensLayout.h"

/*  Log in and Home on the instrument (boards PageLogin / PageHome, owner decision 2026-11-23). The numbers come from
    the boards' CSS and were checked against the rendered boards StageLogin / StageHome (2000 px wide renders). */

namespace FrontScreensLayoutTestValues
{
    const juce::String testName     = "FrontScreensLayout";
    const juce::String testCategory = "Unit";

    const juce::String homeKeys      = "Home: Settings, Play (bigger, lit) and Exit centred in a row 56 apart, as the board";
    const juce::String homeScales    = "Home: at 1.6 x every length is 1.6 x the board's";
    const juce::String loginColumns  = "Log in: a 300 wide title column and a 400 wide form, 60 apart, centred";
    const juce::String loginRows     = "Log in: with every row the board shows, the email field and Log in sit where the board has them";
    const juce::String signUpRows    = "Sign up: Username comes first; the password row keeps a 74 wide Show button";

    constexpr float boardWidth  = 1200.0f;
    constexpr float pageHeight  = 365.0f;
    constexpr float boardScale  = 1.0f;
    constexpr float ownerScale  = 1.6f;
    constexpr float tolerance   = 0.5f;
    constexpr float renderTolerance = 1.5f;   // measured on a 2000 px render of the board

    // PageHome
    constexpr float settingsLeft = 284.0f;
    constexpr float settingsTop = 149.0f;
    constexpr float keyWidth = 150.0f;
    constexpr float keyHeight = 84.0f;
    constexpr float playLeft = 490.0f;
    constexpr float playTop = 124.4f;
    constexpr float playWidth = 220.0f;
    constexpr float playHeight = 110.0f;
    constexpr float exitLeft = 766.0f;
    constexpr float renderedPlayTop = 125.0f;      // StageHome render: 422 px / (2000 / 1200) - 128
    constexpr float renderedSettingsTop = 149.0f;

    // PageLogin
    constexpr float titleLeft = 220.0f;
    constexpr float formLeft = 580.0f;
    constexpr float formWidth = 400.0f;
    constexpr float renderedEmailTop = 97.6f;      // StageLogin render: 376 px / (2000 / 1200) - 128
    constexpr float renderedButtonsTop = 258.9f;   // 645 px / (2000 / 1200) - 128
    constexpr float passwordWidth = 320.0f;
    constexpr float showWidth = 74.0f;
}

using namespace FrontScreensLayoutTestValues;

class FrontScreensLayoutTest : public juce::UnitTest
{
public:
    FrontScreensLayoutTest() : juce::UnitTest (FrontScreensLayoutTestValues::testName, FrontScreensLayoutTestValues::testCategory) {}

    void runTest() override
    {
        const juce::Rectangle<float> board { boardWidth, pageHeight };

        beginTest (homeKeys);
        {
            const auto home = FrontScreensLayout::home (board, boardScale);
            expectRect (home.settingsKey, { settingsLeft, settingsTop, keyWidth, keyHeight });
            expectRect (home.playKey, { playLeft, playTop, playWidth, playHeight });
            expectRect (home.exitKey, { exitLeft, settingsTop, keyWidth, keyHeight });
            expectWithinAbsoluteError (home.playKey.getY(), renderedPlayTop, renderTolerance);
            expectWithinAbsoluteError (home.settingsKey.getY(), renderedSettingsTop, renderTolerance);
            expect (home.playLed.getBottom() < home.playKey.getY() && home.playLabel.getY() > home.playKey.getBottom());
            expectWithinAbsoluteError (home.playLed.getCentreX(), home.playKey.getCentreX(), tolerance);
        }

        beginTest (homeScales);
        {
            const auto small = FrontScreensLayout::home (board, boardScale);
            const auto large = FrontScreensLayout::home (board * ownerScale, ownerScale);
            expectRect (large.playKey, small.playKey * ownerScale);
            expectRect (large.exitKey, small.exitKey * ownerScale);
        }

        beginTest (loginColumns);
        {
            const auto login = FrontScreensLayout::login (board, boardScale, {});
            expectWithinAbsoluteError (login.title.getX(), titleLeft, tolerance);
            expectWithinAbsoluteError (login.emailField.getX(), formLeft, tolerance);
            expectWithinAbsoluteError (login.emailField.getWidth(), formWidth, tolerance);
            expect (login.usernameField.isEmpty() && login.rememberedRow.isEmpty() && login.rememberMe.isEmpty());
        }

        beginTest (loginRows);
        {
            const auto login = FrontScreensLayout::login (board, boardScale, { false, true, true });
            expectWithinAbsoluteError (login.emailField.getY(), renderedEmailTop, renderTolerance);
            expectWithinAbsoluteError (login.buttonRow.getY(), renderedButtonsTop, renderTolerance);
            expect (login.rememberedRow.getY() > login.emailField.getBottom() - tolerance
                    && login.passwordLabel.getY() > login.rememberedRow.getBottom());
            expect (login.rememberMe.getY() > login.passwordField.getBottom() && login.buttonRow.getY() > login.rememberMe.getBottom());
        }

        beginTest (signUpRows);
        {
            const auto signUp = FrontScreensLayout::login (board, boardScale, { true, false, false });
            expect (! signUp.usernameField.isEmpty() && signUp.usernameField.getBottom() < signUp.emailLabel.getY());
            expectWithinAbsoluteError (signUp.passwordField.getWidth(), passwordWidth, tolerance);
            expectWithinAbsoluteError (signUp.showButton.getWidth(), showWidth, tolerance);
            expectWithinAbsoluteError (signUp.showButton.getRight(), formLeft + formWidth, tolerance);
        }
    }

private:
    void expectRect (juce::Rectangle<float> actual, juce::Rectangle<float> expected)
    {
        expectWithinAbsoluteError (actual.getX(), expected.getX(), tolerance);
        expectWithinAbsoluteError (actual.getY(), expected.getY(), tolerance);
        expectWithinAbsoluteError (actual.getWidth(), expected.getWidth(), tolerance);
        expectWithinAbsoluteError (actual.getHeight(), expected.getHeight(), tolerance);
    }
};

static FrontScreensLayoutTest frontScreensLayoutTest;
