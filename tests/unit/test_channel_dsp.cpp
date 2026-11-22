#include <JuceHeader.h>
#include <iostream>
#include "Audio/AudioHandler.h"

namespace ChannelDspTestValues
{
    const juce::String testName     = "ChannelDSP";
    const juce::String testCategory = "Unit";

    const juce::String smoothingReachesTarget   = "smoothing: a parameter reaches its target but does not jump there";
    const juce::String snapIsImmediate          = "smoothing: snap is immediate, for prepare() only";
    const juce::String expressionSweepNoClick   = "expression: a full CC sweep produces no discontinuity";
    const juce::String unsmoothedWouldFail      = "expression: an unsmoothed implementation would fail the same check";
    const juce::String bypassDoesNotFreeze      = "delay: bypassing does not freeze the line and replay stale audio";
    const juce::String delayStillDelays         = "delay: still audibly delays when it is meant to";
    const juce::String logCoshStable            = "ADAA: log(cosh) is stable where the direct form overflows";
    const juce::String logCoshIsAntiderivative  = "ADAA: it really is the antiderivative of tanh";
    const juce::String adaaTracksTanh           = "ADAA: tracks plain tanh on a slow signal, allowing its half-sample delay";
    const juce::String distortionBypassedAtNeutral = "distortion: bypassed at neutral, active when driven";
    const juce::String engagingDriveNoClick     = "distortion: engaging it produces no discontinuity";
    const juce::String cutoffSweepNoClick       = "filter: a cutoff sweep produces no discontinuity";
    const juce::String adaaReducesAliasing      = "distortion: ADAA measurably reduces aliasing versus the naive shaper";
    const juce::String chainStaysFinite         = "the chain stays finite across every parameter at maximum";
    const juce::String blockSizeIndependent     = "block size does not change the result";

    const juce::String afterOneTimeConstant   = "after one time constant, got ";
    const juce::String firstStepIsSmall       = "the first step is small";
    const juce::String largestStepWas         = "largest sample-to-sample step was ";
    const juce::String reachedSilence         = "expression reached silence, tail peak ";
    const juce::String perBlockDoesStep       = "the per-block version does step, by ";
    const juce::String reenablingProduced     = "re-enabling the delay after silence produced ";
    const juce::String burstShouldEcho        = "a burst should come back as an echo, peak was ";
    const juce::String directFormOverflows    = "the direct form overflows at 400";
    const juce::String stableFormDoesNot      = "the stable form does not";
    const juce::String mostlyHalfSampleDelay  = "most of the difference is the half-sample delay: direct ";
    const juce::String alignedLabel           = ", aligned ";
    const juce::String alignedResidual        = "aligned residual ";
    const juce::String drivenPeakWas          = "driven peak was ";
    const juce::String steadySlewsBy          = "steady state slews by ";
    const juce::String engagingSlewsBy        = ", engaging the drive slews by ";
    const juce::String sweepSlewsBy           = ", the sweep slews by ";
    const juce::String closedLowpassTail      = "a closed lowpass should attenuate 220 Hz, tail peak ";
    const juce::String mustNotDivideRate      = "the test frequency must not divide the sample rate";
    const juce::String aliasPrefix            = "    alias/signal: naive ";
    const juce::String adaaLabel              = " dB, ADAA ";
    const juce::String improvementLabel       = " dB, improvement ";
    const juce::String decibels               = " dB";
    const juce::String naiveShouldAlias       = "the naive shaper should alias badly here, measured ";
    const juce::String adaaShouldImprove      = "ADAA should improve on it, measured ";
    const juce::String againstLabel           = " dB against ";
    const juce::String everySampleFinite      = "every sample is finite with all effects at maximum";
    const juce::String outputBounded          = "output stays bounded, peak was ";
    const juce::String blockSizesAgree        = "64 and 256 sample blocks agree to ";

    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 64;
    constexpr int    stereo     = 2;
    constexpr int    leftChannel  = 0;
    constexpr int    rightChannel = 1;
    constexpr size_t firstStep  = 1;      // a step needs a previous sample
    constexpr size_t halves     = 2;

