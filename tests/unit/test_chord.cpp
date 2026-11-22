#include <juce_core/juce_core.h>
#include "Arranger/Chord.h"

namespace ChordTestValues
{
    const juce::String testName     = "ArrangerChord";
    const juce::String testCategory = "Arranger";

    const juce::String qualityRoundTrip     = "quality enum round-trips through string";
    const juce::String unknownIsNone        = "unknown string maps to None";
    const juce::String validNeedsRootAndQuality = "isValid requires a root and a real quality";
    const juce::String intervalsStartAtRoot = "chordIntervals start at the root and have the expected size";

    const juce::String unknownQuality = "not-a-quality";
    constexpr int cRoot   = 0;
    constexpr int noBass  = -1;
    const std::vector<int> majorIntervals          { 0, 4, 7 };
    const std::vector<int> minorSeventhIntervals   { 0, 3, 7, 10 };
    const std::vector<int> suspendedFourthIntervals { 0, 5, 7 };
    constexpr size_t fourNotes = 4;
}

class ChordTest : public juce::UnitTest
{
public:
    ChordTest() : juce::UnitTest (ChordTestValues::testName, ChordTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ChordTestValues;

        beginTest (qualityRoundTrip);
        for (auto quality : { ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7,
                              ChordQuality::Maj7, ChordQuality::Min7, ChordQuality::Dim,
                              ChordQuality::HalfDim, ChordQuality::Aug, ChordQuality::Sus2,
                              ChordQuality::Sus4 })
            expect (chordQualityFromString (toString (quality)) == quality);

        beginTest (unknownIsNone);
        expect (chordQualityFromString (unknownQuality) == ChordQuality::None);

        beginTest (validNeedsRootAndQuality);
        expect (! ArrangerChord{}.isValid());
        expect (! (ArrangerChord{ cRoot, ChordQuality::None, noBass }).isValid());
        expect ((ArrangerChord{ cRoot, ChordQuality::Maj, noBass }).isValid());

        beginTest (intervalsStartAtRoot);
        expect (chordIntervals (ChordQuality::Maj)  == majorIntervals);
        expect (chordIntervals (ChordQuality::Min7) == minorSeventhIntervals);
        expect (chordIntervals (ChordQuality::Dom7).size() == fourNotes);
        expect (chordIntervals (ChordQuality::Sus4) == suspendedFourthIntervals);
    }
};
static ChordTest chordTest;
