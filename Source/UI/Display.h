#pragma once
#include <JuceHeader.h>
#include "PlayBackSettingsCustomComponent.h"
#include "TrackEntry.h"
#include "IOHelper.h"
#include "DisplayListener.h"
#include "trackListComponentListener.h"
#include "StyleSection.h"
#include "StylesListComponent.h"
#include "MyTabbedComponent.h"
#include "CurrentStyleComponent.h"
#include "TrackListComponent.h"

/**
 * @class Display
 * @brief Main UI component for managing styles, tracks, and playback settings.
 *
 * The `Display` class handles the visual representation of styles, the currently
 * selected style, and associated tracks. It provides functionality to add, remove,
 * and rename styles, manage track assignments, and interact with playback settings.
 * This class also persists style and track data in JSON format.
 */
class Display : public juce::Component,
                public juce::ChangeListener,
                public TrackListListener
{
public:

    std::function<void(const juce::String& styleID)> loadSettingsOnStyleChange;

    //==============================================================================
    /**
     * @brief Constructor.
     * @param outputDev Pointer to the MIDI output device.
     * @param props Pointer to the application properties file.
     * @param widthForList Width of the style list component.
     */
    Display(std::weak_ptr<juce::MidiOutput> outputDev, juce::PropertiesFile* props, int widthForList = 0);

    /** @brief Destructor */
    ~Display() override;

    /** @brief Callback from `ChangeBroadcaster` */
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    juce::String getStyleID() const;
    juce::String getStyleName() const;

    /**
     * @brief Gets the tab index for a tab by its name.
     * @param name Name of the tab.
     * @return Index of the tab, or -1 if not found.
     */
    int getTabIndexByName(const juce::String& name);

    /** @brief Initializes all default styles in JSON if missing */
    void initializeAllStyles();

    /** @brief Returns all style names currently in JSON */
    std::vector<juce::String> getAllStylesFromJson();

    /** @brief Loads all styles from JSON into memory */
    void loadAllStyles();

    /** @brief Updates the current style in JSON */
    void updateStyleInJson(const juce::String& name);

    /** @brief Updates a style's name in JSON */
    void updateStyleNameInJson(const juce::String& oldName, const juce::String& newName);

    /** @brief Appends a new style to JSON */
    void appendNewStyleInJson(const juce::String& newName);

    /** @brief Removes a style from JSON */
    void removeStyleInJson(const juce::String& name);

    /** @brief Builds a map of Track UUIDs to `TrackEntry` pointers */
    std::unordered_map<juce::Uuid, TrackEntry*> buildTrackUuidMap();

    /** @brief Handles resizing of the component */
    void resized() override;

    /** @brief Returns the root JSON variable representing all styles */
    const juce::var& getJsonVar();

    /** @brief Displays the currently selected style tab */
    void showCurrentStyleTab(const juce::String& name);

    /**
     * @brief Shows a list of tracks to select from and returns the selected track via callback
     * @param onTrackSelected Callback when a track is selected
     */
    void showListOfTracksToSelectFrom(std::function<void(const juce::String&, const juce::Uuid&, const juce::String& type)> onTrackSelected);

    /** @brief Returns a list of available tracks from a folder */
    std::vector<TrackEntry> getAvailableTracksFromFolder(const juce::File& folder);

    /** @brief Sets the MIDI output device */
    void setDeviceOutput(std::weak_ptr<juce::MidiOutput> devOutput);

    void setMidiInjectCallback(std::function<void(const juce::MidiMessage&)> cb);

    /** Enable/disable Arranger mode on the active style component (remembered across re-creation). */
    void setArrangerModeEnabled(bool shouldEnable);

    /** Enable/disable Auto Fill on variation switches (remembered across re-creation). */
    void setArrangerAutoFillEnabled(bool enabled);

    /** Phase 4: forward the live played chord to the style component's arranger engine. */
    void setArrangerLiveChord(const ArrangerChord& chord);

    /** Phase 4: toggle Bass Inversion (slash chords) on the arranger engine. */
    void setArrangerBassInversion(bool shouldInvert);
    void setArrangerNttEnabled(bool enabled);     // Phase 7a: master NTT bypass
    void setArrangerMinorScale(MinorScaleChoice choice);  // Phase 7a: curated minor scale for NTT
    void setArrangerSynchroStart(bool enabled);   // Phase 6: start the groove on the first chord
    void setArrangerCountIn(bool enabled);        // Phase 6b: one metronome bar before the groove

    /** Forwarded from the style component: true while an authoring overlay is shown (host hides the GL note layer). */
    std::function<void(bool)> onArrangerOverlayVisible;

    /** Forwarded from the style component: asks the host to return keyboard focus to the play surface after Start. */
    std::function<void()> onRequestPlayFocus;

    /** Wired host-side to MidiHandler::currentChord; lets the style component seed the arranger
        with an already-held chord at Start. */
    std::function<ArrangerChord()> getHeldChord;

    /** Forwarded from the style component: show/hide the app's "working" overlay during off-thread loads. */
    std::function<void(bool show, const juce::String& text)> onArrangerBusy;

    /** Forwarded from the style component: the arranger's active section changed (or it stopped, idx<0),
        so the host can highlight the matching live section button. */
    std::function<void(int sectionIndex, ArrangerSectionType type, juce::String name)> onArrangerSectionChanged;

    MultipleTrackPlayer* getTrackPlayer();
    std::vector<CurrentStyleComponent::TrackChannelInstrument> getTrackChannelInstruments() const;

    /** @brief Removes a track from all styles */
    void removeTrackFromAllStyles(const juce::Uuid& uuid);

    /** @brief Removes multiple tracks from all styles */
    void removeTracksFromAllStyles(const std::vector<juce::Uuid>& uuids);

    /** @brief Updates a track name in all styles */
    void updateTrackNameFromAllStyles(const juce::Uuid& uuid, const juce::String& newName);

    /** @brief Stops playback */
    void stoppingPlayer();

    /** @brief Starts playback */
    void startingPlayer();

    /** @brief Updates UI before any loading operation */
    void updateUIbeforeAnyLoadingCase() override;

    /** @brief Sets minimum and maximum MIDI note boundaries */
    void set_min_max(int min, int max);

    /** @brief Sets VID and PID for playback settings */
    void set_VID_PID(const juce::String& VID, const juce::String& PID);

    /** @brief Gets the VID of the current input device */
    juce::String getVID();

    /** @brief Gets the PID of the current input device*/
    juce::String getPID();

    /** @brief Reads and updates playback settings from the properties file */
    void readPlaybackSettingsFromProperties();

    /** @brief Handles home button interactions */
    void homeButtonInteraction();

    /** @brief get the start note of playing range */
    int getStartNote();

    /** @brief get the end note of playing range */
    int getEndNote();

    /** @brief get the left max bound of playing range left hand */
    int getLeftBound();

    /** @brief get the right min bound of playing range rigt hand */
    int getRightBound();

    /** @brief adds display listener */
    void addListener(DisplayListener* listener);

    /** @brief removes display listener */
    void removeListener(DisplayListener* listener);

    /** @brief calling the listener's function that they inherit */
    void callingListeners();

    /** @brief get total number of tabs of the tabbed component */
    int getNumTabs();

    /** @brief sets all the settings including min,max note, playing range on left/right hand */
    void setNewSettingsHelperFunction(int value);

    /** @brief Checks if a certain tab exists in the tabbed component*/
    bool existsTab(const juce::String& name);

    void initializeSectionsForStyle(std::unordered_map<juce::String, StyleSection>& sections);

    void handleIntroDisplay(const juce::String& name);

    void handleEndingDisplay(const juce::String& name);

    void handleVarDisplay(const juce::String& name);

    void handleFillDisplay(const juce::String& name);

    void handleBreakDisplay(const juce::String& name);

private:
    //==============================================================================
    juce::HashMap<juce::String, std::unique_ptr<juce::DynamicObject>> styleDataCache; ///< Cached style data
    std::unique_ptr<StylesListComponent> stylesListComponent; ///< UI component for style list
    std::unique_ptr<MyTabbedComponent> tabComp; ///< Tabbed component for styles & tracks
    std::unique_ptr<CurrentStyleComponent> currentStyleComponent; ///< UI for current style
    std::unique_ptr<PlayBackSettingsComponent> playBackSettings; ///< Playback settings UI
    bool created = false; ///< Flag for style tab creation
    bool createdTracksTab = false; ///< Flag for tracks tab creation
    bool createdPlaybackSettingsTab = false;

    int minNote, maxNote;  ///< min and max notes of playing range
    PlayBackSettings settings; ///< Playback settings

    std::shared_ptr<std::deque<TrackEntry>> availableTracksFromFolder; ///< Tracks loaded from folder
    std::shared_ptr<std::unordered_map<juce::String, std::deque<TrackEntry>>> groupedTracks; ///< Tracks grouped by folder
    std::shared_ptr<std::vector<juce::String>> groupedTrackKeys; ///< Keys for grouped tracks

    std::shared_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> sectionsPerStyleMap;

    std::weak_ptr<juce::MidiOutput> outputDevice; ///< MIDI output device
    juce::PropertiesFile* propertiesFile = nullptr; ///< Application properties file
    std::function<void(const juce::MidiMessage&)> pendingMidiInjectCallback;
    bool arrangerModeEnabled = false;   ///< Remembered Arranger-mode state, applied when a style component is created.
    bool arrangerAutoFillEnabled = false;   ///< Remembered Auto Fill state, applied when a style component is created.
    bool arrangerBassInversion = false;     ///< Remembered Bass Inversion, applied when a style component is created.
    bool arrangerNttEnabled = true;         ///< Remembered NTT master enable, applied when a style component is created.
    MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian; ///< Remembered NTT minor scale.
    bool arrangerSynchroStart = false;      ///< Remembered Synchro Start, applied when a style component is created.
    bool arrangerCountIn = false;           ///< Remembered Count-In, applied when a style component is created.

    std::unique_ptr<TrackListComponent> trackListComp; ///< Track selection component
    juce::var allStylesJsonVar; ///< Root JSON object storing all styles
    std::unordered_map<juce::Uuid, TrackEntry*> mapUuidToTrack; ///< Map of track UUIDs to entries

    juce::ListenerList<DisplayListener> displayListeners; ///< Registered listeners
};
