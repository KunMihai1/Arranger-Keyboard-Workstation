/*
  ==============================================================================

    LoginComponent.cpp
    Created: 25 Feb 2026 9:30:59pm
    Author:  Kisuke

  ==============================================================================
*/

#include "LoginComponent.h"
#include "AppColours.h"

using namespace AppColours;

namespace
{
    namespace Text
    {
        const juce::String emailLabel            = "Email:";
        const juce::String emailHint             = "Enter email...";
        const juce::String passwordLabel         = "Password:";
        const juce::String passwordHint          = "Enter password...";
        const juce::String usernameLabel         = "Username:";
        const juce::String usernameHint          = "Enter username...";
        const juce::String loginButton           = "Login";
        const juce::String signUpButton          = "Sign Up";
        const juce::String forgotButton          = "Forgot Pass?";
        const juce::String doneButton            = "Done";
        const juce::String backButton            = "Back";
        const juce::String eyeButton             = "eye";
        const juce::String createAccount         = "Create Account";
        const juce::String welcomeBack           = "Welcome Back";
        const juce::String loginTitle            = "Login";
        const juce::String enterEmailAndPassword = "Please enter both email and password.";
        const juce::String loginFailedTitle      = "Login Failed";
        const juce::String networkFailure        = "Network failure or invalid JSON";
        const juce::String verifyTitle           = "Email Verification Required";
        const juce::String verifyEmail           = "Please verify your email before logging in. Check your inbox.";
        const juce::String loginSuccessTitle     = "Login Success";
        const juce::String loggedIn              = "You are now logged in!";
        const juce::String unknownError          = "Unknown error occurred:\n";
        const juce::String forgotTitle           = "Forgot Password";
        const juce::String enterEmailFirst       = "Please enter your email first.";
        const juce::String signUpTitle           = "Sign Up";
        const juce::String fillAllFields         = "Please fill in all fields.";
        const juce::String signUpFailedTitle     = "Sign Up Failed";
        const juce::String invalidResponse       = "Invalid server response:\n";
        const juce::String signupFailed          = "Signup failed.";
        const juce::String userNotCreated        = "User was not created.\nServer response:\n";
        const juce::String signUpSuccessTitle    = "Sign Up Success";
        const juce::String createdCheckEmail     = "Account created successfully! Please check your email to verify.";
        const juce::String created               = "Account created successfully!";
    }

    // Fields of the auth server's JSON replies.
    namespace ResponseKeys
    {
        const juce::String error                = "error";
        const juce::String errorDescription     = "error_description";
        const juce::String message              = "msg";
        const juce::String confirmationRequired = "confirmation_required";
        const juce::String user                 = "user";
        const juce::String id                   = "id";
        const juce::String accessToken          = "access_token";
        const juce::String confirmationSentAt   = "confirmation_sent_at";
        const juce::String trueText             = "true";
    }

    constexpr float fieldFontHeight       = 16.0f;
    constexpr float titleFontHeight       = 32.0f;
    constexpr float editorBackgroundAlpha = 0.6f;
    constexpr float cardAlpha             = 0.85f;
    constexpr float borderAlpha           = 0.5f;
    constexpr float cornerRadius          = 15.0f;
    constexpr float borderThickness       = 1.5f;
    constexpr int   headerHeight          = 90;
    constexpr int   headerTopTrim         = 25;
    constexpr int   cardPadding           = 30;
    constexpr int   backButtonInset       = 15;
    constexpr int   backButtonWidth       = 80;
    constexpr int   backButtonHeight      = 30;
    constexpr int   headerSkip            = 60;
    constexpr int   labelWidth            = 90;
    constexpr int   noInset               = 0;
    constexpr int   labelGap              = 10;
    constexpr int   buttonsTopGap         = 10;
    constexpr int   buttonRowExtraHeight  = 5;
    constexpr int   buttonSpacing         = 12;
    constexpr int   buttonsPerRow         = 3;
    constexpr int   gapsBetweenButtons    = 2;     // buttonsPerRow - 1
    constexpr int   doneWidth             = 140;
    constexpr int   doneHeight            = 45;
    constexpr int   doneBottomOffset      = 65;
    constexpr int   centringDivisor       = 2;
    constexpr int   eyeButtonGap          = 20;
    constexpr int   eyeButtonInset        = 2;
    constexpr juce::juce_wchar passwordBullet      = 0x2022;
    constexpr juce::juce_wchar noPasswordCharacter = 0;
}

