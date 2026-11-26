#pragma once
#include <JuceHeader.h>
#include "PlayBackSettingsCustomComponent.h"
#include "TrackEntry.h"
#include "IOHelper.h"
#include "DisplayListener.h"
#include "trackListComponentListener.h"
#include "StyleSection.h"
#include "StylesListComponent.h"
#include "UI/Stage/StageModels.h"
#include "UI/Stage/StagePageWidgets.h"
#include "CurrentStyleComponent.h"
#include "TrackListComponent.h"
#include "InstrumentPicker.h"

/**
 * @class Display
 * @brief Main UI component for managing styles, tracks, and playback settings.
 *
 * The `Display` class handles the visual representation of styles, the currently
 * selected style, and associated tracks. It provides functionality to add, remove,
 * and rename styles, manage track assignments, and interact with playback settings.
 * This class also persists style and track data in JSON format.
 */
/** The Style page's text (board StageRevised openPage=styles). */
namespace StylePageText
{
    inline const juce::String title         = "Style";
    inline const juce::String play          = "Play";
    inline const juce::String allTracks     = "All tracks";
    inline const juce::String solo          = "Solo";
    inline const char* const  sectionsUtf8      = "Sections\xe2\x80\xa6";         // + U+2026 ellipsis
    inline const char* const  styleSettingsUtf8 = "Style settings\xe2\x80\xa6";
    inline const juce::String tracksTitle   = "Choose track";
    inline const juce::String instrumentsTitle = "Choose instrument";
    inline const char* const  slotSeparatorUtf8 = " \xc2\xb7 slot ";   // "Choose track · slot 3"
    inline const juce::String tracks        = "Tracks";
    inline const juce::String openAStyle    = "Open a style on the left";
}

/** The Style page's two columns: the styles list and the tracks area (board: 230, gap 24, the rest). */
class TableContainer;

struct StylePageRects
{
    juce::Rectangle<int> stylesColumn, tracksArea;
};
StylePageRects stylePageLayout (juce::Rectangle<int> content, float scale);

class Display : public juce::Component,
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
    Display(std::weak_ptr<juce::MidiOutput> outputDev, juce::PropertiesFile* props, int widthForList = {});

    /** @brief Destructor */
    ~Display() override;


    juce::String getStyleID() const;
    juce::String getStyleName() const;


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
    void paint(juce::Graphics& g) override;

    //==============================================================================
    // The Style page (board StageRevised openPage=styles; DisplayViews.cpp). The page frame shows the
    // view's title, subtitle and title-row actions; Exit / Esc call closeSubView() first.
    void setScale(float newScale);
    StyleView getView() const;
    void showView(StyleView view);
    /** Back to the style from the track picker, notes or style settings (they are destroyed). */
    void closeSubView();
    /** The Styles page closed: forget any sub-view. */
    void resetView();
    /** The "Choose instrument - Slot N" sub-page; a pick reports the GM program, then the Style page returns. */
    void showInstrumentPicker(int slot, int currentProgram, std::function<void(int program)> onChosen);
    juce::String getViewTitle() const;
    juce::String getViewSubtitle() const;
    std::vector<juce::Component*> getTitleActions();
    /** The view, its title or its actions changed: the host updates the page frame and the screen. */
    std::function<void()> onViewChanged;
    /** A short message for the Stage screen. */
    std::function<void(const juce::String& text)> onInfo;
    /** "Style settings..." (and the style's keybind for it): the host opens the Voices page. */
    std::function<void()> onStyleSettings;
    /** The style's range, split and transpose fields; the Voices page shows them. Created once. */
    PlayBackSettingsComponent& getPlaybackSettings();
    /** A style track's instrument was chosen on its strip (channel, GM program). */
    std::function<void(int channel, int program)> onTrackInstrumentChosen;

    /** @brief Returns the root JSON variable representing all styles */
    const juce::var& getJsonVar();

    /** @brief Displays the currently selected style tab */
    void showCurrentStyleTab(const juce::String& name);

    /**
     * @brief Shows a list of tracks to select from and returns the selected track via callback
     * @param onTrackSelected Callback when a track is selected
     */
    void showListOfTracksToSelectFrom(TrackChosenCallback onTrackSelected);

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

    // --- The open style, for the Stage panel (DisplayStage.cpp) ---
    bool   hasOpenStyle() const;
    void   startOpenStyle();                       ///< the style tab's Start; nothing without an open style
    void   stopOpenStyle();                        ///< the style tab's Stop
    bool   isOpenStylePlaying();
    bool   isOpenStyleInArrangerMode() const;
    double getOpenStyleTempo();
    double getOpenStyleBaseTempo();                ///< the style's own tempo (right-click on the screen tempo)
    void   commitOpenStyleTempo(double tempo);     ///< as if the BPM slider were released at this tempo

    /** Every beat position of the open style's beat bar, with its beats per bar (message thread). */
    std::function<void(double elapsedBeats, int beatsPerBar)> onBeatPosition;

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

    /** @brief sets all the settings including min,max note, playing range on left/right hand */
    void setNewSettingsHelperFunction(int value);

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
    std::unique_ptr<CurrentStyleComponent> currentStyleComponent; ///< UI for current style
    std::unique_ptr<PlayBackSettingsComponent> playBackSettings; ///< Playback settings UI

    // --- The Style page (DisplayViews.cpp) ---
    void destroySubView();
    void layoutViews();
    void refreshTitleActions();
    void viewChanged();
    StyleViewRouter views;
    float scale = StageMetrics::referenceScale;
    SilkLabel    playLabel      { StylePageText::play, SilkLabel::Tone::dim };
    SegButton    allTracksButton { StylePageText::allTracks, SegButton::Style::seg, true };
    SegButton    soloButton     { StylePageText::solo, SegButton::Style::seg, true };
    TitleDivider playDivider;
    SegButton    sectionsButton { juce::String::fromUTF8 (StylePageText::sectionsUtf8), SegButton::Style::seg };
    SegButton    styleSettingsButton { juce::String::fromUTF8 (StylePageText::styleSettingsUtf8), SegButton::Style::seg };

    int minNote, maxNote;  ///< min and max notes of playing range (set_min_max)
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
    juce::String lastTrackFolder;                       ///< the track picker's last opened folder (reopened next time)
    juce::var allStylesJsonVar; ///< Root JSON object storing all styles
    std::unordered_map<juce::Uuid, TrackEntry*> mapUuidToTrack; ///< Map of track UUIDs to entries

    juce::ListenerList<DisplayListener> displayListeners; ///< Registered listeners

    // The notes page of a track (board PageNotes). Declared last: it is destroyed first, before the style
    // component whose player it listens to.
    void showNotes(std::unique_ptr<TableContainer> notes, const juce::String& title, const juce::String& subtitle);
    std::unique_ptr<TableContainer> notesContainer;
    std::unique_ptr<InstrumentPicker> instrumentPicker;
    int instrumentSlot {};
    juce::String notesTitle, notesSubtitle;
};
