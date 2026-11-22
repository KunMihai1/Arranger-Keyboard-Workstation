/*
  ==============================================================================

    ButtonGroupComponent.h
    Created: 30 Nov 2025 10:08:03pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomToolTip.h"
#include "PlayScreenLookAndFeel.h"

class SectionGroupComponent: public juce::Component {
public:
    /** Fired when the group's toggle (e.g. Auto Fill) changes, with its new state. */
    std::function<void(bool)> onToggleChanged;

    SectionGroupComponent(const juce::String& title, const std::vector<juce::String>& buttonNames, bool toggleButton=false);

    void resized() override;

    ~SectionGroupComponent();

    void mouseEnter(const juce::MouseEvent& event) override;

    void mouseExit(const juce::MouseEvent& event) override;

    std::vector<std::unique_ptr<juce::TextButton>>& getButtons();

    juce::String getGroupName();

    std::unordered_map<juce::String, bool>& getActivationMap();

    CustomToolTip* getToggleToolTip();

    juce::ToggleButton* getToggleButton();

private:
    PlayScreenLookAndFeel laf;
    juce::Label titleLabel;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    std::unique_ptr<juce::ToggleButton> toggleButton;
    std::unique_ptr<CustomToolTip> toggleToolTip;
    std::unordered_map<juce::String, bool> activationMap;

};