    // Controllers.
    constexpr int noController         = -1;
    constexpr int neverApply           = -1;
    constexpr int expressionController = 11;
    constexpr int cutoffController     = 74;
    constexpr int driveController      = 80;
    constexpr int delayController      = 94;
    constexpr int controllerOff        = 0;
    constexpr int fullController       = 127;
    constexpr int delayOnValue         = 100;
    constexpr int closedCutoff         = 10;
    const int allEffectControllers[] { 11, 71, 74, 80, 91, 92, 93, 94, 95 };

    // Test tones.
    constexpr float  defaultAmplitude = 0.5f;
    constexpr double sineAmplitude    = 0.5;
    constexpr float  loudAmplitude    = 0.8f;
    constexpr double lowTone          = 220.0;
    constexpr double highTone         = 440.0;
    constexpr double slowTone         = 100.0;
    constexpr double oneKilohertz     = 1000.0;
    constexpr double brightTone       = 3000.0;

    // Smoothing.
    constexpr double smoothingMs            = 20.0;
    constexpr double oneTimeConstantSeconds = 0.020;
    constexpr double settleSeconds          = 0.5;
    constexpr float  zeroLevel              = 0.0f;
    constexpr float  fullLevel              = 1.0f;
    constexpr float  snapLevel              = 0.75f;
    constexpr float  oneTimeConstantLow     = 0.55f;   // roughly 63% of the way
    constexpr float  oneTimeConstantHigh    = 0.70f;
    constexpr float  convergenceTolerance   = 0.001f;
    constexpr float  firstStepLimit         = 0.005f;
    constexpr float  snapTolerance          = 1.0e-6f;

    // Expression and delay.
    constexpr int    expressionBlocks = 40;
    constexpr int    switchBlock      = 10;
    constexpr float  unityGain        = 1.0f;
    constexpr float  mutedGain        = 0.0f;
    constexpr float  clickThreshold   = 0.05f;
    constexpr float  silenceThreshold = 0.01f;
    constexpr size_t tailSamples      = 200;
    constexpr int    loudBlocks       = 60;
    constexpr int    bypassBlocks     = 400;
    constexpr int    reenableBlocks   = 200;
    constexpr int    burstBlocks      = 20;
    constexpr int    echoBlocks       = 500;
    constexpr float  echoThreshold    = 0.05f;

    // ADAA.
    const float logCoshInputs[] { 0.0f, 0.5f, 2.0f, 8.0f };
    constexpr float  logCoshTolerance    = 1.0e-4f;
    constexpr float  overflowInput       = 400.0f;
    constexpr float  logOfTwo            = 0.6931472f;
    constexpr float  overflowTolerance   = 1.0e-3f;
    constexpr float  derivativeStep      = 1.0e-3f;
    constexpr float  centralDifferenceSpan = 2.0f;
    constexpr float  slopeRangeStart     = -6.0f;
    constexpr float  slopeRangeEnd       = 6.0f;
    constexpr float  slopeRangeStep      = 1.1f;
    constexpr float  slopeTolerance      = 2.0e-3f;
    constexpr int    slowSamples         = 4800;
    constexpr double slowDrive           = 8.0;
    constexpr size_t warmUpSamples       = 100;
    constexpr float  half                = 0.5f;
    constexpr float  alignedLimit        = 0.01f;

    // Distortion and filter.
    constexpr int    neutralBlocks       = 30;
    constexpr int    drivenBlocks        = 120;
    constexpr int    driveOnBlock        = 5;
    constexpr int    engageBlock         = 20;
    constexpr float  peakTolerance       = 0.02f;
    constexpr float  drivenPeakFloor     = 0.85f;
    constexpr float  slewMargin          = 1.15f;
    constexpr float  slewAllowance       = 0.005f;
    constexpr int    sweepBlocks         = 200;
    constexpr float  sweepSlewMargin     = 3.0f;
    constexpr float  sweepSlewAllowance  = 0.01f;
    constexpr size_t filterTailSamples   = 500;
    constexpr float  closedFilterCeiling = 0.45f;