LoginComponent::LoginComponent(std::shared_ptr<SupabaseClient> newClient): client{newClient}
{
    auto setupLabel = [](juce::Label& lbl, const juce::String& text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, subtitleText);
        lbl.setFont(juce::FontOptions(fieldFontHeight, juce::Font::bold));
        lbl.setJustificationType(juce::Justification::centredRight);
    };

    auto setupTE = [](juce::TextEditor& te, const juce::String& emptyText) {
        te.setMultiLine(false);
        te.setColour(juce::TextEditor::backgroundColourId, background.withAlpha(editorBackgroundAlpha));
        te.setColour(juce::TextEditor::textColourId, titleText);
        te.setColour(juce::TextEditor::outlineColourId, separator);
        te.setColour(juce::TextEditor::focusedOutlineColourId, accent1);
        te.setFont(juce::FontOptions(fieldFontHeight));
        te.setTextToShowWhenEmpty(emptyText, subtitleText);
    };

    emailLabel = std::make_unique<juce::Label>();
    setupLabel(*emailLabel, Text::emailLabel);
    addAndMakeVisible(emailLabel.get());

    emailTE = std::make_unique<juce::TextEditor>();
    setupTE(*emailTE, Text::emailHint);
    addAndMakeVisible(emailTE.get());

    passwordLabel = std::make_unique<juce::Label>();
    setupLabel(*passwordLabel, Text::passwordLabel);
    addAndMakeVisible(passwordLabel.get());

    passwordField = std::make_unique<PasswordField>();
    addAndMakeVisible(passwordField.get());

    UsernameLabel = std::make_unique<juce::Label>();
    setupLabel(*UsernameLabel, Text::usernameLabel);
    addAndMakeVisible(UsernameLabel.get());
    UsernameLabel->setVisible(false);

    UsernameTE = std::make_unique<juce::TextEditor>();
    setupTE(*UsernameTE, Text::usernameHint);
    addAndMakeVisible(UsernameTE.get());
    UsernameTE->setVisible(false);

    auto styleButton = [](juce::TextButton* btn, juce::Colour bgColour) {
        btn->setColour(juce::TextButton::buttonColourId, bgColour);
        btn->setColour(juce::TextButton::textColourOnId, titleText);
        btn->setColour(juce::TextButton::textColourOffId, titleText);
        btn->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    };

    loginTB = std::make_unique<juce::TextButton>(Text::loginButton);
    loginTB->onClick = [this]() { handleLogin(); };
    styleButton(loginTB.get(), accent1); // Warm orange for main action
    addAndMakeVisible(loginTB.get());
    
    signupTB = std::make_unique<juce::TextButton>(Text::signUpButton);
    signupTB->onClick = [this]() { handleSignup(); };
    styleButton(signupTB.get(), panelBg);
    addAndMakeVisible(signupTB.get());

    forgotPassTB = std::make_unique<juce::TextButton>(Text::forgotButton);
    forgotPassTB->onClick = [this]() { handleForgotPassword(); };
    styleButton(forgotPassTB.get(), panelBg);
    addAndMakeVisible(forgotPassTB.get());

    doneTB = std::make_unique<juce::TextButton>(Text::doneButton);
    doneTB->onClick = [this]() { handleDoneSignup(); };
    styleButton(doneTB.get(), accent1);
    addAndMakeVisible(doneTB.get());
    doneTB->setVisible(false);

    backTB = std::make_unique<juce::TextButton>(Text::backButton);
    backTB->onClick = [this]() { handleBack(); };
    styleButton(backTB.get(), panelBg);
    addAndMakeVisible(backTB.get());
    backTB->setVisible(false);
}

void LoginComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Glassmorphism card background
    g.setColour(cardBg.withAlpha(cardAlpha));
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Subtle border
    g.setColour(separator.withAlpha(borderAlpha));
    g.drawRoundedRectangle(bounds, cornerRadius, borderThickness);

    // Header Title
    g.setFont(juce::FontOptions(titleFontHeight, juce::Font::bold));
    g.setColour(titleText);
    
    // Offset the header down slightly so it doesn't overlap the Back button
    auto headerArea = bounds.removeFromTop(headerHeight).withTrimmedTop(headerTopTrim);
    g.drawText(UsernameLabel->isVisible() ? Text::createAccount : Text::welcomeBack, 
               headerArea, juce::Justification::centred, false);
}


void LoginComponent::resized()
{
    auto bounds = getLocalBounds();
    auto area = bounds.reduced(cardPadding); 
    const int rowHeight = 35; 
    const int spacing = 20;

    if (backTB != nullptr)
        backTB->setBounds(bounds.getX() + backButtonInset, bounds.getY() + backButtonInset, backButtonWidth, backButtonHeight);

    area.removeFromTop(headerSkip); // Skip header

    auto layoutField = [&](juce::Label* label, juce::Component* field) {
        auto row = area.removeFromTop(rowHeight);
        label->setBounds(row.removeFromLeft(labelWidth).reduced(noInset, noInset)); 
        row.removeFromLeft(labelGap); // Spacing between label and input
        field->setBounds(row);
        area.removeFromTop(spacing);
    };

    if (UsernameLabel->isVisible()) {
        layoutField(UsernameLabel.get(), UsernameTE.get());
    } else {
        UsernameLabel->setBounds(juce::Rectangle<int>{});
        UsernameTE->setBounds(juce::Rectangle<int>{});
    }

    layoutField(emailLabel.get(), emailTE.get());
    layoutField(passwordLabel.get(), passwordField.get());

    area.removeFromTop(buttonsTopGap); 

    auto buttonArea = area.removeFromTop(rowHeight + buttonRowExtraHeight);
    
    if (loginTB->isVisible()) {
        int buttonWidth = (buttonArea.getWidth() - gapsBetweenButtons * buttonSpacing) / buttonsPerRow;
        int buttonX = buttonArea.getX();

        loginTB->setBounds(buttonX, buttonArea.getY(), buttonWidth, buttonArea.getHeight());
        buttonX += buttonWidth + buttonSpacing;

        signupTB->setBounds(buttonX, buttonArea.getY(), buttonWidth, buttonArea.getHeight());
        buttonX += buttonWidth + buttonSpacing;

        forgotPassTB->setBounds(buttonX, buttonArea.getY(), buttonWidth, buttonArea.getHeight());
    }

    if (doneTB != nullptr && doneTB->isVisible())
    {
        int doneX = bounds.getCentreX() - doneWidth / centringDivisor;
        int doneY = bounds.getBottom() - doneBottomOffset; 
        doneTB->setBounds(doneX, doneY, doneWidth, doneHeight);
    }
}

LoginComponent::~LoginComponent()
{
}

