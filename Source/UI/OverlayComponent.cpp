#include "OverlayComponent.h"

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

    settingsButton.setButtonText("Settings");
    settingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    settingsButton.onClick = [this]() { 
        if (onSettingsClick)
            onSettingsClick();
    };

    exitButton.setButtonText("Exit");
    exitButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    exitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exitButton.onClick = [this]()
    {

        // Guarded: a headless console test binary has no JUCEApplication instance (getInstance()
        // returns nullptr), so calling through it unconditionally would crash an Overlay test.
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    };

    effectsButton.setButtonText("Effects");
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

    g.fillAll(juce::Colours::black.withAlpha(0.6f));
}

void OverlayComponent::resized()
{
    auto area = getLocalBounds();
    const int panelWidth = 300;
    const int panelHeight = 300;
    menuPanel.setBounds(area.getCentreX() - panelWidth / 2,
        area.getCentreY() - panelHeight / 2,
        panelWidth, panelHeight);

    auto panelArea = menuPanel.getLocalBounds().reduced(30);
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
