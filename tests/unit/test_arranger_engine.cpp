#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Arranger/ArrangerEngine.h"
#include "Arranger/ArrangerModel.h"
#include "Arranger/ArrangerCountIn.h"
#include <map>
#include <utility>

namespace ArrangerEngineTestValues
{
    const juce::String testName     = "ArrangerEngine";
    const juce::String testCategory = "Arranger";

    const juce::String renderRangeDispatchesScheduledEventsThroughOnMidiMessage = "renderRange dispatches scheduled events through onMidiMessage";
    const juce::String loopLengthInBeatsReflectsTheSection = "loop length in beats reflects the section bars";
    const juce::String stopSilencesSoundingNotesAndClearsIsPlaying = "stop silences sounding notes and clears isPlaying";
    const juce::String queueSectionSwitchesActiveSectionOnlyAfterThe = "queueSection switches active section only after the bar boundary";
    const juce::String aSectionSwapFlushesTheOutgoingSection = "a section swap flushes the outgoing section's held note (no hung notes)";
    const juce::String anEndingCompletingSilencesOutputNoHung = "an Ending completing silences output (no hung notes)";
    const juce::String synchroStartOnlyAValidChordReleases = "Synchro Start: only a valid chord releases the armed gate";
    const juce::String synchroStartStopClearsTheArmedGate = "Synchro Start: stop clears the armed gate";
    const juce::String countInHelperOne44Bar = "count-in helper: one 4/4 bar = 4 clicks, beat 0 accented, half-open seam";
    const juce::String countInHelper34Bar3 = "count-in helper: 3/4 bar = 3 clicks; clamps to the bar; sub-beat windows";
    const juce::String countInEmitsABarOfClicks = "count-in: emits a bar of clicks then ends, and the section plays after";
    const juce::String countInStopCancelsAnInProgress = "count-in: stop cancels an in-progress count-in";
    const juce::String selectStartSectionMakesStartBeginOnTheChosen = "selectStartSection makes start begin on the chosen section";
    const juce::String activeChordTransposesAccNotesButLeaves = "active chord transposes Acc notes but leaves Drum notes unchanged";
    const juce::String transposedNoteOffMatchesTheTransposedNote = "transposed note-off matches the transposed note-on pitch (no hung notes)";
    const juce::String perTrackNttTypeReachesEmitTimeTransposition = "per-track NttType reaches emit-time transposition (Parallel != Chord)";

    const juce::String expectedSoundingBeforeStop = "expected a sounding note before stop";
    const juce::String stopLeftNotesHung          = "stop left notes hung";
    const juce::String endingLeftNotesHung        = "ending left notes hung: ";
    const juce::String sectionSilentAfterCountIn  = "section did not play after the count-in";
    const juce::String accShouldShift             = "Acc note 60 should shift to 62 under D major";
    const juce::String drumShouldStay             = "Drum note 36 should be unchanged";
    const juce::String noteOnTransposed           = "note-on should sound the transposed pitch 62";
    const juce::String seamNoteOffTransposed      = "the seam note-off must use the same transposed pitch 62";
    const juce::String parallelShouldEmit66       = "Parallel-typed track should emit 66 (61 + interval)";
    const juce::String parallelMustNotSnap        = "Parallel type must NOT snap to the Chord-type result 65";

    const juce::String variationOneId   = "var_1";
    const juce::String variationOne     = "Variation 1";
    const juce::String variationTwoId   = "var_2";
    const juce::String variationTwo     = "Variation 2";
    const juce::String endingOneId      = "ending_1";
    const juce::String endingOne        = "Ending 1";

    constexpr int         accChannel   = 2;
    constexpr int         drumChannel  = 10;
    constexpr int         middleC      = 60;
    constexpr int         cSharp4      = 61;
    constexpr int         noteD4       = 62;
    constexpr int         noteF4       = 65;
    constexpr int         noteFSharp4  = 66;
    constexpr int         noteG4       = 67;
    constexpr int         noteC5       = 72;
    constexpr int         kickNote     = 36;
    constexpr juce::uint8 velocity     = 100;
    constexpr int         silentVelocity = 0;

