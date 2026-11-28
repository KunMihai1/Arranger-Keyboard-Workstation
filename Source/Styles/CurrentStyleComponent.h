#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "TrackPlayer.h"
#include "Arranger/ArrangerEngine.h"
#include "Arranger/ArrangerStyleEditor.h"
#include "Arranger/ArrangerStyleListComponent.h"
#include "UI/Sections/SectionsView.h"
#include "TrackPlayerListener.h"
#include "StyleSection.h"
#include "CustomBeatBar.h"

class TableContainer;

/** Item ids of the play-settings dropdown. 1 and 2 are play modes (kept ticked); 3 and 4 are actions. */
namespace PlayModeIds
{
    constexpr int allTracks             = 1;
    constexpr int soloTrack             = 2;
    constexpr int settings              = 3;
    constexpr int sectionConfigurations = 4;
}

/** onArrangerSectionChanged's index: 0 or more highlights the named section, a negative one clears it. */
namespace SectionHighlight
{
    constexpr int byTypeAndName = 0;
}

/**
 * @class CurrentStyleComponent
 * @brief Manages and displays all tracks within a musical style.
 *
 * The `CurrentStyleComponent` acts as a container and controller for multiple
 * `Track` components. It manages playback, tempo, instrument synchronization,
 * and communication with a `MultipleTrackPlayer`.
 *
 * It provides control over all tracks simultaneously or individually, and
 * handles serialization, deserialization, and UI updates for the current style.
 *
 * Key features include:
 * - Managing up to 8 tracks per style
 * - Syncing tempo (BPM) changes across all tracks
 * - Controlling playback (start/stop)
 * - Managing track selections, renaming, and deletions
 * - Handling JSON import/export of style data
 */
