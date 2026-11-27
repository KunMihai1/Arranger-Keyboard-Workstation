/*
  ==============================================================================

    LoginComponent.h
    Created: 25 Feb 2026 9:30:59pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "SupabaseClient.h"
#include "Stage/StagePageWidgets.h"

/*  Log in / Sign up on the instrument (boards StageLogin / PageLogin, owner decision 2026-11-23): it fills the page
    area under the panel. Left: the app's name and a line of text; right: the form with LCD fields and seg buttons.
    Log in checks the account (spec 2026-11-27 sign-in); the last email is remembered ("Not you?" forgets it). */
class LoginComponent : public juce::Component
{
public:

    std::function<void (const juce::String& email)> onSuccessfullLogin;
    std::function<void()> onForgetEmail;   ///< "Not you?": the owner of the settings forgets LastEmail

    LoginComponent(std::shared_ptr<SupabaseClient> client);

    ~LoginComponent() override;

    void setScale (float newScale);
    /** The email remembered from last time: filled in, with "Remembered from last time. Not you?". */
    void setRememberedEmail (const juce::String& email);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:

    void handleLogin();
    void askServerToLogIn();     ///< the real check (BackendJson replies)
    void letInWithoutServer();   ///< while the backend is unreachable (owner 2026-11-28): as before, no check
    void handleSignup();
    void handleForgotPassword();
    void handleDoneSignup();
    void handleBack();

    void setSignUpMode (bool shouldSignUp);
    void togglePasswordShown();
    void forgetRememberedEmail();
    bool showsRememberedRow() const;
    void setWaitingForServer (bool waiting);

    SilkLabel usernameLabel, emailLabel, passwordLabel;
    StageLcdInput usernameField, emailField, passwordField;
    SegButton showButton, loginButton, signUpButton, forgotButton, createButton, backButton;
    StageTextLink notYouLink;
    juce::String rememberedEmail;
    bool waitingForServer = false;
    bool signUpMode = false;
    bool passwordShown = false;
    float scale { StageMetrics::referenceScale };
    std::shared_ptr<SupabaseClient> client;
};
