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
    What the buttons do (the account calls) is unchanged. */
class LoginComponent : public juce::Component
{
public:

    std::function<void()> onSuccessfullLogin;

    LoginComponent(std::shared_ptr<SupabaseClient> client);

    ~LoginComponent() override;

    void setScale (float newScale);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:

    void handleLogin();
    void handleSignup();
    void handleForgotPassword();
    void handleDoneSignup();
    void handleBack();

    void setSignUpMode (bool shouldSignUp);
    void togglePasswordShown();

    SilkLabel usernameLabel, emailLabel, passwordLabel;
    StageLcdInput usernameField, emailField, passwordField;
    SegButton showButton, loginButton, signUpButton, forgotButton, createButton, backButton;
    bool signUpMode = false;
    bool passwordShown = false;
    float scale { StageMetrics::referenceScale };
    std::shared_ptr<SupabaseClient> client;
};
