/*
  ==============================================================================

    CustomTableContainer.h
    Created: 15 Jul 2025 9:27:26pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MidiNotesTableModel.h"
#include "ValidatorUI.h"
#include <variant>
#include "SubjectInterface.h"
#include "TrackPlayerListener.h"
#include "TrackPlayer.h"

/** Item ids of the "Undo settings" dropdown (reset to the original track). */
namespace ResetActionIds
{
    constexpr int everything = 1;
    constexpr int notes      = 2;
    constexpr int timeStamps = 3;
    constexpr int velocities = 4;
}

/** Item ids of the "Modify settings" dropdown. */
namespace ModifyActionIds
{
    constexpr int timeStamps = 1;
    constexpr int velocities = 2;
}

/** showModifyChangeDialog: buttons are numbered from 1 in the order given; 0 means dismissed. */
namespace TableDialog
{
    constexpr int dismissed       = 0;
    constexpr int firstButtonId   = 1;
    constexpr int applyToAll      = 1;   // first button
    constexpr int applyToSelected = 2;   // second button
    inline const juce::String changeAll      = "Change All";
    inline const juce::String changeSelected = "Change selected";
    inline const juce::String modifyAll      = "Modify All";
    inline const juce::String modifySelected = "Modify selected";
    inline const juce::String cancel         = "Cancel";
}

/**
 * @class TableContainer
 * @brief A component that displays a table of MIDI notes and allows editing of note properties.
 */
class TableContainer: public juce::Component, public TrackPlayerListenerModifyStateObjects
{
public:
    /// Callback for updating the file when changes occur.
    std::function<void()> updateToFile;

    /// Callback for removing the model from a listener.
    std::function<void(TrackPlayerListener* listener)> removeModelFromListener;

    std::function<void()> removeContainerFromListeners;

    std::function<double()> getCurrentBPMstyle;

    /** Whether the track player is running now; read when a play-state notification arrives.
        The style panel sets it when it opens the table; while unset, notifications change nothing. */
    std::function<bool()> isPlayerRunning;

    /**
     * @brief Constructor.
     * @param seq MIDI message sequence to display and modify.
     * @param displayName Name of the track to display in the table header.
     * @param channel MIDI channel for filtering notes.
     * @param map Map of MIDI changes to track modifications.
     */
    TableContainer(juce::MidiMessageSequence& seq, const juce::String& displayName, int channel, std::unordered_map<int, MidiChangeInfo>& map, bool shouldDisable=false);

    /**
     * @brief Destructor.
     */
    ~TableContainer() override;

    /**
     * @brief Called when the component is resized.
     */
    void resized() override;

    void updateObjects() override;

    /**
     * @brief Reset all note numbers to their original values.
     */
    void onlyNotes();

    /**
     * @brief Reset note numbers of selected rows to their original values.
     * @param allSelected Selected rows.
     */
    void onlyNotesSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Reset all time stamps to their original values.
     */
    void onlyTimeStamps();

    /**
     * @brief Reset time stamps of selected rows to their original values.
     * @param allSelected Selected rows.
     */
    void onlyTimeStampsSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Reset all velocities to their original values.
     */
    void onlyVelocities();

    /**
     * @brief Reset velocities of selected rows to their original values.
     * @param allSelected Selected rows.
     */
    void onlyVelocitiesSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Reset all properties (note, timestamp, velocity) to their original values.
     */
    void allProperties();

    /**
     * @brief Reset properties of selected rows to their original values.
     * @param allSelected Selected rows.
     */
    void allPropertiesSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Check if a MIDI change can be erased (i.e., matches the original state).
     * @param info MIDI change info to check.
     * @return True if change matches the original, false otherwise.
     */
    bool validForErase(MidiChangeInfo& info);

    /**
     * @brief Apply a change to a specific MIDI event in the sequence.
     * @param index Index of the MIDI event.
     * @param info Change information.
     * @param modifyVelocity If true, only modify the velocity.
     */
    void applyChangeFromSequence(int originalIndex, const MidiChangeInfo& info, bool modifyVelocity=false, bool shouldApplyChanges=false);

    /**
     * @brief Refresh table and reapply changes after a modification is confirmed.
     */
    void onConfirmed();

    /**
     * @brief Show a dialog and apply changes to all or selected properties.
     */
    void changeAllUI();

    /**
     * @brief Show a dialog and apply changes to all or selected notes.
     */
    void changeNotesUI();

    /**
     * @brief Show a dialog and apply changes to all or selected time stamps.
     */
    void changeTimeStampsUI();

    /**
     * @brief Show a dialog and apply changes to all or selected velocities.
     */
    void changeVelocitiesUI();

