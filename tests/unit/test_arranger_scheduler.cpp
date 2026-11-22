#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerScheduler.h"

namespace ArrangerSchedulerTestValues
{
    const juce::String testName     = "ArrangerScheduler";
    const juce::String testCategory = "Arranger";

    const juce::String emitsInsideWindow     = "emits events whose phase is inside the swept window";
    const juce::String nothingOutsideWindow  = "does not emit events outside the window";
    const juce::String closesHeldNoteAtSeam  = "closes a held note at the loop seam and retriggers next loop";
    const juce::String resetClearsActive     = "reset clears active-note state (no spurious seam note-off)";
    const juce::String windowLongerThanLoop  = "handles a window longer than the loop (multiple wraps)";
    const juce::String flushClosesSounding   = "flushActiveNotes closes sounding notes and then nothing";
    const juce::String partTagFlows          = "part tag flows from setLoop to the emitted note-on AND its seam note-off";
    const juce::String legacySetLoopIsFixed  = "legacy 2-arg setLoop marks events Fixed (non-transposable)";

    constexpr int         drumChannel   = 10;
    constexpr int         kickNote      = 36;
    constexpr juce::uint8 kickVelocity  = 100;
    constexpr int         heldChannel   = 2;
    constexpr int         heldNote      = 60;
    constexpr juce::uint8 heldVelocity  = 90;
    constexpr int         flushChannel  = 3;
    constexpr int         flushNote     = 64;
    constexpr juce::uint8 flushVelocity = 100;

    constexpr double loopBeats    = 4.0;
    constexpr double kickOnBeat   = 0.0;
    constexpr double kickOffBeat  = 0.5;
    constexpr double heldOnBeat   = 3.0;
    constexpr double barStart     = 0.0;
    constexpr double firstBeatEnd = 1.0;
    constexpr double secondBeatEnd = 2.0;
    constexpr double armStart     = 2.5;
    constexpr double beforeSeam   = 3.5;
    constexpr double afterSeam    = 4.5;
    constexpr double seamBeat     = 4.0;
    constexpr double twoLoopsEnd  = 8.0;
    constexpr double tolerance    = 1e-9;

    constexpr int    noEvents       = 0;
    constexpr int    oneEvent       = 1;
    constexpr int    kickOnAndOff   = 2;
    constexpr size_t seamEventCount = 2;
    constexpr int    twoLoops       = 2;
    constexpr size_t firstEvent     = 0;
    constexpr size_t secondEvent    = 1;
}

using namespace ArrangerSchedulerTestValues;

class ArrangerSchedulerTest : public juce::UnitTest
{
public:
    ArrangerSchedulerTest() : juce::UnitTest (ArrangerSchedulerTestValues::testName, ArrangerSchedulerTestValues::testCategory) {}

    static std::vector<TimedBeatEvent> oneBarKickAndHeld()
    {
        // loop length 4 beats. Kick on beat 0 (off at 0.5). A held note on at beat 3 with NO note-off
        // before the loop end (must be closed at the seam).
        return {
            { kickOnBeat,  juce::MidiMessage::noteOn  (drumChannel, kickNote, kickVelocity) },
            { kickOffBeat, juce::MidiMessage::noteOff (drumChannel, kickNote) },
            { heldOnBeat,  juce::MidiMessage::noteOn  (heldChannel, heldNote, heldVelocity) },
        };
    }

    void runTest() override
    {
        beginTest (emitsInsideWindow);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop (oneBarKickAndHeld(), loopBeats);
            auto out = scheduler.advance (barStart, firstBeatEnd);   // sweeps beats [0,1)
            expectEquals ((int) out.size(), kickOnAndOff); // kick on + kick off
            expectWithinAbsoluteError (out[firstEvent].beats, kickOnBeat, tolerance);
            expect (out[firstEvent].message.isNoteOn());
            expectWithinAbsoluteError (out[secondEvent].beats, kickOffBeat, tolerance);
            expect (out[secondEvent].message.isNoteOff());
        }

