/*
  ==============================================================================

    MidiNotesTableModel.h
    Created: 15 Jul 2025 8:46:13pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "TrackEntry.h"
#include "SelectableLabel.h"
#include "TrackPlayerListener.h"

/** Column ids of the "Notes information" table (CustomTableContainer adds the columns with these ids). */
namespace NoteTableColumns
{
    constexpr int note     = 1;
    constexpr int time     = 2;
    constexpr int velocity = 3;
    constexpr int channel  = 4;
    constexpr int noRow    = -1;
}

namespace NoteNames
{
    constexpr int  middleCOctave      = 4;      // "C4" is MIDI note 60
    constexpr bool useSharps          = true;   // getMidiNoteName flags
    constexpr bool includeOctave      = true;
    constexpr int  octavesBelowMiddleC = 5;     // MIDI note 0 is five octaves below middle C
    constexpr int  invalidNote        = -1;
}

/**
 * @class MidiNotesTableModel
 * @brief A TableListBoxModel for displaying and editing MIDI note events.
 *
 * This model maintains a filtered view of note-on events from a
 * MidiMessageSequence and supports real-time updates, sorting,
 * selection, and MIDI playback highlighting.
 *
 * It also implements TrackPlayerListener to allow updating the table
 * based on playback time.
 */
class MidiNotesTableModel: public juce::TableListBoxModel, public TrackPlayerListener
{
public:
    /** 
     * @struct EventWithIndex
     * @brief Stores a MIDI event pointer along with its original index in the sequence.
     */
    struct EventWithIndex {
        const juce::MidiMessageSequence::MidiEventHolder* event; /**< pointer to the MIDI event */
        int originalIndex; /**< index in the original MidiMessageSequence */
        int indexForChangesMap;
    };

    /** Callbacks for notifying UI or external components of changes */
    std::function<void(int rowNumber)> onUpdate;
    std::function<void(int row)> onRequestSelectRow;
    std::function<void()> refreshData;
    std::function<void(int row)> onMidPlayRepaint;
    std::function<double()> getCurrentBpm;
    std::function<bool(MidiChangeInfo& info)> validForErase;

    /**
     * @brief Constructor.
     * @param sequence The MIDI sequence to model.
     * @param channel The MIDI channel to filter events.
     * @param map Map to track note changes.
     */
    MidiNotesTableModel(const juce::MidiMessageSequence& sequence ,int channel, std::unordered_map<int, MidiChangeInfo>& map);

    /** @brief Returns the number of rows in the table */
    int getNumRows() override;

    /** @brief Sorts the note events based on column ID (note, timestamp, velocity) */
    void sortOrderChanged(int newSortColumnId, bool isForwards);

    /** @brief Paints the background of a table row */
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;

    /** @brief Paints the content of a table cell */
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool) override;
    
    /** @brief Provides a custom component for a cell */
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    /** @brief Converts a note name (e.g., "C4") to a MIDI note number */
    int getMidiNoteNumberFromName(const juce::String& name, int octaveMiddleC = NoteNames::middleCOctave);

    /** @brief Finds the first digit position in a note name string */
    int findFirstDigitName(const juce::String& name);

    /** @brief Handles changes for MIDI messages not yet in the map */
    void handleMapNonExistingCase(juce::MidiMessage& oldMessage, juce::MidiMessage& newMessage, MidiChangeInfo& str);

    /** @brief Handles changes for MIDI messages already in the map */
    void handleMapExistingCase(juce::MidiMessage& newMessage, MidiChangeInfo& str);

    /** @brief Checks if two MIDI messages are equal (raw data and timestamp) */
    bool areMidiMessagesEqual(const juce::MidiMessage& first, const juce::MidiMessage& second);

    /** @brief Refreshes the internal vector of note-on events from a sequence */
    void refreshVectorFromSequence(const juce::MidiMessageSequence& seq);

    /** @brief Returns the original sequence index corresponding to a table row */
    int getOriginalIndexFromRow(int row);

    int getChangesMapIndexFromRow(int row);

    /** @brief Returns the timestamp of the first note-on event */
    double getFirstNoteOnTimeStamps();

    /** @brief Returns the timestamp of the previous note-on event given a row index */
    double getPreviousNoteOnTimeStamp(int currentIndex);

    /** @brief Returns the timestamp of the current note-on event given a row index */
    double getCurrentNoteOnTimeStamp(int currentIndex);

    double getNextNoteOnTimeStamp(int currentIndex);

    /** @brief Returns a vector of all stored note-on events */
    std::vector<EventWithIndex> getEvents();

    /** @brief Returns the row index for a given playback time */
    int getRowFromTime(double currentTime);

    /** @brief Updates the currently highlighted row based on playback time */
    void updateCurrentRowBasedOnTime(double currentTime) override;

private:
    int highlightedRow = NoteTableColumns::noRow; /**< Currently highlighted row during playback */
    //std::unordered_map<int, int> originalIndexToRowMap; /**< Maps original sequence index to table row */
    std::vector<EventWithIndex> noteOnEvents; /**< Filtered note-on events */
    int channel; /**< MIDI channel being displayed */
    std::unordered_map<int, MidiChangeInfo>* changesMap = nullptr; /**< Map of user modifications */
};
