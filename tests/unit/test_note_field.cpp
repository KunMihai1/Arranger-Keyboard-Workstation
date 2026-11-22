#include <juce_core/juce_core.h>
#include "UI/NoteField.h"

namespace NoteFieldTestValues
{
    const juce::String testName     = "NoteField";
    const juce::String testCategory = "Unit";

    const juce::String noteOnSpawns          = "note-on spawns a 10 px note at the bottom, at the key's x and width";
    const juce::String repeatedNoteOnIgnored = "a repeated note-on for a held note is ignored";
    const juce::String heldNoteGrows         = "a held note grows upward at 400 px/s with its bottom anchored";
    const juce::String heldNoteStopsGrowing  = "a held note stops growing at the field height";
    const juce::String noteOffReleases       = "note-off releases the note with its current geometry; unknown note-offs are ignored";
    const juce::String releasedNoteRises     = "a released note rises, shrinks against the top, fades over its last 80 px, then disappears";
    const juce::String releaseAllAndClear    = "releaseAll releases every held note; clear empties everything";
    const juce::String zeroAdvanceAndIdle    = "advance(0) changes nothing and an empty field is idle";

    constexpr double tolerance = 1e-3;

    constexpr int middleC     = 60;
    constexpr int notHeldNote = 61;
    constexpr int noteE4      = 64;
    constexpr int noteG4      = 67;

    constexpr float fieldHeight      = 500.0f;
    constexpr float shortFieldHeight = 200.0f;
    constexpr float smallFieldHeight = 300.0f;
    constexpr float keyWidth         = 20.0f;
    constexpr float keyLeft          = 100.0f;
    constexpr float otherKeyLeft     = 300.0f;
    constexpr float leftEdge         = 0.0f;
    constexpr float keyLeftAt40      = 40.0f;
    constexpr float keyLeftAt50      = 50.0f;
    constexpr float keyLeftAt90      = 90.0f;

    constexpr float tenthSecond      = 0.1f;
    constexpr float quarterSecond    = 0.25f;
    constexpr float halfSecond       = 0.5f;
    constexpr float oneSecond        = 1.0f;
    constexpr float tailIntoFade     = 0.075f;   // 30 px: the tail is left at 70 px
    constexpr float tailGone         = 0.2f;
    constexpr float noTime           = 0.0f;

    constexpr float startHeight          = 10.0f;
    constexpr float startTop             = 490.0f;   // fieldHeight - startHeight
    constexpr float heightAfterTenth     = 50.0f;    // 10 + 400 * 0.1
    constexpr float heightAfterQuarter   = 110.0f;   // 10 + 400 * 0.25
    constexpr float topAfterQuarter      = 390.0f;
    constexpr float topAfterRising       = 190.0f;   // 390 - 400 * 0.5
    constexpr float pinnedTop            = 0.0f;
    constexpr float heightAgainstTop     = 100.0f;
    constexpr float tailHeight           = 70.0f;
    constexpr float tailAlpha            = 0.875f;   // 70 / 80
    constexpr float smallFieldStartTop   = 290.0f;
    constexpr float opaque               = 1.0f;

    constexpr int oneNote  = 1;
    constexpr int twoNotes = 2;
}

using namespace NoteFieldTestValues;

// NoteField is NoteLayer's animation model, pulled out of the GL code so it can be tested.
// The numbers below reproduce what NoteLayer did before the move to the render thread:
// notes rise at 400 px/s, start 10 px tall, grow while held (bottom anchored, clamped to the
// field height), rise at the same speed once released, shrink against the top edge and fade
// over the last 80 px of their tail.
class NoteFieldTest : public juce::UnitTest
{
public:
    NoteFieldTest() : juce::UnitTest (NoteFieldTestValues::testName, NoteFieldTestValues::testCategory) {}

    void runTest() override
    {
        constexpr float eps = (float) tolerance;

        beginTest (noteOnSpawns);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            expect (field.noteOn (middleC, keyLeft, keyWidth));
            expectEquals ((int) field.held().size(), oneNote);
            const auto& note = field.held().at (middleC);
            expectWithinAbsoluteError (note.left, keyLeft, eps);
            expectWithinAbsoluteError (note.width, keyWidth, eps);
            expectWithinAbsoluteError (note.height, startHeight, eps);
            expectWithinAbsoluteError (note.top, startTop, eps);
            expectWithinAbsoluteError (note.alpha, opaque, eps);
            expect (! field.isIdle());
        }

