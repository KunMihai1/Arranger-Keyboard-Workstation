#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
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

namespace KeyListenerTestValues
{
    const juce::String testName     = "KeyboardListener";
    const juce::String testCategory = "Unit";

    const juce::String defaultStateKeyboardInputDisabled = "Default state - keyboard input disabled";
    const juce::String defaultStateStartNoteIs60 = "Default state - startNote is 60";
    const juce::String defaultStateFinishNoteIs77 = "Default state - finishNote is 77";
    const juce::String setIsKeyboardInputEnableAndDisable = "setIsKeyboardInput - enable and disable";
    const juce::String setStartNoteKeyboardInputGetStartNoteKeyboardInput = "setStartNoteKeyboardInput / getStartNoteKeyboardInput";
    const juce::String setFinishNoteKeyboardInputGetFinishNoteKeyboardInput = "setFinishNoteKeyboardInput / getFinishNoteKeyboardInput";
    const juce::String mapKeyMidiAMapsToStartNoteDefault60 = "mapKeyMidi - 'A' maps to startNote (default 60)";
    const juce::String mapKeyMidiWMapsToStartNote1 = "mapKeyMidi - 'W' maps to startNote + 1";
    const juce::String mapKeyMidiSMapsToStartNote2 = "mapKeyMidi - 'S' maps to startNote + 2";
    const juce::String mapKeyMidiEMapsToStartNote3 = "mapKeyMidi - 'E' maps to startNote + 3";
    const juce::String mapKeyMidiDMapsToStartNote4 = "mapKeyMidi - 'D' maps to startNote + 4";
    const juce::String mapKeyMidiFMapsToStartNote5 = "mapKeyMidi - 'F' maps to startNote + 5";
    const juce::String mapKeyMidiKMapsToStartNote12One = "mapKeyMidi - 'K' maps to startNote + 12 (one octave)";
    const juce::String mapKeyMidiHMapsToStartNote9 = "mapKeyMidi - 'H' maps to startNote + 9";
    const juce::String mapKeyMidiUnmappedKeyReturns1 = "mapKeyMidi - unmapped key returns -1";
    const juce::String mapKeyMidiAReturns1WhenStartNote24 = "mapKeyMidi - 'A' returns -1 when startNote < 24";
    const juce::String mapKeyMidiWReturns1WhenStartNote24 = "mapKeyMidi - 'W' returns -1 when startNote < 24";
    const juce::String mapKeyMidiOReturns1WhenFinishNote101 = "mapKeyMidi - 'O' returns -1 when finishNote > 101";
    const juce::String mapKeyMidiLReturns1WhenFinishNote101 = "mapKeyMidi - 'L' returns -1 when finishNote > 101";
    const juce::String mapKeyMidiPReturns1WhenFinishNote101 = "mapKeyMidi - 'P' returns -1 when finishNote > 101";
    const juce::String mapKeyMidiReturns1WhenFinishNote101 = "mapKeyMidi - the key right of L (';' on US) returns -1 when finishNote > 101";
    const juce::String mapKeyMidiApostropheReturns1WhenFinishNote101 = "mapKeyMidi - the second key right of L (apostrophe on US) returns -1 when finishNote > 101";
    const juce::String mapKeyMidiCustomStartNoteShiftsAllMappings = "mapKeyMidi - custom startNote shifts all mappings";
    const juce::String resetStateClearsInternalMapsWithoutCrash = "resetState - clears internal maps without crash";
    const juce::String mapKeyMidiOMapsToStartNote13When = "mapKeyMidi - 'O' maps to startNote+13 when finishNote <= 101";
    const juce::String mapKeyMidiLMapsToStartNote14When = "mapKeyMidi - 'L' maps to startNote+14 when finishNote <= 101";
    const juce::String mapKeyMidiApostropheMapsToStartNote17When = "mapKeyMidi - the second key right of L (apostrophe on US) maps to startNote+17 when finishNote <= 101, on any layout";
    const juce::String mapKeyMidiTMapsToStartNote6 = "mapKeyMidi - 'T' maps to startNote + 6";
    const juce::String mapKeyMidiGMapsToStartNote7 = "mapKeyMidi - 'G' maps to startNote + 7";
    const juce::String mapKeyMidiYMapsToStartNote8 = "mapKeyMidi - 'Y' maps to startNote + 8";
    const juce::String mapKeyMidiTReturns1WhenStartNote24 = "mapKeyMidi - 'T' returns -1 when startNote < 24";
    const juce::String mapKeyMidiGReturns1WhenStartNote24 = "mapKeyMidi - 'G' returns -1 when startNote < 24";
    const juce::String mapKeyMidiYReturns1WhenStartNote24 = "mapKeyMidi - 'Y' returns -1 when startNote < 24";
    const juce::String mapKeyMidiUMapsToStartNote10 = "mapKeyMidi - 'U' maps to startNote + 10";
    const juce::String mapKeyMidiJMapsToStartNote11 = "mapKeyMidi - 'J' maps to startNote + 11";
    const juce::String mapKeyMidiPMapsToStartNote15When = "mapKeyMidi - 'P' maps to startNote+15 when finishNote <= 101";
    const juce::String mapKeyMidiMapsToStartNote16WhenFinishNote = "mapKeyMidi - the key right of L (';' on US) maps to startNote+16 when finishNote <= 101, on any layout";
    const juce::String mapKeyMidiCustomStartNoteShiftsTGY = "mapKeyMidi - custom startNote shifts T, G, Y, U, J mappings";
    const juce::String keyPressedReturnsFalseWhenKeyboardInputIs = "keyPressed - returns false when keyboard input is disabled";
    const juce::String keyPressedReturnsFalseWhenKeyboardInputIs2 = "keyPressed - returns false when keyboard input is enabled";
    const juce::String keyPressedUnmappedKeyDoesNotCrashWhen = "keyPressed - unmapped key does not crash when enabled";

