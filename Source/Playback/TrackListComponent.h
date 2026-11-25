#pragma once
#include <JuceHeader.h>
#include "TrackEntry.h"
#include "SubjectInterface.h"
#include "trackListComponentListener.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageDialog.h"

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

    //==============================================================================
    // The track picker page (board PageTracks)
    void paint(juce::Graphics& g) override;
    void setScale(float newScale);
    /** The slot being filled ("Use in slot N"). */
    void setSlotNumber(int oneBasedSlot);
    juce::String getUseButtonText() const;
    void selectRow(int row);
    void selectedRowsChanged(int lastRowSelected) override;
    /** The title row's controls: SORT and the sort button. */
    std::vector<juce::Component*> getTitleActions();
    /** Steps the sort: Name -> Newest -> Oldest -> Name (today's three choices). */
    void cycleSort();
    juce::String sortCaption() const;

    struct PickerDetail { juce::String label, value; };
    /** The right column: the selected folder (Folder, Tracks, Melodic, Percussion) or track (Track,
        Instrument, Type, Folder). */
    std::vector<PickerDetail> details() const;

    /** A short message for the Stage screen (a warning such as "select a folder first", or a result). */
    std::function<void(const juce::String& text)> onInfo;

    /** The in-app dialog on screen (add / rename / remove), or nullptr. */
    StageDialog* getOpenDialog() const;

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


    /** @brief ListBox displaying folders or tracks. */
    juce::ListBox listBox;

    // UI Controls -----------------------------------------------------------
    SegButton addButton    { {}, SegButton::Style::seg };   /**< Add from MIDI file... (track view) */
    SegButton removeButton { {}, SegButton::Style::seg };   /**< Remove (track view) */
    SegButton backButton   { {}, SegButton::Style::seg };   /**< < Back to the folders */
    SegButton renameButton { {}, SegButton::Style::seg };   /**< Rename... (track view) */
    SegButton useButton    { {}, SegButton::Style::lit };   /**< Use in slot N (= double-click a track) */
    SegButton openFolderButton { {}, SegButton::Style::lit }; /**< Open folder (= double-click a folder) */
    SilkLabel sortLabel    { {}, SilkLabel::Tone::dim };
    SegButton sortButton   { {}, SegButton::Style::seg };
    int slotNumber {};
    float scale = StageMetrics::referenceScale;
    void updateViewControls();
    void openFolderAt(int row);
    juce::Rectangle<int> listArea() const;
    juce::Rectangle<int> detailArea() const;

    SegButton addButtonFolder    { {}, SegButton::Style::seg };   /**< Add folder... (folder view) */
    SegButton removeButtonFolder { {}, SegButton::Style::seg };   /**< Remove (folder view) */
    SegButton renameButtonFolder { {}, SegButton::Style::seg };   /**< Rename... (folder view) */

    /** Today's sort choice, kept as the state the sort button steps (hidden). */
    std::unique_ptr<juce::ComboBox> sortComboBox = std::make_unique<juce::ComboBox>();

    /** @brief Stores the name of the currently selected folder. */
    juce::String currentFolderName;

    /** The page's dialog (one at a time; the next one replaces it). Not juce::AlertWindow: building one stalled
        the UI for 0.6 - 1 s. */
    std::unique_ptr<StageDialog> dialog;
    void openDialog(StageDialogOptions options, StageDialog::OnConfirm onConfirm);
    void showInfo(const juce::String& text);
};
