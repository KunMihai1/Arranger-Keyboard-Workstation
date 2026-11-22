#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ChordDetector.h"

namespace ChordDetectorTestValues
{
    const juce::String testName     = "ChordDetector";
    const juce::String testCategory = "Arranger";

    const juce::String noNotesInvalid           = "no held notes -> invalid chord";
    const juce::String triadsAndSevenths        = "recognizes common triads and sevenths";
    const juce::String inversionsResolve        = "inversions resolve to the same chord and root, bass = lowest note";
    const juce::String sparseSetIsNone          = "ambiguous/sparse set -> None (split mode)";
    const juce::String noteOffRemovesNotes      = "noteOff removes notes; emptying returns invalid (split)";
    const juce::String oneFingerMapping         = "single-finger: Korg one-finger mapping";
    const juce::String bassTriadWins            = "full-keyboard: bass triad wins, melody on top never lurches";
    const juce::String chordToneMelodyKeeps     = "full-keyboard: a chord-tone melody note does not change the chord";
    const juce::String needsThreeNotes          = "full-keyboard: requires >=3 notes forming a known chord";
    const juce::String nonChordMelodyHolds      = "full-keyboard: holds the chord when a non-chord melody note is added";
    const juce::String oneFingerRootsAndColours = "single-finger: roots track the highest note; colour rules with extra keys";
    const juce::String bassHandInversion        = "full-keyboard: recognises a bass-hand inversion (E-G-C low)";
    const juce::String modeSwitchRecomputes     = "mode switch re-recognises the currently held notes";
    const juce::String keyFinderCMajor          = "key finder picks C major from a C-major-heavy pattern";
    const juce::String keyFinderMinorAndEmpty   = "key finder detects minor and falls back to C major on empty";

    const juce::String emptyDetectorGaveRoot = "empty detector gave root=";
    const juce::String twoNoteFullKbGaveRoot = "2-note full-kb gave root=";
    const juce::String qualityLabel          = " q=";
    const juce::String lurchedToSeventh      = "lurched to a 7th on a melody note";

    // Pitch classes (0 = C .. 11 = B).
    constexpr int pitchC      = 0;
    constexpr int pitchD      = 2;
    constexpr int pitchE      = 4;
    constexpr int pitchF      = 5;
    constexpr int pitchFSharp = 6;
    constexpr int pitchG      = 7;
    constexpr int pitchA      = 9;

    // Fingered chords, C4 = 60.
    const std::vector<int> cMajor           { 60, 64, 67 };   // C E G
    const std::vector<int> cMinor           { 60, 63, 67 };   // C Eb G
    const std::vector<int> cDominant7       { 60, 64, 67, 70 };   // C E G Bb
    const std::vector<int> cMajor7          { 60, 64, 67, 71 };   // C E G B
    const std::vector<int> cMinor7          { 60, 63, 67, 70 };   // C Eb G Bb
    const std::vector<int> cDiminished      { 60, 63, 66 };   // C Eb Gb
    const std::vector<int> cSus4            { 60, 65, 67 };   // C F G
    const std::vector<int> cSus2            { 60, 62, 67 };   // C D G
    const std::vector<int> dMajor           { 62, 66, 69 };   // D F# A
    const std::vector<int> cMajorFirstInversion  { 64, 67, 72 };   // E G C
    const std::vector<int> cMajorSecondInversion { 67, 72, 76 };   // G C E

    // Single-finger key sets.
    const std::vector<int> cAlone              { 60 };
    const std::vector<int> cWithWhiteBelow     { 60, 59 };        // C + B
    const std::vector<int> cWithBlackBelow     { 60, 58 };        // C + Bb
    const std::vector<int> cWithWhiteAndBlack  { 60, 59, 58 };    // C + B + Bb
    const std::vector<int> cWithLowG           { 55, 60 };        // root C(60), G(55) white-left
    const std::vector<int> fAlone              { 65 };
    const std::vector<int> gWithWhiteBelow     { 65, 67 };        // G(67) + F(65 white)
    const std::vector<int> fSharpAlone         { 66 };
    const std::vector<int> cWithManyBelow      { 72, 71, 70, 68 };   // C; B(white) + Bb, Ab(black)

    // Full-keyboard bass-hand chords.
    const std::vector<int> lowCMajor          { 48, 52, 55 };
    const std::vector<int> lowFMajor          { 41, 45, 48 };
    const std::vector<int> lowCMajorInversion { 52, 55, 60 };   // E G C

