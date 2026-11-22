#pragma once

#include <JuceHeader.h>
#include "SFZlibrary.h"

namespace SfzLibraryText
{
    inline const juce::String addFile       = "Add SFZ File";
    inline const juce::String removeFile    = "Remove File";
    inline const juce::String assignToStyle = "Assign to Style";
    inline const juce::String importConfig  = "Import Config";
    inline const juce::String close         = "X";
}
#include "InstrumentChooser.h"

class SFZLibraryUI : public juce::Component, public juce::ListBoxModel
{
public:
    SFZLibraryUI(SFZLibraryManager& managerRef);
    ~SFZLibraryUI() override;

    std::function<void()> onClose;
    std::function<void()> onLibraryChanged;
    std::function<juce::String()> getCurrentStyleId;
    std::function<juce::String()> getCurrentStyleName;
    std::function<juce::String(const juce::String&)> getStyleNameForId; // optional: id → display name

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;

private:
    void addFileButtonClicked();
    void removeFileButtonClicked();
    void importConfigButtonClicked();
    void updateStatusLabel();
    void buildInstrumentList();

    SFZLibraryManager& manager;
    juce::ListBox libraryList;
    juce::TextButton addFileButton    { SfzLibraryText::addFile };
    juce::TextButton removeFileButton { SfzLibraryText::removeFile };
    juce::TextButton assignButton     { SfzLibraryText::assignToStyle };
    juce::TextButton importConfigButton { SfzLibraryText::importConfig };
    juce::TextButton closeButton      { SfzLibraryText::close };
    juce::Label statusLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::StringArray instrumentList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SFZLibraryUI)
};
