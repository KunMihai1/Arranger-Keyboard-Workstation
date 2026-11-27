/*
  ==============================================================================

    LoginComponent.cpp
    Created: 25 Feb 2026 9:30:59pm
    Author:  Kisuke

  ==============================================================================
*/

#include "LoginComponent.h"
#include "AppColours.h"
#include "AppInfo.h"
#include "Stage/StageFonts.h"
#include "Stage/FrontScreensLayout.h"

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

    // Board PageLogin: the title (22 px, .24em, two lines), the subtitle (.silk-dim 11 px) and the .seg buttons (30 high).
    constexpr float titleSize          = 22.0f;
    constexpr float titleSpacing       = 0.24f;
    constexpr float subtitleSize       = 11.0f;
    constexpr float subtitleSpacing    = 0.06f;
    constexpr int   segHeight          = 30;
    constexpr int   segPadding         = 14;
    constexpr float segTextSize        = 11.0f;
    constexpr int   buttonGap          = 8;
    constexpr bool  semiBold           = true;
    constexpr bool  medium             = false;
    constexpr juce::juce_wchar passwordBullet      = 0x2022;
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
    resized();
    repaint();
}

void LoginComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, bounds.getY(), StagePalette::pageBottom, bounds.getBottom()));
    g.fillRect (bounds);

    const auto layout = FrontScreensLayout::login (bounds, scale, { signUpMode, false, false });
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
}

void LoginComponent::resized()
{
    const auto layout = FrontScreensLayout::login (getLocalBounds().toFloat(), scale, { signUpMode, false, false });
    usernameLabel.setBounds (layout.usernameLabel.toNearestInt());
    usernameField.setBounds (layout.usernameField.toNearestInt());
    emailLabel.setBounds (layout.emailLabel.toNearestInt());
    emailField.setBounds (layout.emailField.toNearestInt());
    passwordLabel.setBounds (layout.passwordLabel.toNearestInt());
    passwordField.setBounds (layout.passwordField.toNearestInt());
    showButton.setBounds (layout.showButton.toNearestInt());

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
    if (onSuccessfullLogin)
        onSuccessfullLogin();
    return;

    auto email = emailField.getText().trim();
    auto pass = passwordField.getText().trim();

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

                        if (safeThis == nullptr)
                            return;
                        safeThis->emailField.clear();
                        safeThis->passwordField.clear();

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
    if (! signUpMode)
        setSignUpMode (true);
}

void LoginComponent::handleForgotPassword()
{
    auto email = emailField.getText();

    if (email.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            Text::forgotTitle,
            Text::enterEmailFirst);
        return;
    }


    email = emailField.getText();

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
    auto email = emailField.getText().trim();
    auto pass = passwordField.getText().trim();
    auto username = usernameField.getText().trim();

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

                    
                    if (safeThis != nullptr)
                        safeThis->setSignUpMode (false);   // back to Log in, the fields cleared
                });

        }).detach();
}

void LoginComponent::handleBack()
{
    setSignUpMode (false);
}
