/*
  ==============================================================================

    MainComponent.h
    Created: 29 Oct 2025
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cstdlib>
#include "settingsWindow.h"
#include "BinaryData.h"
#include "MidiHandler.h"
#include "KeyboardUI.h"
#include "NoteLayer.h"
#include "keyListener.h"
#include "temporaryNotificationUI.h"
#include "InstrumentTreeItem.h"
#include "MidiRecordPlayer.h"
#include "displayGUI.h"
#include "addDeviceWindow.h"
#include "SectionsComponent.h"
#include "OverlayComponent.h"
#include "chordBrowser.h"
#include "LoginComponent.h"
#include "PlaytimeTracker.h"
#include "SoundEffectWindowComponent.h"
#include "styleSettingsEntry.h"
#include "SFZLibraryUI.h"
#include "AudioHandler.h"
#include "PlayScreenLookAndFeel.h"
/**
 * @class SmoothRotarySlider
 * @brief Custom rotary slider with smooth dragging behavior.
 *
 * Overrides mouse drag behavior to allow fine adjustments.
 */
class SmoothRotarySlider : public juce::Slider
{
public:
    /** @brief Constructor sets rotary style */
    SmoothRotarySlider();

    /** @brief Handles mouse down to store starting value and position */
    void mouseDown(const juce::MouseEvent& e) override;

    /** @brief Handles mouse drag for smooth value changes */
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    float dragStartValue = 0.0f;          ///< Slider value at drag start
    juce::Point<float> dragStartPos;      ///< Mouse position at drag start
};

/**
 * @class KnobLookAndFeel
 * @brief Custom LookAndFeel for rotary knob using an image.
 *
 * Draws a knob from BinaryData image and rotates it according to slider value.
 */
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /** @brief Constructor loads knob image and sets angles */
    KnobLookAndFeel();

    /**
     * @brief Draws the rotary slider
     * @param g Graphics context
     * @param x X position
     * @param y Y position
     * @param width Width of slider
     * @param height Height of slider
     * @param sliderPosProportional Proportional slider position [0..1]
     * @param rotaryStart Start angle (ignored, using internal angles)
     * @param rotaryEnd End angle (ignored, using internal angles)
     * @param slider Reference to slider
     */
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStart, float rotaryEnd,
                          juce::Slider& slider) override;

private:
    juce::Image rawKnobImage; ///< Knob image
    float rotaryStartAngle;   ///< Start angle of rotation
    float rotaryEndAngle;     ///< End angle of rotation
};

/**
 * @class CustomLookAndFeel
 * @brief LookAndFeel customization for buttons and text.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /** @brief Returns font for text buttons */
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

    /**
     * @brief Draws button background
     * @param g Graphics context
     * @param button Button reference
     * @param backgroundColour Button background color
     * @param isMouseOverButton True if mouse is over button
     * @param isButtonDown True if button is pressed
     */
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;
};

/**
 * @class MainComponent
 * @brief Main UI component containing all controls and displays.
 *
 * Handles MIDI input/output, keyboard UI, instrument selection, recording/playback,
 * and all window controls.
 */
