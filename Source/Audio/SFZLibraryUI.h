#pragma once

#include <JuceHeader.h>
#include "SFZlibrary.h"
#include "UI/Stage/StagePageWidgets.h"

/*  The Voices page's SFZ library column (board StageRevised openPage=voices): the library as an LCD list (file name,
    and what it plays in the open style), Load SFZ... / Assign to style track... / Import mappings..., and Remove on
    a row's right-click menu. */

namespace SfzLibraryText
{
    inline const char* const loadFileUtf8     = "Load SFZ\xe2\x80\xa6";
    inline const char* const assignToStyleUtf8 = "Assign to style track\xe2\x80\xa6";
    inline const char* const importConfigUtf8 = "Import mappings\xe2\x80\xa6";
    inline const juce::String removeFile      = "Remove";
}
#include "InstrumentChooser.h"

/** What an entry plays in a style: the General MIDI instrument it is assigned to, "N instruments" for several,
    nothing when it is not assigned there. */
juce::String sfzAssignmentText (const SFZLibraryData& data, const juce::String& styleId, const juce::String& entryId);

class SFZLibraryUI : public juce::Component, public juce::ListBoxModel
{
public:
    SFZLibraryUI(SFZLibraryManager& managerRef);
    ~SFZLibraryUI() override;

    std::function<void()> onLibraryChanged;
    std::function<juce::String()> getCurrentStyleId;
    std::function<juce::String()> getCurrentStyleName;
    std::function<juce::String(const juce::String&)> getStyleNameForId; // optional: id → display name

    void setScale (float newScale);
    /** The library or the open style changed elsewhere: rows and assignments are read again. */
    void refresh();

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

private:
    void addFileButtonClicked();
    void assignButtonClicked();
    void removeEntry(int row);
    void importConfigButtonClicked();
    juce::String currentStyleId() const;

    SFZLibraryManager& manager;
    juce::ListBox libraryList;
    SegButton addFileButton      { juce::String::fromUTF8 (SfzLibraryText::loadFileUtf8), SegButton::Style::seg };
    SegButton assignButton       { juce::String::fromUTF8 (SfzLibraryText::assignToStyleUtf8), SegButton::Style::seg };
    SegButton importConfigButton { juce::String::fromUTF8 (SfzLibraryText::importConfigUtf8), SegButton::Style::seg };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::StringArray instrumentList;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SFZLibraryUI)
};