class CurrentStyleComponent : public juce::Component,
                              private juce::MouseListener,
                              private juce::ComboBox::Listener,
                              public TrackPlayerListenerModifyStateObjects
{
public:

    static constexpr int trackSlots = 8;   ///< Track slots per style (one row).
    static constexpr int firstSlotNumber = 1;   ///< The strips are numbered 1..8.

    //==============================================================================

    std::function<bool(const juce::String& name)> tabExsitsCallback;

    /** @brief Called whenever any track within the style is modified. */
    std::function<void()> anyTrackChanged;

    /**
     * @brief Callback for requesting a track selection from another component.
     *
     * Passes a callback which will be called with the chosen track name, UUID, and type.
     */
    std::function<void(TrackChosenCallback)> onRequestTrackSelectionFromTrack;

    /** @brief Callback to trigger track file updates after changes. */
    std::function<void()> updateTrackFile;

    /** @brief Callback triggered when switching to the keybinds tab. */
    std::function<void()> keybindTabStarting;

    /** The Sections view (spec 2026-11-27 sections v2): the configurations list and the editor, shown in the
        Style page by the host (Display). Show it / leave it; its title-row controls changed. */
    std::function<void()> onShowSections, onLeaveSections, onSectionsActionsChanged;
    SectionsView& getSectionsView() { return sectionsView; }
    /** Exit on the Sections view: closes the editor first (true; also true while a save runs, which blocks Exit),
        false when only the list is open (the view may go). */
    bool sectionsExitStep();
    /** The Sections view goes away (page closed, Home, another style): stop the preview and drop the editor. */
    void closeSections();
    /** True while the Sections view is open: live Start / sections wait until it is closed (it previews). */
    bool isAuthoringSections() const { return sectionsOpen; }

    /** Asks the host to return keyboard focus to the play surface (so the PC keyboard keeps
        playing). Fired after Start, since starting the arranger rebuilds the scene and can leave
        focus on a freshly created child. */
    std::function<void()> onRequestPlayFocus;

    /** Returns the chord the player is currently holding, so startPlaying() can seed the arranger
        with an already-held chord instead of starting on the home key. Wired host-side. */
    std::function<ArrangerChord()> getHeldChord;

    /** Shows/hides the app's full-screen "working" overlay during off-thread load operations (the
        editor's own save/update overlay is internal). Routed host-side to MainComponent's overlay. */
    std::function<void(bool show, const juce::String& text)> onBusy;

    /** Fired when the arranger engine's active section changes (or it stops), so the host can
        highlight the matching live section button. sectionIndex < 0 means nothing is active. */
    std::function<void(int sectionIndex, ArrangerSectionType type, juce::String name)> onArrangerSectionChanged;

    /** Every beat position the beat bar shows, with its beats per bar (message thread). */
    std::function<void(double elapsedBeats, int beatsPerBar)> onBeatPosition;

    /** The notes page of a track (board PageNotes): the host shows it in the Style page, titled
        "Notes · <track>" with "Track n · <instrument> · N notes" under it. */
    std::function<void(std::unique_ptr<TableContainer> notes, const juce::String& title, const juce::String& subtitle)> onShowNotes;

    /** A short message for the Stage screen (from a track strip). */
    std::function<void(const juce::String& text)> onInfo;

    /** A strip's instrument was chosen: its channel and the GM program (the SFZ engine reloads that channel). */
    std::function<void(int channel, int program)> onTrackInstrumentChosen;

    /** Overlay each live strip onto the matching configuration track (by UUID): its slider volume, and its
        instrument when one is chosen, so the strips win over what a saved configuration baked in. */
    static void applyLiveTrackSettings (ArrangerStyle& style, const juce::OwnedArray<Track>& liveTracks);

    // --- The Style page (board StageRevised openPage=styles) ---
    /** All tracks / Solo: set through the old play-mode choice, so its rules (an active section
        configuration is cleared) stay. */
    void setPlayModeSolo(bool solo);
    bool isPlayModeSolo() const;
    /** Sections...: today's section-configuration browser (until the new sections editor). */
    void openSectionConfigurations();
    void setScale(float newScale);
    void paint(juce::Graphics& g) override;
    /** The slot (1-8) whose name was clicked last: the track picker fills it. */
    int getRequestingSlot() const { return requestingSlot; }

    //==============================================================================
    /**
     * @brief Constructs the CurrentStyleComponent.
     *
     * @param name Name of the style.
     * @param map Reference to the UUID-to-TrackEntry map used for synchronization.
     * @param outputDevice Weak pointer to the MIDI output device used for playback.
     */
    CurrentStyleComponent(const juce::String& name,
                          std::unordered_map<juce::Uuid, TrackEntry*>& map,
                          std::weak_ptr<juce::MidiOutput> outputDevice,
                          std::weak_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> styleSectionsMap);

    /** @brief Destructor. Cleans up listeners and internal references. */
    ~CurrentStyleComponent() override;

    //==============================================================================
    /** @brief Triggers the stop button click programmatically. */
    void triggerStopClick();

    /** @brief Triggers the start button click programmatically. */
    void triggerStartClick();

    /** @brief Stops the player immediately and resets internal states. */
    void stoppingPlayer();

    //==============================================================================
    /** @brief Resizes all child components (tracks, sliders, buttons, labels). */
    void resized() override;

    /**
     * @brief Updates the current style's name and tooltip.
     * @param newName New style name to display.
     */
    void updateName(const juce::String& newName);

    void updateObjects() override;

    /** @brief Returns the current style's name. */
    juce::String getName();

    juce::String getStyleID();

    /**
     * @brief Serializes the current style to a JUCE DynamicObject.
     * @return Pointer to a dynamically allocated object representing this style.
     */
    juce::DynamicObject* getJson() const;

    /**
     * @brief Loads style data from a JSON-style `juce::var` object.
     * @param styleVar A `juce::var` representing serialized style data.
     */
    void loadJson(const juce::var& styleVar);

    //==============================================================================
    /** @brief Starts playback of selected tracks based on play mode (all/solo).
        @returns false when nothing was started (no output device, or no tracks to play) -
                 the user has been told why, and the panel must stay in its stopped state. */
    bool startPlaying();

    /** @brief Stops playback of all tracks. */
    void stopPlaying(bool shouldModify=true);

    /** @brief Returns the current playback tempo (BPM). */
    double getTempo();

    /** @brief Returns the base tempo used for scaling sequences. */
    double getBaseTempo();

    /**
     * @brief Sets a new playback tempo (BPM).
     * @param newTempo The new tempo in beats per minute.
     */
    void setTempo(double newTempo);

    /** Applies a tempo the way releasing the BPM slider does (player and arranger BPM, rescaled tracks, style
        marked changed). Used by the Stage panel's screen; while playing, the music changes speed at once. */
    void commitTempo(double newTempo);

    /**
     * @brief Sets a unique style identifier for this component.
     * @param newID New style ID string.
     */
    void setStyleID(const juce::String& newID);

    /**
     * @brief Applies style-based modifications to a single track.
     * @param track Reference to a `TrackEntry` object.
     */
    void applyChangesForOneTrack(TrackEntry& track);

    /** @brief Applies style modifications to all tracks in the current style. */
    void applyChangesForAllTracksCurrentStyle();

    /**
     * @brief Removes a single track by UUID, resetting it visually and logically.
     * @param uuid UUID of the track to remove.
     */
    void removingTrack(const juce::Uuid& uuid);

    /**
     * @brief Removes multiple tracks by their UUIDs.
     * @param uuids List of track UUIDs to remove.
     */
    void removingTracks(const std::vector<juce::Uuid>& uuids);

    /**
     * @brief Renames a track by UUID.
     * @param uuid UUID of the track to rename.
     * @param newName New name for the track.
     */
    void renamingTrack(const juce::Uuid& uuid, const juce::String& newName);

    /**
     * @brief Displays note information for a specific track.
     * @param uuid Track UUID.
     * @param channel MIDI channel associated with the track.
     */
    void showingTheInformationNotesFromTrack(const juce::Uuid& uuid, int channel);

    /**
     * @brief Sets the MIDI output device for the current style.
     * @param newOutput New weak pointer to a MIDI output device.
     */
    void setDeviceOutputCurrentStyle(std::weak_ptr<juce::MidiOutput> newOutput);

    /** @brief Returns all tracks currently contained in this style. */
    juce::OwnedArray<Track>& getAllTracks();

    /** @brief Returns a pointer to the internal track player instance. */
    MultipleTrackPlayer* getTrackPlayer();

    /** The arranger-mode engine (never null once constructed). */
    ArrangerEngine* getArrangerEngine() { return arrangerEngine.get(); }

    /** Enable/disable Arranger mode (vs classic linear playback). */
    void setArrangerModeEnabled(bool shouldEnable);
    bool isArrangerModeEnabled() const { return arrangerModeEnabled; }

    /** Phase 4: feed the live played chord (from the keyboard) to the arranger engine, which
        transposes the accompaniment to it. No-op outside arranger mode. */
    void setLiveChord(const ArrangerChord& chord);

    /** Phase 4: on Start, apply any chord the player is already holding (via getHeldChord) so the
        accompaniment begins on it rather than the home key. No-op if nothing valid is held. */
    void seedHeldChordIntoArranger();

    /** Phase 4: toggle Bass Inversion on the arranger engine (slash chords). */
    void setArrangerBassInversion(bool shouldInvert);

    /** Phase 7a: master NTT bypass; remembered and re-applied on each Start. */
    void setArrangerNttEnabled(bool enabled);
    /** Phase 7a: curated minor scale for NTT passing-tone snap; remembered and re-applied on Start. */
    void setArrangerMinorScale(MinorScaleChoice choice);

    /** Phase 6: enable/disable Synchro Start on the arranger engine (start the groove on the first
        recognised chord). Remembered and re-applied on each Start. */
    void setSynchroStartEnabled(bool enabled);

    /** Phase 6b: enable/disable Count-In (one metronome bar before the groove). */
    void setCountInEnabled(bool enabled);

    /** Enable/disable Auto Fill on variation switches (forwarded to the arranger engine). */
    void setArrangerAutoFillEnabled(bool enabled);

    void setMidiInjectCallback(std::function<void(const juce::MidiMessage&)> cb);

    struct TrackChannelInstrument { int channel; int instrument; };
    std::vector<TrackChannelInstrument> getTrackChannelInstruments() const;

    /**
     * @brief Synchronizes volume changes across all percussion tracks.
     * @param newVolume The new volume value to apply.
     */
    void syncPercussionTracksVolumeChange(double newVolume);

    /**
     * @brief Applies BPM changes globally before playback starts.
     * @param userBPM The new user-defined tempo.
     * @param whenLoad Whether this was triggered during loading.
     */
    void applyBPMchangeBeforePlayback(double userBPM, bool applyStyleChanges=false);

    /**
     * @brief Applies BPM scaling to a specific track only.
     * @param userBPM The new BPM.
     * @param uuid UUID of the target track.
     */
    void applyBPMchangeForOne(double userBPM, const juce::Uuid& uuid);



    //==============================================================================
    /**
     * @brief Handles combo box changes for playback options.
     * @param comboBoxThatHasChanged Pointer to the combo box triggering the event.
     */
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    /**
     * @brief Changes the combo box selection index manually.
     * @param Index The new index to set.
     */
    void comboBoxChangeIndex(int Index);

    bool getIsPlaying();

    void handleIntroCurrentStyle(const juce::String& name, const std::unordered_map<juce::String,StyleSection>& section);

    void handleEndingCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section);

    void handleVarCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section);

    void handleFillCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section);

    void handleBreakCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section);

