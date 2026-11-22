/*
  ==============================================================================

    SelectableLabel.h
    Created: 19 Jul 2025 2:08:23am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * @class SelectableLabel
 * @brief A JUCE Label that supports click callbacks.
 *
 * This class extends juce::Label by adding an `onClick` callback, which is triggered
 * whenever the label is clicked with the mouse.
 */
class SelectableLabel : public juce::Label
{
public:
    /**
     * @brief Callback triggered when the label is clicked.
     */
    std::function<void()> onClick;

    /**
     * @brief Called when the mouse is pressed on this label.
     * @param event The mouse event.
     *
     * Triggers the `onClick` callback if one is assigned, then calls the base class
     * implementation to preserve normal label behavior.
     */
    void mouseDown(const juce::MouseEvent& event) override;

private:
    int row {};                  /**< Optional row identifier, can be used in a TableListBox */
    juce::TableListBox* table = nullptr; /**< Optional pointer to parent table, if any */
};
