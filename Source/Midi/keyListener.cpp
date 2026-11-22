/*
  ==============================================================================

    keyListener.cpp
    Created: 23 May 2025 5:20:30pm
    Author:  Kisuke

  ==============================================================================
*/

#include "keyListener.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX 1
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN 1
 #endif
 #include <windows.h>
#endif

namespace
{
    /** The piano layout expressed as key *positions* (PS/2 set-1 scan codes), in semitone
        order from the start note. On a US board these positions carry A W S E D F T G Y
        H U J K O L P ; ' -- but the position is what matters, not the printed character. */
    constexpr int pianoKeyScanCodes[] =
    {
        0x1E, 0x11, 0x1F, 0x12, 0x20, 0x21, 0x14, 0x22, 0x15,   //  0..8   A W S E D F T G Y
        0x23, 0x16, 0x24, 0x25,                                 //  9..12  H U J K
        0x18, 0x26, 0x19, 0x27, 0x28                            // 13..17  O L P ; '
    };

    /** Same keys as characters, for platforms with no scan-code lookup. */
    [[maybe_unused]] constexpr int pianoKeyCharsUS[] =
    {
        'A', 'W', 'S', 'E', 'D', 'F', 'T', 'G', 'Y',
        'H', 'U', 'J', 'K',
        'O', 'L', 'P', ';', '\''
    };

    constexpr int numPianoKeys = (int) std::size (pianoKeyScanCodes);

    // The scan-code and character tables must describe the same keys.
    static_assert (numPianoKeys == (int) std::size (pianoKeyCharsUS));

    constexpr int noNote       = -1;    // a key that plays nothing
    constexpr int fullVelocity = 127;   // the PC keyboard has no touch sensitivity

    // Range guards: the lowest nine keys (offsets 0..8) drop out once the start note is low enough for
    // them to run off the bottom of the MIDI range, and the top five (13..17) once the finish note
    // would run off the top.
    constexpr int lastOfLowestKeys     = 8;
    constexpr int lowestSafeStartNote  = 24;
    constexpr int firstOfHighestKeys   = 13;
    constexpr int highestSafeFinishNote = 101;

    constexpr int noVirtualKey  = 0;
    constexpr int noCharacter   = 0;
    constexpr int currentThread = 0;   // GetKeyboardLayout(0): the layout of the calling thread
}

KeyboardListener::KeyboardListener(MidiHandler& midiHandler) : midiHandler{ midiHandler }
{

}

bool KeyboardListener::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (!this->isKeyBoardInput)
        return false;

    int midiNote = mapKeyMidi(key);

    auto it = std::find(allPressedKeys.begin(), allPressedKeys.end(), midiNote);

    if (midiNote != noNote && it==allPressedKeys.end())
    {
        allPressedKeys.push_back(midiNote);
        this->midiHandler.noteOnKeyboard(midiNote, fullVelocity);
    }
    return false;
}

bool KeyboardListener::keyStateChanged(bool isKeyDown, juce::Component*)
{

    for (int pressedIndex = static_cast<int>(allPressedKeys.size()); pressedIndex-- > int {};)   // newest first
    {
        int note = allPressedKeys[pressedIndex];
        int keyCode = intToKey[note];
        if (!juce::KeyPress::isKeyCurrentlyDown(keyCode))
        {
            allPressedKeys.erase(allPressedKeys.begin() + pressedIndex);
            this->midiHandler.noteOffKeyboard(note, fullVelocity);
        }
    }
    return false;
}

void KeyboardListener::setIsKeyboardInput(bool state)
{
    this->isKeyBoardInput = state;
}

bool KeyboardListener::getIsKeyboardInput()
{
    return isKeyBoardInput;
}

void KeyboardListener::resetState()
{
    this->intToKey.clear();
    this->keyToInt.clear();
    this->allPressedKeys.clear();
}

int KeyboardListener::getStartNoteKeyboardInput()
{
    return startNoteKeyboardInput;
}

void KeyboardListener::setStartNoteKeyboardInput(int value)
{
    startNoteKeyboardInput = value;
}