class MainComponent : public juce::Component,
    public juce::ChangeListener,
    public juce::KeyListener,
    public juce::Timer,
    public juce::ComboBox::Listener,
    public juce::FocusChangeListener
{
public:
    /** @brief Constructor initializes UI and MIDI */
    MainComponent();

    /** @brief Destructor */
    ~MainComponent() override;

    /** @brief Paints the main window */
    void paint(juce::Graphics& g) override;

    /** @brief Paints loading overlay on top of all children */
    void paintOverChildren(juce::Graphics& g) override;

    void focusGained(juce::Component::FocusChangeType) override;

    void globalFocusChanged(juce::Component* focusedComponent) override;

    /** @brief Handles component resizing */
    void resized() override;

    /** @brief Handles key presses
     *  @param key Pressed key
     *  @param component Component that received the key
     */
    bool keyPressed(const juce::KeyPress& key, juce::Component* component) override;

    /** @brief Reclaims keyboard focus on any non-text click so the PC keyboard
     *         can always keep playing after touching a button/pad/config browser. */
    void mouseDown(const juce::MouseEvent& e) override;

    /** @brief Callback for change events */
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /** @brief Timer callback for periodic updates */
    void timerCallback() override;

    /** @brief Checks the open MIDI input AND output devices; reacts if either was unplugged. */
    void checkMidiDevicesValid();

    /** @brief Output device vanished mid-session: release it and stop the transport cleanly. */
    void handleMidiOutputDisconnected();

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    void populateUpdateComboBoxDevices();

private:



    int initialWidth = 0;  ///< Initial window width
    int initialHeight = 0; ///< Initial window height

    int xPlay = 750;       ///< X coordinate of play button
    int yPlay = 690;       ///< Y coordinate of play button

    bool hasBeenResized = false; ///< True if window has been resized

    /** @brief Initializes save file for the user */
    void initalizeSaveFileForUser();

    std::unique_ptr<LoginComponent> loginWindow=nullptr;

    void loginWindowInitialize();


    /** @brief Loads settings from disk */

    void applySettingsToChannel(const SoundSettings& s, int channel);

    juce::String applyEffectCC(int ccNumber, int value, int channel);

    void getCurrentEffectDeviceIds(juce::String& VID, juce::String& PID);

    juce::String getCurrentEffectSettingsKey(const juce::String& styleID);

    void loadEffectsFromFile(const juce::String& styleID="style_default");

    void loadSettings(const juce::String& styleID="style_default");

    void playChordOnClick(const Chord& c);

    void buildChordLibrary();

    void setCallBacksForEffectWindow();

    void setCallBacksForOverlayWindow();

    void setInitialValuesFromSettingsFileEffectWindow();

    void sendEffectsBeforePlaying();
    void disconnectCurrentOutput();
    void ensureAudioHandlerReady();
    void loadSfzForCurrentStyle(const juce::String& styleId = {});
    void applyCurrentStyleToOutput();

    /** @brief Names the SFZ slot feeding a MIDI channel, in the same terms the assign dialog uses.

        Channels are only how the audio engine routes sound; SFZ files are mapped per
        style + instrument. This reverses the routing done by loadSfzForCurrentStyle so a
        failure can be reported as "Violin (right hand)" rather than "channel 16".
        @param midiChannel Channel 1-16.
        @return The instrument name and its role, or "channel <n>" if the channel is unmapped. */
    juce::String describeSfzSlot(int midiChannel);

    // UI toggles
    void toggleSettingsPanel();
    void toggleSettingsButton();
    void toggleMIDIButton();
    void togglePlayButton();
    void toggleMIDIsettingsIcon();
    void toggleHPanel();
    void toggleHomeButton();
    void toggleForPlaying();
    void toggleColourSelectorButton();
    void toggleInstrumentSelectorButton();
    void toggleRecordButtons();
    void toggleSaveRecordingButton();
    void togglePlayRecordingButton();
    void toggleKnobs();
    void toggleDisplay();
    void toggleParticleToggle();
    void toggleHandInstrumentToggle();
    void toggleUpdateKeysButton();
    void toggleDevicesCBUpdate();
    void toggleSections();
    void toggleAnnotation();
    void toggleChordHelperButton();


    // Initialization functions
    void settingsInit();
    void playButtonInit();
    void keyBoardUIinit(int min, int max);
    void panelInit();
    void midiIconInit();
    void midiSettingsInit();
    void headerPanelInit();
    void homeButtonInit();
    void colourSelectorButtonInit();
    void instrumentSelectorButtonInit();
    void recordButtonsInit();
    void saveRecordingButtonInit();
    void playRecordingButtonInit();
    void knobsInit();
    void displayInit();
    void toggleButtonInit();
    void toggleHandButtonsInit();
    void updateKeysButtonInit();
    void devicesCBUpdateInit();
    void sectionsInit();
    void annotationInit();
    void chordHelperButtonInit();
   

    // Button click callbacks
    void settingsButtonOnClick();
    void midiButtonOnClick();
    void homeButtonOnClick();
    void playButtonOnClick();
    void updateKeysButtonOnClick();

    // Instrument/UI helpers
    juce::Image getImageForInstruments(const std::string& type);
    void buildTree();
    InstrumentTreeItem* createInstrumentItem(const juce::Image& img, const juce::String& name, int program);
    bool openingDevicesForPlay();
    void showColourSelector();
    void showInstrumentSelector();
    void saveRecordingToFile(double tempo = 120.0);
    void playRecordingFromFile(double tempo = 120.0);

    void handleIntro(const juce::String& name);

    void handleEnding(const juce::String& name);

    void handleVar(const juce::String& name);

    void handleFill(const juce::String& name);

    void handleBreak(const juce::String& name);

    void initializeOverlay();

    /** Show/hide the centered loading label. While shown, the OpenGL note layer is hidden so the
        label isn't covered by it (the GL layer renders on top of normal painting), then restored. */
    void setLoadingOverlayVisible(bool show, const juce::String& text = {});

    /** Show/hide the in-app ESC menu overlay. While shown, the note layer + keyboard are hidden so
        the overlay (painted the normal way) is on top of the GL layer, then restored on close. */
    void setOverlayMenuVisible(bool show);

    /** Hide/restore the GL note layer while an arranger authoring overlay (style browser/editor) is
        shown — otherwise the GL layer renders through the middle of that full-screen overlay. */
    void setArrangerOverlaySceneHidden(bool hidden);

    /** Highlight the live section button matching the arranger's active section (idx<0 clears all).
        Driven by the engine so the lit button always matches what is actually sounding. */
    void highlightArrangerSection(int sectionIndex, ArrangerSectionType type, const juce::String& name);

    /** Switch the section buttons between engine-driven highlight (arranger mode) and the classic
        click-toggle highlight. Clears any highlight when turning engine-driven off. */
    void setSectionButtonsEngineDriven(bool engineDriven);

    //==========================================================================
    // Member variables
    juce::ApplicationProperties appProperties; ///< Application properties manager
    juce::PropertiesFile* propertiesFile = nullptr; ///< Pointer to properties file

    CustomLookAndFeel customLookAndFeel; ///< Custom look and feel
    PlayScreenLookAndFeel playScreenLAF;
    juce::Image cachedImageMainWindow;   ///< Cached main window image
    juce::Image playBackground;          ///< Background when playing
    juce::Image currentBackground;       ///< Currently displayed background
    juce::Label helpIcon;                ///< Help icon label
    juce::Label openingAudioLabel;       ///< Loading overlay label
    bool pendingPlayInit = false;        ///< Set on Play click; the blocking init runs once the overlay has actually painted
    juce::TooltipWindow tooltipWindow{ this, 200 }; ///< Tooltip manager

    std::weak_ptr<juce::MidiInput> deviceOpenedIN; ///< Currently opened MIDI input device
    std::weak_ptr<juce::MidiOutput> deviceOpenedOUT; ///< Currently opened MIDI output device

    
    MidiDevice MIDIDevice{};       ///< MIDI device instance

    /// Fires (on the message thread) the instant the MIDI device list changes, so a hot-unplug is
    /// caught immediately rather than up to a second later on the poll timer. See checkMidiDevicesValid().
    juce::MidiDeviceListConnection midiDeviceListConnection;

    InstrumentHandler instrumentHandler;
    MidiHandler midiHandler{ MIDIDevice, &instrumentHandler }; ///< MIDI event handler
    KeyboardListener keyListener{ midiHandler }; ///< Keyboard listener
    MidiDevicesDataBase dataBase{};
    MidiRecordPlayer recordPlayer{}; ///< MIDI recording/playback

    std::vector<std::string> devicesIN;  ///< List of input devices
    std::vector<std::string> devicesOUT; ///< List of output devices
	std::vector<std::string> devicesAudioOUT; ///< List of audio output devices

    int last=-1, lastOfLast=-1;

    bool midiWindowShouldBeVisible = false;
    bool overlayShouldBeVisible = false;
    bool soundEffecttWindowShouldBeVisible = false;

    // Saved scene-layer visibility so overlays can hide the GL note layer (+ keyboard) and restore it.
    bool loadingOverlayActive = false;          ///< Loading label currently using the hide-note-layer path
    bool noteLayerVisibleBeforeLabel = false;   ///< Note-layer visibility to restore after the loading label
    bool noteLayerVisibleBeforeMenu = false;    ///< Note-layer visibility to restore after the ESC menu
    bool keyboardVisibleBeforeMenu = false;     ///< Keyboard visibility to restore after the ESC menu
    bool sceneHiddenForArrangerOverlay = false; ///< True while the arranger authoring overlay hides the note layer
    bool noteLayerVisibleBeforeArranger = false;///< Note-layer visibility to restore after the arranger overlay
    // UI buttons and toggles
    juce::TextButton settingsButton{ "Settings" };
    juce::TextButton midiButton{ "MIDI Settings" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton homeButton{ "Home" };
    juce::ToggleButton particleToggle;
    juce::TextButton colourSelectorButton{ "Select colour" };
    juce::TextButton instrumentSelectorButton{ "Select instrument" };
    juce::TextButton saveRecordingButton{ "Save recording" };
    juce::TextButton playRecordingFileButton{ "Play recording file" };
    juce::ToggleButton leftHandInstrumentToggle, rightHandInstrumentToggle;
    juce::ToggleButton noteNumbersAnnotation;
    juce::TextButton chordHelperButton{ "Chord helper" };
    std::unique_ptr<StyleSectionComponent> introsEndings, variationsFills;


    std::unordered_map<juce::String, juce::String> updateDevicesMap;
    std::unordered_map<juce::String, StyleSettings> styleSettingsMap;

    juce::TextButton updateNumberOfKeysDevice{"Update number of keys"};
    juce::ComboBox devicesCBUpdate;

    juce::DrawableButton startRecording{ "Record", juce::DrawableButton::ImageFitted };
    juce::DrawableButton stopRecording{ "Stop", juce::DrawableButton::ImageFitted };
    juce::DrawableButton startPlayback{ "Play", juce::DrawableButton::ImageFitted };
    std::unique_ptr<juce::Drawable> recordDrawable, stopDrawable, playDrawable;
    juce::Slider volumeKnob, reverbKnob;
    KnobLookAndFeel customKnobLookAndFeel;

    juce::ColourSelector* colourSelector; ///< Colour selector pointer
    std::unique_ptr<juce::TreeView> treeView = nullptr; ///< Instrument tree

    // UI windows
    std::unique_ptr<MIDIWindow> midiWindow = nullptr;
    KeyboardUI keyboard{ midiHandler };
    std::unique_ptr<NoteLayer> noteLayer;
    bool keyboardInitialized = false; ///< True if keyboard is initialized
    bool usingKeyboardInput = false;  ///< True if using keyboard input
    std::unique_ptr<TemporaryMessage> temporaryPopup;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<Display> display = nullptr;
    std::unique_ptr<AddDeviceWindow> addDeviceWindow=nullptr;
    std::unique_ptr<OverlayComponent> overlayWindow=nullptr;
    std::unique_ptr<SoundEffectWindow> soundEffectWindow = nullptr;
    
    std::vector<Chord> myChordLibrary;
    std::vector<int> currentlyPlayingNotes;

    std::unique_ptr<PlaytimeTracker> playtimeTracker;
    std::shared_ptr<SupabaseClient> client;

    SFZLibraryManager sfzManager;
    std::unique_ptr<SFZLibraryUI> sfzLibraryUI;
    std::unique_ptr<AudioHandler> audioHandler;

    //==========================================================================
    // Panels
    struct Panel : public juce::Component
    {
        /** @brief Paints panel background and borders */
        void paint(juce::Graphics& g) override;
    };
    Panel settingsPanel;

    struct headerPanel : public juce::Component
    {
        /** @brief Paints header gradient */
        void paint(juce::Graphics& g) override;
    };
    headerPanel headerPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
