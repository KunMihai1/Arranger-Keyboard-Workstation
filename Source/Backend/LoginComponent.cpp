/*
  ==============================================================================

    LoginComponent.cpp
    Created: 25 Feb 2026 9:30:59pm
    Author:  Kisuke

  ==============================================================================
*/

#include "LoginComponent.h"
#include "Stage/StageAlert.h"
#include "AppColours.h"
#include "AppInfo.h"
#include "Stage/StageFonts.h"
#include "Stage/FrontScreensLayout.h"
#include "BackendJson.h"
#include "RememberedEmail.h"

namespace
{
    namespace Text
    {
        const juce::String emailLabel            = "Email";
        const juce::String emailHint             = "Enter email...";
        const juce::String passwordLabel         = "Password";
        const juce::String passwordHint          = "Enter password...";
        const juce::String usernameLabel         = "Username";
        const juce::String usernameHint          = "Enter username...";
        const juce::String loginButton           = "Log in";
        const juce::String signUpButton          = "Sign up";
        const juce::String forgotButton          = "Forgot password?";
        const juce::String createButton          = "Create account";
        const juce::String backButton            = "Back to log in";
        const juce::String showPassword          = "Show";
        const juce::String hidePassword          = "Hide";
        const juce::String loginSubtitle         = "Welcome back. Log in to start playing.";
        const juce::String signUpSubtitle        = "Create an account to start playing.";
        const juce::String loginTitle            = "Login";
        const juce::String enterEmailAndPassword = "Please enter both email and password.";
        const juce::String loginFailedTitle      = "Login Failed";
        const juce::String networkFailure        = "Network failure or invalid JSON";
        const juce::String verifyTitle           = "Email Verification Required";
        const juce::String verifyEmail           = "Please verify your email before logging in. Check your inbox.";
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
        const juce::String remembered            = "Remembered from last time.";
        const juce::String notYou                = "Not you?";
    }

    // Board PageLogin: the title (22 px, .24em, two lines), the subtitle (.silk-dim 11 px) and the .seg buttons (30 high).
    constexpr float titleSize          = 22.0f;
    constexpr float titleSpacing       = 0.24f;
    constexpr float subtitleSize       = 11.0f;
    constexpr float subtitleSpacing    = 0.06f;
    constexpr int   segHeight          = 30;
    constexpr int   segPadding         = 14;
    constexpr float segTextSize        = 11.0f;
    constexpr int   buttonGap          = 8;
    constexpr int   rememberedGap      = 8;   // "Remembered from last time." and Not you?
    constexpr bool  semiBold           = true;
    constexpr bool  medium             = false;
    constexpr juce::juce_wchar passwordBullet      = 0x2022;

    /*  Log in asks the server (the backend answers again, owner 2026-11-28). While the Supabase project was
        unreachable this was false and Log in let the typed email in; set it back to false if the backend goes away
        again, or a real check locks everyone out. */
    constexpr bool checkAccountWithServer = true;
    constexpr juce::juce_wchar noPasswordCharacter = 0;
}

