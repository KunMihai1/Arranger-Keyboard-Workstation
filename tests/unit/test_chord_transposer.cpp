#include <juce_core/juce_core.h>
#include "Arranger/ChordTransposer.h"

class ChordTransposerTest : public juce::UnitTest
{
public:
    ChordTransposerTest() : juce::UnitTest ("ChordTransposer", "Arranger") {}

    void runTest() override
    {
        beginTest ("fixed parts (drums) are never transposed");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });   // F
            expectEquals (t.transpose (36, PartKind::Fixed), 36);
        }

        beginTest ("invalid active chord -> identity");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({});                            // none
            expectEquals (t.transpose (60, PartKind::Acc), 60);
        }

        beginTest ("identity when active == original");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 0, ChordQuality::Maj, 0 });
            expectEquals (t.transpose (64, PartKind::Acc), 64);
            expectEquals (t.transpose (60, PartKind::Bass), 60);
        }

        beginTest ("non-chord tones are PRESERVED at identity (not snapped to chord tones)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 0, ChordQuality::Maj, 0 });   // identity
            expectEquals (t.transpose (62, PartKind::Acc), 62);  // D stays D
            expectEquals (t.transpose (65, PartKind::Acc), 65);  // F stays F
            expectEquals (t.transpose (69, PartKind::Acc), 69);  // A stays A
            expectEquals (t.transpose (70, PartKind::Acc), 70);  // Bb stays Bb
        }

        beginTest ("melodic/passing notes shift by the root interval (kept relative to the root)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 2, ChordQuality::Maj, 2 });   // D (+2)
            expectEquals (t.transpose (62, PartKind::Acc), 64);  // D (2nd) -> E (2nd above D)
            expectEquals (t.transpose (65, PartKind::Acc), 67);  // F (4th) -> G (4th above D)
            expectEquals (t.transpose (69, PartKind::Acc), 71);  // A (6th) -> B (6th above D)
        }

        beginTest ("only the 3rd flips on major->minor; other notes just shift");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C major
            t.setActiveChord   ({ 0, ChordQuality::Min, 0 });   // C minor (same root)
            expectEquals (t.transpose (64, PartKind::Acc), 63);  // E (maj 3rd) -> Eb (min 3rd)
            expectEquals (t.transpose (60, PartKind::Acc), 60);  // C (root) unchanged
            expectEquals (t.transpose (67, PartKind::Acc), 67);  // G (5th) unchanged
            expectEquals (t.transpose (62, PartKind::Acc), 62);  // D (2nd passing) unchanged
            expectEquals (t.transpose (65, PartKind::Acc), 65);  // F (4th passing) unchanged
        }

        beginTest ("NTT maps the 3rd differently from root/5th (C maj -> A min)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C E G
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A C E
            expectEquals (t.transpose (60, PartKind::Acc) % 12, 9);   // root C -> A
            expectEquals (t.transpose (64, PartKind::Acc) % 12, 0);   // 3rd  E -> C (minor 3rd, not C#)
            expectEquals (t.transpose (67, PartKind::Acc) % 12, 4);   // 5th  G -> E
        }

        beginTest ("block-shift case (same quality) shifts by root interval");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 2, ChordQuality::Maj, 2 });   // D (+2)
            expectEquals (t.transpose (60, PartKind::Acc) % 12, 2);   // C -> D
            expectEquals (t.transpose (64, PartKind::Acc) % 12, 6);   // E -> F#
            expectEquals (t.transpose (67, PartKind::Acc) % 12, 9);   // G -> A
        }

        beginTest ("result stays in 0..127");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            for (int n = 0; n <= 127; ++n)
            {
                const int x = t.transpose (n, PartKind::Acc);
                expect (x >= 0 && x <= 127);
            }
        }

        beginTest ("bass inversion re-bases the WHOLE bass line on the played bass note");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // recorded in C
            t.setActiveChord   ({ 0, ChordQuality::Maj, 4 });   // C major, bass = E (C/E)

            t.setBassInversion (true);
            // the entire bass line shifts onto E: root C -> E, and the 5th G -> B (a 5th above E)
            expectEquals (t.transpose (36, PartKind::Bass) % 12, 4);    // C -> E
            expectEquals (t.transpose (43, PartKind::Bass) % 12, 11);   // G -> B (re-based, not just the root)

            // with inversion off, the bass follows the chord root normally (C, G)
            t.setBassInversion (false);
            expectEquals (t.transpose (36, PartKind::Bass) % 12, 0);    // C
            expectEquals (t.transpose (43, PartKind::Bass) % 12, 7);    // G

            // bass inversion only affects the Bass part, never Acc
            t.setBassInversion (true);
            expectEquals (t.transpose (43, PartKind::Acc) % 12, 7);     // Acc 5th stays G
        }

        // ---- Phase 7a: explicit NTT types ----------------------------------------------------

        beginTest ("Parallel shifts every note by the root interval");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });   // F (+5)
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Parallel), 65);
            expectEquals (t.transpose (62, PartKind::Acc, NttType::Parallel), 67);
            expectEquals (t.transpose (64, PartKind::Acc, NttType::Parallel), 69);
        }

        beginTest ("Chord remaps 3rd/5th to the active chord's roles");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C E G
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A C E
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord) % 12, 9);  // C -> A
            expectEquals (t.transpose (64, PartKind::Acc, NttType::Chord) % 12, 0);  // E -> C (min 3rd)
            expectEquals (t.transpose (67, PartKind::Acc, NttType::Chord) % 12, 4);  // G -> E
        }

        beginTest ("Chord snaps a passing tone to the active scale (tie resolves down)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C maj; passing tones not in {0,4,7}
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A min, Dorian default
            t.setMinorScale (MinorScaleChoice::Dorian);
            // G# (deg 8, passing) + interval 9 -> pc 5 (F); F not in A-Dorian {9,11,0,2,4,6,7};
            // nearest tones 4 and 6 are a tie -> resolve DOWN -> 4 (E)
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 4);
        }

        beginTest ("NoTranspose returns the input note");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });
            expectEquals (t.transpose (61, PartKind::Acc, NttType::NoTranspose), 61);
        }

        beginTest ("Fixed keeps the octave nearest the original note (no drift)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 9, ChordQuality::Maj, 9 });   // A
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord), 69);  // Chord drifts up to A4
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Fixed), 57);  // Fixed: A nearest C4 is A3
        }

        beginTest ("minor-scale selector changes the snap target");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A min
            t.setMinorScale (MinorScaleChoice::Aeolian);        // has F natural (pc 5)
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 5);  // stays on F
            t.setMinorScale (MinorScaleChoice::Dorian);         // F# instead of F
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 4);  // snaps down to E
        }

        beginTest ("wrap-around folds a >1-octave result back toward the origin");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Dim, 0 });   // C dim {0,3,6}
            t.setActiveChord   ({ 11, ChordQuality::Aug, 11 }); // B aug {0,4,8}, interval +11
            // Note 66 is F# = the dim 5th (deg 6, idx 2); active aug 5th is deg 8 -> +2.
            // result = 66+11+2 = 79, which is 13 semitones above origin 66 -> fold down an octave to
            // 67 (G, the aug 5th of B), same pitch class.
            expectEquals (t.transpose (66, PartKind::Acc, NttType::Chord), 67);
            expectEquals (t.transpose (66, PartKind::Acc, NttType::Chord) % 12, 7);
        }

        beginTest ("Chord result stays within 0..127 across the whole range");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            for (int n = 0; n <= 127; ++n)
            {
                const int x = t.transpose (n, PartKind::Acc, NttType::Chord);
                expect (x >= 0 && x <= 127);
            }
        }

        beginTest ("master bypass: disabled NTT returns input notes for all parts/types");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Min, 5 });
            t.setNttEnabled (false);
            expectEquals (t.transpose (60, PartKind::Acc,  NttType::Chord),    60);
            expectEquals (t.transpose (64, PartKind::Acc,  NttType::Parallel), 64);
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 43);
            t.setNttEnabled (true);
            expect (t.transpose (60, PartKind::Acc, NttType::Chord) != 60);   // active again
        }

        beginTest ("master bypass beats bass inversion (regression: bypass used to miss the bass)");
        {
            // The bypass used to rewrite `ntt` instead of returning, so the bass-inversion branch —
            // which returns early — never saw it: the bass stayed transposed while every other part
            // played in the recorded key. ChordDetector sets bassNote on EVERY recognized chord (not
            // just slash voicings), so this fired on ordinary root-position chords too.
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // recorded in C major
            t.setBassInversion (true);
            t.setNttEnabled (false);

            t.setActiveChord ({ 9, ChordQuality::Min, 9 });     // root-position Am: bassNote == root
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 43);   // was 52 (+9)
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Chord),    43);

            t.setActiveChord ({ 0, ChordQuality::Maj, 4 });     // genuine slash chord C/E
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 43);   // was 47 (+4)

            // ...and inversion must still work once the bypass is off.
            t.setNttEnabled (true);
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 47);
        }
    }
};
static ChordTransposerTest chordTransposerTest;

