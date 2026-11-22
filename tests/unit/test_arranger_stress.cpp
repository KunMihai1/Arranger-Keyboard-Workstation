#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Arranger/ArrangerEngine.h"
#include "Arranger/ArrangerModel.h"
#include <map>
#include <utility>

/*
    Arranger hard testing, part A (docs/superpowers/test-plans/2026-11-22-arranger-stress-test-plan.md):
    drive the real ArrangerEngine through thousands of bars of adversarial input - chord storms, section
    switch storms, option toggling, stops at random moments, Endings - and check every MIDI message it
    emits:
      - note numbers, channels and velocities are in the MIDI range;
      - no note is left sounding after a stop or an Ending;
      - the active section is always a real one.
    Deterministic: a fixed random seed, and renderRange() instead of the real-time timer.
*/
namespace ArrangerStressValues
{
    const juce::String testName     = "Arranger stress (headless)";
    const juce::String testCategory = "Arranger";

    constexpr juce::int64 randomSeed = 20261122;

    constexpr int lowestMidiNote     = 0;
    constexpr int highestMidiNote    = 127;
    constexpr int lowestMidiChannel  = 1;
    constexpr int highestMidiChannel = 16;
    constexpr int highestVelocity    = 127;

    constexpr int drumChannel          = 10;
    constexpr int bassChannel          = 2;
    constexpr int accompanimentChannel = 3;
    constexpr int kickNote             = 36;
    constexpr int bassNote             = 40;
    constexpr int lowEdgeNote          = 1;     // near the bottom of the MIDI range: transposition must clamp
    constexpr int highEdgeNote         = 126;   // near the top of the MIDI range
    constexpr juce::uint8 velocity     = 100;

    constexpr int    beatsPerBar   = 4;
    constexpr int    beatUnit      = 4;
    constexpr int    sectionBars   = 1;
    constexpr double styleTempoBpm = 120.0;

    constexpr double onBeat       = 0.0;
    constexpr double quarterBeat  = 0.25;
    constexpr double halfBeat     = 0.5;
    constexpr double threeQuarter = 0.75;
    constexpr double secondBeat   = 1.0;
    constexpr double thirdBeat    = 2.0;
    constexpr double lastBeat     = 3.5;

    // Render windows from a 1/64 note to two bars, like timer ticks at any tempo plus long stalls.
    const std::vector<double> windowLengthsInBeats { 0.0625, 0.25, 0.5, 1.0, 3.0, 8.0 };

    constexpr int stormWindows      = 20000;
    constexpr int stopCycles        = 600;
    constexpr int windowsPerCycle   = 12;
    constexpr int endingSafetyBars  = 16;
    constexpr int chanceDenominator = 4;   // "one time in four": change chord / section / option

    constexpr int pitchClassCount = 12;
    constexpr int pitchClassC     = 0;
    constexpr int pitchClassFSharp = 6;    // a tritone away from C: the largest transposition
    constexpr int noBassNote      = -1;
    constexpr int noChordRoot     = -1;
    constexpr int heldHarmonyNote = 64;
    constexpr int atLeastOneWindow = 1;

    const std::vector<ChordQuality> everyChordQuality {
        ChordQuality::None, ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7, ChordQuality::Maj7,
        ChordQuality::Min7, ChordQuality::Dim, ChordQuality::HalfDim, ChordQuality::Aug, ChordQuality::Sus2,
        ChordQuality::Sus4 };
    const std::vector<MinorScaleChoice> everyMinorScale {
        MinorScaleChoice::Dorian, MinorScaleChoice::Aeolian, MinorScaleChoice::HarmonicMinor };

    // What a random section request asks for; "ending" is last so a storm can leave it out.
    enum class SectionRequest { intro, variation, fill, breakSection, unknownLabel, ending, count };

