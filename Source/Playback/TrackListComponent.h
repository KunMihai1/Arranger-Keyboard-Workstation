#pragma once
#include <JuceHeader.h>
#include "TrackEntry.h"
#include "SubjectInterface.h"
#include "trackListComponentListener.h"
#include "PlayScreenLookAndFeel.h"

/**
 * @class TrackListComponent
 * @brief UI component that manages and displays folders and MIDI tracks.
 *
 * The `TrackListComponent` provides a dynamic interface for organizing, viewing, and manipulating
 * MIDI tracks and folders. It supports hierarchical folder views, drag-and-drop style list management,
 * renaming, sorting, and asynchronous file loading via JUCE's GUI and FileChooser APIs.
 *
 * It inherits from:
 * - **juce::Component** — for GUI rendering and layout.
 * - **juce::ListBoxModel** — to supply data for the JUCE ListBox that displays folders and tracks.
 * - **juce::ComboBox::Listener** — for handling sorting selection changes.
 * - **Subject<TrackListListener>** — to notify registered listeners when tracks are added, removed, or renamed.
 *
 * This component is central to managing track data in the app and synchronizing it with
 * persistent storage and UI updates.
 *
 * @note The component maintains two primary modes of operation:
 * - `FolderView` — Displays available folders (groups of tracks).
 * - `TrackView` — Displays tracks inside a selected folder.
 *
 * @see TrackEntry, TrackIOHelper, Subject, TrackListListener
 */
class TrackListComponent : public juce::Component,
                           private juce::ListBoxModel,
                           public juce::ComboBox::Listener,
                           public Subject<TrackListListener>
{
public:
    //======================================================================
    /** @brief Callback for removing a single track. */
    std::function<void(const juce::Uuid& uuid)> onRemoveTrack;

    /** @brief Callback for removing multiple tracks at once. */
    std::function<void(const std::vector<juce::Uuid>& uuids)> onRemoveMultipleTracks;

    /** @brief Callback for renaming a track from the list. */
    std::function<void(const juce::Uuid& uuid, const juce::String& newName)> onRenameTrackFromList;

    /** @brief Callback to update external data structures when a new track is added. */
    std::function<void(TrackEntry* newEntry)> addToMapOnAdding;

    /**
     * @brief Constructs the track list component.
     *
     * @param tracks Shared pointer to the deque of available track entries.
     * @param groupedTracksMap Shared pointer to a map of folder names to their corresponding tracks.
     * @param groupedKeys Shared pointer to a list of folder names.
     * @param onTrackChosen Function called when a user double-clicks a track.
     */
    TrackListComponent(std::shared_ptr<std::deque<TrackEntry>> tracks,
                       std::shared_ptr<std::unordered_map<juce::String, std::deque<TrackEntry>>> groupedTracksMap,
                       std::shared_ptr<std::vector<juce::String>> groupedKeys,
                       std::function<void(int)> onTrackChosen);

    /** @brief Positions and resizes child components within the main area. */
    void resized() override;

    /** @brief Returns the number of rows to display in the ListBox. */
    int getNumRows() override;

    /** @brief Responds to sorting combo box changes. */
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    /**
     * @brief Paints an individual item in the list box.
     * @param rowNumber The index of the item to paint.
     * @param g The graphics context.
     * @param width The width of the list row.
     * @param height The height of the list row.
     * @param rowIsSelected Whether the current row is selected.
     */
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;

    /**
     * @brief Called when a list box item is clicked (single or double click).
     * @param row The clicked row index.
     * @param event The associated mouse event.
     */
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

    /** @brief Adds new tracks to the current folder from selected MIDI files. */
    void addToTrackList();

    /** @brief Removes selected tracks from the current folder. */
    void removeFromTrackList();

    /** @brief Renames a selected track in the list. */
    void renameFromTrackList();

    /** @brief Creates a new folder (group) for organizing tracks. */
    void addToFolderList();

    /** @brief Removes one or more selected folders and their contained tracks. */
    void removeFromFolderList();

    /** @brief Removes the given rows of the open folder (what Remove does once confirmed). */
    void removeTracks(const juce::SparseSet<int>& rows);

    /** @brief Removes the folders at the given rows (what Remove folder does once confirmed). */
    void removeFolders(const juce::SparseSet<int>& rows);

    /** @brief Renames the currently selected folder. */
    void renameFromFolderList();

    /** @brief Returns to folder view from inside a track list. */
    void backToFolderView();

    /**
     * @brief Returns a reference to the current list of available tracks.
     * @return Reference to the track deque.
     */
    std::deque<TrackEntry>& getAllAvailableTracks() const;

    /**
     * @brief Extracts a readable display name from a given MIDI sequence.
     * @param trackSeq The MIDI sequence to extract from.
     * @return A formatted display name (e.g., "Piano (Grand)").
     */
    juce::String extractDisplayNameFromTrack(const juce::MidiMessageSequence& trackSeq);

    /** @brief Initializes the UI and buttons for viewing tracks inside a folder. */
    void initializeTracksFromList();

    /** @brief Clears track-related UI components when exiting track view. */
    void deallocateTracksFromList();

private:
    //======================================================================
    /**
     * @enum ViewMode
     * @brief Represents the current visual state of the component.
     */
    enum class ViewMode
    {
        FolderView,  /**< Folder hierarchy is displayed. */
        TrackView    /**< Individual tracks are displayed. */
    };

    /** @brief Current view mode of the component (folder or track view). */
    ViewMode viewMode = ViewMode::FolderView;

    /** @brief Shared list of available tracks (flat representation). */
    std::shared_ptr<std::deque<TrackEntry>> availableTracks;

    /** @brief Shared map of folders to their contained tracks. */
    std::shared_ptr<std::unordered_map<juce::String, std::deque<TrackEntry>>> groupedTracks;

    /** @brief Shared list of folder names (keys for the grouped map). */
    std::shared_ptr<std::vector<juce::String>> groupedTrackKeys;

    /** @brief Callback invoked when a user selects a track to play or edit. */
    std::function<void(int)> trackChosenCallBack;

    PlayScreenLookAndFeel laf;

    /** @brief ListBox displaying folders or tracks. */
    juce::ListBox listBox;

    // UI Controls -----------------------------------------------------------
    std::unique_ptr<juce::TextButton> addButton = nullptr;      /**< Add track button. */
    std::unique_ptr<juce::TextButton> removeButton = nullptr;   /**< Remove track button. */
    std::unique_ptr<juce::TextButton> backButton = nullptr;     /**< Back to folders button. */
    std::unique_ptr<juce::TextButton> renameButton = nullptr;   /**< Rename track button. */

    juce::TextButton addButtonFolder{ "Add folder" };           /**< Add folder button (visible in folder view). */
    juce::TextButton removeButtonFolder{ "Remove folder" };     /**< Remove folder button (visible in folder view). */
    juce::TextButton renameButtonFolder{ "Rename folder" };     /**< Rename folder button (visible in folder view). */

    std::unique_ptr<juce::ComboBox> sortComboBox = nullptr;     /**< Sorting options dropdown. */

    /** @brief Stores the name of the currently selected folder. */
    juce::String currentFolderName;
};