    // Aliasing measurement.
    constexpr double aliasTone            = 5000.0;
    constexpr int    fftOrder             = 15;
    constexpr int    fftSize              = 32768;              // 2 to the power fftOrder
    constexpr float  heavyDrive           = 16.0f;              // what CC80 = 127 produces
    constexpr double divisorMargin        = 0.05;
    constexpr size_t complexParts         = 2;
    constexpr int    analysisSkirt        = 16;
    constexpr int    fundamentalSkirt     = 4;
    constexpr int    firstBin             = 0;
    constexpr int    firstHarmonic        = 1;
    constexpr double nyquistFraction      = 0.5;
    constexpr double noiseFloorRatio      = 1.0e-10;   // -100 dB
    constexpr double noPower              = 0.0;
    constexpr double decibelsPerDecade    = 10.0;
    constexpr double silenceDb            = -200.0;
    constexpr int    signalLengthInFfts   = 2;
    constexpr float  unity                = 1.0f;
    constexpr int    reportDecimals       = 2;
    constexpr double badAliasDb           = -20.0;
    constexpr double requiredImprovementDb = 3.0;

    // Stress and block size.
    constexpr int    stressBlocks         = 400;
    constexpr float  boundedPeak          = 8.0f;
    constexpr int    blockTestDelay       = 90;
    constexpr int    blockTestDrive       = 100;
    constexpr int    oneSecondOfSamples   = 48000;
    constexpr double blockTestTone        = 500.0;
    constexpr float  blockTestAmplitude   = 0.6f;
    constexpr int    smallBlockSize       = 64;
    constexpr int    largeBlockSize       = 256;
    constexpr float  blockAgreement       = 1.0e-4f;
}

using namespace ChannelDspTestValues;

// Behavioural cover for the per-channel effects chain. Each test here corresponds
// to a defect that was present before: a delay line that froze when bypassed,
// parameters that stepped once per block, and a waveshaper with no antialiasing.
class ChannelDspTest : public juce::UnitTest
{
public:
    ChannelDspTest() : juce::UnitTest (ChannelDspTestValues::testName, ChannelDspTestValues::testCategory) {}