    const juce::String variationName = "Variation";
    const juce::String fillName      = "Fill";
    const juce::String introName     = "Intro";
    const juce::String breakName     = "Break";
    const juce::String endingName    = "Ending";
    const juce::String nameSeparator = " ";
    const juce::String unknownName   = "Var 9";   // a label no section has
    constexpr int numberedSections   = 4;          // Variation 1-4, Fill 1-4
    constexpr int firstNumber        = 1;
}

namespace ArrangerStressText
{
    const juce::String stormTitle       = "chord + section + option storm: every message valid, nothing left sounding";
    const juce::String stopTitle        = "stopping at random moments never leaves a note sounding";
    const juce::String endingTitle      = "an Ending stops by itself and leaves no note sounding";
    const juce::String overlapTitle     = "the same pitch started twice across a chord change still gets released";

    const juce::String noteOutOfRange     = "note number outside 0..127";
    const juce::String channelOutOfRange  = "channel outside 1..16";
    const juce::String velocityOutOfRange = "velocity above 127";
    const juce::String stuckNote          = "a note is still sounding after the arranger stopped";
    const juce::String noSectionActive    = "the active section is not one of the style's sections";
    const juce::String endingNeverStopped = "the Ending never stopped the arranger";
    const juce::String nothingPlayed      = "the arranger emitted no notes at all";
}

class ArrangerStressTest : public juce::UnitTest
{
public:
    ArrangerStressTest() : juce::UnitTest (ArrangerStressValues::testName, ArrangerStressValues::testCategory) {}

    void runTest() override
    {
        chordSectionAndOptionStorm();
        stoppingAtRandomMomentsLeavesNothingSounding();
        endingStopsAndLeavesNothingSounding();
        samePitchStartedTwiceAcrossAChordChangeIsReleased();
    }

private:
    // Counts sounding notes per (channel, note) from the emitted stream and checks every message.
    struct NoteLedger
    {
        std::map<std::pair<int, int>, int> soundingCount;
        int notesStarted {};
        int invalidNotes {}, invalidChannels {}, invalidVelocities {};

        void record (const juce::MidiMessage& message)
        {
            using namespace ArrangerStressValues;
            if (message.getChannel() < lowestMidiChannel || message.getChannel() > highestMidiChannel)
                ++invalidChannels;
            if (! message.isNoteOnOrOff())
                return;
            if (message.getNoteNumber() < lowestMidiNote || message.getNoteNumber() > highestMidiNote)
                ++invalidNotes;
            if (message.getVelocity() > highestVelocity)
                ++invalidVelocities;

            auto& count = soundingCount[{ message.getChannel(), message.getNoteNumber() }];
            if (message.isNoteOn())
            {
                ++count;
                ++notesStarted;
            }
            else if (count > int {})
            {
                --count;
            }
        }

        int stillSounding() const
        {
            int sounding {};
            for (const auto& entry : soundingCount)
                sounding += entry.second;
            return sounding;
        }
    };

    static ArrangerTrack makeTrack (ArrangerPartType part, NttType transposition, int channel,
                                    std::vector<TimedBeatEvent> pattern)
    {
        ArrangerTrack track;
        track.partType = part;
        track.nttType  = transposition;
        track.channel  = channel;
        track.pattern  = std::move (pattern);
        return track;
    }

    static std::vector<TimedBeatEvent> noteBetween (int channel, int note, double onBeats, double offBeats)
    {
        using namespace ArrangerStressValues;
        return { { onBeats,  juce::MidiMessage::noteOn  (channel, note, velocity) },
                 { offBeats, juce::MidiMessage::noteOff (channel, note) } };
    }

