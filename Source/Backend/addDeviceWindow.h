/*
  ==============================================================================

    addDeviceWindow.h
    Created: 29 Oct 2025 11:46:49pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class AddDeviceWindow: public juce::DialogWindow
{
public:
    std::function<void(const juce::String& name, int keys)> onAddDevice;

    AddDeviceWindow(const juce::String& vid, const juce::String& pid, int testing={});

    void closeButtonPressed() override;

    juce::TextButton& getAddButton();

    juce::TextEditor& getNameEditor();

    juce::TextEditor& getKeysEditor();

private:
    std::unique_ptr<juce::TextEditor> nameEditor=nullptr;
    std::unique_ptr<juce::TextEditor> keysEditor=nullptr;
    std::unique_ptr<juce::TextButton> addButton=nullptr;

    juce::String vidString;
    juce::String pidString;

};