    /**
     * @brief Check if any row is selected.
     * @return True if at least one row is selected.
     */
    bool anySelected();

    /**
     * @brief Show a dialog and modify time stamps for all or selected rows.
     */
    void modifyTimeStampsUI();

    /**
     * @brief Show a dialog and modify velocities for all or selected rows.
     */
    void modifyVelocitiesUI();

    /**
     * @brief Modify time stamps for all rows according to editor value.
     */
    void modifyOnlyTimeStamps();

    /**
     * @brief Modify time stamps for selected rows according to editor value.
     * @param allSelected Selected rows.
     */
    void modifyOnlyTimeStampsSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Modify velocities for all rows according to editor value.
     */
    void modifyOnlyVelocities();

    /**
     * @brief Modify velocities for selected rows according to editor value.
     * @param allSelected Selected rows.
     */
    void modifyOnlyVelocitiesSelected(const juce::SparseSet<int>& allSelected);

    /**
     * @brief Reset a property and apply changes to the sequence.
     * @param resetProperty Function to reset the property.
     * @param selected Optional selected rows.
     * @param modifyVelocity If true, only modify velocity.
     */
    void resetPropertyAndApply(const std::function<void(MidiChangeInfo&)>& resetProperty, const juce::SparseSet<int>* selected = nullptr, bool modifyVelocity=false);

    /**
     * @brief Apply a new property to the sequence.
     * @param newProperty Function that applies the new property.
     * @param selected Optional selected rows.
     * @param modifyVelocity If true, only modify velocity.
     */
    void newPropertyAndApply(const std::function<void(MidiChangeInfo&)>& newProperty, const juce::SparseSet<int>* selected = nullptr, bool modifyVelocity=false);

    /**
     * @brief Helper function for applying a new property to a specific row.
     * @param newProperty Function that applies the new property.
     * @param selected Optional selected rows.
     * @param modifyVelocity If true, only modify velocity.
     * @param originalIndex Index of the MIDI event in the original sequence.
     */
    void newPropertyHelperFunction(const std::function<void(MidiChangeInfo&)>& newProperty, const juce::SparseSet<int>* selected, bool modifyVelocity, int originalIndex, int noteOnIndex);

    /**
     * @brief Show a confirmation dialog for modifying or changing properties.
     * @param title Title of the dialog.
     * @param messageAllSelected Message to show if rows are selected.
     * @param messageNoSelection Message to show if no row is selected.
     * @param buttonsWithSelection Buttons to show if rows are selected.
     * @param buttonsWithoutSelection Buttons to show if no row is selected.
     * @param onValidResults Callback called with the selected button index.
     * @param isModify True if this is a modification dialog.
     */
    void showModifyChangeDialog(const juce::String& title, const juce::String& messageAllSelected, const juce::String& messageNoSelection,
        const std::vector<juce::String>& buttonsWithSelection, const std::vector<juce::String>& buttonsWithoutSelection,
        std::function<void(int)> onValidResults, bool isModify=false);

    /**
     * @brief Add the table model as a listener to a subject.
     * @param subject Subject to add the model as listener to (optional).
     */
    void addModelAsListenerToTrackPlayer(MultipleTrackPlayer* player);

private:
    std::unique_ptr<juce::TableListBox> table;        ///< Table displaying MIDI notes
    std::unique_ptr<MidiNotesTableModel> model;       ///< Model providing data to the table
    std::unique_ptr<juce::Label> disclaimerLabel;     ///< Label showing disclaimer text
    std::unique_ptr<juce::TextButton> changeToOriginalMultipleButton = nullptr; ///< Button for changing properties to original
    std::unique_ptr<juce::ComboBox> actionToOriginalCB = nullptr;              ///< ComboBox to choose which property to reset
    std::unique_ptr<juce::Label> infoLabelChange = nullptr;                    ///< Info label for reset actions

    std::unique_ptr<juce::TextButton> modifyMultipleButton = nullptr;          ///< Button for applying modifications
    std::unique_ptr<juce::ComboBox> actionModifyCB = nullptr;                  ///< ComboBox to choose which property to modify
    std::unique_ptr<juce::Label> infoLabelModify = nullptr;                    ///< Info label for modify actions

    juce::MidiMessageSequence& originalSequence;  ///< Reference to the original MIDI sequence
    static constexpr int noSelectedRow = -1;
    int lastSelectedRow = noSelectedRow;                     ///< Last selected row for shift selection
    std::unordered_map<int, MidiChangeInfo>& changesMap; ///< Map of MIDI changes
    std::variant<int, double> editorValue{};      ///< Value entered in the modify dialog
};
