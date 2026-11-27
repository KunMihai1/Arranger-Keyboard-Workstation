/*
  ==============================================================================

    MIDIWindow.h
    Created: 2025
    Author: Kisuke

    The Settings page's content (board StageRevised openPage=settings, spec part 2
    P.2): the MIDI, Arranger and Chords columns. It was the "MIDI Settings"
    window; it keeps its settings keys, callbacks and device polling, and is now
    hosted on the Settings page frame. The Display column's
    controls belong to MainComponent; settingsPageLayout() places all four.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MidiHandler.h"
#include "UI/Stage/StagePageWidgets.h"

namespace OutputEngineIds
{
    constexpr int externalMidi = 1;
    constexpr int internalSfz  = 2;
}

namespace MinorScaleButtons
{
    constexpr int dorian   = 0;
    constexpr int aeolian  = 1;
    constexpr int harmonic = 2;
}

namespace SettingsWindowText
{
    inline const char* const  addDeviceUtf8     = "Add device\xe2\x80\xa6";
    inline const juce::String arrangerMode      = "Arranger mode";
    inline const juce::String bassInversion     = "Bass inversion";
    inline const juce::String fingered          = "Fingered";
    inline const juce::String oneFinger         = "1-Finger";
    inline const juce::String fullKeyboard      = "Full";
    inline const juce::String chordMemory       = "Chord memory";
    inline const juce::String synchroStart      = "Synchro start";
    inline const juce::String countIn           = "Count-in";
    inline const juce::String muteChordKeys     = "Mute chord keys";
    inline const juce::String chordTransposition = "Chord transposition";
    inline const juce::String dorian            = "Dorian";
    inline const juce::String aeolian           = "Aeolian";
    inline const juce::String harmonic          = "Harm.";
}

/** The page's four columns (board, scale 1: 262 wide, gap 32) and the places in them. */
struct SettingsPageRects
{
    static constexpr int columnCount   = 4;
    static constexpr int fieldCount    = 4;   // output engine, MIDI in, MIDI out, audio out
    static constexpr int arrangerRows  = 7;
    static constexpr int choiceCount   = 3;   // a row of three seg buttons
    static constexpr int displayRows   = 3;   // particles, note names, particle colours
    static constexpr int finishRows    = 2;   // Stage / Rosso / Champagne, Tobacco / Midnight / Concert
    static constexpr int finishesPerRow = 3;
    std::array<juce::Rectangle<int>, columnCount> columns, headings;
    std::array<juce::Rectangle<int>, fieldCount> fieldLabels, fields;
    juce::Rectangle<int> keyboardLabel, keyboardRow;
    std::array<juce::Rectangle<int>, arrangerRows> arrangerToggles;
    juce::Rectangle<int> chordModeLabel, chordModeRow, minorScaleLabel, minorScaleRow;
    std::array<juce::Rectangle<int>, displayRows> displayToggles;
    juce::Rectangle<int> particleStyleLabel, particleStyleRow;
    juce::Rectangle<int> finishLabel;
    std::array<juce::Rectangle<int>, finishRows> finishRowAreas;
};
/** The Keyboard field's sizes: 49 / 61 / 76 / 88, plus the stored count when a device was added with another one. */
std::vector<int> keyboardFieldSizes (int storedKeys);

SettingsPageRects settingsPageLayout (juce::Rectangle<int> content, float scale);

class MIDIWindow : public juce::Component,
                   private juce::ComboBox::Listener,
                   private juce::Timer
{
public:
    std::function<bool()> isMidiDeviceOpen;
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

    /** Keyboard field: the key count stored for a MIDI-in device (0 = unknown device / PC keyboard), saving a
        new one, and adding an unknown device (today's Add MIDI Device window). Set by the owner. */
    std::function<int(const juce::String& deviceName)> getKeyCount;
    std::function<void(const juce::String& deviceName, int keys)> setKeyCount;
    std::function<void(const juce::String& deviceName)> onAddDevice;

    MIDIWindow(MidiDevice& mdevice, std::vector<std::string>& devicesListIN,
               std::vector<std::string>& devicesListOUT, std::vector<std::string>& devicesListAudioOUT, juce::PropertiesFile* prop);
    ~MIDIWindow() override;

    /** The page (or the home-screen window) became visible / hidden: refresh the device lists and poll them while shown. */
    void pageShown();
    void pageHidden();
    /** The key count of the MIDI-in device changed elsewhere (e.g. a device was added). */
    void refreshKeyboardField();

    void setScale(float newScale);
    void paint(juce::Graphics& g) override;
    void resized() override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void timerCallback() override;

private:
    void allInit();
    void populateCBIN();
    void populateCBOUT();
    void populateCBengine();
    void populateCBaudioOUT();
    void restoreCBoxes();
    void switchBetweenEngineOptions(int selectedId);
    void refreshChoiceLeds();

    MidiDevice& MIDIDevice;                   /**< Reference to the MIDI device object */
    std::vector<std::string>& devicesListIN; /**< Vector of available MIDI input devices */
    std::vector<std::string>& devicesListOUT;/**< Vector of available MIDI output devices */
    std::vector<std::string>& devicesListAudioOUT;/**< Vector of available audio output devices */
    juce::PropertiesFile* propertyFile = nullptr; /**< Optional storage for user settings */


    StageField comboBoxDevicesIN;         /**< MIDI input devices */
    StageField comboBoxDevicesOUT;        /**< MIDI output devices */
    StageField comboBoxOutputEngineType;  /**< audio output engine type */
    StageField comboBoxAudioDevicesOUT;   /**< audio output devices */
    StageField keyboardField;             /**< key count of the MIDI-in device */
    SegButton  addDeviceButton { juce::String::fromUTF8 (SettingsWindowText::addDeviceUtf8), SegButton::Style::seg };

    RowToggle arrangerModeToggle        { SettingsWindowText::arrangerMode };
    RowToggle chordBassInversionToggle  { SettingsWindowText::bassInversion };
    RowToggle chordMemoryToggle         { SettingsWindowText::chordMemory };
    RowToggle synchroStartToggle        { SettingsWindowText::synchroStart };
    RowToggle countInToggle             { SettingsWindowText::countIn };
    RowToggle chordZoneMuteToggle       { SettingsWindowText::muteChordKeys };
    RowToggle nttEnabledToggle          { SettingsWindowText::chordTransposition };

    SegButton chordModeFingered { SettingsWindowText::fingered, SegButton::Style::seg, true };
    SegButton chordModeSingle   { SettingsWindowText::oneFinger, SegButton::Style::seg, true };
    SegButton chordModeFull     { SettingsWindowText::fullKeyboard, SegButton::Style::seg, true };
    SegButton nttMinorDorian    { SettingsWindowText::dorian, SegButton::Style::seg, true };
    SegButton nttMinorAeolian   { SettingsWindowText::aeolian, SegButton::Style::seg, true };
    SegButton nttMinorHarmonic  { SettingsWindowText::harmonic, SegButton::Style::seg, true };

    SilkLabel outputEngineTypeLabel, midiDevicesLabelIN, midiDevicesLabelOUT, audioDevicesLabelOUT,
              keyboardLabel, chordModeLabel, nttMinorScaleLabel;

    static constexpr int firstItemId = 1;   ///< ComboBox item ids start at 1
    int lastIndexIN = firstItemId;                       /**< Last selected input device index */
    int lastIndexOUT = firstItemId;                      /**< Last selected output device index */
    int lastIndexEngineOption = firstItemId;
    int lastIndexAudioOut = firstItemId;
    float scale = StageMetrics::referenceScale;
};