    //==============================================================================
    /** Fills a stereo buffer with a sine at the given frequency, continuing from
        `phase` so that successive blocks join without a discontinuity. */
    static void fillSine (juce::AudioBuffer<float>& buffer, int numSamples,
                          double frequency, double& phase, float amplitude = defaultAmplitude)
    {
        const double inc = juce::MathConstants<double>::twoPi * frequency / sampleRate;

        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            const auto value = static_cast<float> (amplitude * std::sin (phase));
            buffer.setSample (leftChannel, sampleIndex, value);
            buffer.setSample (rightChannel, sampleIndex, value);
            phase += inc;
        }
    }

    /** Largest jump between one sample and the next. A discontinuity is what a
        click actually is, so this is the direct measure of one. */
    static float largestStep (const std::vector<float>& samples)
    {
        float worst {};

        for (size_t sampleIndex = firstStep; sampleIndex < samples.size(); ++sampleIndex)
            worst = juce::jmax (worst, std::abs (samples[sampleIndex] - samples[sampleIndex - firstStep]));

        return worst;
    }

    /** Runs `numBlocks` blocks of a steady sine through a channel, optionally
        applying a CC at a chosen block, and returns the left output. */
    static std::vector<float> render (ChannelDSP& dsp, int numBlocks, double frequency,
                                      int ccNumber = noController, int ccValue = controllerOff, int ccAtBlock = neverApply)
    {
        juce::AudioBuffer<float> buffer (stereo, blockSize);
        std::vector<float> out;
        out.reserve (static_cast<size_t> (numBlocks * blockSize));
        double phase {};

        for (int blockIndex {}; blockIndex < numBlocks; ++blockIndex)
        {
            if (ccNumber > noController && blockIndex == ccAtBlock)
                dsp.updateCC (ccNumber, ccValue);

            fillSine (buffer, blockSize, frequency, phase);
            dsp.process (buffer, blockSize);

            for (int sampleIndex {}; sampleIndex < blockSize; ++sampleIndex)
                out.push_back (buffer.getSample (leftChannel, sampleIndex));
        }

        return out;
    }

    /** Peak absolute value of the samples from `from` to the end. */
    static float peakFrom (const std::vector<float>& samples, size_t from)
    {
        float peak {};
        for (size_t sampleIndex = from; sampleIndex < samples.size(); ++sampleIndex)
            peak = juce::jmax (peak, std::abs (samples[sampleIndex]));
        return peak;
    }

    /** Renders silence and returns the loudest left-channel sample. */
    static float silentBlocksPeak (ChannelDSP& dsp, juce::AudioBuffer<float>& buffer, int numBlocks)
    {
        float peak {};
        for (int blockIndex {}; blockIndex < numBlocks; ++blockIndex)
        {
            buffer.clear();
            dsp.process (buffer, blockSize);

            for (int sampleIndex {}; sampleIndex < blockSize; ++sampleIndex)
                peak = juce::jmax (peak, std::abs (buffer.getSample (leftChannel, sampleIndex)));
        }
        return peak;
    }

    //==============================================================================
    void runTest() override
    {
        runSmoothingAndDelayTests();
        runAdaaTests();
        runDistortionAndFilterTests();
        runAliasingTest();
        runRobustnessTests();
    }

    void runSmoothingAndDelayTests()
    {
        beginTest (smoothingReachesTarget);
        {
            SmoothedParam param;
            param.setTimeConstant (smoothingMs, sampleRate);
            param.snap (zeroLevel);
            param.target = fullLevel;

            // One time constant should cover roughly 63% of the distance.
            for (int sampleIndex {}; sampleIndex < static_cast<int> (oneTimeConstantSeconds * sampleRate); ++sampleIndex)
                param.nextValue();

            expect (param.current > oneTimeConstantLow && param.current < oneTimeConstantHigh,
                    afterOneTimeConstant + juce::String (param.current));

            // And it must actually converge, not stall short.
            for (int sampleIndex {}; sampleIndex < static_cast<int> (settleSeconds * sampleRate); ++sampleIndex)
                param.nextValue();

            expectWithinAbsoluteError (param.current, fullLevel, convergenceTolerance);

            // The first step must be small. This is the property that matters:
            // a jump straight to target is what makes the click.
            SmoothedParam freshParam;
            freshParam.setTimeConstant (smoothingMs, sampleRate);
            freshParam.snap (zeroLevel);
            freshParam.target = fullLevel;
            expect (freshParam.nextValue() < firstStepLimit, firstStepIsSmall);
        }

        beginTest (snapIsImmediate);
        {
            SmoothedParam param;
            param.setTimeConstant (smoothingMs, sampleRate);
            param.snap (snapLevel);
            expectWithinAbsoluteError (param.current, snapLevel, snapTolerance);
            expectWithinAbsoluteError (param.nextValue(), snapLevel, snapTolerance);
        }

        beginTest (expressionSweepNoClick);
        {
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);

            // Drop expression from full to zero in one CC event, which is what a
            // pedal sending a coarse value or a preset change actually does.
            const auto out = render (dsp, expressionBlocks, lowTone, expressionController, controllerOff, switchBlock);

            // A 220 Hz sine at 0.5 amplitude steps by at most 0.5*2*pi*220/48000
            // ~= 0.0144 per sample on its own. Allow room for the gain ramp on top
            // and still catch anything resembling a jump to silence.
            expect (largestStep (out) < clickThreshold,
                    largestStepWas + juce::String (largestStep (out)));

            // And it did actually reach silence, so the test is not passing simply
            // because nothing happened.
            const float tail = peakFrom (out, out.size() - tailSamples);

            expect (tail < silenceThreshold, reachedSilence + juce::String (tail));
        }

        beginTest (unsmoothedWouldFail);
        {
            // Guards the test above: if largestStep() could not detect a step
            // change in gain, the previous test would pass no matter what the code
            // did. Here the gain is applied per block, as it used to be.
            std::vector<float> stepped;
            double phase {};
            const double inc = juce::MathConstants<double>::twoPi * lowTone / sampleRate;

            for (int blockIndex {}; blockIndex < expressionBlocks; ++blockIndex)
            {
                const float gain = (blockIndex < switchBlock) ? unityGain : mutedGain;

                for (int sampleIndex {}; sampleIndex < blockSize; ++sampleIndex)
                {
                    stepped.push_back (static_cast<float> (sineAmplitude * std::sin (phase)) * gain);
                    phase += inc;
                }
            }

            expect (largestStep (stepped) > clickThreshold,
                    perBlockDoesStep + juce::String (largestStep (stepped)));
        }

        beginTest (bypassDoesNotFreeze);
        {
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);

            // Delay on, then a loud passage, then delay off.
            dsp.updateCC (delayController, delayOnValue);

            juce::AudioBuffer<float> buffer (stereo, blockSize);
            double phase {};

            for (int blockIndex {}; blockIndex < loudBlocks; ++blockIndex)
            {
                fillSine (buffer, blockSize, highTone, phase, loudAmplitude);
                dsp.process (buffer, blockSize);
            }

            dsp.updateCC (delayController, controllerOff);           // delay off

            // Long enough for the smoothing to settle and for the read head to
            // have travelled well past everything written while it was loud.
            silentBlocksPeak (dsp, buffer, bypassBlocks);             // silence in

            // Now bring it back. If the line froze while bypassed, the loud
            // passage is still sitting in it and comes straight back out.
            dsp.updateCC (delayController, delayOnValue);

            const float worst = silentBlocksPeak (dsp, buffer, reenableBlocks);

            // Silence in, silence out. The line has been recording silence the
            // whole time it was bypassed, so there is nothing stale to return.
            expect (worst < silenceThreshold,
                    reenablingProduced + juce::String (worst));
        }

        beginTest (delayStillDelays);
        {
            // The fix must not have turned the delay off. Feed one loud block, then
            // silence, and confirm something comes back later.
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);
            dsp.updateCC (delayController, fullController);

            juce::AudioBuffer<float> buffer (stereo, blockSize);
            double phase {};

            // Let the mix ramp settle before the burst, so the burst is heard at
            // full mix rather than through a rising ramp.
            silentBlocksPeak (dsp, buffer, loudBlocks);

            for (int blockIndex {}; blockIndex < burstBlocks; ++blockIndex)
            {
                fillSine (buffer, blockSize, highTone, phase, loudAmplitude);
                dsp.process (buffer, blockSize);
            }

            const float echoPeak = silentBlocksPeak (dsp, buffer, echoBlocks);

            expect (echoPeak > echoThreshold,
                    burstShouldEcho + juce::String (echoPeak));
        }
    }

    void runAdaaTests()
    {
        beginTest (logCoshStable);
        {
            for (float input : logCoshInputs)
                expectWithinAbsoluteError (Adaa1Tanh::logCosh (input), std::log (std::cosh (input)), logCoshTolerance);

            expect (std::isinf (std::cosh (overflowInput)), directFormOverflows);
            expect (std::isfinite (Adaa1Tanh::logCosh (overflowInput)), stableFormDoesNot);
            expectWithinAbsoluteError (Adaa1Tanh::logCosh (overflowInput), overflowInput - logOfTwo, overflowTolerance);
        }

        beginTest (logCoshIsAntiderivative);
        {
            // If logCosh were not the integral of tanh, the shaper would quietly
            // produce a different curve rather than an antialiased one.
            for (float input = slopeRangeStart; input <= slopeRangeEnd; input += slopeRangeStep)
            {
                const float slope = (Adaa1Tanh::logCosh (input + derivativeStep) - Adaa1Tanh::logCosh (input - derivativeStep))
                                    / (centralDifferenceSpan * derivativeStep);
                expectWithinAbsoluteError (slope, std::tanh (input), slopeTolerance);
            }
        }

        beginTest (adaaTracksTanh);
        {
            // ADAA returns the average of the curve across the sample interval, so
            // its output sits half a sample in the past. On a low-frequency input
            // there is no aliasing to remove, so once that offset is accounted for
            // it should be almost identical to naive shaping. A large residual
            // would mean it is changing the tone rather than cleaning it up.
            Adaa1Tanh adaa;
            std::vector<float> naive, processed;
            double phase {};
            const double inc = juce::MathConstants<double>::twoPi * slowTone / sampleRate;

            for (int sampleIndex {}; sampleIndex < slowSamples; ++sampleIndex)
            {
                const auto input = static_cast<float> (sineAmplitude * std::sin (phase) * slowDrive);
                naive.push_back (std::tanh (input));
                processed.push_back (adaa.process (input));
                phase += inc;
            }

            float worstDirect {}, worstAligned {};

            for (size_t sampleIndex = warmUpSamples; sampleIndex < naive.size(); ++sampleIndex)
            {
                worstDirect = juce::jmax (worstDirect, std::abs (naive[sampleIndex] - processed[sampleIndex]));
                const float midpoint = half * (naive[sampleIndex] + naive[sampleIndex - firstStep]);
                worstAligned = juce::jmax (worstAligned, std::abs (midpoint - processed[sampleIndex]));
            }

            expect (worstAligned < worstDirect * half,
                    mostlyHalfSampleDelay
                    + juce::String (worstDirect) + alignedLabel + juce::String (worstAligned));
            expect (worstAligned < alignedLimit, alignedResidual + juce::String (worstAligned));
        }
    }

    void runDistortionAndFilterTests()
    {
        beginTest (distortionBypassedAtNeutral);
        {
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);

            const auto clean = render (dsp, neutralBlocks, oneKilohertz);
            const float cleanPeak = peakFrom (clean, clean.size() / halves);

            // Untouched: a 0.5 amplitude sine in, a 0.5 amplitude sine out.
            expectWithinAbsoluteError (cleanPeak, defaultAmplitude, peakTolerance);

            ChannelDSP driven;
            driven.prepare (sampleRate, blockSize);
            const auto hot = render (driven, drivenBlocks, oneKilohertz, driveController, fullController, driveOnBlock);
            const float hotPeak = peakFrom (hot, hot.size() / halves);

            // Heavy drive squares the sine up toward the normalisation ceiling.
            expect (hotPeak > drivenPeakFloor, drivenPeakWas + juce::String (hotPeak));
        }

        beginTest (engagingDriveNoClick);
        {
            // An absolute step threshold cannot be used here. At drive 16 the
            // output is close to a square wave, whose legitimate slew at 220 Hz is
            // 16 * 0.5 * 2pi * 220/48000 ~= 0.23 per sample - far larger than any
            // click this test would be looking for. So the ramp is compared
            // against the same effect in steady state: whatever slew the driven
            // waveform has, engaging the drive must not exceed it.
            ChannelDSP steady;
            steady.prepare (sampleRate, blockSize);
            steady.updateCC (driveController, fullController);
            const auto settled = render (steady, drivenBlocks, lowTone);

            ChannelDSP ramped;
            ramped.prepare (sampleRate, blockSize);
            const auto engaging = render (ramped, drivenBlocks, lowTone, driveController, fullController, engageBlock);

            const float steadyStep = largestStep (settled);
            const float rampStep   = largestStep (engaging);

            expect (rampStep <= steadyStep * slewMargin + slewAllowance,
                    steadySlewsBy + juce::String (steadyStep)
                    + engagingSlewsBy + juce::String (rampStep));
        }

        beginTest (cutoffSweepNoClick);
        {
            // Same reasoning, and this one caught a real defect: smoothing the
            // cutoff linearly in hertz sweeps it at roughly 70 octaves per second
            // at the start of a large move, which registers here as a step an
            // order of magnitude above the signal's own slew. Smoothing log2(Hz)
            // instead gives a constant octaves-per-second ramp.
            ChannelDSP steady;
            steady.prepare (sampleRate, blockSize);
            steady.updateCC (cutoffController, closedCutoff);
            const auto settledAll = render (steady, sweepBlocks, lowTone);

            // Second half only. The reference render engages the filter on its own
            // first block, so its opening samples carry the very transient this
            // test exists to detect, and including them would let the comparison
            // pass by measuring the same fault twice.
            const std::vector<float> settled (settledAll.begin() + (std::ptrdiff_t) (settledAll.size() / halves),
                                              settledAll.end());

            ChannelDSP swept;
            swept.prepare (sampleRate, blockSize);
            const auto sweeping = render (swept, sweepBlocks, lowTone, cutoffController, closedCutoff, engageBlock);

            const float steadyStep = largestStep (settled);
            const float sweepStep  = largestStep (sweeping);

            expect (sweepStep <= steadyStep * sweepSlewMargin + sweepSlewAllowance,
                    steadySlewsBy + juce::String (steadyStep)
                    + sweepSlewsBy + juce::String (sweepStep));

            // And the filter did something, so this is not passing on a no-op.
            const float tail = peakFrom (sweeping, sweeping.size() - filterTailSamples);

            expect (tail < closedFilterCeiling, closedLowpassTail
                                                + juce::String (tail));
        }
    }

    /** Folded (aliased) power relative to the fundamental, in dB. */
    static double aliasToSignalDb (const std::vector<float>& signal)
    {
        juce::dsp::FFT fft (fftOrder);
        juce::dsp::WindowingFunction<float> window (
            (size_t) fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        std::vector<float> data ((size_t) fftSize * complexParts, float {});
        std::copy (signal.end() - fftSize, signal.end(), data.begin());
        window.multiplyWithWindowingTable (data.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (data.data());

        const int numBins = fftSize / (int) halves;
        const double binHz = sampleRate / fftSize;

        // Blackman-Harris spreads a tone over roughly 8 bins, and its
        // sidelobes reach -92 dB, so a generous skirt is excluded around
        // anything genuine rather than counted as folded content.
        std::vector<bool> claimed ((size_t) numBins, false);

        auto claim = [&] (double hz)
        {
            const int centre = (int) std::lround (hz / binHz);
            for (int bin = centre - analysisSkirt; bin <= centre + analysisSkirt; ++bin)
                if (bin >= firstBin && bin < numBins)
                    claimed[(size_t) bin] = true;
        };

        for (int bin = firstBin; bin <= analysisSkirt && bin < numBins; ++bin)   // DC
            claimed[(size_t) bin] = true;

        double fundamental {};
        const int centre = (int) std::lround (aliasTone / binHz);

        for (int bin = centre - fundamentalSkirt; bin <= centre + fundamentalSkirt; ++bin)
            if (bin >= firstBin && bin < numBins)
                fundamental += (double) data[(size_t) bin] * data[(size_t) bin];

        for (int order = firstHarmonic; order * aliasTone < sampleRate * nyquistFraction; ++order)
            claim (order * aliasTone);

        double alias {};
        const double floorPower = fundamental * noiseFloorRatio;   // -100 dB

        for (int bin = firstBin; bin < numBins; ++bin)
        {
            if (claimed[(size_t) bin])
                continue;

            const double power = (double) data[(size_t) bin] * data[(size_t) bin];

            if (power > floorPower)
                alias += power;
        }

        return (alias > noPower && fundamental > noPower)
                 ? decibelsPerDecade * std::log10 (alias / fundamental)
                 : silenceDb;
    }

    void runAliasingTest()
    {
        beginTest (adaaReducesAliasing);
        {
            // The reason the distortion stage changed at all. A nonlinearity makes
            // harmonics without limit; the ones above Nyquist cannot exist in the
            // sampled signal, so they fold back to |fs - n*f| - frequencies that
            // are not harmonics of anything, and which descend as the played note
            // rises. That is what makes heavy drive sound metallic in the top
            // octaves rather than merely distorted.
            //
            // 5 kHz at 48 kHz is the clearest case: the 3rd harmonic at 15 kHz
            // still fits, but the 5th at 25 kHz does not, and folds to 23 kHz.

            // Deliberately NOT a whole divisor of the sample rate. When fs/f is an
            // integer, every folded harmonic lands exactly on a genuine one and
            // becomes unmeasurable - 1 kHz at 48 kHz is the obvious test tone and
            // one of the worst possible choices. 48000/5000 = 9.6.
            expect (std::abs (sampleRate / aliasTone - std::round (sampleRate / aliasTone)) > divisorMargin,
                    mustNotDivideRate);

            const int total = fftSize * signalLengthInFfts;
            std::vector<float> naive, viaChannel;
            naive.reserve ((size_t) total);
            viaChannel.reserve ((size_t) total);

            // The naive reference: exactly the expression this stage used before.
            const float norm = unity / std::tanh (heavyDrive);
            double phase {};
            const double inc = juce::MathConstants<double>::twoPi * aliasTone / sampleRate;

            for (int sampleIndex {}; sampleIndex < total; ++sampleIndex)
            {
                const auto input = (float) (sineAmplitude * std::sin (phase));
                naive.push_back (std::tanh (input * heavyDrive) * norm);
                phase += inc;
            }

            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);
            dsp.updateCC (driveController, fullController);

            juce::AudioBuffer<float> buffer (stereo, blockSize);
            phase = {};

            for (int done {}; done < total; done += blockSize)
            {
                fillSine (buffer, blockSize, aliasTone, phase);
                dsp.process (buffer, blockSize);

                for (int sampleIndex {}; sampleIndex < blockSize; ++sampleIndex)
                    viaChannel.push_back (buffer.getSample (leftChannel, sampleIndex));
            }

            const double naiveDb = aliasToSignalDb (naive);
            const double adaaDb  = aliasToSignalDb (viaChannel);

            // std::cout rather than logMessage: this target installs no juce
            // Logger, so logMessage would silently go nowhere, and this number is
            // the whole justification for the change.
            std::cout << aliasPrefix << juce::String (naiveDb, reportDecimals)
                      << adaaLabel << juce::String (adaaDb, reportDecimals)
                      << improvementLabel << juce::String (naiveDb - adaaDb, reportDecimals)
                      << decibels << std::endl;

            expect (naiveDb > badAliasDb,
                    naiveShouldAlias
                    + juce::String (naiveDb, reportDecimals) + decibels);
            expect (adaaDb < naiveDb - requiredImprovementDb,
                    adaaShouldImprove + juce::String (adaaDb, reportDecimals)
                    + againstLabel + juce::String (naiveDb, reportDecimals) + decibels);
        }
    }

    void runRobustnessTests()
    {
        beginTest (chainStaysFinite);
        {
            ChannelDSP dsp;
            dsp.prepare (sampleRate, blockSize);

            for (int controller : allEffectControllers)
                dsp.updateCC (controller, fullController);

            juce::AudioBuffer<float> buffer (stereo, blockSize);
            double phase {};
            bool allFinite = true;
            float peak {};

            for (int blockIndex {}; blockIndex < stressBlocks; ++blockIndex)
            {
                fillSine (buffer, blockSize, brightTone, phase, loudAmplitude);
                dsp.process (buffer, blockSize);

                for (int channel {}; channel < stereo; ++channel)
                    for (int sampleIndex {}; sampleIndex < blockSize; ++sampleIndex)
                    {
                        const float value = buffer.getSample (channel, sampleIndex);
                        allFinite = allFinite && std::isfinite (value);
                        if (std::isfinite (value))
                            peak = juce::jmax (peak, std::abs (value));
                    }
            }

            expect (allFinite, everySampleFinite);
            expect (peak < boundedPeak, outputBounded + juce::String (peak));
        }

        beginTest (blockSizeIndependent);
        {
            // The smoothers and the delay carry state across blocks, so the output
            // must depend on the audio, not on how the host happens to chop it up.
            auto renderAt = [] (int size)
            {
                ChannelDSP dsp;
                dsp.prepare (sampleRate, size);
                dsp.updateCC (delayController, blockTestDelay);
                dsp.updateCC (driveController, blockTestDrive);

                juce::AudioBuffer<float> buffer (stereo, size);
                std::vector<float> out;
                double phase {};

                for (int done {}; done < oneSecondOfSamples; done += size)
                {
                    const int count = juce::jmin (size, oneSecondOfSamples - done);
                    fillSine (buffer, count, blockTestTone, phase, blockTestAmplitude);
                    dsp.process (buffer, count);

                    for (int sampleIndex {}; sampleIndex < count; ++sampleIndex)
                        out.push_back (buffer.getSample (leftChannel, sampleIndex));
                }

                return out;
            };

            const auto small = renderAt (smallBlockSize);
            const auto large = renderAt (largeBlockSize);

            expectEquals ((int) small.size(), (int) large.size());

            float worst {};
            for (size_t sampleIndex {}; sampleIndex < juce::jmin (small.size(), large.size()); ++sampleIndex)
                worst = juce::jmax (worst, std::abs (small[sampleIndex] - large[sampleIndex]));

            expect (worst < blockAgreement,
                    blockSizesAgree + juce::String (worst));
        }
    }
};

static ChannelDspTest channelDspTest;