    constexpr int    oneBar       = 1;
    constexpr int    beatsPerBar  = 4;
    constexpr int    beatUnit     = 4;
    constexpr double tempo        = 120.0;
    constexpr double tolerance    = 1e-9;

    // Beat positions and render windows.
    constexpr double barStart      = 0.0;
    constexpr double tenthOfBeat   = 0.1;
    constexpr double quarterBeat   = 0.25;
    constexpr double halfBeat      = 0.5;
    constexpr double nineTenths    = 0.9;
    constexpr double oneBeat       = 1.0;
    constexpr double beatAndAHalf  = 1.5;
    constexpr double twoBeats      = 2.0;
    constexpr double threeBeats    = 3.0;
    constexpr double fourBeats     = 4.0;   // one 4/4 bar
    constexpr double pastTheSeam   = 4.5;
    constexpr double afterBarOne   = 5.0;
    constexpr double eightBeats    = 8.0;
    constexpr double afterBarTwo   = 9.0;

    constexpr int    twoEvents       = 2;
    constexpr size_t firstEvent      = 0;
    constexpr size_t secondEvent     = 1;
    constexpr int    firstSection    = 0;
    constexpr int    secondSection   = 1;
    constexpr int    notFound        = -1;
    constexpr int    none            = 0;
    constexpr int    atLeastOne      = 1;

    // Count-in clicks.
    constexpr int    clickChannel     = 10;
    constexpr int    clickNote        = 37;
    constexpr int    clicksPerBar     = 4;
    constexpr int    clicksPerHalfBar = 2;
    constexpr int    clicksIn34       = 3;
    constexpr int    oneClick         = 1;
    constexpr int    oneAccent        = 1;
    constexpr int    plainClickLimit  = 100;   // accented clicks are louder than this
    constexpr size_t firstClick       = 0;
    constexpr size_t secondClick      = 1;

    // Chords as { root, quality, bass note }.
    constexpr int pitchC = 0;
    constexpr int pitchD = 2;
    constexpr int pitchF = 5;
    const ArrangerChord cMajor { pitchC, ChordQuality::Maj, pitchC };
    const ArrangerChord dMajor { pitchD, ChordQuality::Maj, pitchD };
    const ArrangerChord fMajor { pitchF, ChordQuality::Maj, pitchF };
}

using namespace ArrangerEngineTestValues;

class ArrangerEngineTest : public juce::UnitTest
{
public:
    ArrangerEngineTest() : juce::UnitTest (ArrangerEngineTestValues::testName, ArrangerEngineTestValues::testCategory) {}

    static ArrangerStyle fourFourStyle()
    {
        ArrangerStyle style; style.timeSigNum = beatsPerBar; style.timeSigDenom = beatUnit; style.originalTempo = tempo;
        return style;
    }

    static ArrangerStyle makeStyle()
    {
        ArrangerTrack track;
        track.channel = accChannel;
        track.partType = ArrangerPartType::Acc;
        track.pattern = {
            { barStart, juce::MidiMessage::noteOn  (accChannel, middleC, velocity) },
            { halfBeat, juce::MidiMessage::noteOff (accChannel, middleC) },
        };
        ArrangerSection section; section.lengthBars = oneBar; section.tracks.push_back (track);
        ArrangerStyle style = fourFourStyle();
        style.sections.push_back (section);
        return style;
    }

    // Two one-bar variations on channel 2. Variation 1 holds a note across the bar
    // (no note-off) so a section switch must flush it; Variation 2 plays a short note.
    static ArrangerStyle makeTwoVarStyle()
    {
        ArrangerTrack v1; v1.channel = accChannel; v1.partType = ArrangerPartType::Acc;
        v1.pattern = { { barStart, juce::MidiMessage::noteOn (accChannel, middleC, velocity) } }; // held
        ArrangerSection s1; s1.id = variationOneId; s1.name = variationOne;
        s1.type = ArrangerSectionType::Variation; s1.lengthBars = oneBar; s1.tracks.push_back (v1);

        ArrangerTrack v2; v2.channel = accChannel; v2.partType = ArrangerPartType::Acc;
        v2.pattern = { { barStart, juce::MidiMessage::noteOn  (accChannel, noteG4, velocity) },
                       { halfBeat, juce::MidiMessage::noteOff (accChannel, noteG4) } };
        ArrangerSection s2; s2.id = variationTwoId; s2.name = variationTwo;
        s2.type = ArrangerSectionType::Variation; s2.lengthBars = oneBar; s2.tracks.push_back (v2);

        ArrangerStyle st = fourFourStyle();
        st.sections.push_back (s1); st.sections.push_back (s2);
        return st;
    }

