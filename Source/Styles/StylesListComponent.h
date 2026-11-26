#pragma once
#include <JuceHeader.h>
#include "StyleViewComponent.h"
#include "UI/Stage/StagePageWidgets.h"

namespace StylesListText
{
    inline const juce::String heading   = "Styles";
    inline const juce::String add       = "Add";
    inline const juce::String rename    = "Rename";
    inline const juce::String remove    = "Delete";
}

/**
 * @class StylesListComponent
 * @brief A scrollable container managing multiple `StyleViewComponent` instances.
 *
 * This component displays a list of styles in a 2-column grid layout, allows adding,
 * renaming, and removing styles, and handles callbacks for user interactions.
 * It uses a `juce::Viewport` to make the list scrollable.
 */
class StylesListComponent : public juce::Component
{
public:
    //==============================================================================
    /**
     * @brief Constructs a StylesListComponent with initial styles.
     * @param stylesNamesOut Vector of initial style names.
     * @param onStyleClicked Callback when a style is clicked.
     * @param widthSize Optional width size for layout calculations.
     */
    StylesListComponent(std::vector<juce::String> stylesNamesOut,
                        std::function<void(const juce::String&)> onStyleClicked,
                        int widthSize = {});

    /**
     * @brief Callback invoked when a style is renamed.
     * @param oldName Original name of the style.
     * @param newName New name of the style.
     */
    std::function<void(const juce::String& oldName, const juce::String& newName)> onStyleRename;

    /**
     * @brief Callback invoked when a new style is added.
     * @param newName Name of the newly added style.
     */
    std::function<void(const juce::String& newName)> onStyleAdd;

    /**
     * @brief Callback invoked when a style is removed.
     * @param name Name of the removed style.
     */
    std::function<void(const juce::String& name)> onStyleRemove;

    /** A short message for the Stage screen (a refused name, a style added or removed). */
    std::function<void(const juce::String& text)> onInfo;

    /** The in-app name / remove dialog on screen, or nullptr. */
    StageDialog* getOpenDialog() const;

    //==============================================================================
    /** @brief Handles resizing of the component and lays out child components. */
    void resized() override;

    /** @brief Paints the background and control bar of the component. */
    void paint(juce::Graphics& g) override;

    /**
     * @brief Sets the width size used for layout calculations.
     * @param newWidth New width size.
     */
    void setWidthSize(const int newWidth);

    /** @brief Lays out all `StyleViewComponent`s in a grid. */
    void layoutStyles();

    /** @brief Clears and repopulates the list of styles. */
    void repopulate();

    /** @brief Opens a dialog to add a new style. */
    void addNewStyle();

    /**
     * @brief Assigns callbacks for a new `StyleViewComponent`.
     * @param newStyle Pointer to the style component.
     * @param currentName Name of the style.
     */
    void allCallBacks(StyleViewComponent* newStyle, const juce::String& currentName);

    /**
     * @brief Removes a style locally from the list without external prompts.
     * @param name Name of the style to remove.
     */
    void removeStyleLocally(const juce::String& name);

    /**
     * @brief Adds a style locally to the list without external prompts.
     * @param newName Name of the style to add.
     */
    void addStyleLocally(const juce::String& newName);

    /** @brief Rebuilds the internal vector of style names from the current components. */
    void rebuildStyleNames();

    /** The open style's row is marked; Rename and Delete act on it (DEFAULT Style: disabled). */
    void setOpenStyle(const juce::String& name);
    juce::String getOpenStyle() const;
    void setScale(float newScale);

    /** The LCD list (inside this component) and the three buttons under it. */
    juce::Rectangle<int> getListBounds() const;
    SegButton& getAddButton();
    SegButton& getRenameButton();
    SegButton& getDeleteButton();

private:
    //==============================================================================
    /** @brief Populates the container with `StyleViewComponent`s based on `stylesNames`. */
    void populate();
    void refreshRows();
    StyleViewComponent* openRow() const;

    juce::OwnedArray<StyleViewComponent> allStyles;  ///< All style components.
    std::function<void(const juce::String&)> onStyleClicked;  ///< Callback when a style is clicked.
    std::vector<juce::String> stylesNames;  ///< Names of all styles.
    int widthSize;  ///< Width used for layout calculations.

    SegButton addButton    { StylesListText::add,    SegButton::Style::seg };
    SegButton renameButton { StylesListText::rename, SegButton::Style::seg };
    SegButton deleteButton { StylesListText::remove, SegButton::Style::seg };
    juce::String openStyle;
    float scale = StageMetrics::referenceScale;
    juce::Rectangle<int> listBounds;
    std::unique_ptr<juce::Component> styleItemsContainer = nullptr;  ///< Container holding all styles.
    juce::Viewport viewport;  ///< Scrollable viewport for the style list.

    /** One dialog at a time, owned here (a removal deletes the row that asked). Not juce::AlertWindow: building
        one stalled the UI for 0.6 - 1 s. */
    std::unique_ptr<StageDialog> dialog;
    void openDialog(StageDialogOptions options, StageDialog::OnConfirm onConfirm);
    void showInfo(const juce::String& text);

    JUCE_LEAK_DETECTOR(StylesListComponent)
};
