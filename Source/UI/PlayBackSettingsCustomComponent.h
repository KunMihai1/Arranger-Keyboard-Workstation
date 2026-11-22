/*
  ==============================================================================

    PlayBackSettingsCustomComponent.h
    Created: 10 Oct 2025 10:16:25pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "HelperFunctions.h"
#include "PersistedKeys.h"

/**
 * @struct PlayBackSettings
 * @brief Holds configuration for MIDI playback ranges and hand bounds.
 *
 * This struct stores the start and end notes for playback, left/right hand boundaries,
 * and optionally USB device identifiers (VID/PID) for connected MIDI devices.
 */
struct PlayBackSettings
{
    int startNote = PlaybackSettingsKeys::notSet;      /**< Starting note for playback */
    int endNote = PlaybackSettingsKeys::notSet;        /**< Ending note for playback */
    int leftHandBound = PlaybackSettingsKeys::notSet;  /**< Highest note for the left hand */
    int rightHandBound = PlaybackSettingsKeys::notSet; /**< Lowest note for the right hand */
    juce::String VID;        /**< USB Vendor ID (optional) */
    juce::String PID;        /**< USB Product ID (optional) */
};

/**
 * @class PlayBackSettingsComponent
 * @brief Custom JUCE component for configuring playback note ranges and hand intervals.
 *
 * This component provides ComboBoxes and Labels for selecting:
 * - Start and end note of playback.
 * - Left and right hand boundaries.
 *
 * It also supports updating a provided PlayBackSettings struct and saving the settings
 * to a JSON file.
 */
class PlayBackSettingsComponent : public juce::Component, private juce::ComboBox::Listener
{
public:
    /**
     * @brief Callback triggered when the user changes playback settings.
     */
    std::function<void(PlayBackSettings settings)> onChangingSettings;
    std::function<void(int transposeValue)> onChangingTranspose;
    std::function<juce::String()> getStyleID;

    /**
     * @brief Constructor
     * @param lowestNote The lowest MIDI note available for selection.
     * @param highestNote The highest MIDI note available for selection.
     * @param settings Reference to a PlayBackSettings struct to be updated.
     */
    PlayBackSettingsComponent(int lowestNote, int highestNote, PlayBackSettings& settings);

    /** Destructor */
    ~PlayBackSettingsComponent();

    /** @brief Lays out the child ComboBoxes and Labels */
    void resized() override;

    /**
     * @brief Called when a ComboBox changes value.
     * @param comboBoxThatHasChanged Pointer to the ComboBox that changed.
     */
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    /** @brief Initializes left and right hand interval ComboBoxes */
    void handIntervalsInit();

    /** @brief Initializes start and end note ComboBoxes */
    void startStopInit();

    void transposeSliderInit();

    /**
     * @brief Updates the component with a new set of settings.
     * @param newSettings The new PlayBackSettings values to apply.
     */
    void setNewSettings(PlayBackSettings& newSettings);

    /**
     * @brief Updates the available lowest and highest note range.
     * @param lowest The lowest MIDI note to allow.
     * @param highest The highest MIDI note to allow.
     */
    void setLowestHighest(int lowest, int highest);

private:
    int lowestNote;               /**< Lowest selectable MIDI note */
    int highestNote;              /**< Highest selectable MIDI note */
    PlayBackSettings& settings;   /**< Reference to external settings struct */

    juce::ComboBox startNoteBox;  /**< ComboBox to select start note */
    juce::ComboBox endNoteBox;    /**< ComboBox to select end note */
    juce::Label startNoteLabel;   /**< Label for start note ComboBox */
    juce::Label endNoteLabel;     /**< Label for end note ComboBox */

    juce::Label leftHandBoundLabel;   /**< Label for left hand ComboBox */
    juce::Label rightHandBoundLabel;  /**< Label for right hand ComboBox */

    juce::ComboBox leftHandBoundBox;  /**< ComboBox to select left hand range */
    juce::ComboBox rightHandBoundBox; /**< ComboBox to select right hand range */

    juce::Slider transposeSlider;
    
};