    static ArrangerSection makeSection (ArrangerSectionType type, const juce::String& name, ArrangerAfterComplete after)
    {
        using namespace ArrangerStressValues;
        ArrangerSection section;
        section.type          = type;
        section.name          = name;
        section.id            = name;
        section.lengthBars    = sectionBars;
        section.afterComplete = after;

        auto accompaniment = noteBetween (accompanimentChannel, lowEdgeNote, onBeat, halfBeat);
        for (const auto& event : noteBetween (accompanimentChannel, highEdgeNote, secondBeat, thirdBeat))
            accompaniment.push_back (event);
        for (const auto& event : noteBetween (accompanimentChannel, heldHarmonyNote, quarterBeat, lastBeat))
            accompaniment.push_back (event);   // held across most of the bar, so section switches cut it

        section.tracks.push_back (makeTrack (ArrangerPartType::Drum, NttType::NoTranspose, drumChannel,
                                             noteBetween (drumChannel, kickNote, onBeat, quarterBeat)));
        section.tracks.push_back (makeTrack (ArrangerPartType::Bass, NttType::Chord, bassChannel,
                                             noteBetween (bassChannel, bassNote, onBeat, lastBeat)));
        section.tracks.push_back (makeTrack (ArrangerPartType::Acc, NttType::Chord, accompanimentChannel, accompaniment));
        return section;
    }

    static juce::String numbered (const juce::String& name, int number)
    {
        return name + ArrangerStressValues::nameSeparator + juce::String (number);
    }

    // Intro, Variation 1-4, Fill 1-4, Break and Ending, each one bar with drums, bass and harmony.
    static ArrangerStyle makeFullStyle()
    {
        using namespace ArrangerStressValues;
        ArrangerStyle style;
        style.timeSigNum    = beatsPerBar;
        style.timeSigDenom  = beatUnit;
        style.originalTempo = styleTempoBpm;
        for (int number = firstNumber; number <= numberedSections; ++number)
            style.sections.push_back (makeSection (ArrangerSectionType::Variation, numbered (variationName, number), ArrangerAfterComplete::Loop));
        style.sections.push_back (makeSection (ArrangerSectionType::Intro, numbered (introName, firstNumber), ArrangerAfterComplete::FallThrough));
        for (int number = firstNumber; number <= numberedSections; ++number)
            style.sections.push_back (makeSection (ArrangerSectionType::Fill, numbered (fillName, number), ArrangerAfterComplete::FallThrough));
        style.sections.push_back (makeSection (ArrangerSectionType::Break, numbered (breakName, firstNumber), ArrangerAfterComplete::FallThrough));
        style.sections.push_back (makeSection (ArrangerSectionType::Ending, numbered (endingName, firstNumber), ArrangerAfterComplete::Stop));
        return style;
    }

    template <typename Choice>
    static Choice pickOne (juce::Random& random, const std::vector<Choice>& choices)
    {
        return choices[(size_t) random.nextInt ((int) choices.size())];
    }

    static ArrangerChord randomChord (juce::Random& random)
    {
        using namespace ArrangerStressValues;
        ArrangerChord chord;
        chord.quality  = pickOne (random, everyChordQuality);   // includes "no chord"
        chord.root     = chord.quality == ChordQuality::None ? noChordRoot : random.nextInt (pitchClassCount);
        chord.bassNote = random.nextBool() ? noBassNote : random.nextInt (pitchClassCount);   // slash chords
        return chord;
    }

    static void queueRandomSection (ArrangerEngine& engine, juce::Random& random, bool allowEnding)
    {
        using namespace ArrangerStressValues;
        const int number = random.nextInt (numberedSections) + firstNumber;
        const auto lastAllowed = allowEnding ? SectionRequest::count : SectionRequest::ending;
        switch (static_cast<SectionRequest> (random.nextInt (static_cast<int> (lastAllowed))))
        {
            case SectionRequest::intro:        engine.queueSection (ArrangerSectionType::Intro, numbered (introName, firstNumber)); break;
            case SectionRequest::variation:    engine.queueSection (ArrangerSectionType::Variation, numbered (variationName, number)); break;
            case SectionRequest::fill:         engine.queueSection (ArrangerSectionType::Fill, numbered (fillName, number)); break;
            case SectionRequest::breakSection: engine.queueSection (ArrangerSectionType::Break, numbered (breakName, firstNumber)); break;
            case SectionRequest::ending:       engine.queueSection (ArrangerSectionType::Ending, numbered (endingName, firstNumber)); break;
            case SectionRequest::unknownLabel:
            case SectionRequest::count:        engine.queueSection (ArrangerSectionType::Variation, unknownName); break;
        }
    }

