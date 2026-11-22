#pragma once
#include <JuceHeader.h>
#include <functional>

namespace StyleListText
{
    inline const juce::String title      = "Saved section configurations";
    inline const juce::String listName   = "styles";
    inline const juce::String newButton    = "New";
    inline const juce::String editButton   = "Edit";
    inline const juce::String deleteButton = "Delete";
    inline const juce::String loadButton   = "Load";
    inline const juce::String closeButton  = "Close";
}

/** Lists *.style files from the ArrangerStyles folder; fires callbacks to load or edit. */
class ArrangerStyleListComponent : public juce::Component,
                                   private juce::ListBoxModel
{
public:
    std::function<void (const juce::File&)> onLoadStyle;
    std::function<void (const juce::File&)> onEditStyle;
    std::function<void (const juce::File&)> onDeleteStyle;   // fired after a config file is deleted
    std::function<void()> onCreateNew;
    std::function<void()> onClose;

    ArrangerStyleListComponent();
    void refresh();                 // rescan the folder
    /** Mark the configuration that is currently active for the style (check + "active"
        + subtle highlight). Pass an empty string to clear the marker. */
    void setActiveConfigName (const juce::String& name);
    void resized() override;
    void paint (juce::Graphics& g) override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int rowWidth, int rowHeight, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

private:
    bool isRow (int row) const { return row >= int {} && row < files.size(); }

    juce::Label title { {}, StyleListText::title };
    juce::ListBox list { StyleListText::listName, this };
    juce::TextButton newBtn { StyleListText::newButton }, editBtn { StyleListText::editButton },
                     deleteBtn { StyleListText::deleteButton }, loadBtn { StyleListText::loadButton },
                     closeBtn { StyleListText::closeButton };
    juce::Array<juce::File> files;
    juce::String activeName;        // configuration currently active for the style (marked in the list)
};
