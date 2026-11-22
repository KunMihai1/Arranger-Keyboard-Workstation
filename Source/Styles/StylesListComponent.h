#pragma once
#include <JuceHeader.h>
#include "StyleViewComponent.h"
#include "PlayScreenLookAndFeel.h"

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

private:
    //==============================================================================
    /** @brief Populates the container with `StyleViewComponent`s based on `stylesNames`. */
    void populate();

    PlayScreenLookAndFeel laf;
    juce::OwnedArray<StyleViewComponent> allStyles;  ///< All style components.
    std::function<void(const juce::String&)> onStyleClicked;  ///< Callback when a style is clicked.
    std::vector<juce::String> stylesNames;  ///< Names of all styles.
    int widthSize;  ///< Width used for layout calculations.

    inline static const juce::String addCaption = "Add";
    juce::TextButton addButton{ addCaption };  ///< Button to add a new style.
    std::unique_ptr<juce::Component> styleItemsContainer = nullptr;  ///< Container holding all styles.
    juce::Viewport viewport;  ///< Scrollable viewport for the style list.

    JUCE_LEAK_DETECTOR(StylesListComponent)
};