    static bool oneTimeIn (juce::Random& random) { return random.nextInt (ArrangerStressValues::chanceDenominator) == int {}; }

    static double randomWindow (juce::Random& random)
    {
        const auto& lengths = ArrangerStressValues::windowLengthsInBeats;
        return lengths[(size_t) random.nextInt ((int) lengths.size())];
    }

    void expectAllMessagesValid (const NoteLedger& ledger)
    {
        using namespace ArrangerStressText;
        expect (ledger.invalidNotes == int {}, noteOutOfRange);
        expect (ledger.invalidChannels == int {}, channelOutOfRange);
        expect (ledger.invalidVelocities == int {}, velocityOutOfRange);
    }

    void chordSectionAndOptionStorm()
    {
        using namespace ArrangerStressValues;
        using namespace ArrangerStressText;
        beginTest (stormTitle);

        juce::Random random (randomSeed);
        ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
        NoteLedger ledger;
        engine.onMidiMessage = [&] (const juce::MidiMessage& message) { ledger.record (message); };
        const auto style = makeFullStyle();
        engine.setStyle (style);

        bool sectionAlwaysValid = true;
        double playhead {};
        for (int window {}; window < stormWindows; ++window)
        {
            if (oneTimeIn (random)) engine.setActiveChord (randomChord (random));
            if (oneTimeIn (random)) queueRandomSection (engine, random, false);
            if (oneTimeIn (random)) engine.setBassInversion (random.nextBool());
            if (oneTimeIn (random)) engine.setNttEnabled (random.nextBool());
            if (oneTimeIn (random)) engine.setMinorScale (pickOne (random, everyMinorScale));

            const double windowLength = randomWindow (random);
            engine.renderRange (playhead, playhead + windowLength);
            playhead += windowLength;

            const int active = engine.getActiveSectionIndex();
            sectionAlwaysValid = sectionAlwaysValid && active >= int {} && active < (int) style.sections.size();
        }
        engine.stop();

        expect (ledger.notesStarted > int {}, nothingPlayed);
        expect (sectionAlwaysValid, noSectionActive);
        expectAllMessagesValid (ledger);
        expectEquals (ledger.stillSounding(), int {}, stuckNote);
    }

    void stoppingAtRandomMomentsLeavesNothingSounding()
    {
        using namespace ArrangerStressValues;
        using namespace ArrangerStressText;
        beginTest (stopTitle);

        juce::Random random (randomSeed);
        ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
        NoteLedger ledger;
        engine.onMidiMessage = [&] (const juce::MidiMessage& message) { ledger.record (message); };
        engine.setStyle (makeFullStyle());

        int cyclesLeavingNotes {};
        double playhead {};
        for (int cycle {}; cycle < stopCycles; ++cycle)
        {
            const int windowsThisCycle = random.nextInt (windowsPerCycle) + atLeastOneWindow;
            for (int window {}; window < windowsThisCycle; ++window)
            {
                if (oneTimeIn (random)) engine.setActiveChord (randomChord (random));
                if (oneTimeIn (random)) queueRandomSection (engine, random, false);
                const double windowLength = randomWindow (random);
                engine.renderRange (playhead, playhead + windowLength);
                playhead += windowLength;
            }
            engine.stop();
            if (ledger.stillSounding() != int {})
                ++cyclesLeavingNotes;
        }

        expectAllMessagesValid (ledger);
        expectEquals (cyclesLeavingNotes, int {}, stuckNote);
    }

