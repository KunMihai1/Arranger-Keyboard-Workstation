/*
  ==============================================================================

    PlayBackSettingsCustomComponent.h
    Created: 10 Oct 2025 10:16:25pm
    Author:  Kisuke

    The Voices page's "Range, split & transpose" column (board StageRevised
    openPage=voices): the style's playing range, the two hands' split and the
    transpose, as LCD fields. Display owns it (per-style settings live there);
    the Voices page shows it.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "HelperFunctions.h"
#include "PersistedKeys.h"
#include "UI/Stage/StagePageWidgets.h"

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

/** The transpose field's choices: -12 ... +12 semitones, one item each. */
namespace TransposeField
{
    inline constexpr int maxSemitones = 12;
    juce::String itemText (int semitones);   ///< "0 semitones", "+3 semitones", "-12 semitones"
    int itemId (int semitones);
    int semitonesFor (int itemId);
}

/**
 * @class PlayBackSettingsComponent
 * @brief The playback range, the hands' split and the transpose of the open style.
 *
 * A change made in a field updates the PlayBackSettings struct it was given and reports it (onChangingSettings,
 * which saves it per style); showing settings that changed elsewhere (another style opened, an octave shift)
 * only updates the fields.
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
    ~PlayBackSettingsComponent() override;

    void setScale (float newScale);

    /** No style open: the fields are disabled (they save per style). */
    void setStyleOpen (bool open);

    void paint (juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Called when a ComboBox changes value.
     * @param comboBoxThatHasChanged Pointer to the ComboBox that changed.
     */
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    /** Shows the settings struct's current values without reporting them (they were loaded or shifted elsewhere). */
    void showSettings();

    /**
     * @brief Updates the available lowest and highest note range (the fields' choices follow).
     * @param lowest The lowest MIDI note to allow.
     * @param highest The highest MIDI note to allow.
     */
    void setLowestHighest(int lowest, int highest);

    int getTransposeSemitones() const;

private:
    void refreshFields();

    int lowestNote;               /**< Lowest selectable MIDI note */
    int highestNote;              /**< Highest selectable MIDI note */
    PlayBackSettings& settings;   /**< Reference to external settings struct */
    float scale { StageMetrics::referenceScale };

    StageField startNoteBox;      /**< start note */
    StageField endNoteBox;        /**< end note */
    StageField leftHandBoundBox;  /**< the left hand's highest note */
    StageField rightHandBoundBox; /**< the right hand's lowest note */
    StageField transposeField;

    SilkLabel rangeLabel, leftHandBoundLabel, rightHandBoundLabel, transposeLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayBackSettingsComponent)
};