    // The US characters of the PC keys the listener maps (and two it does not).
    constexpr int keyCodeA = 'A';
    constexpr int keyCodeW = 'W';
    constexpr int keyCodeS = 'S';
    constexpr int keyCodeE = 'E';
    constexpr int keyCodeD = 'D';
    constexpr int keyCodeF = 'F';
    constexpr int keyCodeT = 'T';
    constexpr int keyCodeG = 'G';
    constexpr int keyCodeY = 'Y';
    constexpr int keyCodeH = 'H';
    constexpr int keyCodeU = 'U';
    constexpr int keyCodeJ = 'J';
    constexpr int keyCodeK = 'K';
    constexpr int keyCodeO = 'O';
    constexpr int keyCodeL = 'L';
    constexpr int keyCodeP = 'P';
    constexpr int keyCodeSemicolon = ';';
    constexpr int keyCodeApostrophe = '\'';
    constexpr int keyCodeX = 'X';
    constexpr int keyCodeZ = 'Z';

    // Where those keys sit (PS/2 set-1 scan codes). The listener maps key positions, not characters,
    // so a test presses a position and lets the active layout say which character it types:
    // on a Romanian layout the key right of L types s-comma, not ';' (the 2026-11-23 failures).
    constexpr int scanCodeA = 0x1E;
    constexpr int scanCodeW = 0x11;
    constexpr int scanCodeS = 0x1F;
    constexpr int scanCodeE = 0x12;
    constexpr int scanCodeD = 0x20;
    constexpr int scanCodeF = 0x21;
    constexpr int scanCodeT = 0x14;
    constexpr int scanCodeG = 0x22;
    constexpr int scanCodeY = 0x15;
    constexpr int scanCodeH = 0x23;
    constexpr int scanCodeU = 0x16;
    constexpr int scanCodeJ = 0x24;
    constexpr int scanCodeK = 0x25;
    constexpr int scanCodeO = 0x18;
    constexpr int scanCodeL = 0x26;
    constexpr int scanCodeP = 0x19;
    constexpr int scanCodeSemicolon = 0x27;
    constexpr int scanCodeApostrophe = 0x28;
    constexpr int scanCodeX = 0x2D;
    constexpr int scanCodeZ = 0x2C;
    constexpr int currentThread = 0;   // GetKeyboardLayout(0): the layout of the calling thread