    void endingStopsAndLeavesNothingSounding()
    {
        using namespace ArrangerStressValues;
        using namespace ArrangerStressText;
        beginTest (endingTitle);

        juce::Random random (randomSeed);
        ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
        NoteLedger ledger;
        engine.onMidiMessage = [&] (const juce::MidiMessage& message) { ledger.record (message); };
        engine.setStyle (makeFullStyle());

        double playhead {};
        for (int bar {}; bar < endingSafetyBars; ++bar)   // play a little first, with chords
        {
            engine.setActiveChord (randomChord (random));
            engine.renderRange (playhead, playhead + beatsPerBar);
            playhead += beatsPerBar;
        }

        // When the Ending completes the engine halts and resets to the first section (in the app its timer
        // then stops; here nothing stops the rendering, so watch for that moment, one beat at a time).
        const auto style = makeFullStyle();
        const auto ending = std::find_if (style.sections.begin(), style.sections.end(),
                                          [] (const ArrangerSection& section) { return section.type == ArrangerSectionType::Ending; });
        const int endingIndex = (int) std::distance (style.sections.begin(), ending);
        engine.queueSection (ArrangerSectionType::Ending, numbered (endingName, firstNumber));
        bool endingPlayed = false, ended = false;
        int soundingWhenEnded {};
        for (int beat {}; beat < endingSafetyBars * beatsPerBar && ! ended; ++beat)
        {
            endingPlayed = endingPlayed || engine.getActiveSectionIndex() == endingIndex;
            engine.renderRange (playhead, playhead + secondBeat);
            playhead += secondBeat;
            if (endingPlayed && engine.getActiveSectionIndex() != endingIndex)
            {
                ended = true;
                soundingWhenEnded = ledger.stillSounding();
            }
        }

        expect (ended, endingNeverStopped);
        expectEquals (soundingWhenEnded, int {}, stuckNote);
        expectAllMessagesValid (ledger);
    }

    void samePitchStartedTwiceAcrossAChordChangeIsReleased()
    {
        using namespace ArrangerStressValues;
        using namespace ArrangerStressText;
        beginTest (overlapTitle);

        // One harmony track that starts the same pitch twice before releasing it. The chord changes in
        // between, so the two starts sound at different transposed pitches.
        ArrangerSection section;
        section.type = ArrangerSectionType::Variation;
        section.name = numbered (variationName, firstNumber);
        section.lengthBars = sectionBars;
        section.tracks.push_back (makeTrack (ArrangerPartType::Acc, NttType::Chord, accompanimentChannel, {
            { onBeat,       juce::MidiMessage::noteOn  (accompanimentChannel, bassNote, velocity) },
            { halfBeat,     juce::MidiMessage::noteOn  (accompanimentChannel, bassNote, velocity) },
            { threeQuarter, juce::MidiMessage::noteOff (accompanimentChannel, bassNote) },
            { lastBeat,     juce::MidiMessage::noteOff (accompanimentChannel, bassNote) } }));
        ArrangerStyle style;
        style.timeSigNum = beatsPerBar;
        style.timeSigDenom = beatUnit;
        style.originalTempo = styleTempoBpm;
        style.sections.push_back (section);

        ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
        NoteLedger ledger;
        engine.onMidiMessage = [&] (const juce::MidiMessage& message) { ledger.record (message); };
        engine.setStyle (style);

        ArrangerChord cMajor;
        cMajor.root = pitchClassC;
        cMajor.quality = ChordQuality::Maj;
        ArrangerChord fSharpMajor = cMajor;
        fSharpMajor.root = pitchClassFSharp;

        engine.setActiveChord (cMajor);
        engine.renderRange (onBeat, quarterBeat);          // first start sounds under C
        engine.setActiveChord (fSharpMajor);
        engine.renderRange (quarterBeat, beatsPerBar);     // second start under F#, then both releases

        expectEquals (ledger.stillSounding(), int {}, stuckNote);
        engine.stop();
    }
};

static ArrangerStressTest arrangerStressTest;
