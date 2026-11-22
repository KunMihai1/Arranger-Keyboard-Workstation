#include <juce_core/juce_core.h>
#include "Arranger/ChordTransposer.h"

namespace ChordTransposerTestValues
{
    const juce::String transposerTestName = "ChordTransposer";
    const juce::String scalesTestName     = "NttScales";
    const juce::String testCategory       = "Arranger";

    const juce::String fixedNeverTransposed     = "fixed parts (drums) are never transposed";
    const juce::String invalidActiveIdentity    = "invalid active chord -> identity";
    const juce::String sameChordIdentity        = "identity when active == original";
    const juce::String nonChordTonesPreserved   = "non-chord tones are PRESERVED at identity (not snapped to chord tones)";
    const juce::String passingNotesShift        = "melodic/passing notes shift by the root interval (kept relative to the root)";
    const juce::String onlyThirdFlips           = "only the 3rd flips on major->minor; other notes just shift";
    const juce::String thirdMappedDifferently   = "NTT maps the 3rd differently from root/5th (C maj -> A min)";
    const juce::String blockShift               = "block-shift case (same quality) shifts by root interval";
    const juce::String resultInMidiRange        = "result stays in 0..127";
    const juce::String bassInversionRebases     = "bass inversion re-bases the WHOLE bass line on the played bass note";
    const juce::String parallelShifts           = "Parallel shifts every note by the root interval";
    const juce::String chordRemapsRoles         = "Chord remaps 3rd/5th to the active chord's roles";
    const juce::String chordSnapsPassingTone    = "Chord snaps a passing tone to the active scale (tie resolves down)";
    const juce::String noTransposeIsInput       = "NoTranspose returns the input note";
    const juce::String fixedKeepsNearestOctave  = "Fixed keeps the octave nearest the original note (no drift)";
    const juce::String minorScaleChangesSnap    = "minor-scale selector changes the snap target";
    const juce::String wrapAroundFolds          = "wrap-around folds a >1-octave result back toward the origin";
    const juce::String chordResultInMidiRange   = "Chord result stays within 0..127 across the whole range";
    const juce::String bypassReturnsInput       = "master bypass: disabled NTT returns input notes for all parts/types";
    const juce::String bypassBeatsBassInversion = "master bypass beats bass inversion (regression: bypass used to miss the bass)";
    const juce::String majorUsesIonian          = "major qualities use Ionian";
    const juce::String dominantUsesMixolydian   = "dominant + sus use Mixolydian (flat 7)";
    const juce::String minorRowSwaps            = "minor row swaps with the MinorScaleChoice";
    const juce::String augAndDimScales          = "aug is whole-tone, dim is whole-half";

    // Pitch classes (0 = C .. 11 = B).
    constexpr int pitchC      = 0;
    constexpr int pitchD      = 2;
    constexpr int pitchE      = 4;
    constexpr int pitchF      = 5;
    constexpr int pitchFSharp = 6;
    constexpr int pitchG      = 7;
    constexpr int pitchA      = 9;
    constexpr int pitchB      = 11;
    constexpr int semitonesPerOctave = 12;

    // Chords as { root, quality, bass note }.
    const ArrangerChord cMajor        { pitchC, ChordQuality::Maj,  pitchC };
    const ArrangerChord cMinor        { pitchC, ChordQuality::Min,  pitchC };
    const ArrangerChord cMajorOverE   { pitchC, ChordQuality::Maj,  pitchE };
    const ArrangerChord cDiminished   { pitchC, ChordQuality::Dim,  pitchC };
    const ArrangerChord dMajor        { pitchD, ChordQuality::Maj,  pitchD };
    const ArrangerChord fMajor        { pitchF, ChordQuality::Maj,  pitchF };
    const ArrangerChord fMinor        { pitchF, ChordQuality::Min,  pitchF };
    const ArrangerChord gDominant7    { pitchG, ChordQuality::Dom7, pitchG };
    const ArrangerChord aMajor        { pitchA, ChordQuality::Maj,  pitchA };
    const ArrangerChord aMinor        { pitchA, ChordQuality::Min,  pitchA };
    const ArrangerChord bAugmented    { pitchB, ChordQuality::Aug,  pitchB };

    // MIDI notes.
    constexpr int noteC2      = 36;
    constexpr int noteB2      = 47;
    constexpr int noteG2      = 43;
    constexpr int noteA3      = 57;
    constexpr int noteC4      = 60;
    constexpr int noteCSharp4 = 61;
    constexpr int noteD4      = 62;
    constexpr int noteEFlat4  = 63;
    constexpr int noteE4      = 64;
    constexpr int noteF4      = 65;
    constexpr int noteFSharp4 = 66;
    constexpr int noteG4      = 67;
    constexpr int noteGSharp4 = 68;
    constexpr int noteA4      = 69;
    constexpr int noteBFlat4  = 70;
    constexpr int noteB4      = 71;
    constexpr int lowestNote  = 0;
    constexpr int highestNote = 127;

