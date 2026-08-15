#include <juce_core/juce_core.h>
#include "Arranger/Chord.h"

class ChordTest : public juce::UnitTest
{
public:
    ChordTest() : juce::UnitTest ("ArrangerChord", "Arranger") {}

    void runTest() override
    {
        beginTest ("quality enum round-trips through string");
        for (auto q : { ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7,
                        ChordQuality::Maj7, ChordQuality::Min7, ChordQuality::Dim,
                        ChordQuality::HalfDim, ChordQuality::Aug, ChordQuality::Sus2,
                        ChordQuality::Sus4 })
            expect (chordQualityFromString (toString (q)) == q);

        beginTest ("unknown string maps to None");
        expect (chordQualityFromString ("not-a-quality") == ChordQuality::None);

        // ---------------------------------------------------------------------------------------
        // TEMPORARY - DO NOT MERGE. Exists only to prove the CI gate goes red and that branch
        // protection blocks the merge (plan Task 6 Steps 1/3). Delete this block, and the branch
        // ci-gate-redcheck, once the red check has been observed.
        // ---------------------------------------------------------------------------------------
        beginTest ("TEMP intentional failure - proves the CI gate goes red");
        expect (false, "intentional CI red check");

        beginTest ("isValid requires a root and a real quality");
        expect (! ArrangerChord{}.isValid());
        expect (! (ArrangerChord{ 0, ChordQuality::None, -1 }).isValid());
        expect ((ArrangerChord{ 0, ChordQuality::Maj, -1 }).isValid());

        beginTest ("chordIntervals start at the root and have the expected size");
        expect (chordIntervals (ChordQuality::Maj)  == (std::vector<int>{0,4,7}));
        expect (chordIntervals (ChordQuality::Min7) == (std::vector<int>{0,3,7,10}));
        expect (chordIntervals (ChordQuality::Dom7).size() == 4);
        expect (chordIntervals (ChordQuality::Sus4) == (std::vector<int>{0,5,7}));
    }
};
static ChordTest chordTest;