    /** The key press JUCE hands keyPressed() for the key at this position under the active layout
        (the US character where there is no layout lookup). */
    juce::KeyPress keyAt ([[maybe_unused]] int scanCode, [[maybe_unused]] int usCharacter)
    {
       #if JUCE_WINDOWS
        const auto layout = GetKeyboardLayout (currentThread);
        const auto virtualKey = MapVirtualKeyExW (static_cast<UINT> (scanCode), MAPVK_VSC_TO_VK, layout);
        const auto character = static_cast<int> (LOWORD (MapVirtualKeyExW (virtualKey, MAPVK_VK_TO_CHAR, layout)));
        return juce::KeyPress (character);
       #else
        return juce::KeyPress (usCharacter);
       #endif
    }

    // Semitones above the start note that each key plays.
    constexpr int offsetA = 0;
    constexpr int offsetW = 1;
    constexpr int offsetS = 2;
    constexpr int offsetE = 3;
    constexpr int offsetD = 4;
    constexpr int offsetF = 5;
    constexpr int offsetT = 6;
    constexpr int offsetG = 7;
    constexpr int offsetY = 8;
    constexpr int offsetH = 9;
    constexpr int offsetU = 10;
    constexpr int offsetJ = 11;
    constexpr int offsetK = 12;
    constexpr int offsetO = 13;
    constexpr int offsetL = 14;
    constexpr int offsetP = 15;
    constexpr int offsetSemicolon = 16;
    constexpr int offsetApostrophe = 17;

    constexpr int defaultStartNote  = 60;
    constexpr int defaultFinishNote = 77;
    constexpr int customStartNote   = 48;
    constexpr int otherStartNote    = 72;
    constexpr int customFinishNote  = 84;
    constexpr int unmappedKey       = -1;
    // Keys below the start note need a start note of 24 or more; keys above it need a finish note of 101 or less.
    constexpr int startNoteTooLow       = 20;
    constexpr int startNoteFarTooLow    = 10;
    constexpr int startNoteStillTooLow  = 15;
    constexpr int finishNoteTooHigh     = 105;
    constexpr int finishNoteFarTooHigh  = 110;
    constexpr int finishNoteJustTooHigh = 102;
    constexpr int finishNoteWayTooHigh  = 120;
    constexpr int finishNoteStillTooHigh = 115;
    constexpr int allowedFinishNote     = 100;
    constexpr int lowerFinishNote       = 90;
    constexpr int finishNoteAt95        = 95;
}

using namespace KeyListenerTestValues;

class KeyboardListenerTest : public juce::UnitTest
{
public:
    KeyboardListenerTest() : juce::UnitTest (KeyListenerTestValues::testName, KeyListenerTestValues::testCategory) {}