        beginTest (nothingOutsideWindow);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop (oneBarKickAndHeld(), loopBeats);
            auto out = scheduler.advance (firstBeatEnd, secondBeatEnd);
            expectEquals ((int) out.size(), noEvents);
        }

        beginTest (closesHeldNoteAtSeam);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop (oneBarKickAndHeld(), loopBeats);
            scheduler.advance (armStart, beforeSeam);            // arm: turns the held note (2,60) on at beat 3.0
            // Now sweep across the seam: from beat 3.5 to beat 4.5.
            auto out = scheduler.advance (beforeSeam, afterSeam);
            // Expected, in order: synthetic noteOff for held (2,60) at beat 4.0 (seam),
            // then kick noteOn (10,36) at beat 4.0 (phase 0 of next loop).
            expect (out.size() >= seamEventCount);
            expectWithinAbsoluteError (out[firstEvent].beats, seamBeat, tolerance);
            expect (out[firstEvent].message.isNoteOff());
            expectEquals (out[firstEvent].message.getNoteNumber(), heldNote);
            expectEquals (out[firstEvent].message.getChannel(), heldChannel);
            expect (out[secondEvent].message.isNoteOn());
            expectEquals (out[secondEvent].message.getNoteNumber(), kickNote); // kick retriggers at seam
        }

        beginTest (resetClearsActive);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop (oneBarKickAndHeld(), loopBeats);
            scheduler.advance (heldOnBeat, beforeSeam);   // turns the held note (2,60) on
            scheduler.reset();              // forget active notes
            auto out = scheduler.advance (beforeSeam, afterSeam); // crossing seam, but nothing is active to close
            for (auto& event : out)
                expect (! (event.message.isNoteOff() && event.message.getNoteNumber() == heldNote));
        }

        beginTest (windowLongerThanLoop);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop (oneBarKickAndHeld(), loopBeats);
            auto out = scheduler.advance (barStart, twoLoopsEnd); // two full loops
            int kickOns {};
            for (auto& event : out) if (event.message.isNoteOn() && event.message.getNoteNumber() == kickNote) ++kickOns;
            expectEquals (kickOns, twoLoops);
        }

        beginTest (flushClosesSounding);
        {
            ArrangerScheduler scheduler;
            // A held note on at beat 0 with no note-off in the bar.
            scheduler.setLoop ({ { barStart, juce::MidiMessage::noteOn (flushChannel, flushNote, flushVelocity) } }, loopBeats);
            scheduler.advance (barStart, firstBeatEnd);                 // turns (3,64) on

            auto flushed = scheduler.flushActiveNotes (firstBeatEnd);
            expectEquals ((int) flushed.size(), oneEvent);
            expect (flushed[firstEvent].message.isNoteOff());
            expectEquals (flushed[firstEvent].message.getChannel(), flushChannel);
            expectEquals (flushed[firstEvent].message.getNoteNumber(), flushNote);

            auto again = scheduler.flushActiveNotes (firstBeatEnd); // nothing left to close
            expectEquals ((int) again.size(), noEvents);
        }

        beginTest (partTagFlows);
        {
            ArrangerScheduler scheduler;
            std::vector<TimedBeatEvent> evs { { barStart, juce::MidiMessage::noteOn (heldChannel, heldNote, heldVelocity) } };
            std::vector<PartKind> parts { PartKind::Acc };
            scheduler.setLoop (evs, parts, loopBeats);

            auto on = scheduler.advance (barStart, firstBeatEnd);
            expectEquals ((int) on.size(), oneEvent);
            expect (on[firstEvent].part == PartKind::Acc);

            auto seam = scheduler.advance (beforeSeam, afterSeam);   // crosses seam -> synthetic note-off
            bool foundOff = false;
            for (auto& event : seam)
                if (event.message.isNoteOff() && event.message.getNoteNumber() == heldNote)
                    { foundOff = true; expect (event.part == PartKind::Acc); }
            expect (foundOff);
        }

        beginTest (legacySetLoopIsFixed);
        {
            ArrangerScheduler scheduler;
            scheduler.setLoop ({ { barStart, juce::MidiMessage::noteOn (heldChannel, heldNote, heldVelocity) } }, loopBeats);
            auto on = scheduler.advance (barStart, firstBeatEnd);
            expectEquals ((int) on.size(), oneEvent);
            expect (on[firstEvent].part == PartKind::Fixed);
        }
    }
};

static ArrangerSchedulerTest arrangerSchedulerTest;
