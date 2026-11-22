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

    constexpr int three   = 3;
    constexpr int four    = 4;
    constexpr int six     = 6;
    constexpr int seven   = 7;
    constexpr int eight   = 8;
    constexpr int sixteen = 16;

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
            expect(bar.getNumerator() == four);
            expect(bar.getDenominator() == four);
        }

        beginTest(setGetNumerator);
        {
            BeatBar bar;

            bar.setNumerator(three);
            expect(bar.getNumerator() == three);

            bar.setNumerator(seven);
            expect(bar.getNumerator() == seven);
        }

        beginTest(setGetDenominator);
        {
            BeatBar bar;

            bar.setDenominator(eight);
            expect(bar.getDenominator() == eight);

            bar.setDenominator(four);
            expect(bar.getDenominator() == four);
        }

        beginTest(subBeatsPerBeat);
        {
            BeatBar bar;

            bar.setDenominator(four);
            expect(bar.getSubBeatsPerBeat() == oneSubBeat);

            bar.setDenominator(eight);
            expect(bar.getSubBeatsPerBeat() == twoSubBeats);

            bar.setDenominator(sixteen);
            expect(bar.getSubBeatsPerBeat() == fourSubBeats);
        }

        beginTest(totalSubdivisions);
        {
            BeatBar bar;

            bar.setNumerator(four);
            bar.setDenominator(four);
            expect(bar.getTotalSubdivisions() == four);

            bar.setNumerator(three);
            bar.setDenominator(four);
            expect(bar.getTotalSubdivisions() == three);

            bar.setNumerator(seven);
            bar.setDenominator(eight);
            expect(bar.getTotalSubdivisions() == sevenEightSubdivisions);

            bar.setNumerator(six);
            bar.setDenominator(eight);
            expect(bar.getTotalSubdivisions() == sixEightSubdivisions);
        }

        beginTest(subdivisionIn44);
        {
            BeatBar bar;

            bar.setNumerator(four);
            bar.setDenominator(four);

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

            bar.setNumerator(four);
            bar.setDenominator(four);

            bar.setCurrentBeatsElapsed(nextBarStart);
            expect(bar.getCurrentSubdivision() == firstSubdivision);

            bar.setCurrentBeatsElapsed(nextBarBeatTwo);
            expect(bar.getCurrentSubdivision() == secondSubdivision);
        }

        beginTest(negativeBeatsWrap);
        {
            BeatBar bar;

            bar.setNumerator(four);
            bar.setDenominator(four);

            bar.setCurrentBeatsElapsed(oneBeatBack);
            expect(bar.getCurrentSubdivision() == fourthSubdivision);
        }

        beginTest(subdivisionIn78);
        {
            BeatBar bar;

            bar.setNumerator(seven);
            bar.setDenominator(eight);

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
            bar.setNumerator(four);
            bar.setDenominator(four);


            bar.isPlayingCheck = []() { return true; };

            juce::Image img(juce::Image::ARGB, imageWidth, imageHeight, true);
            juce::Graphics g(img);

            bar.paint(g);

            expect(true);
        }
    }
};

static BeatBarTest beatBarTest;