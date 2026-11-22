#include <juce_core/juce_core.h>
#include "UI/NoteField.h"

// NoteField is NoteLayer's animation model, pulled out of the GL code so it can be tested.
// The numbers below reproduce what NoteLayer did before the move to the render thread:
// notes rise at 400 px/s, start 10 px tall, grow while held (bottom anchored, clamped to the
// field height), rise at the same speed once released, shrink against the top edge and fade
// over the last 80 px of their tail.
class NoteFieldTest : public juce::UnitTest
{
public:
    NoteFieldTest() : juce::UnitTest ("NoteField", "Unit") {}

    void runTest() override
    {
        constexpr double eps = 1e-3;

        beginTest ("note-on spawns a 10 px note at the bottom, at the key's x and width");
        {
            NoteField f;
            f.setHeight (500.0f);
            expect (f.noteOn (60, 100.0f, 20.0f));
            expectEquals ((int) f.held().size(), 1);
            const auto& n = f.held().at (60);
            expectWithinAbsoluteError (n.left, 100.0f, (float) eps);
            expectWithinAbsoluteError (n.width, 20.0f, (float) eps);
            expectWithinAbsoluteError (n.height, 10.0f, (float) eps);
            expectWithinAbsoluteError (n.top, 490.0f, (float) eps);
            expectWithinAbsoluteError (n.alpha, 1.0f, (float) eps);
            expect (! f.isIdle());
        }

        beginTest ("a repeated note-on for a held note is ignored");
        {
            NoteField f;
            f.setHeight (500.0f);
            f.noteOn (60, 100.0f, 20.0f);
            f.advance (0.1f);
            expect (! f.noteOn (60, 300.0f, 20.0f));
            expectEquals ((int) f.held().size(), 1);
            expectWithinAbsoluteError (f.held().at (60).left, 100.0f, (float) eps);
            expectWithinAbsoluteError (f.held().at (60).height, 50.0f, (float) eps);
        }

        beginTest ("a held note grows upward at 400 px/s with its bottom anchored");
        {
            NoteField f;
            f.setHeight (500.0f);
            f.noteOn (60, 0.0f, 20.0f);
            f.advance (0.25f);
            const auto& n = f.held().at (60);
            expectWithinAbsoluteError (n.height, 110.0f, (float) eps);
            expectWithinAbsoluteError (n.top, 390.0f, (float) eps);
            expectWithinAbsoluteError (n.top + n.height, 500.0f, (float) eps);
        }

        beginTest ("a held note stops growing at the field height");
        {
            NoteField f;
            f.setHeight (200.0f);
            f.noteOn (60, 0.0f, 20.0f);
            f.advance (1.0f);
            f.advance (1.0f);
            const auto& n = f.held().at (60);
            expectWithinAbsoluteError (n.height, 200.0f, (float) eps);
            expectWithinAbsoluteError (n.top, 0.0f, (float) eps);
        }

        beginTest ("note-off releases the note with its current geometry; unknown note-offs are ignored");
        {
            NoteField f;
            f.setHeight (500.0f);
            f.noteOn (60, 40.0f, 20.0f);
            f.advance (0.25f);
            f.noteOff (61);
            expectEquals ((int) f.held().size(), 1);
            f.noteOff (60);
            expect (f.held().empty());
            expectEquals ((int) f.falling().size(), 1);
            const auto& n = f.falling().front();
            expectWithinAbsoluteError (n.left, 40.0f, (float) eps);
            expectWithinAbsoluteError (n.height, 110.0f, (float) eps);
            expectWithinAbsoluteError (n.top, 390.0f, (float) eps);
            expectWithinAbsoluteError (n.alpha, 1.0f, (float) eps);
        }

        beginTest ("a released note rises, shrinks against the top, fades over its last 80 px, then disappears");
        {
            NoteField f;
            f.setHeight (500.0f);
            f.noteOn (60, 0.0f, 20.0f);
            f.advance (0.25f);                     // h 110, y 390
            f.noteOff (60);

            f.advance (0.5f);                      // rises 200 px, keeps its height
            expectWithinAbsoluteError (f.falling().front().top, 190.0f, (float) eps);
            expectWithinAbsoluteError (f.falling().front().height, 110.0f, (float) eps);

            f.advance (0.5f);                      // would reach y -10: pinned at 0, 10 px shorter
            expectWithinAbsoluteError (f.falling().front().top, 0.0f, (float) eps);
            expectWithinAbsoluteError (f.falling().front().height, 100.0f, (float) eps);
            expectWithinAbsoluteError (f.falling().front().alpha, 1.0f, (float) eps);

            f.advance (0.075f);                    // tail at 70 px -> alpha 70/80
            expectWithinAbsoluteError (f.falling().front().height, 70.0f, (float) eps);
            expectWithinAbsoluteError (f.falling().front().alpha, 0.875f, (float) eps);

            f.advance (0.2f);                      // tail gone
            expect (f.falling().empty());
            expect (f.isIdle());
        }

        beginTest ("releaseAll releases every held note; clear empties everything");
        {
            NoteField f;
            f.setHeight (500.0f);
            f.noteOn (60, 0.0f, 20.0f);
            f.noteOn (64, 50.0f, 20.0f);
            f.releaseAll();
            expect (f.held().empty());
            expectEquals ((int) f.falling().size(), 2);

            f.noteOn (67, 90.0f, 20.0f);
            f.clear();
            expect (f.held().empty());
            expect (f.falling().empty());
            expect (f.isIdle());
        }

        beginTest ("advance(0) changes nothing and an empty field is idle");
        {
            NoteField f;
            expect (f.isIdle());
            f.setHeight (300.0f);
            f.noteOn (60, 0.0f, 20.0f);
            f.advance (0.0f);
            expectWithinAbsoluteError (f.held().at (60).top, 290.0f, (float) eps);
            expectWithinAbsoluteError (f.held().at (60).height, 10.0f, (float) eps);
        }
    }
};

static NoteFieldTest noteFieldTest;
