/*
  ==============================================================================

    ButtonGroupComponent.cpp
    Created: 30 Nov 2025 10:08:03pm
    Author:  Kisuke

  ==============================================================================
*/

#include "SectionGroupComponent.h"

namespace
{
    namespace Text
    {
        const juce::String disableAutoFill = "Disable Auto Fill";
        const juce::String enableAutoFill  = "Enable Auto Fill";
    }

    constexpr int titleExtraHeight = 2;
    constexpr int titleGap         = 5;
    constexpr int minButtonWidth   = 35;
    constexpr int centringDivisor  = 2;
    constexpr int tooltipLift      = 20;   // tooltip sits this far above the toggle's centre
}

SectionGroupComponent::SectionGroupComponent(const juce::String& title, const std::vector<juce::String>& buttonNames, bool toggleButtonBool) : titleLabel{ title,title }
{
    addAndMakeVisible(titleLabel);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, AppColours::subtitleText);

    for (auto& name : buttonNames)
    {
        auto btn = std::make_unique<juce::TextButton>(name);
        btn->setButtonText(name);
        btn->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        btn->setMouseClickGrabsKeyboardFocus(false);   // keep PC-keyboard focus when clicking sections
        activationMap[name] = false;

        btn->setLookAndFeel(&laf);
        btn->setColour(juce::TextButton::buttonColourId, AppColours::accent2);

        addAndMakeVisible(*btn);
        buttons.push_back(std::move(btn));
    }

    if (toggleButtonBool)
    {
        toggleButton = std::make_unique<juce::ToggleButton>();
        toggleButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        toggleButton->setMouseClickGrabsKeyboardFocus(false);
        toggleButton->setToggleState(false, juce::dontSendNotification);

        toggleButton->setColour(juce::ToggleButton::tickColourId, juce::Colours::green);      
        toggleButton->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);

        toggleButton->onClick = [this]()
        {
            if (toggleToolTip->isVisible())
            {
                if (toggleButton->getToggleState())
                    toggleToolTip->setNewText(Text::disableAutoFill);
                else
                    toggleToolTip->setNewText(Text::enableAutoFill);
            }

            if (onToggleChanged)
                onToggleChanged(toggleButton->getToggleState());
        };

        addAndMakeVisible(toggleButton.get());

        toggleToolTip = std::make_unique<CustomToolTip>(Text::enableAutoFill);
        addAndMakeVisible(toggleToolTip.get());
        toggleToolTip->setVisible(false);

        toggleButton->addMouseListener(this, false);
        
    }

    if (title.equalsIgnoreCase(SectionGroupNames::variations))
    {
        auto btn = std::make_unique<juce::TextButton>(SectionGroupNames::breakButton);
        btn->setButtonText(SectionGroupNames::breakButton);
        btn->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        btn->setMouseClickGrabsKeyboardFocus(false);
        btn->setLookAndFeel(&laf);
        btn->setColour(juce::TextButton::buttonColourId, AppColours::accent2);
        addAndMakeVisible(*btn);
        buttons.push_back(std::move(btn));
    }
}

void SectionGroupComponent::resized()
{

    int labelHeight = (int)titleLabel.getFont().getHeight() + titleExtraHeight;
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(labelHeight));

    area.removeFromTop(titleGap);

    int count = (int)buttons.size();
    if (count == int{})
        return;
    
    int toDivideWith = count;
    if (titleLabel.getText().equalsIgnoreCase(SectionGroupNames::fills))
        toDivideWith++;

    auto row = area;
    int width = juce::jmax(minButtonWidth, row.getWidth() / toDivideWith);

    for (int buttonIndex {}; buttonIndex < count; buttonIndex++)
    {
        buttons[buttonIndex]->setBounds(row.removeFromLeft(width));
    }

    if (titleLabel.getText().equalsIgnoreCase(SectionGroupNames::fills))
        toggleButton->setBounds(row.removeFromLeft(width));

}

void SectionGroupComponent::mouseEnter(const juce::MouseEvent& event)
{
    if (event.eventComponent == toggleButton.get())
    {
        auto bounds = toggleButton->getBounds();

        int tooltipX = bounds.getX() - toggleToolTip->getWidth(); 
        int tooltipY = bounds.getY() + (bounds.getHeight() - toggleToolTip->getHeight()) / centringDivisor - tooltipLift; 

        toggleToolTip->setTopLeftPosition(tooltipX, tooltipY);
        

        toggleToolTip->setVisible(true);
        toggleToolTip->toFront(true);
    }
}

void SectionGroupComponent::mouseExit(const juce::MouseEvent& event)
{
    if (event.eventComponent == toggleButton.get())
    {
        toggleToolTip->setVisible(false);
    }
}

SectionGroupComponent::~SectionGroupComponent()
{
    if (toggleButton)
        toggleButton->removeMouseListener(this);
}

std::vector<std::unique_ptr<juce::TextButton>>& SectionGroupComponent::getButtons()
{
    return buttons;
}

juce::String SectionGroupComponent::getGroupName()
{
    return titleLabel.getText();
}

std::unordered_map<juce::String, bool>& SectionGroupComponent::getActivationMap()
{
    return this->activationMap;
}

CustomToolTip* SectionGroupComponent::getToggleToolTip()
{
    return this->toggleToolTip.get();
}

juce::ToggleButton* SectionGroupComponent::getToggleButton()
{
    return this->toggleButton.get();
}