LoginComponent::LoginComponent(std::shared_ptr<SupabaseClient> newClient)
    : usernameLabel (Text::usernameLabel, SilkLabel::Tone::bright),
      emailLabel (Text::emailLabel, SilkLabel::Tone::bright),
      passwordLabel (Text::passwordLabel, SilkLabel::Tone::bright),
      usernameField (Text::usernameHint),
      emailField (Text::emailHint),
      passwordField (Text::passwordHint),
      showButton (Text::showPassword, SegButton::Style::seg),
      loginButton (Text::loginButton, SegButton::Style::lit, true, RubberButton::LedColour::green),
      signUpButton (Text::signUpButton, SegButton::Style::seg),
      forgotButton (Text::forgotButton, SegButton::Style::seg),
      createButton (Text::createButton, SegButton::Style::lit, true, RubberButton::LedColour::green),
      backButton (Text::backButton, SegButton::Style::seg),
      notYouLink (Text::notYou),
      client{newClient}
{
    for (auto* label : { &usernameLabel, &emailLabel, &passwordLabel })
        addChildComponent (*label);
    for (auto* field : { &usernameField, &emailField, &passwordField })
        addChildComponent (*field);
    passwordField.setPasswordCharacter (passwordBullet);

    for (auto* button : { &showButton, &loginButton, &signUpButton, &forgotButton, &createButton, &backButton })
    {
        button->setBoardHeight (segHeight);
        button->setPaddingSide (segPadding);
        button->setTextSize (segTextSize);
        addChildComponent (*button);
    }
    addChildComponent (notYouLink);
    notYouLink.onClick = [this] { forgetRememberedEmail(); };
    emailField.onTextChange = [this] { resized(); repaint(); };   // the remembered row follows the field
    loginButton.setLedOn (true);
    createButton.setLedOn (true);

    showButton.onClick   = [this] { togglePasswordShown(); };
    loginButton.onClick  = [this] { handleLogin(); };
    signUpButton.onClick = [this] { handleSignup(); };
    forgotButton.onClick = [this] { handleForgotPassword(); };
    createButton.onClick = [this] { handleDoneSignup(); };
    backButton.onClick   = [this] { handleBack(); };

    // The board's form submits on Enter; Esc still reaches the Esc menu while a field is focused.
    for (auto* field : { &usernameField, &emailField, &passwordField })
    {
        field->onReturnKey = [this] { if (signUpMode) handleDoneSignup(); else handleLogin(); };
        field->setEscapeAndReturnKeysConsumed (false);
    }

    setOpaque (true);
    setSignUpMode (false);
}

void LoginComponent::setScale (float newScale)
{
    scale = newScale;
    for (auto* label : { &usernameLabel, &emailLabel, &passwordLabel })
        label->setScale (scale);
    for (auto* field : { &usernameField, &emailField, &passwordField })
        field->setScale (scale);
    for (auto* button : { &showButton, &loginButton, &signUpButton, &forgotButton, &createButton, &backButton })
        button->setScale (scale);
    notYouLink.setScale (scale);
    resized();
    repaint();
}

void LoginComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, bounds.getY(), StagePalette::pageBottom, bounds.getBottom()));
    g.fillRect (bounds);

    const auto layout = FrontScreensLayout::login (bounds, scale, { signUpMode, showsRememberedRow(), false });
    g.setColour (StagePalette::silkscreenText);
    g.setFont (StageFonts::silkscreen (titleSize * scale, semiBold, titleSpacing));
    // One word a line, as the board (its 300 px column wraps "Arranger Workstation").
    const auto titleLines = juce::StringArray::fromTokens (juce::String (AppInfo::appName).toUpperCase(), false);
    auto line = layout.title.withHeight (layout.title.getHeight() / static_cast<float> (FrontScreensLayout::titleLineCount));
    for (const auto& word : titleLines)
    {
        g.drawText (word, line, juce::Justification::centredLeft, false);
        line.translate (float {}, line.getHeight());
    }

    g.setColour (StagePalette::ruleLine);
    g.fillRect (layout.rule);

    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (subtitleSize * scale, medium, subtitleSpacing));
    g.drawText (signUpMode ? Text::signUpSubtitle : Text::loginSubtitle, layout.subtitle, juce::Justification::centredLeft, false);
    if (showsRememberedRow())
        g.drawText (Text::remembered, layout.rememberedRow, juce::Justification::centredLeft, false);
}