    static ArrangerStyle makeEndingStyle()
    {
        ArrangerTrack track; track.channel = accChannel; track.partType = ArrangerPartType::Acc;
        track.pattern = { { barStart, juce::MidiMessage::noteOn  (accChannel, middleC, velocity) },
                          { halfBeat, juce::MidiMessage::noteOff (accChannel, middleC) } };
        ArrangerSection var; var.id = variationOneId; var.name = variationOne;
        var.type = ArrangerSectionType::Variation; var.lengthBars = oneBar; var.tracks.push_back (track);

        ArrangerSection end; end.id = endingOneId; end.name = endingOne;
        end.type = ArrangerSectionType::Ending; end.lengthBars = oneBar;
        end.afterComplete = ArrangerAfterComplete::Stop; end.tracks.push_back (track);

        ArrangerStyle st = fourFourStyle();
        st.sections.push_back (var); st.sections.push_back (end);
        return st;
    }

    // One section: an Acc track (ch2, note 60) + a Drum track (ch10, note 36), both short.
    static ArrangerStyle makeAccAndDrumStyle()
    {
        ArrangerTrack acc; acc.channel = accChannel; acc.partType = ArrangerPartType::Acc;
        acc.pattern = { { barStart, juce::MidiMessage::noteOn  (accChannel, middleC, velocity) },
                        { halfBeat, juce::MidiMessage::noteOff (accChannel, middleC) } };
        ArrangerTrack drum; drum.channel = drumChannel; drum.partType = ArrangerPartType::Drum;
        drum.pattern = { { barStart, juce::MidiMessage::noteOn  (drumChannel, kickNote, velocity) },
                         { halfBeat, juce::MidiMessage::noteOff (drumChannel, kickNote) } };
        ArrangerSection section; section.id = variationOneId; section.name = variationOne;
        section.type = ArrangerSectionType::Variation; section.lengthBars = oneBar;
        section.tracks.push_back (acc); section.tracks.push_back (drum);
        ArrangerStyle st = fourFourStyle();
        st.sections.push_back (section);
        return st;
    }

    // One section: an Acc track holding note 60 across the bar (no note-off) -> closed at the seam.
    static ArrangerStyle makeAccHeldNoteStyle()
    {
        ArrangerTrack acc; acc.channel = accChannel; acc.partType = ArrangerPartType::Acc;
        acc.pattern = { { barStart, juce::MidiMessage::noteOn (accChannel, middleC, velocity) } };
        ArrangerSection section; section.id = variationOneId; section.name = variationOne;
        section.type = ArrangerSectionType::Variation; section.lengthBars = oneBar; section.tracks.push_back (acc);
        ArrangerStyle st = fourFourStyle();
        st.sections.push_back (section);
        return st;
    }

    // Counts note-ons minus note-offs per (channel, note).
    using SoundingNotes = std::map<std::pair<int,int>, int>;
    static void trackSounding (SoundingNotes& sounding, const juce::MidiMessage& message)
    {
        const std::pair<int,int> key { message.getChannel(), message.getNoteNumber() };
        if (message.isNoteOn() && message.getVelocity() > silentVelocity)                     sounding[key]++;
        else if (message.isNoteOff() || (message.isNoteOn() && message.getVelocity() == silentVelocity)) sounding[key]--;
    }

    static int countSounding (const SoundingNotes& sounding)
    {
        int count {};
        for (auto& entry : sounding) if (entry.second > none) ++count;
        return count;
    }

