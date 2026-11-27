#pragma once

#include <juce_graphics/juce_graphics.h>

/*  Where the Log in and Home screens put things in the page area (boards PageLogin / PageHome, 1200 x 365 at
    scale 1, content centred). Pure: the components place their widgets from these rectangles. */
namespace FrontScreensLayout
{
    inline constexpr int titleLineCount = 2;   // "Arranger" / "Workstation"

    struct Home
    {
        juce::Rectangle<float> settingsLed, settingsKey, playLed, playKey, playLabel, exitLed, exitKey;
        juce::Rectangle<float> signedInRow;   ///< top right: "Signed in as ..." and Log out
    };

    Home home (juce::Rectangle<float> pageArea, float scale);

    struct LoginOptions
    {
        bool signUp {};           ///< Username above Email; Create account / Back to log in
        bool showRemembered {};   ///< "Remembered from last time. Not you?" under the email
        bool showRememberMe {};   ///< the Remember me toggle under the password
    };

    struct Login
    {
        juce::Rectangle<float> title, rule, subtitle;
        juce::Rectangle<float> usernameLabel, usernameField;   ///< empty unless signing up
        juce::Rectangle<float> emailLabel, emailField, rememberedRow;
        juce::Rectangle<float> passwordLabel, passwordField, showButton;
        juce::Rectangle<float> rememberMe;
        juce::Rectangle<float> buttonRow;
    };

    Login login (juce::Rectangle<float> pageArea, float scale, LoginOptions options);
}