void LoginComponent::resized()
{
    const auto layout = FrontScreensLayout::login (getLocalBounds().toFloat(), scale, { signUpMode, showsRememberedRow(), false });
    usernameLabel.setBounds (layout.usernameLabel.toNearestInt());
    usernameField.setBounds (layout.usernameField.toNearestInt());
    emailLabel.setBounds (layout.emailLabel.toNearestInt());
    emailField.setBounds (layout.emailField.toNearestInt());
    passwordLabel.setBounds (layout.passwordLabel.toNearestInt());
    passwordField.setBounds (layout.passwordField.toNearestInt());
    showButton.setBounds (layout.showButton.toNearestInt());

    notYouLink.setVisible (showsRememberedRow());
    const auto rememberedWidth = juce::GlyphArrangement::getStringWidth (StageFonts::silkscreen (subtitleSize * scale, medium, subtitleSpacing),
                                                                         Text::remembered);
    notYouLink.setBounds (layout.rememberedRow.withTrimmedLeft (rememberedWidth + static_cast<float> (rememberedGap) * scale)
                                               .withWidth (notYouLink.getExactWidth()).toNearestInt());

    // The lit button takes the room the others leave (flex-grow), 8 apart.
    auto row = layout.buttonRow;
    const auto gap = static_cast<float> (buttonGap) * scale;
    auto& primary = signUpMode ? createButton : loginButton;
    std::vector<SegButton*> others { signUpMode ? &backButton : &signUpButton };
    if (! signUpMode)
        others.push_back (&forgotButton);
    for (auto other = others.rbegin(); other != others.rend(); ++other)
    {
        (*other)->setBounds (row.removeFromRight ((*other)->getExactWidth()).toNearestInt());
        row.removeFromRight (gap);
    }
    primary.setBounds (row.toNearestInt());
}

LoginComponent::~LoginComponent()
{
}

void LoginComponent::setSignUpMode (bool shouldSignUp)
{
    signUpMode = shouldSignUp;
    if (passwordShown)
        togglePasswordShown();   // each form starts with the password hidden
    usernameField.clear();
    emailField.clear();
    passwordField.clear();
    if (! signUpMode)
        emailField.setText (rememberedEmail, false);

    for (juce::Component* child : { static_cast<juce::Component*> (&emailLabel), static_cast<juce::Component*> (&emailField),
                                    static_cast<juce::Component*> (&passwordLabel), static_cast<juce::Component*> (&passwordField),
                                    static_cast<juce::Component*> (&showButton) })
        child->setVisible (true);
    usernameLabel.setVisible (signUpMode);
    usernameField.setVisible (signUpMode);
    createButton.setVisible (signUpMode);
    backButton.setVisible (signUpMode);
    loginButton.setVisible (! signUpMode);
    signUpButton.setVisible (! signUpMode);
    forgotButton.setVisible (! signUpMode);

    resized();
    repaint();
}

void LoginComponent::togglePasswordShown()
{
    passwordShown = ! passwordShown;
    passwordField.setPasswordCharacter (passwordShown ? noPasswordCharacter : passwordBullet);
    showButton.setButtonText (passwordShown ? Text::hidePassword : Text::showPassword);
    resized();
}

void LoginComponent::handleLogin()
{
    if (waitingForServer)
        return;

    if constexpr (checkAccountWithServer)
        askServerToLogIn();
    else
        letInWithoutServer();
}

void LoginComponent::askServerToLogIn()
{
    auto email = emailField.getText().trim();
    auto pass = passwordField.getText().trim();

    if (email.isEmpty() || pass.isEmpty())
    {
        StageAlert::showMessage (Text::loginTitle, Text::enterEmailAndPassword);
        return;
    }

    juce::Component::SafePointer<LoginComponent> safeThis(this);
    auto clientPtr = client;
    setWaitingForServer (true);

    std::thread([email, pass, safeThis, clientPtr]()
        {
            const auto response = clientPtr->login(email, pass);
            const auto reply = BackendJson::readLoginReply (response.body);

            juce::MessageManager::callAsync([email, reply, body = response.body, safeThis, clientPtr]()
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->setWaitingForServer (false);

                    using Outcome = BackendJson::LoginReply::Outcome;
                    switch (reply.outcome)
                    {
                        case Outcome::signedIn:
                            clientPtr->setUserId (reply.userId);
                            clientPtr->setAccessToken (reply.accessToken);
                            safeThis->passwordField.clear();
                            safeThis->rememberedEmail = email;
                            if (safeThis->onSuccessfullLogin)
                                safeThis->onSuccessfullLogin (email);   // straight to Home: no "logged in" box
                            return;
                        case Outcome::rejected:
                            StageAlert::showMessage (Text::loginFailedTitle, reply.message);
                            return;
                        case Outcome::confirmEmail:
                            StageAlert::showMessage (Text::verifyTitle, Text::verifyEmail);
                            return;
                        case Outcome::unreadable:
                            StageAlert::showMessage (Text::loginFailedTitle, Text::networkFailure);
                            return;
                        case Outcome::unexpected:
                            StageAlert::showMessage (Text::loginFailedTitle, Text::unknownError + body);
                            return;
                    }
                });
        }).detach();
}


