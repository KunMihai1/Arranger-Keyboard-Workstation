#pragma once
#include <JuceHeader.h>
#include <functional>
#include "UI/Stage/StagePageWidgets.h"

namespace StyleListText
{
    inline const juce::String heading      = "Configurations";
    inline const juce::String listName     = "styles";
    inline const juce::String newButton    = "New";
    inline const juce::String editButton   = "Edit";
    inline const juce::String loadButton   = "Load";
    inline const juce::String deleteButton = "Del";
}

/** The Sections page's configurations (board PageSections, left column): the *.style files of the
    ArrangerStyles folder in an LCD list, the active one marked; New / Edit / Load / Del. Load (or a
    double-click) makes the selected one the style's configuration; Del asks first. */
class ArrangerStyleListComponent : public juce::Component,
                                   private juce::ListBoxModel
{
public:
    std::function<void (const juce::File&)> onLoadStyle;
    std::function<void (const juce::File&)> onEditStyle;
    std::function<void (const juce::File&)> onDeleteStyle;   // fired after a config file is deleted
    std::function<void()> onCreateNew;

    ArrangerStyleListComponent();
    ~ArrangerStyleListComponent() override;
    void refresh();                 // rescan the folder
    /** Mark the configuration that is currently active for the style (● at its right). Pass an empty
        string to clear the marker. */
    void setActiveConfigName (const juce::String& name);
    void setScale (float newScale);

    /** The selected row's file, or {} (for tests and the page). */
    juce::File getSelectedFile() const;
    void selectFile (const juce::File& file);
    int getNumFiles() const { return files.size(); }
    SegButton& getButton (const juce::String& text);

    void resized() override;
    void paint (juce::Graphics& g) override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int rowWidth, int rowHeight, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

private:
    bool isRow (int row) const { return row >= int {} && row < files.size(); }

    SilkLabel heading { StyleListText::heading, SilkLabel::Tone::bright };
    juce::ListBox list { StyleListText::listName, this };
    SegButton newBtn { StyleListText::newButton, SegButton::Style::seg },
              editBtn { StyleListText::editButton, SegButton::Style::seg },
              loadBtn { StyleListText::loadButton, SegButton::Style::seg },
              deleteBtn { StyleListText::deleteButton, SegButton::Style::seg };
    juce::Array<juce::File> files;
    juce::String activeName;        // configuration currently active for the style (marked in the list)
    float scale { StageMetrics::referenceScale };
};
