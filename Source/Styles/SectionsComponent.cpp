/*
  ==============================================================================

    SectionsComponent.cpp
    Created: 30 Nov 2025 10:08:19pm
    Author:  Kisuke

  ==============================================================================
*/

#include "SectionsComponent.h"

namespace
{
    constexpr int   oneGroupMinimum = 1;     // avoid dividing by zero before any group exists
    constexpr int   groupGap        = 5;
    constexpr float idleButtonAlpha = 0.85f;
}

StyleSectionComponent::StyleSectionComponent(const std::vector<juce::String>& names, const std::vector<std::vector<juce::String>>& groupNames,
    const std::unordered_map<juce::String, std::function<void()>>& buttonCallbacks): callbacks{buttonCallbacks}
{
    for (int groupIndex {}; groupIndex < names.size(); groupIndex++)
    {
        bool toggleNeeded = false;
        if (names[groupIndex].equalsIgnoreCase(SectionGroupNames::fills))
            toggleNeeded = true;
        auto group = std::make_unique<SectionGroupComponent>(names[groupIndex], groupNames[groupIndex],toggleNeeded);
        addAndMakeVisible(group.get());
        sectionGroups.add(std::move(group));
    }

    assignCallBacks();
}

void StyleSectionComponent::resized()
{
    auto area = getLocalBounds();
    int groupWidth = area.getWidth() / (sectionGroups.size() > int{} ? sectionGroups.size() : oneGroupMinimum);

    for (auto* group : sectionGroups)
    {
        group -> setBounds(area.removeFromLeft(groupWidth).reduced(groupGap));
    }
}

void StyleSectionComponent::applyChangeColour(juce::TextButton& button, bool activated)
{
    if (activated)
    {
        button.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    }
    else
    {
        button.setColour(juce::TextButton::buttonColourId, AppColours::accent3.withAlpha(idleButtonAlpha));
    }

    button.repaint();
}

juce::TextButton* StyleSectionComponent::getLastUsedButton()
{
    return lastClickedButton;
}

void StyleSectionComponent::deactivateLastClicked()
{
    if (lastClickedButton && lastActivationMap)
    {
        auto btnText = lastClickedButton->getButtonText();

        bool oldState = false;
        auto it = lastActivationMap->find(btnText);
        if (it != lastActivationMap->end())
            oldState = it->second;

        applyChangeColour(*lastClickedButton, oldState);
        (*lastActivationMap)[btnText] = !oldState;
        lastClickedButton = nullptr;
        lastActivationMap = nullptr;
    }
}

juce::OwnedArray<SectionGroupComponent>& StyleSectionComponent::getSectionGroups()
{
    return this->sectionGroups;
}

void StyleSectionComponent::setHighlightedButton(const juce::String& buttonName)
{
    juce::TextButton*                       found    = nullptr;
    std::unordered_map<juce::String, bool>* foundMap = nullptr;

    for (auto* group : sectionGroups)
    {
        auto& activationMap = group->getActivationMap();
        for (auto& button : group->getButtons())
        {
            const bool match = buttonName.isNotEmpty()
                            && button->getButtonText().equalsIgnoreCase(buttonName);

            // applyChangeColour(btn, activated): activated==true -> accent2 (idle), false -> accent3
            // (the pressed/active look). So pass !match to render the active button as accent3.
            applyChangeColour(*button, ! match);
            activationMap[button->getButtonText()] = match;

            if (match) { found = button.get(); foundMap = &activationMap; }
        }
    }

    lastClickedButton = found;       // keep the click-system's "current" pointer consistent
    lastActivationMap = foundMap;
}

void StyleSectionComponent::assignCallBacks()
{
    for (auto* group : sectionGroups)
    {
        auto& buttons = group->getButtons();
        for (auto& button : buttons)
        {
            juce::String buttonText = button->getButtonText();
            auto it = callbacks.find(buttonText);

            if (it != callbacks.end())
            {
                auto callbackCopy = it->second;
                auto btnPtr = button.get();
                auto& activationMap = group->getActivationMap();
                button->onClick = [callbackCopy, buttonText, this, btnPtr, &activationMap]()
                {
                    // Arranger mode: don't self-highlight on click. Just route to the engine; the
                    // engine's active-section callback drives the highlight so it always matches what
                    // is actually sounding (e.g. Var 2 click -> Fill lights, then Var 2 when it lands).
                    if (engineDrivenHighlight)
                    {
                        callbackCopy();
                        return;
                    }

                    if (undoLastClickedForOtherGroupSections)
                        undoLastClickedForOtherGroupSections();
                    if (lastClickedButton && lastActivationMap && lastClickedButton!=btnPtr)
                    {
                        bool activatedStateLast = false;
                        auto itLast = lastActivationMap->find(lastClickedButton->getButtonText());
                        if (itLast != lastActivationMap->end())
                        {
                            activatedStateLast = itLast->second;
                            applyChangeColour(*lastClickedButton, activatedStateLast);
                            (*lastActivationMap)[lastClickedButton->getButtonText()] = !activatedStateLast;
                        }
                    }
                    

                    bool activatedState = false;
                    auto itAct = activationMap.find(btnPtr->getButtonText());
                    if (itAct != activationMap.end())
                        activatedState = itAct->second;

                    bool newState = !activatedState;
                    
                    applyChangeColour(*btnPtr, activatedState);

                    activationMap[buttonText] = newState;
                    
                    if (newState)
                    {
                        lastClickedButton = btnPtr;
                        lastActivationMap = &activationMap;
                    }
                    else {
                        lastClickedButton = nullptr;
                        lastActivationMap = nullptr;
                    }

                    callbackCopy();
                };
            }
        }
    }
}