    // Scale degrees (semitones above the root).
    const std::vector<int> ionianDegrees    { 0, 2, 4, 5, 7, 9, 11 };
    const std::vector<int> wholeToneDegrees { 0, 2, 4, 6, 8, 10 };
    const std::vector<int> wholeHalfDegrees { 0, 2, 3, 5, 6, 8, 9, 11 };
    constexpr int sharpFourth   = 6;
    constexpr int flatSixth     = 8;
    constexpr int naturalSixth  = 9;
    constexpr int flatSeventh   = 10;
    constexpr int majorSeventh  = 11;
    constexpr int ionianCount    = 7;
    constexpr int wholeToneCount = 6;
    constexpr int wholeHalfCount = 8;
}

using namespace ChordTransposerTestValues;

class ChordTransposerTest : public juce::UnitTest
{
public:
    ChordTransposerTest() : juce::UnitTest (ChordTransposerTestValues::transposerTestName, ChordTransposerTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (fixedNeverTransposed);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (fMajor);   // F
            expectEquals (transposer.transpose (noteC2, PartKind::Fixed), noteC2);
        }

        beginTest (invalidActiveIdentity);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   ({});                            // none
            expectEquals (transposer.transpose (noteC4, PartKind::Acc), noteC4);
        }

        beginTest (sameChordIdentity);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (cMajor);
            expectEquals (transposer.transpose (noteE4, PartKind::Acc), noteE4);
            expectEquals (transposer.transpose (noteC4, PartKind::Bass), noteC4);
        }

        beginTest (nonChordTonesPreserved);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (cMajor);   // identity
            expectEquals (transposer.transpose (noteD4, PartKind::Acc), noteD4);  // D stays D
            expectEquals (transposer.transpose (noteF4, PartKind::Acc), noteF4);  // F stays F
            expectEquals (transposer.transpose (noteA4, PartKind::Acc), noteA4);  // A stays A
            expectEquals (transposer.transpose (noteBFlat4, PartKind::Acc), noteBFlat4);  // Bb stays Bb
        }

        beginTest (passingNotesShift);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C
            transposer.setActiveChord   (dMajor);   // D (+2)
            expectEquals (transposer.transpose (noteD4, PartKind::Acc), noteE4);  // D (2nd) -> E (2nd above D)
            expectEquals (transposer.transpose (noteF4, PartKind::Acc), noteG4);  // F (4th) -> G (4th above D)
            expectEquals (transposer.transpose (noteA4, PartKind::Acc), noteB4);  // A (6th) -> B (6th above D)
        }

        beginTest (onlyThirdFlips);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C major
            transposer.setActiveChord   (cMinor);   // C minor (same root)
            expectEquals (transposer.transpose (noteE4, PartKind::Acc), noteEFlat4);  // E (maj 3rd) -> Eb (min 3rd)
            expectEquals (transposer.transpose (noteC4, PartKind::Acc), noteC4);  // C (root) unchanged
            expectEquals (transposer.transpose (noteG4, PartKind::Acc), noteG4);  // G (5th) unchanged
            expectEquals (transposer.transpose (noteD4, PartKind::Acc), noteD4);  // D (2nd passing) unchanged
            expectEquals (transposer.transpose (noteF4, PartKind::Acc), noteF4);  // F (4th passing) unchanged
        }

        beginTest (thirdMappedDifferently);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C E G
            transposer.setActiveChord   (aMinor);   // A C E
            expectEquals (transposer.transpose (noteC4, PartKind::Acc) % semitonesPerOctave, pitchA);   // root C -> A
            expectEquals (transposer.transpose (noteE4, PartKind::Acc) % semitonesPerOctave, pitchC);   // 3rd  E -> C (minor 3rd, not C#)
            expectEquals (transposer.transpose (noteG4, PartKind::Acc) % semitonesPerOctave, pitchE);   // 5th  G -> E
        }

        beginTest (blockShift);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C
            transposer.setActiveChord   (dMajor);   // D (+2)
            expectEquals (transposer.transpose (noteC4, PartKind::Acc) % semitonesPerOctave, pitchD);   // C -> D
            expectEquals (transposer.transpose (noteE4, PartKind::Acc) % semitonesPerOctave, pitchFSharp);   // E -> F#
            expectEquals (transposer.transpose (noteG4, PartKind::Acc) % semitonesPerOctave, pitchA);   // G -> A
        }

        beginTest (resultInMidiRange);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (gDominant7);
            for (int note = lowestNote; note <= highestNote; ++note)
            {
                const int transposed = transposer.transpose (note, PartKind::Acc);
                expect (transposed >= lowestNote && transposed <= highestNote);
            }
        }

        beginTest (bassInversionRebases);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);        // recorded in C
            transposer.setActiveChord   (cMajorOverE);   // C major, bass = E (C/E)

            transposer.setBassInversion (true);
            // the entire bass line shifts onto E: root C -> E, and the 5th G -> B (a 5th above E)
            expectEquals (transposer.transpose (noteC2, PartKind::Bass) % semitonesPerOctave, pitchE);    // C -> E
            expectEquals (transposer.transpose (noteG2, PartKind::Bass) % semitonesPerOctave, pitchB);   // G -> B (re-based, not just the root)

            // with inversion off, the bass follows the chord root normally (C, G)
            transposer.setBassInversion (false);
            expectEquals (transposer.transpose (noteC2, PartKind::Bass) % semitonesPerOctave, pitchC);    // C
            expectEquals (transposer.transpose (noteG2, PartKind::Bass) % semitonesPerOctave, pitchG);    // G

            // bass inversion only affects the Bass part, never Acc
            transposer.setBassInversion (true);
            expectEquals (transposer.transpose (noteG2, PartKind::Acc) % semitonesPerOctave, pitchG);     // Acc 5th stays G
        }

        // ---- Phase 7a: explicit NTT types ----------------------------------------------------

        beginTest (parallelShifts);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C
            transposer.setActiveChord   (fMajor);   // F (+5)
            expectEquals (transposer.transpose (noteC4, PartKind::Acc, NttType::Parallel), noteF4);
            expectEquals (transposer.transpose (noteD4, PartKind::Acc, NttType::Parallel), noteG4);
            expectEquals (transposer.transpose (noteE4, PartKind::Acc, NttType::Parallel), noteA4);
        }

        beginTest (chordRemapsRoles);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C E G
            transposer.setActiveChord   (aMinor);   // A C E
            expectEquals (transposer.transpose (noteC4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchA);  // C -> A
            expectEquals (transposer.transpose (noteE4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchC);  // E -> C (min 3rd)
            expectEquals (transposer.transpose (noteG4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchE);  // G -> E
        }

        beginTest (chordSnapsPassingTone);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C maj; passing tones not in {0,4,7}
            transposer.setActiveChord   (aMinor);   // A min, Dorian default
            transposer.setMinorScale (MinorScaleChoice::Dorian);
            // G# (deg 8, passing) + interval 9 -> pc 5 (F); F not in A-Dorian {9,11,0,2,4,6,7};
            // nearest tones 4 and 6 are a tie -> resolve DOWN -> 4 (E)
            expectEquals (transposer.transpose (noteGSharp4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchE);
        }

        beginTest (noTransposeIsInput);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (fMajor);
            expectEquals (transposer.transpose (noteCSharp4, PartKind::Acc, NttType::NoTranspose), noteCSharp4);
        }

        beginTest (fixedKeepsNearestOctave);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // C
            transposer.setActiveChord   (aMajor);   // A
            expectEquals (transposer.transpose (noteC4, PartKind::Acc, NttType::Chord), noteA4);  // Chord drifts up to A4
            expectEquals (transposer.transpose (noteC4, PartKind::Acc, NttType::Fixed), noteA3);  // Fixed: A nearest C4 is A3
        }

        beginTest (minorScaleChangesSnap);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (aMinor);   // A min
            transposer.setMinorScale (MinorScaleChoice::Aeolian);        // has F natural (pc 5)
            expectEquals (transposer.transpose (noteGSharp4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchF);  // stays on F
            transposer.setMinorScale (MinorScaleChoice::Dorian);         // F# instead of F
            expectEquals (transposer.transpose (noteGSharp4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchE);  // snaps down to E
        }

        beginTest (wrapAroundFolds);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cDiminished);   // C dim {0,3,6}
            transposer.setActiveChord   (bAugmented);    // B aug {0,4,8}, interval +11
            // Note 66 is F# = the dim 5th (deg 6, idx 2); active aug 5th is deg 8 -> +2.
            // result = 66+11+2 = 79, which is 13 semitones above origin 66 -> fold down an octave to
            // 67 (G, the aug 5th of B), same pitch class.
            expectEquals (transposer.transpose (noteFSharp4, PartKind::Acc, NttType::Chord), noteG4);
            expectEquals (transposer.transpose (noteFSharp4, PartKind::Acc, NttType::Chord) % semitonesPerOctave, pitchG);
        }

        beginTest (chordResultInMidiRange);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (gDominant7);
            for (int note = lowestNote; note <= highestNote; ++note)
            {
                const int transposed = transposer.transpose (note, PartKind::Acc, NttType::Chord);
                expect (transposed >= lowestNote && transposed <= highestNote);
            }
        }

        beginTest (bypassReturnsInput);
        {
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);
            transposer.setActiveChord   (fMinor);
            transposer.setNttEnabled (false);
            expectEquals (transposer.transpose (noteC4, PartKind::Acc,  NttType::Chord),    noteC4);
            expectEquals (transposer.transpose (noteE4, PartKind::Acc,  NttType::Parallel), noteE4);
            expectEquals (transposer.transpose (noteG2, PartKind::Bass, NttType::Parallel), noteG2);
            transposer.setNttEnabled (true);
            expect (transposer.transpose (noteC4, PartKind::Acc, NttType::Chord) != noteC4);   // active again
        }

        beginTest (bypassBeatsBassInversion);
        {
            // The bypass used to rewrite `ntt` instead of returning, so the bass-inversion branch —
            // which returns early — never saw it: the bass stayed transposed while every other part
            // played in the recorded key. ChordDetector sets bassNote on EVERY recognized chord (not
            // just slash voicings), so this fired on ordinary root-position chords too.
            ChordTransposer transposer;
            transposer.setOriginalChord (cMajor);   // recorded in C major
            transposer.setBassInversion (true);
            transposer.setNttEnabled (false);

            transposer.setActiveChord (aMinor);     // root-position Am: bassNote == root
            expectEquals (transposer.transpose (noteG2, PartKind::Bass, NttType::Parallel), noteG2);   // was 52 (+9)
            expectEquals (transposer.transpose (noteG2, PartKind::Bass, NttType::Chord),    noteG2);

            transposer.setActiveChord (cMajorOverE);     // genuine slash chord C/E
            expectEquals (transposer.transpose (noteG2, PartKind::Bass, NttType::Parallel), noteG2);   // was 47 (+4)

            // ...and inversion must still work once the bypass is off.
            transposer.setNttEnabled (true);
            expectEquals (transposer.transpose (noteG2, PartKind::Bass, NttType::Parallel), noteB2);
        }
    }
};
static ChordTransposerTest chordTransposerTest;