private:
    //==============================================================================
    /** @brief Handles mouse clicks for selecting tracks. */
    void mouseDown(const juce::MouseEvent& ev) override;

    //==============================================================================
    juce::String name;                                      ///< Name of the current style.
    juce::Label nameOfStyle;                                ///< Label displaying the style name.
    juce::Label selectedTrackLabel, selectedTrackKey, selectedTrackChord; ///< Info labels.
    juce::OwnedArray<Track> allTracks;                      ///< Container for all track components.
    juce::ComboBox playSettingsTracks;                      ///< Combo box for selecting play mode.
    juce::TextButton startPlayingTracks;                    ///< Button to start playback.
    juce::TextButton stopPlayingTracks;                     ///< Button to stop playback.

    std::weak_ptr<juce::MidiOutput> outputDevice;           ///< MIDI output device used for playback.
    std::unique_ptr<MultipleTrackPlayer> trackPlayer = nullptr; ///< Handles multi-track playback.
    std::unique_ptr<ArrangerEngine> arrangerEngine = nullptr;   ///< Arranger-mode looping engine (parallel to trackPlayer).
    bool arrangerModeEnabled = false;                           ///< When true, play/stop route to arrangerEngine.
    bool arrangerBassInversion = false;                         ///< Remembered Bass Inversion; re-applied on each Start.
    bool arrangerNttEnabled = true;                             ///< Remembered NTT master enable; re-applied on each Start.
    MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian; ///< Remembered NTT minor scale; re-applied on each Start.
    bool synchroStartEnabled = false;                           ///< Remembered Synchro Start; re-applied on each Start.
    bool countInEnabled = false;                                ///< Remembered Count-In; re-applied on each Start.

    // --- Phase 3: self-contained style authoring (opened from the play-settings dropdown) ---
    ArrangerStyleListComponent arrangerStyleList;                ///< Full-bounds browser of saved *.style files.
    std::unique_ptr<ArrangerStyleEditor> arrangerStyleEditor;    ///< The configuration being edited (lazy).
    SectionsView  sectionsView { arrangerStyleList };            ///< Hosts the list and the editor in the Style page.
    bool          sectionsOpen = false;

    bool          hasActiveArrangerConfig = false;              ///< When set, Start + section buttons use activeArrangerConfig.
    ArrangerStyle activeArrangerConfig;                         ///< The loaded/saved configuration in effect (vs. a demo from live tracks).
    juce::String  activeArrangerConfigName;                     ///< File name (no ext) of the active config; persisted so the style reopens with it selected.
    int           lastPlayModeId = PlayModeIds::allTracks;      ///< Live-track play mode (all tracks or solo). Survives menu-action picks.

    ArrangerSectionType pendingStartType = ArrangerSectionType::Variation;  ///< Section Start begins on (chosen pre-play).
    juce::String        pendingStartName = DefaultSectionNames::variation;  ///< ...defaults to Variation 1, highlighted while stopped.

    /** Load a .style file and build a runtime style; returns false on failure (showing an error
        unless showError is false, e.g. silent restore during loadJson). */
    bool buildConfigFromFile (const juce::File& styleFile, ArrangerStyle& out, bool showError = true);
    /** Rebuild the play-settings dropdown (adds the active-config row when present) and tick the active entry. */
    void rebuildPlaySettingsItems();

    /** Gather the style's current tracks (instrument/volume synced) to seed the editor. */
    std::vector<TrackEntry> collectSelectedTracks();
    void showStyleList (bool shouldShow);
    void openStyleEditorNew();
    void openStyleEditorFromFile (const juce::File& styleFile);
    void loadStyleFileIntoEngine (const juce::File& styleFile);
    /** Wire the standard editor callbacks (current-tracks / saved / title row) onto arrangerStyleEditor. */
    void configureEditorCallbacks();
    /** Make a loaded/built style the active arranger configuration and refresh the UI selection. */
    void applyActiveConfig (const juce::File& styleFile, const ArrangerStyle& style);
    /** Section-button click in arranger mode: queue the switch if playing, else arm + highlight it as
        the section Start will begin on. */
    void selectOrQueueSection (ArrangerSectionType type, const juce::String& name);
    /** Highlight (and arm on the engine) the section Start will begin on, while stopped. */
    void highlightPendingStartSection();
    void closeStyleEditor();
    /** Re-point the engine's elapsed-beats callback at our beat bar (the editor hijacks it). */
    void restoreEngineBeatBar();
    /** Authoring is a separate mode from live performance and shares the one ArrangerEngine: stop live playback. */
    void stopLivePlayback();
    /** Stop the preview and take the editor off the Sections view (before a new one, or on close). */
    void dropEditor();
    /** Show `arrangerStyleEditor` (just created) in the Sections view. */
    void showEditorInSections();
    std::unordered_map<juce::Uuid, TrackEntry*>& mapUuidToTrackEntry; ///< Global map of all track entries.
    Track* lastSelectedTrack = nullptr;                     ///< Last track selected by the user.
    std::weak_ptr<std::vector<StyleSection>> sections;

    juce::Slider tempoSlider;                               ///< Slider for controlling BPM.
    int oldTempo = ArrangerStyleDefaults::tempoBpm;         ///< Previously stored tempo.
    double currentTempo = ArrangerStyleDefaults::tempoBpm;  ///< Current tempo in BPM.
    double baseTempo = ArrangerStyleDefaults::tempoBpm;     ///< Base tempo for playback ratio.
    bool isPlaying = false;                                 ///< Indicates whether playback is active.
    juce::String styleID;                                   ///< Unique ID of the current style.

    std::unique_ptr<Track> copiedTrack = nullptr;           ///< Track object used for copy/paste operations.
    float scale = StageMetrics::referenceScale;
    int requestingSlot {};
    /** Made on the message thread (with the component): copied by callbacks that may run on the timer
        thread, where making a new SafePointer is not thread-safe. */
    juce::Component::SafePointer<CurrentStyleComponent> selfPointer { this };
    BeatBar customBeatBar;                                  ///< Visual component displaying beat progression.

    std::weak_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> styleSectionsMap;
};