    void runTest() override
    {
        beginTest (renderRangeDispatchesScheduledEventsThroughOnMidiMessage);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };

            engine.setStyle (makeStyle());
            engine.renderRange (barStart, oneBeat);   // beats [0,1): note on then off

            expectEquals ((int) captured.size(), twoEvents);
            expect (captured[firstEvent].isNoteOn());
            expectEquals (captured[firstEvent].getNoteNumber(), middleC);
            expect (captured[secondEvent].isNoteOff());
        }

        beginTest (loopLengthInBeatsReflectsTheSection);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeStyle());
            expectWithinAbsoluteError (engine.getLoopLengthBeats(), fourBeats, tolerance); // 1 bar of 4/4
        }

        beginTest (stopSilencesSoundingNotesAndClearsIsPlaying);
        {
            // The engine silences ONLY its own sounding notes (targeted note-offs), not a blanket
            // all-channels all-notes-off (which used to kill the player's manually-held notes too).
            // So we assert the real contract: a note that was sounding is closed, none left hung.
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            SoundingNotes sounding;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { trackSounding (sounding, message); };
            engine.setStyle (makeStyle());
            engine.renderRange (barStart, quarterBeat);     // note 60 on, not yet off -> sounding
            int onBefore = countSounding (sounding);
            engine.stop();
            int stillOn = countSounding (sounding);
            expect (! engine.isPlaying());
            expect (onBefore >= atLeastOne, expectedSoundingBeforeStop);
            expect (stillOn == none, stopLeftNotesHung);
        }

        beginTest (queueSectionSwitchesActiveSectionOnlyAfterThe);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeTwoVarStyle());
            engine.queueSection (ArrangerSectionType::Variation, variationTwo);

            expectEquals (engine.getActiveSectionIndex(), firstSection);
            engine.renderRange (barStart, twoBeats);                    // no boundary yet
            expectEquals (engine.getActiveSectionIndex(), firstSection);
            engine.renderRange (twoBeats, afterBarOne);                 // crosses beat 4
            expectEquals (engine.getActiveSectionIndex(), secondSection);
        }

        beginTest (aSectionSwapFlushesTheOutgoingSection);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };

            engine.setStyle (makeTwoVarStyle());
            engine.queueSection (ArrangerSectionType::Variation, variationTwo);
            engine.renderRange (barStart, oneBeat);   // Variation 1 note 60 on (held)
            engine.renderRange (oneBeat, afterBarOne);   // boundary -> swap to Variation 2

            int offIdx = notFound, onIdx = notFound;
            for (int messageIndex {}; messageIndex < (int) captured.size(); ++messageIndex)
            {
                if (captured[(size_t) messageIndex].isNoteOff() && captured[(size_t) messageIndex].getNoteNumber() == middleC) offIdx = messageIndex;
                if (captured[(size_t) messageIndex].isNoteOn()  && captured[(size_t) messageIndex].getNoteNumber() == noteG4 && onIdx < none) onIdx = messageIndex;
            }
            expect (offIdx >= none);            // held note 60 was closed at the swap
            expect (onIdx  >= none);            // Variation 2 note 67 started
            expect (offIdx < onIdx);         // closed before the new section sounded
        }

        beginTest (anEndingCompletingSilencesOutputNoHung);
        {
            // Ending holds note 72 with no note-off, so completing the ending must flush it. Verifies
            // the targeted-silence path actually closes the ending's sounding notes (no all-notes-off).
            ArrangerStyle st = fourFourStyle();
            {
                ArrangerTrack track; track.channel = accChannel; track.partType = ArrangerPartType::Acc;
                track.pattern = { { barStart, juce::MidiMessage::noteOn  (accChannel, middleC, velocity) },
                                  { halfBeat, juce::MidiMessage::noteOff (accChannel, middleC) } };
                ArrangerSection var; var.id = variationOneId; var.name = variationOne;
                var.type = ArrangerSectionType::Variation; var.lengthBars = oneBar; var.tracks.push_back (track);
                st.sections.push_back (var);
            }
            {
                ArrangerTrack endingTrack; endingTrack.channel = accChannel; endingTrack.partType = ArrangerPartType::Acc;
                endingTrack.pattern = { { barStart, juce::MidiMessage::noteOn (accChannel, noteC5, velocity) } }; // held, no off
                ArrangerSection end; end.id = endingOneId; end.name = endingOne;
                end.type = ArrangerSectionType::Ending; end.lengthBars = oneBar;
                end.afterComplete = ArrangerAfterComplete::Stop; end.tracks.push_back (endingTrack);
                st.sections.push_back (end);
            }

            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            SoundingNotes sounding;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { trackSounding (sounding, message); };
            engine.setStyle (st);
            engine.queueSection (ArrangerSectionType::Ending, endingOne);
            engine.renderRange (barStart, afterBarOne);   // enter ending at beat 4 (note 72 on, held)
            engine.renderRange (afterBarOne, afterBarTwo);   // ending completes at beat 8 -> stop -> must flush note 72
            int stillOn = countSounding (sounding);
            expect (! engine.isPlaying());
            expect (stillOn == none, endingLeftNotesHung + juce::String (stillOn));
        }

        beginTest (synchroStartOnlyAValidChordReleases);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeStyle());

            engine.armSynchro();
            expect (engine.isSynchroArmed());                 // armed and waiting

            engine.setActiveChord (ArrangerChord{});          // invalid -> stays armed
            expect (engine.isSynchroArmed());

            engine.setActiveChord (cMajor);   // C major -> releases the gate
            expect (! engine.isSynchroArmed());
        }

        beginTest (synchroStartStopClearsTheArmedGate);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeStyle());
            engine.armSynchro();
            expect (engine.isSynchroArmed());
            engine.stop();
            expect (! engine.isSynchroArmed());
        }

        beginTest (countInHelperOne44Bar);
        {
            auto clicks = ArrangerCountIn::clicksInWindow (barStart, fourBeats, fourBeats);
            expectEquals ((int) clicks.size(), clicksPerBar);
            expect (clicks[firstClick].beat == barStart && clicks[firstClick].velocity > clicks[secondClick].velocity);   // accent on beat 1
            for (auto& click : clicks) { expectEquals (click.channel, clickChannel); expectEquals (click.note, clickNote); }
            expectEquals ((int) ArrangerCountIn::clicksInWindow (barStart, twoBeats, fourBeats).size(), clicksPerHalfBar);
            expectEquals ((int) ArrangerCountIn::clicksInWindow (twoBeats, fourBeats, fourBeats).size(), clicksPerHalfBar);  // no seam dup
        }

        beginTest (countInHelper34Bar3);
        {
            expectEquals ((int) ArrangerCountIn::clicksInWindow (barStart, threeBeats, threeBeats).size(), clicksIn34);   // 3/4
            expect (ArrangerCountIn::clicksInWindow (fourBeats, eightBeats, fourBeats).empty());                 // beyond the bar -> none
            expect (ArrangerCountIn::clicksInWindow (tenthOfBeat, nineTenths, fourBeats).empty());                 // inside a beat gap -> none
            expectEquals ((int) ArrangerCountIn::clicksInWindow (halfBeat, beatAndAHalf, fourBeats).size(), oneClick);   // crossing beat 1 -> one
        }

        beginTest (countInEmitsABarOfClicks);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };
            engine.setStyle (makeStyle());          // 4/4

            engine.armCountIn();
            expect (engine.isCountingIn());
            engine.renderCountIn (fourBeats);             // one full bar
            expect (! engine.isCountingIn());

            int clicks {}, accents {};
            for (auto& message : captured)
                if (message.isNoteOn() && message.getChannel() == clickChannel && message.getNoteNumber() == clickNote)
                    { ++clicks; if (message.getVelocity() > plainClickLimit) ++accents; }
            expectEquals (clicks, clicksPerBar);
            expectEquals (accents, oneAccent);             // only beat 1 accented

            // After the count-in, the section plays normally.
            captured.clear();
            engine.renderRange (barStart, quarterBeat);        // makeStyle: note 60 on at beat 0
            bool note60 = false;
            for (auto& message : captured) if (message.isNoteOn() && message.getNoteNumber() == middleC) note60 = true;
            expect (note60, sectionSilentAfterCountIn);
        }

        beginTest (countInStopCancelsAnInProgress);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeStyle());
            engine.armCountIn();
            expect (engine.isCountingIn());
            engine.stop();
            expect (! engine.isCountingIn());
        }

        beginTest (selectStartSectionMakesStartBeginOnTheChosen);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeTwoVarStyle());
            engine.selectStartSection (ArrangerSectionType::Variation, variationTwo);
            expectEquals (engine.peekPendingStartIndex(), secondSection);
        }

        beginTest (activeChordTransposesAccNotesButLeaves);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };

            engine.setStyle (makeAccAndDrumStyle());                 // original = C major (default)
            engine.setActiveChord (dMajor);    // play D major (+2)
            engine.renderRange (barStart, oneBeat);

            bool accShifted = false, drumKept = false;
            for (auto& message : captured)
            {
                if (message.isNoteOn() && message.getChannel() == accChannel  && message.getNoteNumber() == noteD4) accShifted = true;
                if (message.isNoteOn() && message.getChannel() == drumChannel && message.getNoteNumber() == kickNote) drumKept   = true;
            }
            expect (accShifted, accShouldShift);
            expect (drumKept,   drumShouldStay);
        }

        beginTest (transposedNoteOffMatchesTheTransposedNote);
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };

            engine.setStyle (makeAccHeldNoteStyle());               // original C major
            engine.setActiveChord (dMajor);    // D major (+2): 60 -> 62
            engine.renderRange (barStart, pastTheSeam);                          // cross the seam -> synthetic off

            int on62 {}, off62 {}, on60 {}, off60 {};
            for (auto& message : captured)
            {
                if (message.getChannel() == accChannel && message.isNoteOn()  && message.getNoteNumber() == noteD4) ++on62;
                if (message.getChannel() == accChannel && message.isNoteOff() && message.getNoteNumber() == noteD4) ++off62;
                if (message.getChannel() == accChannel && message.isNoteOn()  && message.getNoteNumber() == middleC) ++on60;
                if (message.getChannel() == accChannel && message.isNoteOff() && message.getNoteNumber() == middleC) ++off60;
            }
            expect (on62  >= atLeastOne, noteOnTransposed);
            expect (off62 >= atLeastOne, seamNoteOffTransposed);
            expectEquals (on60,  none);   // nothing at the original pitch
            expectEquals (off60, none);
        }

        beginTest (perTrackNttTypeReachesEmitTimeTransposition);
        {
            // One Acc track whose NTT type is Parallel, playing a chromatic seed note (61 = C#).
            ArrangerTrack track; track.channel = accChannel; track.partType = ArrangerPartType::Acc;
            track.nttType = NttType::Parallel;
            track.pattern = { { barStart, juce::MidiMessage::noteOn  (accChannel, cSharp4, velocity) },
                              { halfBeat, juce::MidiMessage::noteOff (accChannel, cSharp4) } };
            ArrangerSection section; section.lengthBars = oneBar; section.tracks.push_back (track);
            ArrangerStyle style = fourFourStyle();
            style.originalRoot = pitchC; style.originalQuality = ChordQuality::Maj;   // C major
            style.sections.push_back (section);

            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { captured.push_back (message); };
            engine.setStyle (style);
            engine.setActiveChord (fMajor);   // F major (+5)
            engine.renderRange (barStart, oneBeat);

            // Parallel: 61 + 5 = 66. Chord would snap the passing tone C# to F-Ionian -> 65.
            bool got66 = false, got65 = false;
            for (auto& message : captured)
                if (message.isNoteOn() && message.getChannel() == accChannel)
                {
                    if (message.getNoteNumber() == noteFSharp4) got66 = true;
                    if (message.getNoteNumber() == noteF4) got65 = true;
                }
            expect (got66,   parallelShouldEmit66);
            expect (! got65, parallelMustNotSnap);
        }
    }
};

static ArrangerEngineTest arrangerEngineTest;
