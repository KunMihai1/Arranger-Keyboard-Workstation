/*
  ==============================================================================

    keyListener.h
    Created: 23 May 2025 5:20:30pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "MidiHandler.h"

/**
 * @class KeyboardListener
 * @brief Listens to computer keyboard input and converts it to MIDI notes.
 *
 * Handles:
 *   - Key press and release events
 *   - Mapping keyboard keys to MIDI note numbers
 *   - Tracking currently pressed keys
 *   - Configurable start and end notes for the keyboard input range
 *
 * Inherits from juce::KeyListener.
 */
class KeyboardListener : public juce::KeyListener
{
public:
    /**
     * @brief Constructor.
     * @param midiHandler Reference to the MidiHandler to send note events.
     */
    KeyboardListener(MidiHandler& midiHandler);

    /**
     * @brief Called when a key is pressed.
     * @param key The key that was pressed.
     * @param component The component that received the key press.
     * @return true if the event was consumed, false otherwise.
     */
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

    /**
     * @brief Called when a key state changes (pressed/released).
     * @param isKeyDown true if a key is pressed, false if released.
     * @param component The component that received the change.
     * @return true if the event was consumed, false otherwise.
     */
    bool keyStateChanged(bool isKeyDown, juce::Component*) override;

    /**
     * @brief Enables or disables keyboard input for MIDI.
     * @param state true to enable, false to disable.
     */
    void setIsKeyboardInput(bool state);

    /**
     * @brief Checks whether keyboard input is currently enabled.
     * @return true if enabled, false otherwise.
     */
    bool getIsKeyboardInput();

    /**
     * @brief Resets the internal state of the listener.
     * Clears all key mappings and pressed key records.
     */
    void resetState();

    /**
     * @brief Gets the starting MIDI note for keyboard input.
     * @return The starting note number.
     */
    int getStartNoteKeyboardInput();

    /**
     * @brief Sets the starting MIDI note for keyboard input.
     * @param value The new starting note number.
     */
    void setStartNoteKeyboardInput(int value);

    /**
     * @brief Gets the ending MIDI note for keyboard input.
     * @return The ending note number.
     */
    int getFinishNoteKeyboardInput();

    /**
     * @brief Sets the ending MIDI note for keyboard input.
     * @param value The new ending note number.
     */
    void setFinishNoteKeyboardInput(int value);

    /**
     * @brief Maps a computer key to a MIDI note number.
     * @param key The key pressed.
     * @return MIDI note number, or -1 if the key is not mapped.
     */
    int mapKeyMidi(const juce::KeyPress& key);

private:

    /**
     * @brief Rebuilds the key-code table when the active keyboard layout changes.
     *
     * JUCE reports a key's code as the character that key produces under the layout active at
     * the time, so the same physical key is a different code on a US and a Romanian board.
     * Re-deriving the table from scan codes keeps the piano layout tied to key positions.
     */
    void refreshKeyMapIfLayoutChanged();

    /**
     * @brief Sends note-offs for every note still held and clears the held list.
     * Used when the layout changes underneath a held key, which would otherwise hang the note.
     */
    void releaseAllHeldNotes();

    // Member variables
    MidiHandler& midiHandler;                  /**< Reference to the MIDI handler */
    int startNoteKeyboardInput = 60;          /**< Starting MIDI note number */
    int finishNoteKeyboardInput = 77;         /**< Ending MIDI note number */
    std::unordered_map<int, int> keyToInt;    /**< Map from key codes to MIDI notes */
    std::unordered_map<int, int> intToKey;    /**< Map from MIDI notes to key codes */
    std::unordered_map<int, int> keyCodeToOffset; /**< Key code -> semitone offset from the start note */
    void* keyboardLayoutOfMap = nullptr;      /**< Layout (HKL) the table was built for; null = not built */
    bool isKeyBoardInput = false;             /**< Whether keyboard input is active */
    std::vector<int> allPressedKeys;          /**< Tracks currently pressed MIDI notes */
};
