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

    constexpr int numPianoKeys = (int) (sizeof (pianoKeyScanCodes) / sizeof (pianoKeyScanCodes[0]));

    static_assert (numPianoKeys == (int) (sizeof (pianoKeyCharsUS) / sizeof (pianoKeyCharsUS[0])),
                   "the scan-code and character tables must describe the same keys");
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

    if (midiNote != -1 && it==allPressedKeys.end())
    {
        allPressedKeys.push_back(midiNote);
        this->midiHandler.noteOnKeyboard(midiNote, 127);
    }
    return false;
}

bool KeyboardListener::keyStateChanged(bool isKeyDown, juce::Component*)
{

    for (int i = static_cast<int>(allPressedKeys.size()) - 1; i >= 0; i--)
    {
        int note = allPressedKeys[i];
        int keyCode = intToKey[note];
        if (!juce::KeyPress::isKeyCurrentlyDown(keyCode))
        {
            allPressedKeys.erase(allPressedKeys.begin() + i);
            this->midiHandler.noteOffKeyboard(note, 127);
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
    for (int i = static_cast<int>(allPressedKeys.size()) - 1; i >= 0; i--)
    {
        int note = allPressedKeys[i];
        allPressedKeys.erase(allPressedKeys.begin() + i);
        this->midiHandler.noteOffKeyboard(note, 127);
    }
}

void KeyboardListener::refreshKeyMapIfLayoutChanged()
{
#if JUCE_WINDOWS
    void* const activeLayout = (void*) GetKeyboardLayout (0);

    if (activeLayout == keyboardLayoutOfMap && ! keyCodeToOffset.empty())
        return;

    const bool layoutWasSwitched = (keyboardLayoutOfMap != nullptr);
    keyboardLayoutOfMap = activeLayout;
    keyCodeToOffset.clear();

    for (int offset = 0; offset < numPianoKeys; ++offset)
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
        if (virtualKey == 0)
            continue;

        const UINT character = LOWORD (MapVirtualKeyExW (virtualKey, MAPVK_VK_TO_CHAR, layout));
        if (character == 0)
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

    for (int offset = 0; offset < numPianoKeys; ++offset)
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
        return -1;

    const int offset = entry->second;

    // Range guards carried over from the previous per-key mapping: the lowest nine keys drop out
    // once the start note has been transposed down far enough to run off the bottom of the MIDI
    // range, and the top five once the finish note would run off the top.
    if (offset <= 8 && startNoteKeyboardInput < 24)
        return -1;

    if (offset >= 13 && finishNoteKeyboardInput > 101)
        return -1;

    const int note = startNoteKeyboardInput + offset;

    keyToInt[key.getKeyCode()] = note;
    intToKey[note] = key.getKeyCode();

    return note;
}
