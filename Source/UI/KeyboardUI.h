/*
  ==============================================================================

    NoteLayer.h
    Created: 20 May 2025 1:20:49am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "MidiHandler.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>
#include <vector>

/**
 * @class KeyboardUI
 * @brief Visual representation of a piano keyboard that responds to MIDI events.
 *
 * Displays active notes and supports a configurable visible range. 
 * Implements MidiHandlerListener to receive note-on and note-off events.
 */
class KeyboardUI : public juce::Component, public MidiHandlerListener
{
public:


    /**
     * @brief Constructor
     * @param midiHandler Reference to MidiHandler for receiving events
     */
    KeyboardUI(MidiHandler& midiHandler);

    /** @brief Destructor */
    ~KeyboardUI();

    /**
     * @brief Paints the keyboard
     * @param g Graphics context
     */
    void paint(juce::Graphics& g) override;

    void resized() override;

    /**
     * @brief Sets the minimum and maximum MIDI notes to display
     * @param min Minimum MIDI note
     * @param max Maximum MIDI note
     */
    void set_min_and_max(const int min, const int max);

    /** @brief Returns the current minimum note being displayed */
    int get_min();

    /** @brief Returns the current maximum note being displayed */
    int get_max();

    /** @brief Resets all notes to inactive state */
    void resetStateActiveNotes();

    void resetStateActiveNotesNoExplicitRepaint();

    /**
     * @brief Receives note-on events from MidiHandler
     * @param midiNote MIDI note number
     */
    void noteOnReceived(int midiNote) override;

    /**
     * @brief Receives note-off events from MidiHandler
     * @param midiNote MIDI note number
     */
    void noteOffReceived(int midiNote) override;

    /**
     * @brief Sets whether the keyboard has been drawn or not
     * @param state True if the keyboard has been drawn
     */
    void setIsDrawn(bool state);

    void setAnnotationState(bool newState);


private:
    friend class NoteLayer;

    /**
     * @brief Handles the drawing of keys including active state and coloring
     * @param g Graphics context
     */
    void paintKeyboard(juce::Graphics& g);

    void paintKeyboardB(juce::Graphics& g);


    /** @brief True if the keyboard has been drawn */
    bool isDrawn = false;

    /** @brief Stores each key's bounds, active state, and type */
    struct note {
        juce::Rectangle<int> bounds {}; ///< Key bounds
        bool isActive = false; ///< Whether the key is currently pressed
        std::string type = ""; ///< "white" or "black"
    };

    MidiHandler& midiHandler; ///< Reference to MIDI handler
    std::unordered_map<int, note> keys; ///< Map from MIDI note number to note data

    static constexpr int lowestMidiNote  = 0;
    static constexpr int highestMidiNote = 127;
    int min_draw = lowestMidiNote; ///< Minimum visible MIDI note
    int max_draw = highestMidiNote; ///< Maximum visible MIDI note
    bool annotationState=false;
};
