#include "FrontScreensLayout.h"

namespace
{
    constexpr float half = 0.5f;
    constexpr float cssLineHeight = 1.2f;   // CSS "normal" line height for the silkscreen font

    // PageHome: .key columns (LED, gap 10, .big 150 x 84; Play .big-lit 220 x 110 plus "Start playing"), gap 56.
    namespace HomeBoard
    {
        constexpr float ledSize       = 7.0f;
        constexpr float columnGap     = 10.0f;
        constexpr float keyWidth      = 150.0f;
        constexpr float keyHeight     = 84.0f;
        constexpr float playWidth     = 220.0f;
        constexpr float playHeight    = 110.0f;
        constexpr float labelSize     = 11.0f;
        constexpr float keysGap       = 56.0f;
        constexpr float signedInRight = 28.0f;
        constexpr float signedInTop   = 20.0f;
        constexpr float signedInWidth = 420.0f;
        constexpr float signedInHeight = 24.0f;   // the .small Log out button
        constexpr float boardHalfWidth = 600.0f;  // the board is 1200 wide, centred in the page area
    }

    // PageLogin: a 300 wide title column, gap 60, a 400 wide form with rows 9 apart.
    namespace LoginBoard
    {
        constexpr float titleColumnWidth = 300.0f;
        constexpr float columnsGap       = 60.0f;
        constexpr float formWidth        = 400.0f;
        constexpr float titleSize        = 22.0f;
        constexpr float titleLineHeight  = 1.3f;
        constexpr float titleLines       = 2.0f;   // "Arranger" / "Workstation"
        constexpr float titleGap         = 10.0f;
        constexpr float ruleHeight       = 1.0f;
        constexpr float subtitleSize     = 11.0f;
        constexpr float rowGap           = 9.0f;
        constexpr float labelSize        = 10.0f;
        constexpr float fieldHeight      = 30.0f;
        constexpr float rememberedSize   = 11.0f;
        constexpr float rememberedLift   = 3.0f;   // margin-top: -3px
        constexpr float showWidth        = 74.0f;
        constexpr float showGap          = 6.0f;
        constexpr float rememberMeHeight = 26.0f;
        constexpr float buttonsLift      = 8.0f;   // margin-top: 8px
    }
}

namespace FrontScreensLayout
{
    Home home (juce::Rectangle<float> pageArea, float scale)
    {
        using namespace HomeBoard;
        auto scaled = [scale] (float boardLength) { return boardLength * scale; };
        const auto centreY = pageArea.getCentreY();
        const auto rowWidth = scaled (keyWidth + keysGap + playWidth + keysGap + keyWidth);
        auto left = pageArea.getCentreX() - rowWidth * half;

        auto column = [&] (float width, float keyTall, bool withLabel, juce::Rectangle<float>& led, juce::Rectangle<float>& key,
                           juce::Rectangle<float>* label)
        {
            const auto labelHeight = withLabel ? scaled (columnGap + labelSize * cssLineHeight) : float {};
            const auto height = scaled (ledSize + columnGap + keyTall) + labelHeight;
            const auto top = centreY - height * half;
            const auto centreX = left + scaled (width) * half;
            led = juce::Rectangle<float> (scaled (ledSize), scaled (ledSize)).withCentre ({ centreX, top + scaled (ledSize) * half });
            key = { left, top + scaled (ledSize + columnGap), scaled (width), scaled (keyTall) };
            if (label != nullptr)
                *label = { left, key.getBottom() + scaled (columnGap), scaled (width), scaled (labelSize * cssLineHeight) };
            left += scaled (width + keysGap);
        };

        Home layout;
        column (keyWidth, keyHeight, false, layout.settingsLed, layout.settingsKey, nullptr);
        column (playWidth, playHeight, true, layout.playLed, layout.playKey, &layout.playLabel);
        column (keyWidth, keyHeight, false, layout.exitLed, layout.exitKey, nullptr);
        layout.signedInRow = { pageArea.getCentreX() + scaled (boardHalfWidth) - scaled (signedInRight) - scaled (signedInWidth),
                               pageArea.getY() + scaled (signedInTop), scaled (signedInWidth), scaled (signedInHeight) };
        return layout;
    }

    Login login (juce::Rectangle<float> pageArea, float scale, LoginOptions options)
    {
        using namespace LoginBoard;
        auto scaled = [scale] (float boardLength) { return boardLength * scale; };
        const auto centreY = pageArea.getCentreY();
        const auto left = pageArea.getCentreX() - scaled (titleColumnWidth + columnsGap + formWidth) * half;
        const auto labelHeight = labelSize * cssLineHeight;

        Login layout;

        // Title column, centred vertically.
        const auto titleHeight = titleSize * titleLineHeight * titleLines;
        const auto subtitleHeight = subtitleSize * cssLineHeight;
        const auto columnHeight = scaled (titleHeight + titleGap + ruleHeight + titleGap + subtitleHeight);
        auto top = centreY - columnHeight * half;
        layout.title    = { left, top, scaled (titleColumnWidth), scaled (titleHeight) };
        layout.rule     = { left, layout.title.getBottom() + scaled (titleGap), scaled (titleColumnWidth), scaled (ruleHeight) };
        layout.subtitle = { left, layout.rule.getBottom() + scaled (titleGap), scaled (titleColumnWidth), scaled (subtitleHeight) };

        // Form, centred vertically: every row 9 apart.
        auto formHeight = labelHeight + rowGap + fieldHeight + rowGap      // email
                        + labelHeight + rowGap + fieldHeight + rowGap      // password
                        + buttonsLift + fieldHeight;
        if (options.signUp)
            formHeight += labelHeight + rowGap + fieldHeight + rowGap;
        if (options.showRemembered)
            formHeight += rememberedSize * cssLineHeight - rememberedLift + rowGap;
        if (options.showRememberMe)
            formHeight += rememberMeHeight + rowGap;

        const auto formLeft = left + scaled (titleColumnWidth + columnsGap);
        const auto width = scaled (formWidth);
        top = centreY - scaled (formHeight) * half;
        auto take = [&] (float boardHeight) { const juce::Rectangle<float> row { formLeft, top, width, scaled (boardHeight) };
                                              top += scaled (boardHeight + rowGap); return row; };
        if (options.signUp)
        {
            layout.usernameLabel = take (labelHeight);
            layout.usernameField = take (fieldHeight);
        }
        layout.emailLabel = take (labelHeight);
        layout.emailField = take (fieldHeight);
        if (options.showRemembered)
        {
            top -= scaled (rememberedLift);
            layout.rememberedRow = take (rememberedSize * cssLineHeight);
        }
        layout.passwordLabel = take (labelHeight);
        const auto passwordRow = take (fieldHeight);
        layout.passwordField = passwordRow.withTrimmedRight (scaled (showWidth + showGap));
        layout.showButton = passwordRow.withLeft (passwordRow.getRight() - scaled (showWidth));
        if (options.showRememberMe)
            layout.rememberMe = take (rememberMeHeight);
        top += scaled (buttonsLift);
        layout.buttonRow = { formLeft, top, width, scaled (fieldHeight) };
        return layout;
    }
}
