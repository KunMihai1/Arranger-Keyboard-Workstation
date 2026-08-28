/*
  ==============================================================================

    MIDIWindow.h
    Created: 2025
    Author: Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "MidiHandler.h"
#include "SFZLibraryUI.h"

/**
 * @class MIDIWindow
 * @brief Window for configuring MIDI input/output devices.
 *
 * Provides a user interface for:
 *   - Selecting MIDI input and output devices
 *   - Storing and restoring user settings
 *   - Automatically updating device lists when changes occur
 *
 * Inherits from juce::DocumentWindow and implements ComboBox::Listener and Timer.
 */
class MIDIWindow : public juce::DocumentWindow,
                   private juce::ComboBox::Listener,
                   private juce::Timer,
                   public juce::KeyListener
{
public:

    std::function<bool()> isMidiDeviceOpen;
    std::function<void()> onWindowClosed;
    std::function<juce::String()> getCurrentStyleId;  // optional, set by owner
    std::function<juce::String()> getCurrentStyleName; // optional, set by owner
    std::function<void(int engineOption)> onOutputEngineChanged; // fired when engine changes while playing
    std::function<void(bool)> onArrangerModeChanged;             // fired when the Classic/Arranger toggle changes
    std::function<void(bool)> onChordBassInversionChanged;       // Phase 4: bass follows the lowest fingered note
    std::function<void(ChordMode)> onChordModeChanged;           // Phase 5: Fingered / Single-Finger / Full-Keyboard
    std::function<void(bool)> onChordMemoryChanged;              // Phase 4: hold last chord when the zone empties
    std::function<void(bool)> onSynchroStartChanged;            // Phase 6: start the groove on the first chord
    std::function<void(bool)> onCountInChanged;                 // Phase 6b: one metronome bar before the groove
    std::function<void(bool)> onChordZoneMuteChanged;          // silence the chord-recognition keys (Korg-style)
    std::function<void(bool)> onNttEnabledChanged;             // Phase 7a: master Chord-Transposition on/off
    std::function<void(int)>  onNttMinorScaleChanged;         // Phase 7a: 0=Dorian, 1=Aeolian, 2=HarmonicMinor
    std::function<void()> onSfzLibraryChanged;                  // fired when SFZ mappings are imported/changed

    /**
     * @brief Constructor.
     * @param mdevice Reference to the MIDI device object.
     * @param devicesListIN Reference to a vector containing available MIDI input devices.
     * @param devicesListOUT Reference to a vector containing available MIDI output devices.
     * @param prop Optional pointer to a JUCE PropertiesFile for storing settings.
     *
     * Initializes the window, its sliders, ComboBoxes, and panel, and sets default visibility.
     */
    MIDIWindow(MidiDevice& mdevice, std::vector<std::string>& devicesListIN,
               std::vector<std::string>& devicesListOUT, std::vector<std::string>& devicesListAudioOUT, juce::PropertiesFile* prop, SFZLibraryManager* sfzManagerPtr = nullptr);

    /** @brief Destructor stops the update timer and cleans up. */
    ~MIDIWindow() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;


    /** @brief Called when the window visibility changes. */
    void visibilityChanged() override;

    /** @brief Called when the window close button is pressed. */
    void closeButtonPressed() override;

    /** @brief Handles ComboBox selection changes for MIDI devices. */
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    /** @brief Timer callback to update device lists if they change. */
    void timerCallback() override;

private:

    /** @brief Toggles visibility of the settings panel. */
    void toggleSettingsPanel();

    /** @brief Toggles visibility of device ComboBoxes. */
    void toggleSettingsCB();

	void toggleSettingsInMIDICB();

	void toggleSettingsOutMIDICB();

	void toggleSettingsOutputEngineCB();

	void toggleSettingsAudioOutCB();

	void toggleSFZbutton();

    /** @brief Toggles visibility of all settings components. */
    void toggleSettingsAll();

    /** @brief Sets bounds of all window components manually. */
    void setBounds_components();

    /** @brief Initializes the main panel. */
    void panelInit();

    /** @brief Initializes device ComboBoxes. */
    void devicesCBinit();

	void deviceInMIDICBinit();

	void deviceOutMIDICBinit();

	void outputEngineCBinit();

	void deviceAudioOutCBinit();

    void sfzButtonInit();

    /** @brief Calls all initialization functions. */
    void allInit();

    /** @brief Populates the input device ComboBox. */
    void populateCBIN();

    /** @brief Populates the output device ComboBox. */
    void populateCBOUT();

    void populateCBengine();

	void populateCBaudioOUT();

    /** @brief Restores ComboBox selections from previously saved indices. */
    void restoreCBoxes();

	void static setStyleComboBox(juce::ComboBox& comboBox);
    
	void static setStyleLabel(juce::Label& label);
    
	void switchBetweenEngineOptions(int selectedId);

    // Member variables
    MidiDevice& MIDIDevice;                   /**< Reference to the MIDI device object */
    std::vector<std::string>& devicesListIN; /**< Vector of available MIDI input devices */
    std::vector<std::string>& devicesListOUT;/**< Vector of available MIDI output devices */
	std::vector<std::string>& devicesListAudioOUT;/**< Vector of available audio output devices */
    juce::PropertiesFile* propertyFile = nullptr; /**< Optional storage for user settings */

    juce::Component settingsPanel;            /**< Panel containing all settings components */
	juce::TextButton sfzLibraryButton{ "SFZ Library" };
    SFZLibraryManager* sfzManager = nullptr;
    std::unique_ptr<SFZLibraryUI> sfzLibraryUI;

    juce::ComboBox comboBoxDevicesIN;         /**< ComboBox for selecting MIDI input devices */
    juce::ComboBox comboBoxDevicesOUT;        /**< ComboBox for selecting MIDI output devices */
    juce::ComboBox comboBoxOutputEngineType;  /**< ComboBox for selecting audio output engine type */
    juce::ToggleButton arrangerModeToggle { "Arranger mode (loop accompaniment)" }; /**< Classic vs Arranger playback */
    juce::ToggleButton chordBassInversionToggle { "Bass inversion (slash chords)" };   /**< Phase 4 */
    juce::Label        chordModeLabel;                                                  /**< Phase 5 */
    juce::TextButton   chordModeFingered { "Fingered" };                                /**< Phase 5 */
    juce::TextButton   chordModeSingle   { "1-Finger" };                                /**< Phase 5 */
    juce::TextButton   chordModeFull     { "Full-Kbd" };                                /**< Phase 5 */
    juce::ToggleButton chordMemoryToggle        { "Chord memory (hold on release)" };   /**< Phase 4 */
    juce::ToggleButton synchroStartToggle       { "Synchro Start (begin on first chord)" }; /**< Phase 6 */
    juce::ToggleButton countInToggle            { "Count-In (1 bar before start)" };        /**< Phase 6b */
    juce::ToggleButton chordZoneMuteToggle      { "Mute chord keys (hear only the backing)" };
    juce::ToggleButton nttEnabledToggle         { "Chord transposition (NTT)" };                 /**< Phase 7a master */
    juce::Label        nttMinorScaleLabel;                                                        /**< Phase 7a */
    juce::TextButton   nttMinorDorian   { "Dorian" };                                             /**< Phase 7a */
    juce::TextButton   nttMinorAeolian  { "Aeolian" };                                            /**< Phase 7a */
    juce::TextButton   nttMinorHarmonic { "Harmonic" };                                           /**< Phase 7a */
	juce::ComboBox comboBoxAudioDevicesOUT;       /**< ComboBox for selecting audio output devices */
    juce::Label midiDevicesLabelIN;           /**< Label for input devices */
    juce::Label midiDevicesLabelOUT;          /**< Label for output devices */
	juce::Label outputEngineTypeLabel;        /**< Label for audio output engine type */
	juce::Label audioDevicesLabelOUT;        /**< Label for audio output devices */
    int lastIndexIN = 1;                       /**< Last selected input device index */
    int lastIndexOUT = 1;                      /**< Last selected output device index */
    int lastIndexEngineOption = 1;
	int lastIndexAudioOut = 1;
};