void LoginComponent::handleSignup()
{
    if (! signUpMode)
        setSignUpMode (true);
}

void LoginComponent::handleForgotPassword()
{
    auto email = emailField.getText();

    if (email.isEmpty())
    {
        StageAlert::showMessage (Text::forgotTitle, Text::enterEmailFirst);
        return;
    }


    email = emailField.getText();

    if (email.isEmpty())
    {
        StageAlert::showMessage (Text::forgotTitle, Text::enterEmailFirst);
        return;
    }
}

void LoginComponent::handleDoneSignup()
{
    if (waitingForServer)
        return;

    auto email = emailField.getText().trim();
    auto pass = passwordField.getText().trim();
    auto username = usernameField.getText().trim();

    if (email.isEmpty() || pass.isEmpty() || username.isEmpty())
    {
        StageAlert::showMessage (Text::signUpTitle, Text::fillAllFields);
        return;
    }

    juce::Component::SafePointer<LoginComponent> safeThis(this);
    auto clientPtr = client;
    setWaitingForServer (true);

    std::thread([email, pass, username, safeThis, clientPtr]()
        {
            const auto response = clientPtr->signup(email, pass, username);
            const auto reply = BackendJson::readSignupReply (response.body);

            juce::MessageManager::callAsync([email, reply, body = response.body, safeThis]()
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->setWaitingForServer (false);

                    using Outcome = BackendJson::SignupReply::Outcome;
                    switch (reply.outcome)
                    {
                        case Outcome::unreadable:
                            StageAlert::showMessage (Text::signUpFailedTitle, Text::invalidResponse + body);
                            return;
                        case Outcome::rejected:
                            StageAlert::showMessage (Text::signUpFailedTitle, reply.message.isNotEmpty() ? reply.message : Text::signupFailed);
                            return;
                        case Outcome::notCreated:
                            StageAlert::showMessage (Text::signUpFailedTitle, Text::userNotCreated + body);
                            return;
                        case Outcome::createdCheckEmail:
                            StageAlert::showMessage (Text::signUpSuccessTitle, Text::createdCheckEmail);
                            break;
                        case Outcome::created:
                            StageAlert::showMessage (Text::signUpSuccessTitle, Text::created);
                            break;
                    }
                    safeThis->setSignUpMode (false);   // back to Log in, the new email filled in
                    safeThis->emailField.setText (email);
                });
        }).detach();
}

void LoginComponent::handleBack()
{
    setSignUpMode (false);
}

void LoginComponent::setRememberedEmail (const juce::String& email)
{
    rememberedEmail = email;
    if (! signUpMode)
        emailField.setText (email, false);
    resized();
    repaint();
}

void LoginComponent::forgetRememberedEmail()
{
    rememberedEmail = {};
    emailField.clear();
    if (onForgetEmail)
        onForgetEmail();
    resized();
    repaint();
    emailField.grabKeyboardFocus();
}

bool LoginComponent::showsRememberedRow() const
{
    return RememberedEmail::showRow (rememberedEmail, emailField.getText(), signUpMode);
}

void LoginComponent::setWaitingForServer (bool waiting)
{
    // One request at a time: the form's buttons rest until the server answers.
    waitingForServer = waiting;
    for (auto* button : { &loginButton, &signUpButton, &forgotButton, &createButton, &backButton })
        button->setEnabled (! waiting);
}

void LoginComponent::letInWithoutServer()
{
    const auto typedEmail = emailField.getText().trim();
    passwordField.clear();
    if (typedEmail.isNotEmpty())   // Log in with nothing typed keeps what was remembered
        rememberedEmail = typedEmail;
    if (onSuccessfullLogin)
        onSuccessfullLogin (typedEmail);
}