    constexpr int bassE           = 4;    // lowest note of the first inversion
    constexpr int bassG           = 7;    // lowest note of the second inversion
    constexpr int middleC         = 60;
    constexpr int cSharp          = 61;
    constexpr int noteE4          = 64;
    constexpr int noteG4          = 67;
    constexpr int noteB3          = 59;
    constexpr int melodyBFlat     = 70;
    constexpr int highC           = 84;
    constexpr int melodyD         = 86;

    // Key finder patterns.
    const std::vector<int> cMajorHeavy { 60, 64, 67, 60, 64, 67, 60, 67 };
    const std::vector<int> aMinorHeavy { 57, 60, 64, 57, 60, 64, 57 };   // A C E
    constexpr double      patternBeat     = 0.0;
    constexpr int         patternChannel  = 2;
    constexpr juce::uint8 patternVelocity = 100;
}

using namespace ChordDetectorTestValues;

class ChordDetectorTest : public juce::UnitTest
{
public:
    ChordDetectorTest() : juce::UnitTest (ChordDetectorTestValues::testName, ChordDetectorTestValues::testCategory) {}

    static ArrangerChord detect (const std::vector<int>& notes, ChordMode mode = ChordMode::Fingered)
    {
        ChordDetector detector; detector.setMode (mode);
        for (int note : notes) detector.noteOn (note);
        return detector.current();
    }

    static std::vector<TimedBeatEvent> noteOns (const std::vector<int>& notes)
    {
        std::vector<TimedBeatEvent> events;
        for (int note : notes)
            events.push_back ({ patternBeat, juce::MidiMessage::noteOn (patternChannel, note, patternVelocity) });
        return events;
    }

