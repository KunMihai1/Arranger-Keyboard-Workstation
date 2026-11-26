#pragma once
#include <JuceHeader.h>
#include "UI/Stage/StageMetrics.h"
#include "UI/Stage/StageDialog.h"

/** The style-name dialog shared by StylesListComponent (add) and StyleViewComponent (rename). */
namespace StyleDialogText
{
    inline const juce::String nameLabel = "Style name";
    inline const juce::String okButton  = "OK";
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

    /** Opens the list's in-app dialog (the list owns it: a removal deletes this row). */
    std::function<void(StageDialogOptions options, StageDialog::OnConfirm onConfirm)> openDialog;

    /** A short message for the Stage screen. */
    std::function<void(const juce::String& text)> onInfo;

    /** The open style's row is inverted and marked (board .lcd-row .lcd-sel with the arrow). */
    void setOpen(bool shouldBeOpen);
    bool isOpen() const;
    void setScale(float newScale);
    void paint(juce::Graphics& g) override;

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
    bool open = false;
    float scale = StageMetrics::referenceScale;
    juce::Label label;  ///< Label displaying the style name.
};
