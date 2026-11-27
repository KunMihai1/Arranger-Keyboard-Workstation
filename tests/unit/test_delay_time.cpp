#include <JuceHeader.h>
#include "Audio/AudioHandler.h"
#include "Audio/FractionalDelayLine.h"

/*  CC94 delay time (wiki: "delay read offset still jumps"). The delay used to read whole samples and move its read head
    instantly when CC94 changed: the delayed signal stepped, a click. Now the time glides and the head is read between
    samples (FractionalDelayLine, Lagrange-3 from va-effects), so a change is a short bend, not a step. */

namespace DelayTimeTestValues
{
    const juce::String testName     = "Delay time";
    const juce::String testCategory = "Unit";

    const juce::String wholeDelayExact   = "a whole-sample delay returns exactly the sample written that long ago";
    const juce::String betweenSamples    = "between samples it follows a smooth signal (a sine read at half a sample)";
    const juce::String clampsShortest    = "a delay shorter than one sample reads one sample back";
    const juce::String timeChangeNoClick = "a CC94 time change mid-note glides: no step in the output";
    const juce::String stepWas           = "largest step after the change: ";
    const juce::String integerWouldClick = "guard: reading at the old and new whole offsets instantly does step";

    constexpr double sampleRate     = 48000.0;
    constexpr int    blockSize      = 64;
    constexpr int    stereo         = 2;
    constexpr int    leftChannel    = 0;
    constexpr double tone           = 220.0;
    constexpr double slowTone       = 100.0;
    constexpr float  amplitude      = 0.5f;
    constexpr int    lineLength     = 1024;
    constexpr int    writtenSamples = 600;
    constexpr int    wholeDelay     = 37;
    constexpr int    newestOffset   = 1;   // the newest write is sample writtenSamples - 1
    constexpr double halfSample     = 0.5;
    constexpr float  readTolerance  = 1.0e-4f;
    constexpr double tooShort       = 0.25;

    constexpr int    delayController = 94;
    constexpr int    shortTime       = 40;    // CC94: ~0.16 s, mix 0.25
    constexpr int    longTime        = 100;   //       ~0.33 s, mix 0.63
    constexpr int    settleBlocks    = 600;   // 0.8 s: the line holds the tone, the mix has settled
    constexpr int    afterBlocks     = 300;
    constexpr float  clickThreshold  = 0.05f; // a 220 Hz sine at 0.5 steps by <= 0.0144 a sample on its own
    constexpr double shortSeconds    = 0.16;
    constexpr double longSeconds     = 0.33;
    constexpr float  jumpMix         = 0.63f;   // CC94 at longTime
    constexpr int    halves          = 2;
    constexpr size_t secondSample    = 1;
}

using namespace DelayTimeTestValues;

class DelayTimeTest : public juce::UnitTest
{
public:
    DelayTimeTest() : juce::UnitTest (DelayTimeTestValues::testName, DelayTimeTestValues::testCategory) {}

    static float sineAt (double frequency, double sampleIndex)
    {
        return amplitude * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * frequency * sampleIndex / sampleRate));
    }

    static float largestStep (const std::vector<float>& samples)
    {
        float worst {};
        for (size_t sampleIndex = secondSample; sampleIndex < samples.size(); ++sampleIndex)
            worst = juce::jmax (worst, std::abs (samples[sampleIndex] - samples[sampleIndex - secondSample]));
        return worst;
    }

    void runTest() override
    {
        beginTest (wholeDelayExact);
        {
            FractionalDelayLine line;
            line.prepare (lineLength);
            for (int sampleIndex {}; sampleIndex < writtenSamples; ++sampleIndex)
                line.write (static_cast<float> (sampleIndex));
            // The newest write is writtenSamples - 1, so `wholeDelay` back is writtenSamples - 1 - wholeDelay.
            expectEquals (line.read (wholeDelay), static_cast<float> (writtenSamples - newestOffset - wholeDelay));
        }

        beginTest (betweenSamples);
        {
            FractionalDelayLine line;
            line.prepare (lineLength);
            for (int sampleIndex {}; sampleIndex < writtenSamples; ++sampleIndex)
                line.write (sineAt (slowTone, sampleIndex));
            const double newest = writtenSamples - newestOffset;
            const double wanted = newest - wholeDelay - halfSample;
            expectWithinAbsoluteError (line.read (wholeDelay + halfSample), sineAt (slowTone, wanted), readTolerance);

            beginTest (clampsShortest);
            expectEquals (line.read (tooShort), line.read (FractionalDelayLine::shortestDelay));
        }

        beginTest (timeChangeNoClick);
        {
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);
            juce::AudioBuffer<float> buffer (stereo, blockSize);
            std::vector<float> after;
            double sampleIndex {};

            auto renderBlocks = [&] (int blocks, bool keep)
            {
                for (int blockIndex {}; blockIndex < blocks; ++blockIndex)
                {
                    for (int offset {}; offset < blockSize; ++offset, ++sampleIndex)
                        for (int channel {}; channel < stereo; ++channel)
                            buffer.setSample (channel, offset, sineAt (tone, sampleIndex));
                    dsp.process (buffer, blockSize);
                    if (keep)
                        after.insert (after.end(), buffer.getReadPointer (leftChannel), buffer.getReadPointer (leftChannel) + blockSize);
                }
            };

            dsp.updateCC (delayController, shortTime);
            renderBlocks (settleBlocks, false);
            const auto lastBefore = buffer.getSample (leftChannel, blockSize - newestOffset);
            dsp.updateCC (delayController, longTime);
            renderBlocks (afterBlocks, true);
            after.insert (after.begin(), lastBefore);   // the step across the change counts too

            expect (largestStep (after) < clickThreshold, stepWas + juce::String (largestStep (after)));
        }

        beginTest (integerWouldClick);
        {
            // Guards the test above: the same tone through the same line, with the read position switched at once from
            // the short time to the long one (what the old code did), steps far above the threshold.
            FractionalDelayLine line;
            line.prepare (static_cast<int> (longSeconds * sampleRate) + blockSize);
            std::vector<float> out;
            const int switchAt = static_cast<int> (longSeconds * sampleRate) + blockSize / halves;
            for (int sampleIndex {}; sampleIndex < switchAt + blockSize; ++sampleIndex)
            {
                const float dry = sineAt (tone, sampleIndex);
                line.write (dry);
                const double time = (sampleIndex < switchAt ? shortSeconds : longSeconds) * sampleRate;
                out.push_back (dry + jumpMix * line.read (std::round (time)));
            }
            expect (largestStep (out) > clickThreshold, stepWas + juce::String (largestStep (out)));
        }
    }
};

static DelayTimeTest delayTimeTest;
