/*
  ==============================================================================

    addDeviceWindow.cpp
    Created: 29 Oct 2025 11:46:49pm
    Author:  Kisuke

  ==============================================================================
*/

#include "addDeviceWindow.h"

namespace
{
    namespace Text
    {
        const juce::String windowTitle     = "Add MIDI Device";
        const juce::String namePlaceholder = "Device name";
        const juce::String keysPlaceholder = "# of keys";
        const juce::String addButton       = "Add";
        const juce::String clickLogged     = "on click called here";
        const juce::String addedTitle      = "Successfull adding";
        const juce::String addedStart      = "Your device named: ";
        const juce::String addedWith       = " with: ";
        const juce::String addedEnd        = " keys has been added sucesffuly!";
        const juce::String invalidTitle    = "Invalid Input";
        const juce::String invalidMessage  = "Please enter a valid device name and number of keys.";
    }

    constexpr int fieldLeft     = 20;
    constexpr int nameTop       = 20;
    constexpr int keysTop       = 60;
    constexpr int buttonTop     = 100;
    constexpr int fieldWidth    = 200;
    constexpr int buttonWidth   = 100;
    constexpr int rowHeight     = 24;
    constexpr int windowWidth   = 300;
    constexpr int windowHeight  = 200;
    constexpr int noKeys        = 0;
}

AddDeviceWindow::AddDeviceWindow(const juce::String& vid, const juce::String& pid, int testing)
    : DialogWindow(Text::windowTitle, juce::Colours::darkgrey, true),
    vidString{ vid }, pidString{ pid }
{
    setContentOwned(new juce::Component(), true); // create content component
    auto* content = getContentComponent();

    nameEditor = std::make_unique<juce::TextEditor>();
    nameEditor->setText(Text::namePlaceholder);
    content->addAndMakeVisible(nameEditor.get());
    nameEditor->setBounds(fieldLeft, nameTop, fieldWidth, rowHeight);

    keysEditor = std::make_unique<juce::TextEditor>();
    keysEditor->setText(Text::keysPlaceholder);
    content->addAndMakeVisible(keysEditor.get());
    keysEditor->setBounds(fieldLeft, keysTop, fieldWidth, rowHeight);

    addButton = std::make_unique<juce::TextButton>(Text::addButton);
    content->addAndMakeVisible(addButton.get());
    addButton->setBounds(fieldLeft, buttonTop, buttonWidth, rowHeight);

    addButton->onClick = [this, vid, pid,testing]()
    {
        DBG(Text::clickLogged);
        juce::String name = nameEditor->getText();
        int keys = keysEditor->getText().getIntValue();
        if (!name.isEmpty() && keys > noKeys)
        {
            if (onAddDevice)
                onAddDevice(name, keys);
            if (!testing)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    Text::addedTitle,
                    Text::addedStart + name + Text::addedWith + juce::String(keys) + Text::addedEnd
                );
            }
            

            closeButtonPressed();
        }
        else
        {
            if (!testing)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    Text::invalidTitle,
                    Text::invalidMessage
                );
            }
        }
    };

    setSize(windowWidth, windowHeight);
    centreWithSize(windowWidth, windowHeight);

}

void AddDeviceWindow::closeButtonPressed()
{
    setVisible(false);
}

juce::TextButton& AddDeviceWindow::getAddButton() {
    
    return *(this->addButton);
}

juce::TextEditor& AddDeviceWindow::getNameEditor()
{
    return *(this->nameEditor);
}

juce::TextEditor& AddDeviceWindow::getKeysEditor()
{
    return *(this->keysEditor);
}