#include <JuceHeader.h>
#include "Audio/BbdChorus.h"

/*  CC93's bucket-brigade chorus (ported from va-effects). The model's central claim is arithmetic - a longer delay
    needs a slower clock, which leaves less bandwidth - and the rest is behaviour: silent and untouched until CC93
    opens it, no click when it opens, a real chorus (different from the dry signal, different on each side). */

namespace BbdChorusTestValues
{
    const juce::String testName     = "BBD chorus";
    const juce::String testCategory = "Unit";

    const juce::String clockArithmetic = "delay = 256 stages / (2 x clock): 5 ms needs 25.6 kHz, 20 ms only 6.4 kHz";
    const juce::String darkerWhenLonger = "bandwidth is the lower of 12 kHz and 0.45 x the clock: longer is darker";
    const juce::String closedIsDry      = "closed (no CC93 yet) it is inactive and leaves the signal untouched";
    const juce::String opensWithoutClick = "opening it fully on a sounding note does not click";
    const juce::String stepWas          = "largest step: ";
    const juce::String itChoruses       = "open, it changes the sound, keeps its level, and the two sides differ";
    const juce::String closesToInactive = "closed again, it glides to silence and then stops running";
    const juce::String reopensWithoutClick = "opened again after it has run, it starts clean and does not click";

    constexpr double sampleRate   = 48000.0;
    constexpr int    blockSize    = 256;
    constexpr double tone         = 220.0;
    constexpr float  amplitude    = 0.5f;
    constexpr double shortMs      = 5.0;
    constexpr double longMs       = 20.0;
    constexpr double shortClockHz = 25600.0;
    constexpr double longClockHz  = 6400.0;
    constexpr double fixedCornerHz = 12000.0;
    constexpr double guardBand    = 0.45;
    constexpr double hzTolerance  = 0.5;
    constexpr float  fullMix      = 1.0f;
    constexpr float  closedMix    = 0.0f;
    constexpr int    settleBlocks = 200;     // ~1 s: the compander and mix have settled
    constexpr int    measureBlocks = 100;
    constexpr float  clickThreshold = 0.05f;
    constexpr float  changedShare  = 0.1f;   // the wet signal differs from the dry by at least 10 % of its level
    constexpr float  keptShare     = 0.3f;   // and keeps at least 30 % of it
    constexpr float  sidesShare    = 0.02f;  // the sides differ by at least 2 % of the level
    constexpr int    closeBlocks   = 100;
    constexpr size_t previousSample = 1;
}

using namespace BbdChorusTestValues;

class BbdChorusTest : public juce::UnitTest
{
public:
    BbdChorusTest() : juce::UnitTest (BbdChorusTestValues::testName, BbdChorusTestValues::testCategory) {}

    struct Rendered { std::vector<float> dry, left, right; };

    /** Renders blocks of a sine through the chorus, from sample `start`. */
    static Rendered render (BbdChorus& chorus, int blocks, double& sampleIndex)
    {
        Rendered out;
        std::vector<float> left (blockSize), right (blockSize);
        for (int blockIndex {}; blockIndex < blocks; ++blockIndex)
        {
            for (int offset {}; offset < blockSize; ++offset, ++sampleIndex)
            {
                const auto value = amplitude * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * tone * sampleIndex / sampleRate));
                left[static_cast<size_t> (offset)] = right[static_cast<size_t> (offset)] = value;
                out.dry.push_back (value);
            }
            if (chorus.isActive())
                chorus.process (left.data(), right.data(), blockSize);
            out.left.insert (out.left.end(), left.begin(), left.end());
            out.right.insert (out.right.end(), right.begin(), right.end());
        }
        return out;
    }

    static float rms (const std::vector<float>& signal, const std::vector<float>& minus = {})
    {
        double sum {};
        for (size_t index {}; index < signal.size(); ++index)
        {
            const double value = signal[index] - (minus.empty() ? float {} : minus[index]);
            sum += value * value;
        }
        return static_cast<float> (std::sqrt (sum / static_cast<double> (signal.size())));
    }

    static float largestStep (const std::vector<float>& samples)
    {
        float worst {};
        for (size_t index = previousSample; index < samples.size(); ++index)
            worst = juce::jmax (worst, std::abs (samples[index] - samples[index - previousSample]));
        return worst;
    }

    void runTest() override
    {
        beginTest (clockArithmetic);
        expectWithinAbsoluteError (BbdChorus::clockHzForDelayMs (shortMs), shortClockHz, hzTolerance);
        expectWithinAbsoluteError (BbdChorus::clockHzForDelayMs (longMs), longClockHz, hzTolerance);

        beginTest (darkerWhenLonger);
        expectWithinAbsoluteError (BbdChorus::bandwidthHzForDelayMs (shortMs), juce::jmin (fixedCornerHz, shortClockHz * guardBand), hzTolerance);
        expectWithinAbsoluteError (BbdChorus::bandwidthHzForDelayMs (longMs), longClockHz * guardBand, hzTolerance);
        expect (BbdChorus::bandwidthHzForDelayMs (longMs) < BbdChorus::bandwidthHzForDelayMs (shortMs));

        beginTest (closedIsDry);
        {
            BbdChorus chorus;
            chorus.prepare (sampleRate);
            expect (! chorus.isActive());
            double sampleIndex {};
            auto out = render (chorus, measureBlocks, sampleIndex);
            expect (out.left == out.dry && out.right == out.dry);
        }

        beginTest (opensWithoutClick);
        {
            BbdChorus chorus;
            chorus.prepare (sampleRate);
            double sampleIndex {};
            render (chorus, measureBlocks, sampleIndex);
            chorus.setMix (fullMix);
            auto out = render (chorus, settleBlocks, sampleIndex);
            expect (largestStep (out.left) < clickThreshold, stepWas + juce::String (largestStep (out.left)));

            beginTest (itChoruses);
            out = render (chorus, measureBlocks, sampleIndex);
            const auto level = rms (out.dry);
            expect (rms (out.left, out.dry) > changedShare * level);
            expect (rms (out.left) > keptShare * level);
            expect (rms (out.left, out.right) > sidesShare * level);

            beginTest (closesToInactive);
            chorus.setMix (closedMix);
            render (chorus, closeBlocks, sampleIndex);
            expect (! chorus.isActive());

            beginTest (reopensWithoutClick);
            chorus.setMix (fullMix);
            out = render (chorus, settleBlocks, sampleIndex);
            expect (largestStep (out.left) < clickThreshold, stepWas + juce::String (largestStep (out.left)));
            expect (rms (out.left, out.dry) > changedShare * rms (out.dry));
        }
    }
};

static BbdChorusTest bbdChorusTest;