    void runTest() override
    {
        beginTest (noNotesInvalid);
        {
            ChordDetector detector;
            ArrangerChord chord = detector.current();
            expect (! chord.isValid(), emptyDetectorGaveRoot + juce::String (chord.root)
                                       + qualityLabel + toString (chord.quality));
        }

        beginTest (triadsAndSevenths);
        {
            expect (detect(cMajor).quality      == ChordQuality::Maj);
            expect (detect(cMinor).quality      == ChordQuality::Min);
            expect (detect(cDominant7).quality  == ChordQuality::Dom7);
            expect (detect(cMajor7).quality     == ChordQuality::Maj7);
            expect (detect(cMinor7).quality     == ChordQuality::Min7);
            expect (detect(cDiminished).quality == ChordQuality::Dim);
            expect (detect(cSus4).quality       == ChordQuality::Sus4);
            expect (detect(cSus2).quality       == ChordQuality::Sus2);
            expectEquals (detect(cMajor).root, pitchC);
            expectEquals (detect(dMajor).root, pitchD);                    // D F# A -> D major
        }

        beginTest (inversionsResolve);
        {
            // E G C and G C E are both C major, root C
            expect (detect(cMajorFirstInversion).quality == ChordQuality::Maj);
            expectEquals (detect(cMajorFirstInversion).root, pitchC);
            expectEquals (detect(cMajorFirstInversion).bassNote, bassE);   // lowest = E
            expectEquals (detect(cMajorSecondInversion).root, pitchC);
            expectEquals (detect(cMajorSecondInversion).bassNote, bassG);  // lowest = G
        }

        beginTest (sparseSetIsNone);
        {
            ChordDetector detector; detector.noteOn (middleC); detector.noteOn (cSharp);   // C + C#: not a known chord
            expect (! detector.current().isValid());
        }

        beginTest (noteOffRemovesNotes);
        {
            ChordDetector detector;
            detector.noteOn (middleC); detector.noteOn (noteE4); detector.noteOn (noteG4);
            expect (detector.current().isValid());
            detector.noteOff (middleC); detector.noteOff (noteE4); detector.noteOff (noteG4);
            expect (! detector.current().isValid());
        }

        beginTest (oneFingerMapping);
        {
            expect (detect(cAlone,             ChordMode::SingleFinger).quality == ChordQuality::Maj);   // C alone
            expect (detect(cWithWhiteBelow,    ChordMode::SingleFinger).quality == ChordQuality::Dom7);  // C + B  (white left)
            expect (detect(cWithBlackBelow,    ChordMode::SingleFinger).quality == ChordQuality::Min);   // C + Bb (black left)
            expect (detect(cWithWhiteAndBlack, ChordMode::SingleFinger).quality == ChordQuality::Min7);  // C + B + Bb
            expectEquals (detect(cAlone, ChordMode::SingleFinger).root, pitchC);                      // root C
            // root is the HIGHEST held note; lower notes are modifiers:
            expect (detect(cWithLowG,          ChordMode::SingleFinger).quality == ChordQuality::Dom7);  // root C(60), G(55) white-left
            expectEquals (detect(cWithLowG, ChordMode::SingleFinger).root, pitchC);
            {
                ChordDetector detector; detector.setMode (ChordMode::SingleFinger);
                expect (! detector.current().isValid());
            }
        }

        beginTest (bassTriadWins);
        {
            ChordDetector detector; detector.setMode (ChordMode::FullKeyboard);
            for (int note : lowCMajor) detector.noteOn (note);            // C major in the bass hand
            expect (detector.current().quality == ChordQuality::Maj);
            detector.noteOn (melodyBFlat);                                // Bb melody on top: full set would read C7
            expect (detector.current().quality == ChordQuality::Maj, lurchedToSeventh);
            expectEquals (detector.current().root, pitchC);

            // Moving the bass hand DOES change the chord (responsive, no debounce):
            for (int note : lowCMajor) detector.noteOff (note);
            for (int note : lowFMajor) detector.noteOn (note);            // F major in the bass
            expectEquals (detector.current().root, pitchF);               // F
            expect (detector.current().quality == ChordQuality::Maj);
        }

        beginTest (chordToneMelodyKeeps);
        {
            ChordDetector detector; detector.setMode (ChordMode::FullKeyboard);
            for (int note : lowCMajor) detector.noteOn (note);   // C major (low)
            ArrangerChord before = detector.current();
            expect (before.quality == ChordQuality::Maj);
            detector.noteOn (highC);                              // high C (a chord tone)
            expect (detector.current() == before);
        }

        beginTest (needsThreeNotes);
        {
            ChordDetector detector; detector.setMode (ChordMode::FullKeyboard);
            detector.noteOn (middleC); detector.noteOn (highC);   // only 2 notes
            ArrangerChord chord = detector.current();
            expect (! chord.isValid(), twoNoteFullKbGaveRoot + juce::String (chord.root)
                                       + qualityLabel + toString (chord.quality));
        }

        beginTest (nonChordMelodyHolds);
        {
            ChordDetector detector; detector.setMode (ChordMode::FullKeyboard);
            for (int note : lowCMajor) detector.noteOn (note);    // C major
            detector.noteOn (melodyD);                             // D (not a C-major tone) -> hold C major
            expect (detector.current().quality == ChordQuality::Maj);
            expectEquals (detector.current().root, pitchC);
        }

        beginTest (oneFingerRootsAndColours);
        {
            expect (detect(fAlone, ChordMode::SingleFinger).quality == ChordQuality::Maj);   // F alone
            expectEquals (detect(fAlone, ChordMode::SingleFinger).root, pitchF);           // F
            expect (detect(gWithWhiteBelow, ChordMode::SingleFinger).quality == ChordQuality::Dom7); // G(67)+F(65 white) -> G7
            expectEquals (detect(gWithWhiteBelow, ChordMode::SingleFinger).root, pitchG);  // G
            expectEquals (detect(fSharpAlone, ChordMode::SingleFinger).root, pitchFSharp); // F# major
            // any white AND any black below the root -> minor 7, regardless of how many keys
            expect (detect(cWithManyBelow, ChordMode::SingleFinger).quality == ChordQuality::Min7); // C; B(white)+Bb,Ab(black)
        }

        beginTest (bassHandInversion);
        {
            ChordDetector detector; detector.setMode (ChordMode::FullKeyboard);
            for (int note : lowCMajorInversion) detector.noteOn (note);   // E G C low = C major, first inversion
            expectEquals (detector.current().root, pitchC);
            expect (detector.current().quality == ChordQuality::Maj);
        }

        beginTest (modeSwitchRecomputes);
        {
            ChordDetector detector; detector.setMode (ChordMode::Fingered);
            detector.noteOn (middleC); detector.noteOn (noteB3);            // C + B: not a Fingered chord
            expect (! detector.current().isValid());
            detector.setMode (ChordMode::SingleFinger);     // setMode() recomputes from the held notes
            expect (detector.current().quality == ChordQuality::Dom7);   // C7 (root C, B is a white key to the left)
            expectEquals (detector.current().root, pitchC);
        }

        beginTest (keyFinderCMajor);
        {
            ArrangerChord key = detectKeyFromEvents (noteOns (cMajorHeavy));
            expectEquals (key.root, pitchC);
            expect (key.quality == ChordQuality::Maj);
        }

        beginTest (keyFinderMinorAndEmpty);
        {
            ArrangerChord key = detectKeyFromEvents (noteOns (aMinorHeavy));   // A C E -> A minor
            expectEquals (key.root, pitchA);                  // A
            expect (key.quality == ChordQuality::Min);

            ArrangerChord fallback = detectKeyFromEvents ({});
            expectEquals (fallback.root, pitchC);
            expect (fallback.quality == ChordQuality::Maj);
        }
    }
};
static ChordDetectorTest chordDetectorTest;