        beginTest (repeatedNoteOnIgnored);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            field.noteOn (middleC, keyLeft, keyWidth);
            field.advance (tenthSecond);
            expect (! field.noteOn (middleC, otherKeyLeft, keyWidth));
            expectEquals ((int) field.held().size(), oneNote);
            expectWithinAbsoluteError (field.held().at (middleC).left, keyLeft, eps);
            expectWithinAbsoluteError (field.held().at (middleC).height, heightAfterTenth, eps);
        }

        beginTest (heldNoteGrows);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            field.noteOn (middleC, leftEdge, keyWidth);
            field.advance (quarterSecond);
            const auto& note = field.held().at (middleC);
            expectWithinAbsoluteError (note.height, heightAfterQuarter, eps);
            expectWithinAbsoluteError (note.top, topAfterQuarter, eps);
            expectWithinAbsoluteError (note.top + note.height, fieldHeight, eps);
        }

        beginTest (heldNoteStopsGrowing);
        {
            NoteField field;
            field.setHeight (shortFieldHeight);
            field.noteOn (middleC, leftEdge, keyWidth);
            field.advance (oneSecond);
            field.advance (oneSecond);
            const auto& note = field.held().at (middleC);
            expectWithinAbsoluteError (note.height, shortFieldHeight, eps);
            expectWithinAbsoluteError (note.top, pinnedTop, eps);
        }

        beginTest (noteOffReleases);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            field.noteOn (middleC, keyLeftAt40, keyWidth);
            field.advance (quarterSecond);
            field.noteOff (notHeldNote);
            expectEquals ((int) field.held().size(), oneNote);
            field.noteOff (middleC);
            expect (field.held().empty());
            expectEquals ((int) field.falling().size(), oneNote);
            const auto& note = field.falling().front();
            expectWithinAbsoluteError (note.left, keyLeftAt40, eps);
            expectWithinAbsoluteError (note.height, heightAfterQuarter, eps);
            expectWithinAbsoluteError (note.top, topAfterQuarter, eps);
            expectWithinAbsoluteError (note.alpha, opaque, eps);
        }

        beginTest (releasedNoteRises);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            field.noteOn (middleC, leftEdge, keyWidth);
            field.advance (quarterSecond);                     // h 110, y 390
            field.noteOff (middleC);

            field.advance (halfSecond);                      // rises 200 px, keeps its height
            expectWithinAbsoluteError (field.falling().front().top, topAfterRising, eps);
            expectWithinAbsoluteError (field.falling().front().height, heightAfterQuarter, eps);

            field.advance (halfSecond);                      // would reach top -10: pinned at 0, 10 px shorter
            expectWithinAbsoluteError (field.falling().front().top, pinnedTop, eps);
            expectWithinAbsoluteError (field.falling().front().height, heightAgainstTop, eps);
            expectWithinAbsoluteError (field.falling().front().alpha, opaque, eps);

            field.advance (tailIntoFade);                    // tail at 70 px -> alpha 70/80
            expectWithinAbsoluteError (field.falling().front().height, tailHeight, eps);
            expectWithinAbsoluteError (field.falling().front().alpha, tailAlpha, eps);

            field.advance (tailGone);                      // tail gone
            expect (field.falling().empty());
            expect (field.isIdle());
        }

        beginTest (releaseAllAndClear);
        {
            NoteField field;
            field.setHeight (fieldHeight);
            field.noteOn (middleC, leftEdge, keyWidth);
            field.noteOn (noteE4, keyLeftAt50, keyWidth);
            field.releaseAll();
            expect (field.held().empty());
            expectEquals ((int) field.falling().size(), twoNotes);

            field.noteOn (noteG4, keyLeftAt90, keyWidth);
            field.clear();
            expect (field.held().empty());
            expect (field.falling().empty());
            expect (field.isIdle());
        }

        beginTest (zeroAdvanceAndIdle);
        {
            NoteField field;
            expect (field.isIdle());
            field.setHeight (smallFieldHeight);
            field.noteOn (middleC, leftEdge, keyWidth);
            field.advance (noTime);
            expectWithinAbsoluteError (field.held().at (middleC).top, smallFieldStartTop, eps);
            expectWithinAbsoluteError (field.held().at (middleC).height, startHeight, eps);
        }
    }
};

static NoteFieldTest noteFieldTest;