void LoginComponent::handleLogin()
{
    if (onSuccessfullLogin)
        onSuccessfullLogin();
    return;

    auto email = emailTE->getText().trim();
    auto pass = passwordField->getTextEditor().getText().trim();

    if (email.isEmpty() || pass.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            Text::loginTitle,
            Text::enterEmailAndPassword
        );
        return;
    }

    juce::Component::SafePointer<LoginComponent> safeThis(this);
    auto clientPtr = client;

    

    std::thread([email, pass, safeThis, clientPtr]()
        {

            auto response = clientPtr->login(email, pass);

            if (safeThis == nullptr)
                return;

            auto body = response.body;

            auto parsed = juce::JSON::parse(body);

            juce::MessageManager::callAsync([body,parsed,response, safeThis, clientPtr]()
                {
                    

                    if (!parsed.isObject())
                    {
                        
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::loginFailedTitle,
                            Text::networkFailure
                        );
                        return;
                    }

                    auto* obj = parsed.getDynamicObject();

                    
                    if (obj->hasProperty(ResponseKeys::error) || obj->hasProperty(ResponseKeys::errorDescription) || obj->hasProperty(ResponseKeys::message))
                    {
                        juce::String errorMessage;

                        if (obj->hasProperty(ResponseKeys::errorDescription))
                            errorMessage = obj->getProperty(ResponseKeys::errorDescription).toString();
                        else if (obj->hasProperty(ResponseKeys::error))
                            errorMessage = obj->getProperty(ResponseKeys::error).toString();
                        else if (obj->hasProperty(ResponseKeys::message))
                            errorMessage = obj->getProperty(ResponseKeys::message).toString();

                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::loginFailedTitle,
                            errorMessage
                        );
                        return;
                    }

                    
                    
                    
                    if (obj->hasProperty(ResponseKeys::confirmationRequired) && obj->getProperty(ResponseKeys::confirmationRequired).toString() == ResponseKeys::trueText)
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            Text::verifyTitle,
                            Text::verifyEmail
                        );
                        return;
                    }
                    

                    if (obj->hasProperty(ResponseKeys::user))
                    {
                        auto userObj = obj->getProperty(ResponseKeys::user).getDynamicObject();
                        if (userObj != nullptr)
                        {
                            clientPtr->setUserId(userObj->getProperty(ResponseKeys::id).toString());
                        }
                    }

                    if (obj->hasProperty(ResponseKeys::accessToken))
                    {

                        clientPtr->setAccessToken(obj->getProperty(ResponseKeys::accessToken).toString());

                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            Text::loginSuccessTitle,
                            Text::loggedIn
                        );

                        

                        safeThis->emailTE->clear();
                        safeThis->passwordField->getTextEditor().clear();

                        if (safeThis->onSuccessfullLogin)
                            safeThis->onSuccessfullLogin();
                    }
                    else
                    {
                        
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::loginFailedTitle,
                            Text::unknownError + body
                        );
                    }
                });

        }).detach();
}


void LoginComponent::handleSignup()
{
    if (!UsernameLabel->isVisible())
    {
        toSignupVisibility();
    }
}

void LoginComponent::handleForgotPassword()
{
    auto email = emailTE->getText();

    if (email.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            Text::forgotTitle,
            Text::enterEmailFirst);
        return;
    }


    email = emailTE->getText();

    if (email.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            Text::forgotTitle,
            Text::enterEmailFirst);
        return;
    }
}

void LoginComponent::handleDoneSignup()
{
    auto email = emailTE->getText().trim();
    auto pass = passwordField->getTextEditor().getText().trim();
    auto username = UsernameTE->getText().trim();

    if (email.isEmpty() || pass.isEmpty() || username.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            Text::signUpTitle,
            Text::fillAllFields
        );
        return;
    }

    juce::Component::SafePointer<LoginComponent> safeThis(this);
    auto clientPtr = client;

    std::thread([email, pass, username, safeThis, clientPtr]()
        {

            auto response = clientPtr->signup(email, pass, username);

            if (safeThis == nullptr)
                return;

            auto body = response.body;

            auto parsed = juce::JSON::parse(body);

            juce::MessageManager::callAsync([body,parsed,response, safeThis]()
                {
                    

                    if (!parsed.isObject())
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::signUpFailedTitle,
                            Text::invalidResponse + body
                        );
                        return;
                    }

                    auto* obj = parsed.getDynamicObject();

                    
                    if (obj->hasProperty(ResponseKeys::error) || obj->hasProperty(ResponseKeys::errorDescription))
                    {
                        juce::String errorMessage = Text::signupFailed;
                        if (obj->hasProperty(ResponseKeys::errorDescription))
                            errorMessage = obj->getProperty(ResponseKeys::errorDescription).toString();
                        else if (obj->hasProperty(ResponseKeys::error))
                            errorMessage = obj->getProperty(ResponseKeys::error).toString();
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::signUpFailedTitle,
                            errorMessage
                        );
                        return;
                    }

                    
                    bool isSession = obj->hasProperty(ResponseKeys::accessToken);
                    bool isUser = obj->hasProperty(ResponseKeys::id);
                    if (!isSession && !isUser)
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            Text::signUpFailedTitle,
                            Text::userNotCreated + body
                        );
                        return;
                    }

                    
                    if (isUser && obj->hasProperty(ResponseKeys::confirmationSentAt))
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            Text::signUpSuccessTitle,
                            Text::createdCheckEmail
                        );
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            Text::signUpSuccessTitle,
                            Text::created
                        );
                    }

                    
                    safeThis->UsernameLabel->setVisible(false);
                    safeThis->UsernameTE->setVisible(false);
                    safeThis->backTB->setVisible(false);
                    safeThis->doneTB->setVisible(false);
                    safeThis->loginTB->setVisible(true);
                    safeThis->forgotPassTB->setVisible(true);
                    safeThis->signupTB->setVisible(true);

                    safeThis->emailTE->clear();
                    safeThis->passwordField->getTextEditor().clear();
                    safeThis->UsernameTE->clear();
                });

        }).detach();
}

