#include "OverlayComponent.h"
#include "Common/AppColours.h"

namespace OverlayNames
{
    const juce::String heading  = "Menu";
    const juce::String settings = "Settings";
    const juce::String exit     = "Exit";
    const juce::String effects  = "Effects";
}

namespace
{
    // Board px at scale 1, in StageDialog's materials: panel, heading, then three full-width buttons.
    constexpr int   panelWidth     = 260;
    constexpr int   panelPadding   = 18;
    constexpr int   headingHeight  = 12;
    constexpr int   headingGap     = 14;
    constexpr int   buttonHeight   = 30;   // the board's tall seg ("Use in slot")
    constexpr int   buttonGap      = 8;
    constexpr float buttonTextSize = 12.0f;
    constexpr float panelCorner    = 4.0f;
    constexpr float backdropAlpha  = 0.55f;
    constexpr float shadowAlpha    = 0.6f;
    constexpr int   shadowRadius   = 18;
    constexpr int   shadowDrop     = 6;
    constexpr int   buttonCount    = static_cast<int> (escMenuButtonCount);
    constexpr int   oneFewer       = 1;
    constexpr int   gapsBetween    = buttonCount - oneFewer;   // gaps only between the buttons
    constexpr int   sides          = 2;
    constexpr size_t settingsSlot  = 0;
    constexpr size_t effectsSlot   = 1;
    constexpr size_t exitSlot      = 2;

    int scaledLength (int boardLength, float scale) { return juce::roundToInt (static_cast<float> (boardLength) * scale); }
}

EscMenuRects escMenuLayout (juce::Rectangle<int> host, float scale)
{
    const auto padding = scaledLength (panelPadding, scale);
    const auto button = scaledLength (buttonHeight, scale);
    const auto gap = scaledLength (buttonGap, scale);
    const auto height = padding * sides + scaledLength (headingHeight, scale) + scaledLength (headingGap, scale)
                      + button * buttonCount + gap * gapsBetween;

    EscMenuRects rects;
    rects.panel = host.withSizeKeepingCentre (scaledLength (panelWidth, scale), height);
    auto area = rects.panel.reduced (padding);
    rects.heading = area.removeFromTop (scaledLength (headingHeight, scale));
    area.removeFromTop (scaledLength (headingGap, scale));
    for (auto& place : rects.buttons)
    {
        place = area.removeFromTop (button);
        area.removeFromTop (gap);
    }
    return rects;
}

OverlayComponent::OverlayComponent()
    : settingsButton (OverlayNames::settings, SegButton::Style::seg),
      effectsButton (OverlayNames::effects, SegButton::Style::seg),
      exitButton (OverlayNames::exit, SegButton::Style::seg)
{
    setVisible(false);
    setOpaque(false);
    setWantsKeyboardFocus(true);

    for (auto* button : { &settingsButton, &effectsButton, &exitButton })
    {
        button->setBoardHeight (buttonHeight);
        button->setTextSize (buttonTextSize);
        button->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (button);
    }

    settingsButton.onClick = [this]()
    {
        if (onSettingsClick)
            onSettingsClick();
    };

    effectsButton.onClick = [this]()
    {
        if (onEffectsClick)
            onEffectsClick();
    };

    exitButton.onClick = []()
    {
        // Guarded: a headless console test binary has no JUCEApplication instance (getInstance()
        // returns nullptr), so calling through it unconditionally would crash an Overlay test.
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    };
}

void OverlayComponent::setScale (float newScale)
{
    scale = newScale;
    for (auto* button : { &settingsButton, &effectsButton, &exitButton })
        button->setScale (scale);
    resized();
    repaint();
}

void OverlayComponent::paint(juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (backdropAlpha));

    const auto rects = escMenuLayout (getLocalBounds(), scale);
    const auto panel = rects.panel.toFloat();
    const auto corner = panelCorner * scale;

    juce::Path outline;
    outline.addRoundedRectangle (panel, corner);
    juce::DropShadow (juce::Colours::black.withAlpha (shadowAlpha), scaledLength (shadowRadius, scale), { int {}, scaledLength (shadowDrop, scale) })
        .drawForPath (g, outline);
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, panel.getY(), StagePalette::pageBottom, panel.getBottom()));
    g.fillPath (outline);
    g.setColour (StagePalette::ruleLine);
    g.drawRoundedRectangle (panel, corner, static_cast<float> (StageMetrics::ruleWidth) * scale);

    StageLcd::paintHeading (g, rects.heading.toFloat(), scale, OverlayNames::heading, {});
}

void OverlayComponent::resized()
{
    const auto rects = escMenuLayout (getLocalBounds(), scale);
    settingsButton.setBounds (rects.buttons[settingsSlot]);
    effectsButton.setBounds (rects.buttons[effectsSlot]);
    exitButton.setBounds (rects.buttons[exitSlot]);
}

void OverlayComponent::showOverlay()
{
    setVisible(true);
    toFront(true);

    if (isShowing() || isOnDesktop())
        grabKeyboardFocus();

    resized();
    repaint();
}

void OverlayComponent::hideOverlay()
{
    setVisible(false);
}

SegButton& OverlayComponent::getExitButton()     { return exitButton; }
SegButton& OverlayComponent::getSettingsButton() { return settingsButton; }
SegButton& OverlayComponent::getEffectsButton()  { return effectsButton; }

void OverlayComponent::mouseDown(const juce::MouseEvent& ev)
{
    clickAt (ev.getPosition());
}

void OverlayComponent::clickAt (juce::Point<int> position)
{
    if (! escMenuLayout (getLocalBounds(), scale).panel.contains (position))
        requestClose();   // a click on the dimmed app closes the menu, as Esc does
}

void OverlayComponent::requestClose()
{
    if (onRequestClose)
    {
        onRequestClose();
    }
    else
    {
        if (setWindowFlag)
            setWindowFlag();
        setVisible(false);
    }
}

bool OverlayComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        requestClose();
        return true;
    }
    return false;
}
