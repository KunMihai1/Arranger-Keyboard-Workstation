#pragma once
#include <JuceHeader.h>

/** The style-name dialog shared by StylesListComponent (add) and StyleViewComponent (rename). */
namespace StyleDialogText
{
    inline const juce::String nameDialogTitle  = "Rename track";
    inline const juce::String nameDialogPrompt = "Enter a name for your track";
    inline const juce::String nameEditor       = "nameEditor";
    inline const juce::String nameEditorLabel  = "Style Name:";
    inline const juce::String okButton         = "OK";
    inline const juce::String cancelButton     = "Cancel";

    constexpr int dialogOk     = 1;   // AlertWindow button results
    constexpr int dialogCancel = 0;
}

/**
 * @class StyleViewComponent
 * @brief Represents a single style item in a style list UI.
 *
 * This component displays a style name and allows the user to:
 * - Click to select the style
 * - Rename the style
 * - Delete the style
 *
 * Supports left-click for selection and right-click for context menu actions.
 */
class StyleViewComponent : public juce::Component, public juce::MouseListener
{
public:
    //==============================================================================
    /**
     * @brief Callback invoked when the style is renamed.
     * @param oldName Original name of the style.
     * @param newName New name after renaming.
     */
    std::function<void(const juce::String& oldName, const juce::String& newName)> onStyleRenamed;

    /**
     * @brief Function to check if a style name already exists.
     * @param name The name to check.
     * @return True if the name exists in the list, false otherwise.
     */
    std::function<bool(const juce::String& name)> isInListNames;

    /**
     * @brief Callback invoked when the style is removed.
     * @param name Name of the style being removed.
     */
    std::function<void(const juce::String& name)> onStyleRemoveComponent;

    /**
     * @brief Callback invoked when the style is clicked.
     * @param name Name of the clicked style.
     */
    std::function<void(const juce::String&)> onStyleClicked;

    //==============================================================================
    /**
     * @brief Constructs a StyleViewComponent with the given style name.
     * @param styleName Name of the style to display.
     */
    StyleViewComponent(const juce::String& styleName);

    /** @brief Lays out the label inside the component. */
    void resized() override;

    /**
     * @brief Handles mouse click events.
     *
     * Left-click triggers `onStyleClicked`.
     * Right-click opens a context menu with "Rename" and "Delete" actions.
     *
     * @param event The mouse event details.
     */
    void mouseUp(const juce::MouseEvent& event) override;

    /**
     * @brief Updates the displayed style name.
     * @param name New name to display.
     */
    void setNameLabel(const juce::String& name);

    /** @brief Returns the current displayed style name. */
    juce::String getNameLabel() const;

    /** @brief Opens a dialog to rename the style and validates the new name. */
    void changeNameLabel();

    /** @brief Opens a confirmation dialog and removes the style if confirmed. */
    void removeStyle();

private:
    //==============================================================================
    juce::Label label;  ///< Label displaying the style name.
};