class NttScalesTest : public juce::UnitTest
{
public:
    NttScalesTest() : juce::UnitTest (ChordTransposerTestValues::scalesTestName, ChordTransposerTestValues::testCategory) {}

    static bool hasDegree (const NttScale& scale, int degree)
    {
        for (int degreeIndex {}; degreeIndex < scale.count; ++degreeIndex) if (scale.degrees[degreeIndex] == degree) return true;
        return false;
    }

    void runTest() override
    {
        beginTest (majorUsesIonian);
        {
            auto scale = scaleForQuality (ChordQuality::Maj, MinorScaleChoice::Dorian);
            expectEquals (scale.count, ionianCount);
            for (int degree : ionianDegrees) expect (hasDegree (scale, degree));
            expect (! hasDegree (scale, sharpFourth));   // no #4 in Ionian
        }

        beginTest (dominantUsesMixolydian);
        {
            for (auto quality : { ChordQuality::Dom7, ChordQuality::Sus2, ChordQuality::Sus4 })
            {
                auto scale = scaleForQuality (quality, MinorScaleChoice::Dorian);
                expect (hasDegree (scale, flatSeventh));        // b7
                expect (! hasDegree (scale, majorSeventh));      // not a major 7
            }
        }

        beginTest (minorRowSwaps);
        {
            auto dor = scaleForQuality (ChordQuality::Min,  MinorScaleChoice::Dorian);
            auto aeo = scaleForQuality (ChordQuality::Min,  MinorScaleChoice::Aeolian);
            auto har = scaleForQuality (ChordQuality::Min7, MinorScaleChoice::HarmonicMinor);
            expect (  hasDegree (dor, naturalSixth));   // Dorian natural-6
            expect (! hasDegree (dor, flatSixth));
            expect (  hasDegree (aeo, flatSixth));   // Aeolian b6
            expect (! hasDegree (aeo, naturalSixth));
            expect (  hasDegree (har, majorSeventh));  // harmonic raised-7
            expect (  hasDegree (har, flatSixth));
        }

        beginTest (augAndDimScales);
        {
            auto aug = scaleForQuality (ChordQuality::Aug, MinorScaleChoice::Dorian);
            expectEquals (aug.count, wholeToneCount);
            for (int degree : wholeToneDegrees) expect (hasDegree (aug, degree));
            auto dim = scaleForQuality (ChordQuality::Dim, MinorScaleChoice::Dorian);
            expectEquals (dim.count, wholeHalfCount);
            for (int degree : wholeHalfDegrees) expect (hasDegree (dim, degree));
        }
    }
};
static NttScalesTest nttScalesTest;
