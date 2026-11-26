#include <JuceHeader.h>
#include "UI/PlaybackRange.h"

/*  Owner (2026-11-27): "it should move the value but something made it to be on the wrong value at a point" — a split
    set to A3, shifted with X and saved by any edit, came back as A4 (saving folded shifted notes into the middle-C
    octave); a "none" split became A0 after a Z press; a split clamped at the piano's edge did not come back. */

namespace PlaybackRangeTestValues
{
    const juce::String testName     = "PlaybackRange";
    const juce::String testCategory = "Unit";

    const juce::String shiftFromRange  = "a PC keyboard is shifted by where it starts against middle C; a hardware one (any size) never";
    const juce::String shownMoves      = "shown = saved moved by the shift; none stays none";
    const juce::String clampKeepsSaved = "a note clamped at the piano's edge comes back when shifted home";
    const juce::String editSavedHome   = "an edit while shifted is saved moved home, only for the edited field";
    const juce::String editToNone      = "a field edited to none is saved as none";

    constexpr int noteA1  = 33;
    constexpr int noteA3  = 57;
    constexpr int noteA4  = 69;
    constexpr int noteA5  = 81;
    constexpr int noteA6  = 93;
    constexpr int noteAs4 = 70;
    constexpr int noteAs6 = 94;
    constexpr int noteC2  = 36;
    constexpr int noteC6  = 84;
    constexpr bool pcKeyboard       = true;
    constexpr bool hardwareKeyboard = false;
    constexpr int twoOctaves  = 24;
    constexpr int homeShift   = 0;
    constexpr int downTwo     = -24;
    constexpr int notSet      = PlaybackSettingsKeys::notSet;
}

class PlaybackRangeTest : public juce::UnitTest
{
public:
    PlaybackRangeTest() : juce::UnitTest (PlaybackRangeTestValues::testName, PlaybackRangeTestValues::testCategory) {}

    void runTest() override
    {
        using namespace PlaybackRangeTestValues;
        using namespace PlaybackRange;

        beginTest (shiftFromRange);
        {
            expectEquals (octaveShift (middleC, pcKeyboard), homeShift);
            expectEquals (octaveShift (noteC6, pcKeyboard), twoOctaves);
            expectEquals (octaveShift (noteC2, pcKeyboard), downTwo);
            expectEquals (octaveShift (lowestPianoNote, hardwareKeyboard), homeShift);
            expectEquals (octaveShift (noteC2, hardwareKeyboard), homeShift);   // a 49-key board on C2: never shifted
        }

        beginTest (shownMoves);
        {
            const auto now = shown (split (noteA4, notSet), twoOctaves);
            expectEquals (now.leftHandBound, noteA6);
            expectEquals (now.rightHandBound, notSet);
            expectEquals (now.startNote, notSet);
        }

        beginTest (clampKeepsSaved);
        {
            const auto saved = split (noteA1, notSet);
            expectEquals (shown (saved, downTwo).leftHandBound, lowestPianoNote);
            expectEquals (shown (saved, homeShift).leftHandBound, noteA1);
        }

        beginTest (editSavedHome);
        {
            const auto saved = split (noteA3, notSet);
            auto edited = shown (saved, twoOctaves);            // left shows A5
            expectEquals (edited.leftHandBound, noteA5);
            edited.rightHandBound = noteAs6;                    // the owner sets "Right hand from" while shifted
            const auto after = savedAfterEdit (saved, edited, twoOctaves);
            expectEquals (after.rightHandBound, noteAs4);       // saved home
            expectEquals (after.leftHandBound, noteA3);         // untouched: stays A3 (was saved as A4)

            const auto clampedSaved = split (noteA1, notSet);
            auto clampedEdit = shown (clampedSaved, downTwo);   // left shows A0 (clamped)
            clampedEdit.rightHandBound = noteC2;
            expectEquals (savedAfterEdit (clampedSaved, clampedEdit, downTwo).leftHandBound, noteA1);
        }

        beginTest (editToNone);
        {
            const auto saved = split (noteA3, noteAs4);
            auto edited = shown (saved, twoOctaves);
            edited.rightHandBound = notSet;
            expectEquals (savedAfterEdit (saved, edited, twoOctaves).rightHandBound, notSet);
        }
    }

private:
    static PlayBackSettings split (int leftUpTo, int rightFrom)
    {
        PlayBackSettings settings;
        settings.leftHandBound  = leftUpTo;
        settings.rightHandBound = rightFrom;
        return settings;
    }
};
static PlaybackRangeTest playbackRangeTest;