    void runTest() override
    {
        // ---- Default State ----

        beginTest(defaultStateKeyboardInputDisabled);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);
            expect(listener.getIsKeyboardInput() == false);
        }

        beginTest(defaultStateStartNoteIs60);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);
            expect(listener.getStartNoteKeyboardInput() == defaultStartNote);
        }

        beginTest(defaultStateFinishNoteIs77);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);
            expect(listener.getFinishNoteKeyboardInput() == defaultFinishNote);
        }

        // ---- setIsKeyboardInput / getIsKeyboardInput ----

        beginTest(setIsKeyboardInputEnableAndDisable);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setIsKeyboardInput(true);
            expect(listener.getIsKeyboardInput() == true);

            listener.setIsKeyboardInput(false);
            expect(listener.getIsKeyboardInput() == false);
        }

        // ---- Start/Finish Note ----

        beginTest(setStartNoteKeyboardInputGetStartNoteKeyboardInput);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(customStartNote);
            expect(listener.getStartNoteKeyboardInput() == customStartNote);

            listener.setStartNoteKeyboardInput(otherStartNote);
            expect(listener.getStartNoteKeyboardInput() == otherStartNote);
        }

        beginTest(setFinishNoteKeyboardInputGetFinishNoteKeyboardInput);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(customFinishNote);
            expect(listener.getFinishNoteKeyboardInput() == customFinishNote);

            listener.setFinishNoteKeyboardInput(allowedFinishNote);
            expect(listener.getFinishNoteKeyboardInput() == allowedFinishNote);
        }

        // ---- mapKeyMidi ----

        beginTest(mapKeyMidiAMapsToStartNoteDefault60);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyA = keyAt (scanCodeA, keyCodeA);
            expect(listener.mapKeyMidi(keyA) == defaultStartNote + offsetA);
        }

        beginTest(mapKeyMidiWMapsToStartNote1);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyW = keyAt (scanCodeW, keyCodeW);
            expect(listener.mapKeyMidi(keyW) == defaultStartNote + offsetW);
        }

        beginTest(mapKeyMidiSMapsToStartNote2);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyS = keyAt (scanCodeS, keyCodeS);
            expect(listener.mapKeyMidi(keyS) == defaultStartNote + offsetS);
        }

        beginTest(mapKeyMidiEMapsToStartNote3);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyE = keyAt (scanCodeE, keyCodeE);
            expect(listener.mapKeyMidi(keyE) == defaultStartNote + offsetE);
        }

        beginTest(mapKeyMidiDMapsToStartNote4);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyD = keyAt (scanCodeD, keyCodeD);
            expect(listener.mapKeyMidi(keyD) == defaultStartNote + offsetD);
        }

        beginTest(mapKeyMidiFMapsToStartNote5);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyF = keyAt (scanCodeF, keyCodeF);
            expect(listener.mapKeyMidi(keyF) == defaultStartNote + offsetF);
        }

        beginTest(mapKeyMidiKMapsToStartNote12One);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyK = keyAt (scanCodeK, keyCodeK);
            expect(listener.mapKeyMidi(keyK) == defaultStartNote + offsetK);
        }

        beginTest(mapKeyMidiHMapsToStartNote9);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyH = keyAt (scanCodeH, keyCodeH);
            expect(listener.mapKeyMidi(keyH) == defaultStartNote + offsetH);
        }

        beginTest(mapKeyMidiUnmappedKeyReturns1);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyZ = keyAt (scanCodeZ, keyCodeZ);
            expect(listener.mapKeyMidi(keyZ) == unmappedKey);

            const auto keyX = keyAt (scanCodeX, keyCodeX);
            expect(listener.mapKeyMidi(keyX) == unmappedKey);
        }

        // ---- mapKeyMidi boundary conditions ----

        beginTest(mapKeyMidiAReturns1WhenStartNote24);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(startNoteTooLow);  // below 24
            const auto keyA = keyAt (scanCodeA, keyCodeA);
            expect(listener.mapKeyMidi(keyA) == unmappedKey);
        }

        beginTest(mapKeyMidiWReturns1WhenStartNote24);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(startNoteFarTooLow);
            const auto keyW = keyAt (scanCodeW, keyCodeW);
            expect(listener.mapKeyMidi(keyW) == unmappedKey);
        }

        beginTest(mapKeyMidiOReturns1WhenFinishNote101);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteTooHigh);  // above 101
            const auto keyO = keyAt (scanCodeO, keyCodeO);
            expect(listener.mapKeyMidi(keyO) == unmappedKey);
        }

        beginTest(mapKeyMidiLReturns1WhenFinishNote101);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteFarTooHigh);
            const auto keyL = keyAt (scanCodeL, keyCodeL);
            expect(listener.mapKeyMidi(keyL) == unmappedKey);
        }

        beginTest(mapKeyMidiPReturns1WhenFinishNote101);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteJustTooHigh);
            const auto keyP = keyAt (scanCodeP, keyCodeP);
            expect(listener.mapKeyMidi(keyP) == unmappedKey);
        }

        beginTest(mapKeyMidiReturns1WhenFinishNote101);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteWayTooHigh);
            const auto keySemicolon = keyAt (scanCodeSemicolon, keyCodeSemicolon);
            expect(listener.mapKeyMidi(keySemicolon) == unmappedKey);
        }

        beginTest(mapKeyMidiApostropheReturns1WhenFinishNote101);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteStillTooHigh);
            const auto keyApostrophe = keyAt (scanCodeApostrophe, keyCodeApostrophe);
            expect(listener.mapKeyMidi(keyApostrophe) == unmappedKey);
        }

        // ---- Custom startNote changes mapping ----

        beginTest(mapKeyMidiCustomStartNoteShiftsAllMappings);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(customStartNote);

            const auto keyA = keyAt (scanCodeA, keyCodeA);
            expect(listener.mapKeyMidi(keyA) == customStartNote + offsetA);

            const auto keyW = keyAt (scanCodeW, keyCodeW);
            expect(listener.mapKeyMidi(keyW) == customStartNote + offsetW);

            const auto keyK = keyAt (scanCodeK, keyCodeK);
            expect(listener.mapKeyMidi(keyK) == customStartNote + offsetK);

            const auto keyH = keyAt (scanCodeH, keyCodeH);
            expect(listener.mapKeyMidi(keyH) == customStartNote + offsetH);
        }

        // ---- resetState ----

        beginTest(resetStateClearsInternalMapsWithoutCrash);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            // map some keys first
            const auto keyA = keyAt (scanCodeA, keyCodeA);
            listener.mapKeyMidi(keyA);
            const auto keyW = keyAt (scanCodeW, keyCodeW);
            listener.mapKeyMidi(keyW);

            listener.resetState();

            // after reset, basic getters still work
            expect(listener.getIsKeyboardInput() == false);
            expect(listener.getStartNoteKeyboardInput() == defaultStartNote);
            expect(listener.getFinishNoteKeyboardInput() == defaultFinishNote);
        }

        // ---- O, L, P, ;, ' allowed when finishNote <= 101 ----

        beginTest(mapKeyMidiOMapsToStartNote13When);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(allowedFinishNote);
            const auto keyO = keyAt (scanCodeO, keyCodeO);
            expect(listener.mapKeyMidi(keyO) == defaultStartNote + offsetO);
        }

        beginTest(mapKeyMidiLMapsToStartNote14When);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(lowerFinishNote);
            const auto keyL = keyAt (scanCodeL, keyCodeL);
            expect(listener.mapKeyMidi(keyL) == defaultStartNote + offsetL);
        }

        beginTest(mapKeyMidiApostropheMapsToStartNote17When);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(finishNoteAt95);
            const auto keyApostrophe = keyAt (scanCodeApostrophe, keyCodeApostrophe);
            expect(listener.mapKeyMidi(keyApostrophe) == defaultStartNote + offsetApostrophe);
        }

        // ---- mapKeyMidi - missing key mappings: T, G, Y ----

        beginTest(mapKeyMidiTMapsToStartNote6);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyT = keyAt (scanCodeT, keyCodeT);
            expect(listener.mapKeyMidi(keyT) == defaultStartNote + offsetT);
        }

        beginTest(mapKeyMidiGMapsToStartNote7);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyG = keyAt (scanCodeG, keyCodeG);
            expect(listener.mapKeyMidi(keyG) == defaultStartNote + offsetG);
        }

        beginTest(mapKeyMidiYMapsToStartNote8);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyY = keyAt (scanCodeY, keyCodeY);
            expect(listener.mapKeyMidi(keyY) == defaultStartNote + offsetY);
        }

        beginTest(mapKeyMidiTReturns1WhenStartNote24);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(startNoteTooLow);
            const auto keyT = keyAt (scanCodeT, keyCodeT);
            expect(listener.mapKeyMidi(keyT) == unmappedKey);
        }

        beginTest(mapKeyMidiGReturns1WhenStartNote24);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(startNoteFarTooLow);
            const auto keyG = keyAt (scanCodeG, keyCodeG);
            expect(listener.mapKeyMidi(keyG) == unmappedKey);
        }

        beginTest(mapKeyMidiYReturns1WhenStartNote24);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(startNoteStillTooLow);
            const auto keyY = keyAt (scanCodeY, keyCodeY);
            expect(listener.mapKeyMidi(keyY) == unmappedKey);
        }

        // ---- mapKeyMidi - U, J (no boundary guard) ----

        beginTest(mapKeyMidiUMapsToStartNote10);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyU = keyAt (scanCodeU, keyCodeU);
            expect(listener.mapKeyMidi(keyU) == defaultStartNote + offsetU);
        }

        beginTest(mapKeyMidiJMapsToStartNote11);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            const auto keyJ = keyAt (scanCodeJ, keyCodeJ);
            expect(listener.mapKeyMidi(keyJ) == defaultStartNote + offsetJ);
        }

        // ---- mapKeyMidi - P, ; (finishNote <= 101 guard, valid path) ----

        beginTest(mapKeyMidiPMapsToStartNote15When);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(lowerFinishNote);
            const auto keyP = keyAt (scanCodeP, keyCodeP);
            expect(listener.mapKeyMidi(keyP) == defaultStartNote + offsetP);
        }

        beginTest(mapKeyMidiMapsToStartNote16WhenFinishNote);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setFinishNoteKeyboardInput(lowerFinishNote);
            const auto keySemicolon = keyAt (scanCodeSemicolon, keyCodeSemicolon);
            expect(listener.mapKeyMidi(keySemicolon) == defaultStartNote + offsetSemicolon);
        }

        // ---- mapKeyMidi - custom startNote with T, G, Y, U, J ----

        beginTest(mapKeyMidiCustomStartNoteShiftsTGY);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setStartNoteKeyboardInput(customStartNote);

            const auto keyT = keyAt (scanCodeT, keyCodeT);
            expect(listener.mapKeyMidi(keyT) == customStartNote + offsetT);  // 48 + 6

            const auto keyG = keyAt (scanCodeG, keyCodeG);
            expect(listener.mapKeyMidi(keyG) == customStartNote + offsetG);  // 48 + 7

            const auto keyY = keyAt (scanCodeY, keyCodeY);
            expect(listener.mapKeyMidi(keyY) == customStartNote + offsetY);  // 48 + 8

            const auto keyU = keyAt (scanCodeU, keyCodeU);
            expect(listener.mapKeyMidi(keyU) == customStartNote + offsetU);  // 48 + 10

            const auto keyJ = keyAt (scanCodeJ, keyCodeJ);
            expect(listener.mapKeyMidi(keyJ) == customStartNote + offsetJ);  // 48 + 11
        }

        // ---- keyPressed ----

        beginTest(keyPressedReturnsFalseWhenKeyboardInputIs);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            // keyboard input is disabled by default
            const auto keyA = keyAt (scanCodeA, keyCodeA);
            expect(listener.keyPressed(keyA, nullptr) == false);
        }

        beginTest(keyPressedReturnsFalseWhenKeyboardInputIs2);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setIsKeyboardInput(true);
            const auto keyA = keyAt (scanCodeA, keyCodeA);
            // keyPressed always returns false regardless of key validity
            expect(listener.keyPressed(keyA, nullptr) == false);
        }

        beginTest(keyPressedUnmappedKeyDoesNotCrashWhen);
        {
            MidiDevice device;
            MidiHandler handler(device);
            KeyboardListener listener(handler);

            listener.setIsKeyboardInput(true);
            const auto keyZ = keyAt (scanCodeZ, keyCodeZ);
            expect(listener.keyPressed(keyZ, nullptr) == false);
        }
    }
};

static KeyboardListenerTest keyboardListenerTest;
