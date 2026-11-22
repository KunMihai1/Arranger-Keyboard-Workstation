#include "OverlayComponent.h"

namespace OverlayText
{
    const juce::String settings = "Settings";
    const juce::String exit     = "Exit";
    const juce::String effects  = "Effects";
    constexpr float dimAlpha        = 0.6f;
    constexpr int   centringDivisor = 2;
    constexpr int   panelPadding    = 30;
}

OverlayComponent::OverlayComponent()
{
    setVisible(false);
    setOpaque(false);

    setWantsKeyboardFocus(true);

    addAndMakeVisible(menuPanel);
    menuPanel.setInterceptsMouseClicks(true, true);

    menuPanel.addAndMakeVisible(settingsButton);
    menuPanel.addAndMakeVisible(effectsButton);
    menuPanel.addAndMakeVisible(exitButton);

    settingsButton.setButtonText(OverlayText::settings);
    settingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    settingsButton.onClick = [this]() { 
        if (onSettingsClick)
            onSettingsClick();
    };

    exitButton.setButtonText(OverlayText::exit);
    exitButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    exitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exitButton.onClick = [this]()
    {

        // Guarded: a headless console test binary has no JUCEApplication instance (getInstance()
        // returns nullptr), so calling through it unconditionally would crash an Overlay test.
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    };

    effectsButton.setButtonText(OverlayText::effects);
    effectsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    effectsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    effectsButton.onClick = [this]()
    {
        if (onEffectsClick)
            onEffectsClick();
    };
}

void OverlayComponent::paint(juce::Graphics& g)
{

    g.fillAll(juce::Colours::black.withAlpha(OverlayText::dimAlpha));
}

void OverlayComponent::resized()
{
    auto area = getLocalBounds();
    const int panelWidth = 300;
    const int panelHeight = 300;
    menuPanel.setBounds(area.getCentreX() - panelWidth / OverlayText::centringDivisor,
        area.getCentreY() - panelHeight / OverlayText::centringDivisor,
        panelWidth, panelHeight);

    auto panelArea = menuPanel.getLocalBounds().reduced(OverlayText::panelPadding);
    const int buttonHeight = 40;
    const int spacing = 20;

    settingsButton.setBounds(panelArea.removeFromTop(buttonHeight));
    panelArea.removeFromTop(spacing);

    effectsButton.setBounds(panelArea.removeFromTop(buttonHeight));
    panelArea.removeFromTop(spacing);

    exitButton.setBounds(panelArea.removeFromTop(buttonHeight));
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

juce::TextButton& OverlayComponent::getExitButton()
{
    return this->exitButton;
}

juce::TextButton& OverlayComponent::getSettingsButton()
{
    return this->settingsButton;
}

juce::TextButton& OverlayComponent::getEffectsButton()
{
    return this->effectsButton;
}

void OverlayComponent::mouseDown(const juce::MouseEvent& ev)
{
    if (bringSeparateWindowFront)
        bringSeparateWindowFront();
}


bool OverlayComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
       
        if (onRequestClose)
        {
            onRequestClose();
        }
        else
        {
            if (setWindowFlag)
                setWindowFlag();
            //removeFromDesktop();
            setVisible(false);
        }
        return true;
    }
    return false;
}
