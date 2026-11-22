#include <juce_core/juce_core.h>
#include "CustomBeatBar.h"

namespace BeatBarTestValues
{
    const juce::String testName     = "BeatBar";
    const juce::String testCategory = "Unit";

    const juce::String defaultValues          = "Default constructor values";
    const juce::String setGetNumerator        = "Set and get numerator";
    const juce::String setGetDenominator      = "Set and get denominator";
    const juce::String subBeatsPerBeat        = "Sub-beats per beat calculation";
    const juce::String totalSubdivisions      = "Total subdivisions calculation";
    const juce::String subdivisionIn44        = "Current subdivision in 4/4";
    const juce::String wrapsAcrossBars        = "Subdivision wraps correctly across bars";
    const juce::String negativeBeatsWrap      = "Negative beat values wrap correctly";
    const juce::String subdivisionIn78        = "Subdivision calculation in 7/8";
    const juce::String paintWhileStopped      = "Paint executes without crash - playing off";
    const juce::String paintFirstBeatPlaying  = "Paint executes red branch - first beat playing";

    constexpr int threeBeatsPerBar    = 3;   // numerators: beats per bar
    constexpr int fourBeatsPerBar     = 4;
    constexpr int sixBeatsPerBar      = 6;
    constexpr int sevenBeatsPerBar    = 7;
    constexpr int quarterNote   = 4;   // denominators: the note value of one beat
    constexpr int eighthNote    = 8;
    constexpr int sixteenthNote = 16;
    constexpr int fourSubdivisions  = 4;
    constexpr int threeSubdivisions = 3;

    constexpr float oneSubBeat   = 1.0f;
    constexpr float twoSubBeats  = 2.0f;
    constexpr float fourSubBeats = 4.0f;

    constexpr int sevenEightSubdivisions = 14;
    constexpr int sixEightSubdivisions   = 12;

    constexpr int firstSubdivision   = 0;
    constexpr int secondSubdivision  = 1;
    constexpr int thirdSubdivision   = 2;
    constexpr int fourthSubdivision  = 3;
    constexpr int seventhSubdivision = 6;

    constexpr double barStart        = 0.0;
    constexpr double insideBeatTwo   = 1.2;
    constexpr double insideBeatThree = 2.9;
    constexpr double nextBarStart    = 4.0;
    constexpr double nextBarBeatTwo  = 5.0;
    constexpr double oneBeatBack     = -1.0;
    constexpr double oneBeat         = 1.0;
    constexpr double threeBeats      = 3.0;

    constexpr int imageWidth  = 200;
    constexpr int imageHeight = 50;
}

using namespace BeatBarTestValues;

class BeatBarTest : public juce::UnitTest
{
public:
    BeatBarTest() : juce::UnitTest(BeatBarTestValues::testName, BeatBarTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(defaultValues);
        {
            BeatBar bar;
            expect(bar.getNumerator() == fourBeatsPerBar);
            expect(bar.getDenominator() == quarterNote);
        }

        beginTest(setGetNumerator);
        {
            BeatBar bar;

            bar.setNumerator(threeBeatsPerBar);
            expect(bar.getNumerator() == threeBeatsPerBar);

            bar.setNumerator(sevenBeatsPerBar);
            expect(bar.getNumerator() == sevenBeatsPerBar);
        }

        beginTest(setGetDenominator);
        {
            BeatBar bar;

            bar.setDenominator(eighthNote);
            expect(bar.getDenominator() == eighthNote);

            bar.setDenominator(quarterNote);
            expect(bar.getDenominator() == quarterNote);
        }

        beginTest(subBeatsPerBeat);
        {
            BeatBar bar;

            bar.setDenominator(quarterNote);
            expect(bar.getSubBeatsPerBeat() == oneSubBeat);

            bar.setDenominator(eighthNote);
            expect(bar.getSubBeatsPerBeat() == twoSubBeats);

            bar.setDenominator(sixteenthNote);
            expect(bar.getSubBeatsPerBeat() == fourSubBeats);
        }

        beginTest(totalSubdivisions);
        {
            BeatBar bar;

            bar.setNumerator(fourBeatsPerBar);
            bar.setDenominator(quarterNote);
            expect(bar.getTotalSubdivisions() == fourSubdivisions);

            bar.setNumerator(threeBeatsPerBar);
            bar.setDenominator(quarterNote);
            expect(bar.getTotalSubdivisions() == threeSubdivisions);

            bar.setNumerator(sevenBeatsPerBar);
            bar.setDenominator(eighthNote);
            expect(bar.getTotalSubdivisions() == sevenEightSubdivisions);

            bar.setNumerator(sixBeatsPerBar);
            bar.setDenominator(eighthNote);
            expect(bar.getTotalSubdivisions() == sixEightSubdivisions);
        }

        beginTest(subdivisionIn44);
        {
            BeatBar bar;

            bar.setNumerator(fourBeatsPerBar);
            bar.setDenominator(quarterNote);

            bar.setCurrentBeatsElapsed(barStart);
            expect(bar.getCurrentSubdivision() == firstSubdivision);

            bar.setCurrentBeatsElapsed(insideBeatTwo);
            expect(bar.getCurrentSubdivision() == secondSubdivision);

            bar.setCurrentBeatsElapsed(insideBeatThree);
            expect(bar.getCurrentSubdivision() == thirdSubdivision);
        }

        beginTest(wrapsAcrossBars);
        {
            BeatBar bar;

            bar.setNumerator(fourBeatsPerBar);
            bar.setDenominator(quarterNote);

            bar.setCurrentBeatsElapsed(nextBarStart);
            expect(bar.getCurrentSubdivision() == firstSubdivision);

            bar.setCurrentBeatsElapsed(nextBarBeatTwo);
            expect(bar.getCurrentSubdivision() == secondSubdivision);
        }

        beginTest(negativeBeatsWrap);
        {
            BeatBar bar;

            bar.setNumerator(fourBeatsPerBar);
            bar.setDenominator(quarterNote);

            bar.setCurrentBeatsElapsed(oneBeatBack);
            expect(bar.getCurrentSubdivision() == fourthSubdivision);
        }

        beginTest(subdivisionIn78);
        {
            BeatBar bar;

            bar.setNumerator(sevenBeatsPerBar);
            bar.setDenominator(eighthNote);

            bar.setCurrentBeatsElapsed(barStart);
            expect(bar.getCurrentSubdivision() == firstSubdivision);

            bar.setCurrentBeatsElapsed(oneBeat);
            expect(bar.getCurrentSubdivision() == thirdSubdivision);

            bar.setCurrentBeatsElapsed(threeBeats);
            expect(bar.getCurrentSubdivision() == seventhSubdivision);
        }

        beginTest(paintWhileStopped);
        {
            BeatBar bar;

            juce::Image img(juce::Image::ARGB, imageWidth, imageHeight, true);
            juce::Graphics g(img);

            bar.paint(g);

            expect(true);
        }

        beginTest(paintFirstBeatPlaying);
        {
            BeatBar bar;

            bar.setCurrentBeatsElapsed(barStart);
            bar.setNumerator(fourBeatsPerBar);
            bar.setDenominator(quarterNote);


            bar.isPlayingCheck = []() { return true; };

            juce::Image img(juce::Image::ARGB, imageWidth, imageHeight, true);
            juce::Graphics g(img);

            bar.paint(g);

            expect(true);
        }
    }
};

static BeatBarTest beatBarTest;