void LoginComponent::handleBack()
{
    toLoginVisbility();

}

void LoginComponent::toLoginVisbility()
{
    UsernameTE->clear();
    emailTE->clear();
    passwordField->getTextEditor().clear();

    UsernameLabel->setVisible(false);
    UsernameTE->setVisible(false);
    doneTB->setVisible(false);
    backTB->setVisible(false);
    forgotPassTB->setVisible(true);
    loginTB->setVisible(true);
    signupTB->setVisible(true);
    
    resized();
    repaint();
}

void LoginComponent::toSignupVisibility()
{
    UsernameTE->clear();
    emailTE->clear();
    passwordField->getTextEditor().clear();

    UsernameLabel->setVisible(true);
    UsernameTE->setVisible(true);
    doneTB->setVisible(true);
    backTB->setVisible(true);
    forgotPassTB->setVisible(false);
    loginTB->setVisible(false);
    signupTB->setVisible(false);
    
    resized();
    repaint();
}

PasswordField::PasswordField()
{
    passwordTE.setMultiLine(false);
    passwordTE.setPasswordCharacter(passwordBullet);
    passwordTE.setCaretVisible(true);
    passwordTE.setTextToShowWhenEmpty(Text::passwordHint, subtitleText);
    
    passwordTE.setColour(juce::TextEditor::backgroundColourId, background.withAlpha(editorBackgroundAlpha));
    passwordTE.setColour(juce::TextEditor::textColourId, titleText);
    passwordTE.setColour(juce::TextEditor::outlineColourId, separator);
    passwordTE.setColour(juce::TextEditor::focusedOutlineColourId, accent1);
    passwordTE.setFont(juce::FontOptions(fieldFontHeight));
    addAndMakeVisible(passwordTE);

    
    eyeButton = std::make_unique<juce::DrawableButton>(Text::eyeButton, juce::DrawableButton::ImageFitted);

    eyeVisibleDrawable = juce::Drawable::createFromImageData(BinaryData::PasswordVisible_png, BinaryData::PasswordVisible_pngSize);
    eyeInvisibleDrawable = juce::Drawable::createFromImageData(BinaryData::PasswordInvisible_png, BinaryData::PasswordInvisible_pngSize);

    eyeButton->setImages(eyeInvisibleDrawable.get());

    eyeButton->onClick = [this]()
        {
            visible = !visible;
            passwordTE.setPasswordCharacter(visible ? noPasswordCharacter : passwordBullet); 
            eyeButton->setImages(visible ? eyeVisibleDrawable.get() : eyeInvisibleDrawable.get());
        };

    eyeButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addAndMakeVisible(eyeButton.get());
}

void PasswordField::resized()
{
    auto area = getLocalBounds();
    int buttonSize = area.getHeight();

    passwordTE.setBounds(area.removeFromLeft(area.getWidth() - buttonSize - eyeButtonGap));
    eyeButton->setBounds(area.reduced(eyeButtonInset));
}

juce::TextEditor& PasswordField::getTextEditor()
{
    return passwordTE;
}