class NttScalesTest : public juce::UnitTest
{
public:
    NttScalesTest() : juce::UnitTest ("NttScales", "Arranger") {}

    static bool hasDegree (const NttScale& s, int d)
    {
        for (int i = 0; i < s.count; ++i) if (s.degrees[i] == d) return true;
        return false;
    }

    void runTest() override
    {
        beginTest ("major qualities use Ionian");
        {
            auto s = scaleForQuality (ChordQuality::Maj, MinorScaleChoice::Dorian);
            expectEquals (s.count, 7);
            for (int d : { 0,2,4,5,7,9,11 }) expect (hasDegree (s, d));
            expect (! hasDegree (s, 6));   // no #4 in Ionian
        }

        beginTest ("dominant + sus use Mixolydian (flat 7)");
        {
            for (auto q : { ChordQuality::Dom7, ChordQuality::Sus2, ChordQuality::Sus4 })
            {
                auto s = scaleForQuality (q, MinorScaleChoice::Dorian);
                expect (hasDegree (s, 10));        // b7
                expect (! hasDegree (s, 11));      // not a major 7
            }
        }

        beginTest ("minor row swaps with the MinorScaleChoice");
        {
            auto dor = scaleForQuality (ChordQuality::Min,  MinorScaleChoice::Dorian);
            auto aeo = scaleForQuality (ChordQuality::Min,  MinorScaleChoice::Aeolian);
            auto har = scaleForQuality (ChordQuality::Min7, MinorScaleChoice::HarmonicMinor);
            expect (  hasDegree (dor, 9));   // Dorian natural-6
            expect (! hasDegree (dor, 8));
            expect (  hasDegree (aeo, 8));   // Aeolian b6
            expect (! hasDegree (aeo, 9));
            expect (  hasDegree (har, 11));  // harmonic raised-7
            expect (  hasDegree (har, 8));
        }

        beginTest ("aug is whole-tone, dim is whole-half");
        {
            auto aug = scaleForQuality (ChordQuality::Aug, MinorScaleChoice::Dorian);
            expectEquals (aug.count, 6);
            for (int d : { 0,2,4,6,8,10 }) expect (hasDegree (aug, d));
            auto dim = scaleForQuality (ChordQuality::Dim, MinorScaleChoice::Dorian);
            expectEquals (dim.count, 8);
            for (int d : { 0,2,3,5,6,8,9,11 }) expect (hasDegree (dim, d));
        }
    }
};
static NttScalesTest nttScalesTest;