int KeyboardListener::getFinishNoteKeyboardInput()
{
    return finishNoteKeyboardInput;
}

void KeyboardListener::setFinishNoteKeyboardInput(int value)
{
    finishNoteKeyboardInput = value;
}

void KeyboardListener::releaseAllHeldNotes()
{
    for (int pressedIndex = static_cast<int>(allPressedKeys.size()); pressedIndex-- > int {};)   // newest first
    {
        int note = allPressedKeys[pressedIndex];
        allPressedKeys.erase(allPressedKeys.begin() + pressedIndex);
        this->midiHandler.noteOffKeyboard(note, fullVelocity);
    }
}

void KeyboardListener::refreshKeyMapIfLayoutChanged()
{
#if JUCE_WINDOWS
    void* const activeLayout = (void*) GetKeyboardLayout (currentThread);

    if (activeLayout == keyboardLayoutOfMap && ! keyCodeToOffset.empty())
        return;

    const bool layoutWasSwitched = (keyboardLayoutOfMap != nullptr);
    keyboardLayoutOfMap = activeLayout;
    keyCodeToOffset.clear();

    for (int offset {}; offset < numPianoKeys; ++offset)
    {
        // Mirrors JUCE's own scan-code -> key-code computation (juce_Windowing_windows.cpp,
        // doKeyChar), so these are exactly the codes keyPressed() will be handed.
        //
        // The Ex/W forms are deliberate, not incidental. Unsuffixed MapVirtualKey resolves to the
        // ANSI variant, which cannot represent a character outside the current code page and
        // returns '?' (63) instead -- on a Romanian layout both ';' and ''' came back as 63, so
        // the second silently overwrote the first in the table and neither key worked. The W form
        // returns the real UTF-16 value (537 's-comma', 539 't-comma'), which is what JUCE reports.
        const HKL layout = (HKL) activeLayout;

        const UINT virtualKey = MapVirtualKeyExW ((UINT) pianoKeyScanCodes[offset], MAPVK_VSC_TO_VK, layout);
        if (virtualKey == noVirtualKey)
            continue;

        const UINT character = LOWORD (MapVirtualKeyExW (virtualKey, MAPVK_VK_TO_CHAR, layout));
        if (character == noCharacter)
            continue;   // dead key or unused position on this layout

        // emplace, not operator[]: if a layout ever does map two positions to the same character,
        // keep the lower offset rather than letting the later one silently replace it.
        keyCodeToOffset.emplace ((int) character, offset);
    }

    // A note held across the switch keeps a key code the new layout cannot resolve
    // (KeyPress::isKeyCurrentlyDown maps the code back through VkKeyScan), so keyStateChanged
    // would never see it lift and the note would hang. Let go of everything instead.
    if (layoutWasSwitched)
        releaseAllHeldNotes();

#else
    if (! keyCodeToOffset.empty())
        return;

    for (int offset {}; offset < numPianoKeys; ++offset)
        keyCodeToOffset[pianoKeyCharsUS[offset]] = offset;
#endif
}

int KeyboardListener::mapKeyMidi(const juce::KeyPress& key)
{
    // JUCE reports a key's code as the character it produces under the layout active at that
    // moment, so the same physical key is 59 (';') on a US board and 537 ('s-comma') on a
    // Romanian one. Keying off position instead makes the piano layout independent of that.
    refreshKeyMapIfLayoutChanged();

    const auto entry = keyCodeToOffset.find (key.getKeyCode());


    if (entry == keyCodeToOffset.end())
        return noNote;

    const int offset = entry->second;

    // Range guards carried over from the previous per-key mapping: the lowest nine keys drop out
    // once the start note has been transposed down far enough to run off the bottom of the MIDI
    // range, and the top five once the finish note would run off the top.
    if (offset <= lastOfLowestKeys && startNoteKeyboardInput < lowestSafeStartNote)
        return noNote;

    if (offset >= firstOfHighestKeys && finishNoteKeyboardInput > highestSafeFinishNote)
        return noNote;

    const int note = startNoteKeyboardInput + offset;

    keyToInt[key.getKeyCode()] = note;
    intToKey[note] = key.getKeyCode();

    return note;
}
