#include "StageHomePage.h"
#include "StageFonts.h"
#include "StageWidgets.h"
#include "FrontScreensLayout.h"
#include "AppColours.h"

namespace
{
    namespace Text
    {
        const juce::String settings     = "Settings";
        const juce::String play         = "Play";
        const juce::String exit         = "Exit";
        const juce::String startPlaying = "Start playing";
        const juce::String signedInAs   = "Signed in as ";
        const juce::String logOut       = "Log out";
    }

    // Board PageHome .big / .big-lit
    namespace KeyLook
    {
        constexpr float cornerRadius   = 5.0f;
        constexpr float borderWidth    = 1.0f;
        constexpr float dropDepth      = 4.0f;   // box-shadow 0 4px 0 (the key's side); 2 px when pressed
        constexpr float pressedDrop    = 2.0f;
        constexpr float shadowOffset   = 6.0f;   // 0 6px 10px
        constexpr int   shadowBlur     = 10;
        constexpr float shineHeight    = 1.0f;
        constexpr float keyTextSize    = 15.0f;
        constexpr float playTextSize   = 22.0f;
        constexpr float keyTextSpacing = 0.2f;
        constexpr bool  semiBold       = true;
        constexpr float halfLine       = 0.5f;   // a stroke centred half a line inside the face
    }

    constexpr float labelSize    = 11.0f;   // "Start playing"
    constexpr float labelSpacing = 0.14f;

    // Board PageHome: the signed-in row (.silk, text-transform none, .06em, dim) and its .small Log out button.
    constexpr float signedInSpacing = 0.06f;
    constexpr int   signedInGap     = 12;
    constexpr int   logOutHeight    = 24;
    constexpr int   logOutPadding   = 12;
    constexpr int   signedInLines   = 1;   // a long email shrinks, then ends in an ellipsis
}

//==============================================================================
StageBigKey::StageBigKey (const juce::String& text, bool isPlayKey) : juce::Button (text), isPlay (isPlayKey)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void StageBigKey::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

void StageBigKey::paintButton (juce::Graphics& g, bool, bool isDown)
{
    using namespace KeyLook;
    auto scaled = [this] (float boardLength) { return boardLength * scale; };
    const auto drop = scaled (isDown ? pressedDrop : dropDepth);
    // The key travels down by the drop it loses when pressed (.big:active translateY 2px).
    const auto face = getLocalBounds().toFloat().withTrimmedBottom (scaled (dropDepth))
                                     .translated (float {}, isDown ? scaled (dropDepth - pressedDrop) : float {});
    const auto radius = scaled (cornerRadius);

    juce::Path facePath;
    facePath.addRoundedRectangle (face, radius);

    juce::DropShadow (StagePalette::bigKeyShadow, juce::roundToInt (scaled (static_cast<float> (shadowBlur))),
                      { int {}, juce::roundToInt (scaled (shadowOffset)) }).drawForPath (g, facePath);
    g.setColour (StagePalette::bigKeyDrop);
    g.fillRoundedRectangle (face.translated (float {}, drop), radius);

    g.setGradientFill (juce::ColourGradient::vertical (isPlay ? StagePalette::playKeyTop : StagePalette::bigKeyTop, face.getY(),
                                                       isPlay ? StagePalette::playKeyBottom : StagePalette::bigKeyBottom, face.getBottom()));
    g.fillPath (facePath);
    {
        juce::Graphics::ScopedSaveState insideTheFace (g);
        g.reduceClipRegion (facePath);
        g.setColour (StagePalette::bigKeyShine);
        g.fillRect (face.withHeight (scaled (shineHeight)));
    }
    g.setColour (isPlay ? StagePalette::playKeyBorder : StagePalette::bigKeyBorder);
    g.drawRoundedRectangle (face.reduced (scaled (borderWidth) * halfLine), radius, scaled (borderWidth));

    g.setColour (isPlay ? StagePalette::playKeyText : StagePalette::bigKeyText);
    g.setFont (StageFonts::silkscreen (scaled (isPlay ? playTextSize : keyTextSize), semiBold, keyTextSpacing));
    g.drawText (getButtonText().toUpperCase(), face, juce::Justification::centred, false);
}

//==============================================================================
StageHomePage::StageHomePage() : settingsKey (Text::settings, false), playKey (Text::play, true), exitKey (Text::exit, false),
                                 logOutButton (Text::logOut, SegButton::Style::seg)
{
    logOutButton.setBoardHeight (logOutHeight);
    logOutButton.setPaddingSide (logOutPadding);
    logOutButton.onClick = [this] { if (onLogOut) onLogOut(); };
    addAndMakeVisible (logOutButton);   // Home is only reached signed in, so Log out is always there
    settingsKey.onClick = [this] { if (onSettings) onSettings(); };
    playKey.onClick     = [this] { if (onPlay) onPlay(); };
    exitKey.onClick     = [this] { if (onExit) onExit(); };
    for (auto* key : { &settingsKey, &playKey, &exitKey })
        addAndMakeVisible (*key);
    setOpaque (true);
}

void StageHomePage::setScale (float newScale)
{
    scale = newScale;
    for (auto* key : { &settingsKey, &playKey, &exitKey })
        key->setScale (scale);
    logOutButton.setScale (scale);
    resized();
    repaint();
}

void StageHomePage::resized()
{
    // The key's drop (its side) hangs under the board's key box, so the component reaches that much lower.
    const auto layout = FrontScreensLayout::home (getLocalBounds().toFloat(), scale);
    const auto drop = KeyLook::dropDepth * scale;
    settingsKey.setBounds (layout.settingsKey.withTrimmedBottom (-drop).toNearestInt());
    playKey.setBounds (layout.playKey.withTrimmedBottom (-drop).toNearestInt());
    exitKey.setBounds (layout.exitKey.withTrimmedBottom (-drop).toNearestInt());

    logOutButton.setBounds (layout.signedInRow.withLeft (layout.signedInRow.getRight() - logOutButton.getExactWidth()).toNearestInt());
}

void StageHomePage::setSignedInAs (const juce::String& email)
{
    signedInEmail = email;
    resized();
    repaint();
}

void StageHomePage::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, bounds.getY(), StagePalette::pageBottom, bounds.getBottom()));
    g.fillRect (bounds);

    const auto layout = FrontScreensLayout::home (bounds, scale);
    StageDrawing::drawLed (g, layout.settingsLed, false, StagePalette::ledGreen, scale);
    StageDrawing::drawLed (g, layout.playLed, true, StagePalette::ledGreen, scale);
    StageDrawing::drawLed (g, layout.exitLed, false, StagePalette::ledGreen, scale);

    g.setColour (StagePalette::silkscreenText);
    g.setFont (StageFonts::silkscreen (labelSize * scale, KeyLook::semiBold, labelSpacing));
    g.drawText (Text::startPlaying.toUpperCase(), layout.playLabel, juce::Justification::centred, false);

    if (signedInEmail.isNotEmpty())
    {
        const auto textArea = layout.signedInRow.withTrimmedRight (logOutButton.getExactWidth() + static_cast<float> (signedInGap) * scale);
        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (StageFonts::silkscreen (labelSize * scale, KeyLook::semiBold, signedInSpacing));
        g.drawFittedText (Text::signedInAs + signedInEmail, textArea.toNearestInt(), juce::Justification::centredRight, signedInLines);
    }
}
