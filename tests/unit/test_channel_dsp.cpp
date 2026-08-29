#include <JuceHeader.h>
#include <iostream>
#include "Audio/AudioHandler.h"

// Behavioural cover for the per-channel effects chain. Each test here corresponds
// to a defect that was present before: a delay line that froze when bypassed,
// parameters that stepped once per block, and a waveshaper with no antialiasing.
class ChannelDspTest : public juce::UnitTest
{
public:
    ChannelDspTest() : juce::UnitTest ("ChannelDSP", "Unit") {}

    //==============================================================================
    static constexpr double sr = 48000.0;
    static constexpr int    blockSize = 64;

    /** Fills a stereo buffer with a sine at the given frequency, continuing from
        `phase` so that successive blocks join without a discontinuity. */
    static void fillSine (juce::AudioBuffer<float>& buffer, int numSamples,
                          double frequency, double& phase, float amplitude = 0.5f)
    {
        const double inc = juce::MathConstants<double>::twoPi * frequency / sr;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto v = static_cast<float> (amplitude * std::sin (phase));
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v);
            phase += inc;
        }
    }

    /** Largest jump between one sample and the next. A discontinuity is what a
        click actually is, so this is the direct measure of one. */
    static float largestStep (const std::vector<float>& x)
    {
        float worst = 0.0f;

        for (size_t i = 1; i < x.size(); ++i)
            worst = juce::jmax (worst, std::abs (x[i] - x[i - 1]));

        return worst;
    }

    /** Runs `numBlocks` blocks of a steady sine through a channel, optionally
        applying a CC at a chosen block, and returns the left output. */
    static std::vector<float> render (ChannelDSP& dsp, int numBlocks, double frequency,
                                      int ccNumber = -1, int ccValue = 0, int ccAtBlock = -1)
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        std::vector<float> out;
        out.reserve (static_cast<size_t> (numBlocks * blockSize));
        double phase = 0.0;

        for (int b = 0; b < numBlocks; ++b)
        {
            if (ccNumber >= 0 && b == ccAtBlock)
                dsp.updateCC (ccNumber, ccValue);

            fillSine (buffer, blockSize, frequency, phase);
            dsp.process (buffer, blockSize);

            for (int i = 0; i < blockSize; ++i)
                out.push_back (buffer.getSample (0, i));
        }

        return out;
    }

    //==============================================================================
    void runTest() override
    {
        beginTest ("smoothing: a parameter reaches its target but does not jump there");
        {
            SmoothedParam p;
            p.setTimeConstant (20.0, sr);
            p.snap (0.0f);
            p.target = 1.0f;

            // One time constant should cover roughly 63% of the distance.
            for (int i = 0; i < static_cast<int> (0.020 * sr); ++i)
                p.nextValue();

            expect (p.current > 0.55f && p.current < 0.70f,
                    "after one time constant, got " + juce::String (p.current));

            // And it must actually converge, not stall short.
            for (int i = 0; i < static_cast<int> (0.5 * sr); ++i)
                p.nextValue();

            expectWithinAbsoluteError (p.current, 1.0f, 0.001f);

            // The first step must be small. This is the property that matters:
            // a jump straight to target is what makes the click.
            SmoothedParam q;
            q.setTimeConstant (20.0, sr);
            q.snap (0.0f);
            q.target = 1.0f;
            expect (q.nextValue() < 0.005f, "the first step is small");
        }

        beginTest ("smoothing: snap is immediate, for prepare() only");
        {
            SmoothedParam p;
            p.setTimeConstant (20.0, sr);
            p.snap (0.75f);
            expectWithinAbsoluteError (p.current, 0.75f, 1.0e-6f);
            expectWithinAbsoluteError (p.nextValue(), 0.75f, 1.0e-6f);
        }

        beginTest ("expression: a full CC sweep produces no discontinuity");
        {
            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);

            // Drop expression from full to zero in one CC event, which is what a
            // pedal sending a coarse value or a preset change actually does.
            const auto out = render (dsp, 40, 220.0, 11, 0, 10);

            // A 220 Hz sine at 0.5 amplitude steps by at most 0.5*2*pi*220/48000
            // ~= 0.0144 per sample on its own. Allow room for the gain ramp on top
            // and still catch anything resembling a jump to silence.
            expect (largestStep (out) < 0.05f,
                    "largest sample-to-sample step was " + juce::String (largestStep (out)));

            // And it did actually reach silence, so the test is not passing simply
            // because nothing happened.
            float tail = 0.0f;
            for (size_t i = out.size() - 200; i < out.size(); ++i)
                tail = juce::jmax (tail, std::abs (out[i]));

            expect (tail < 0.01f, "expression reached silence, tail peak " + juce::String (tail));
        }

        beginTest ("expression: an unsmoothed implementation would fail the same check");
        {
            // Guards the test above: if largestStep() could not detect a step
            // change in gain, the previous test would pass no matter what the code
            // did. Here the gain is applied per block, as it used to be.
            std::vector<float> stepped;
            double phase = 0.0;
            const double inc = juce::MathConstants<double>::twoPi * 220.0 / sr;

            for (int b = 0; b < 40; ++b)
            {
                const float gain = (b < 10) ? 1.0f : 0.0f;

                for (int i = 0; i < blockSize; ++i)
                {
                    stepped.push_back (static_cast<float> (0.5 * std::sin (phase)) * gain);
                    phase += inc;
                }
            }

            expect (largestStep (stepped) > 0.05f,
                    "the per-block version does step, by " + juce::String (largestStep (stepped)));
        }

        beginTest ("delay: bypassing does not freeze the line and replay stale audio");
        {
            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);

            // Delay on, then a loud passage, then delay off.
            dsp.updateCC (94, 100);

            juce::AudioBuffer<float> buffer (2, blockSize);
            double phase = 0.0;

            for (int b = 0; b < 60; ++b)
            {
                fillSine (buffer, blockSize, 440.0, phase, 0.8f);
                dsp.process (buffer, blockSize);
            }

            dsp.updateCC (94, 0);           // delay off

            // Long enough for the smoothing to settle and for the read head to
            // have travelled well past everything written while it was loud.
            for (int b = 0; b < 400; ++b)
            {
                buffer.clear();             // silence in
                dsp.process (buffer, blockSize);
            }

            // Now bring it back. If the line froze while bypassed, the loud
            // passage is still sitting in it and comes straight back out.
            dsp.updateCC (94, 100);

            float worst = 0.0f;

            for (int b = 0; b < 200; ++b)
            {
                buffer.clear();
                dsp.process (buffer, blockSize);

                for (int i = 0; i < blockSize; ++i)
                    worst = juce::jmax (worst, std::abs (buffer.getSample (0, i)));
            }

            // Silence in, silence out. The line has been recording silence the
            // whole time it was bypassed, so there is nothing stale to return.
            expect (worst < 0.01f,
                    "re-enabling the delay after silence produced " + juce::String (worst));
        }

        beginTest ("delay: still audibly delays when it is meant to");
        {
            // The fix must not have turned the delay off. Feed one loud block, then
            // silence, and confirm something comes back later.
            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);
            dsp.updateCC (94, 127);

            juce::AudioBuffer<float> buffer (2, blockSize);
            double phase = 0.0;

            // Let the mix ramp settle before the burst, so the burst is heard at
            // full mix rather than through a rising ramp.
            for (int b = 0; b < 60; ++b)
            {
                buffer.clear();
                dsp.process (buffer, blockSize);
            }

            for (int b = 0; b < 20; ++b)
            {
                fillSine (buffer, blockSize, 440.0, phase, 0.8f);
                dsp.process (buffer, blockSize);
            }

            float echoPeak = 0.0f;

            for (int b = 0; b < 500; ++b)
            {
                buffer.clear();
                dsp.process (buffer, blockSize);

                for (int i = 0; i < blockSize; ++i)
                    echoPeak = juce::jmax (echoPeak, std::abs (buffer.getSample (0, i)));
            }

            expect (echoPeak > 0.05f,
                    "a burst should come back as an echo, peak was " + juce::String (echoPeak));
        }

        beginTest ("ADAA: log(cosh) is stable where the direct form overflows");
        {
            for (float x : { 0.0f, 0.5f, 2.0f, 8.0f })
                expectWithinAbsoluteError (Adaa1Tanh::logCosh (x), std::log (std::cosh (x)), 1.0e-4f);

            expect (std::isinf (std::cosh (400.0f)), "the direct form overflows at 400");
            expect (std::isfinite (Adaa1Tanh::logCosh (400.0f)), "the stable form does not");
            expectWithinAbsoluteError (Adaa1Tanh::logCosh (400.0f), 400.0f - 0.6931472f, 1.0e-3f);
        }

        beginTest ("ADAA: it really is the antiderivative of tanh");
        {
            // If logCosh were not the integral of tanh, the shaper would quietly
            // produce a different curve rather than an antialiased one.
            const float h = 1.0e-3f;

            for (float x = -6.0f; x <= 6.0f; x += 1.1f)
            {
                const float slope = (Adaa1Tanh::logCosh (x + h) - Adaa1Tanh::logCosh (x - h)) / (2.0f * h);
                expectWithinAbsoluteError (slope, std::tanh (x), 2.0e-3f);
            }
        }

        beginTest ("ADAA: tracks plain tanh on a slow signal, allowing its half-sample delay");
        {
            // ADAA returns the average of the curve across the sample interval, so
            // its output sits half a sample in the past. On a low-frequency input
            // there is no aliasing to remove, so once that offset is accounted for
            // it should be almost identical to naive shaping. A large residual
            // would mean it is changing the tone rather than cleaning it up.
            Adaa1Tanh adaa;
            std::vector<float> naive, processed;
            double phase = 0.0;
            const double inc = juce::MathConstants<double>::twoPi * 100.0 / sr;

            for (int i = 0; i < 4800; ++i)
            {
                const auto x = static_cast<float> (0.5 * std::sin (phase) * 8.0);
                naive.push_back (std::tanh (x));
                processed.push_back (adaa.process (x));
                phase += inc;
            }

            float worstDirect = 0.0f, worstAligned = 0.0f;

            for (size_t i = 100; i < naive.size(); ++i)
            {
                worstDirect = juce::jmax (worstDirect, std::abs (naive[i] - processed[i]));
                const float midpoint = 0.5f * (naive[i] + naive[i - 1]);
                worstAligned = juce::jmax (worstAligned, std::abs (midpoint - processed[i]));
            }

            expect (worstAligned < worstDirect * 0.5f,
                    "most of the difference is the half-sample delay: direct "
                    + juce::String (worstDirect) + ", aligned " + juce::String (worstAligned));
            expect (worstAligned < 0.01f, "aligned residual " + juce::String (worstAligned));
        }

        beginTest ("distortion: bypassed at neutral, active when driven");
        {
            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);

            const auto clean = render (dsp, 30, 1000.0);

            float cleanPeak = 0.0f;
            for (size_t i = clean.size() / 2; i < clean.size(); ++i)
                cleanPeak = juce::jmax (cleanPeak, std::abs (clean[i]));

            // Untouched: a 0.5 amplitude sine in, a 0.5 amplitude sine out.
            expectWithinAbsoluteError (cleanPeak, 0.5f, 0.02f);

            ChannelDSP driven;
            driven.prepare (sr, blockSize);
            const auto hot = render (driven, 120, 1000.0, 80, 127, 5);

            float hotPeak = 0.0f;
            for (size_t i = hot.size() / 2; i < hot.size(); ++i)
                hotPeak = juce::jmax (hotPeak, std::abs (hot[i]));

            // Heavy drive squares the sine up toward the normalisation ceiling.
            expect (hotPeak > 0.85f, "driven peak was " + juce::String (hotPeak));
        }

        beginTest ("distortion: engaging it produces no discontinuity");
        {
            // An absolute step threshold cannot be used here. At drive 16 the
            // output is close to a square wave, whose legitimate slew at 220 Hz is
            // 16 * 0.5 * 2pi * 220/48000 ~= 0.23 per sample - far larger than any
            // click this test would be looking for. So the ramp is compared
            // against the same effect in steady state: whatever slew the driven
            // waveform has, engaging the drive must not exceed it.
            ChannelDSP steady;
            steady.prepare (sr, blockSize);
            steady.updateCC (80, 127);
            const auto settled = render (steady, 120, 220.0);

            ChannelDSP ramped;
            ramped.prepare (sr, blockSize);
            const auto engaging = render (ramped, 120, 220.0, 80, 127, 20);

            const float steadyStep = largestStep (settled);
            const float rampStep   = largestStep (engaging);

            expect (rampStep <= steadyStep * 1.15f + 0.005f,
                    "steady state slews by " + juce::String (steadyStep)
                    + ", engaging the drive slews by " + juce::String (rampStep));
        }

        beginTest ("filter: a cutoff sweep produces no discontinuity");
        {
            // Same reasoning, and this one caught a real defect: smoothing the
            // cutoff linearly in hertz sweeps it at roughly 70 octaves per second
            // at the start of a large move, which registers here as a step an
            // order of magnitude above the signal's own slew. Smoothing log2(Hz)
            // instead gives a constant octaves-per-second ramp.
            ChannelDSP steady;
            steady.prepare (sr, blockSize);
            steady.updateCC (74, 10);
            const auto settledAll = render (steady, 200, 220.0);

            // Second half only. The reference render engages the filter on its own
            // first block, so its opening samples carry the very transient this
            // test exists to detect, and including them would let the comparison
            // pass by measuring the same fault twice.
            const std::vector<float> settled (settledAll.begin() + settledAll.size() / 2,
                                              settledAll.end());

            ChannelDSP swept;
            swept.prepare (sr, blockSize);
            const auto sweeping = render (swept, 200, 220.0, 74, 10, 20);

            const float steadyStep = largestStep (settled);
            const float sweepStep  = largestStep (sweeping);

            expect (sweepStep <= steadyStep * 3.0f + 0.01f,
                    "steady state slews by " + juce::String (steadyStep)
                    + ", the sweep slews by " + juce::String (sweepStep));

            // And the filter did something, so this is not passing on a no-op.
            float tail = 0.0f;
            for (size_t i = sweeping.size() - 500; i < sweeping.size(); ++i)
                tail = juce::jmax (tail, std::abs (sweeping[i]));

            expect (tail < 0.45f, "a closed lowpass should attenuate 220 Hz, tail peak "
                                  + juce::String (tail));
        }

        beginTest ("distortion: ADAA measurably reduces aliasing versus the naive shaper");
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
            constexpr double freq = 5000.0;
            constexpr int fftOrder = 15;                 // 32768
            constexpr int fftSize = 1 << fftOrder;
            constexpr float drive = 16.0f;               // what CC80 = 127 produces

            // Deliberately NOT a whole divisor of the sample rate. When fs/f is an
            // integer, every folded harmonic lands exactly on a genuine one and
            // becomes unmeasurable - 1 kHz at 48 kHz is the obvious test tone and
            // one of the worst possible choices. 48000/5000 = 9.6.
            expect (std::abs (sr / freq - std::round (sr / freq)) > 0.05,
                    "the test frequency must not divide the sample rate");

            auto aliasToSignalDb = [&] (const std::vector<float>& signal)
            {
                juce::dsp::FFT fft (fftOrder);
                juce::dsp::WindowingFunction<float> window (
                    (size_t) fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

                std::vector<float> data ((size_t) fftSize * 2, 0.0f);
                std::copy (signal.end() - fftSize, signal.end(), data.begin());
                window.multiplyWithWindowingTable (data.data(), (size_t) fftSize);
                fft.performFrequencyOnlyForwardTransform (data.data());

                const int numBins = fftSize / 2;
                const double binHz = sr / fftSize;

                // Blackman-Harris spreads a tone over roughly 8 bins, and its
                // sidelobes reach -92 dB, so a generous skirt is excluded around
                // anything genuine rather than counted as folded content.
                constexpr int skirt = 16;
                std::vector<bool> claimed ((size_t) numBins, false);

                auto claim = [&] (double hz)
                {
                    const int centre = (int) std::lround (hz / binHz);
                    for (int b = centre - skirt; b <= centre + skirt; ++b)
                        if (b >= 0 && b < numBins)
                            claimed[(size_t) b] = true;
                };

                for (int b = 0; b <= skirt && b < numBins; ++b)   // DC
                    claimed[(size_t) b] = true;

                double fundamental = 0.0;
                const int centre = (int) std::lround (freq / binHz);

                for (int b = centre - 4; b <= centre + 4; ++b)
                    if (b >= 0 && b < numBins)
                        fundamental += (double) data[(size_t) b] * data[(size_t) b];

                for (int order = 1; order * freq < sr * 0.5; ++order)
                    claim (order * freq);

                double alias = 0.0;
                const double floorPower = fundamental * 1.0e-10;   // -100 dB

                for (int b = 0; b < numBins; ++b)
                {
                    if (claimed[(size_t) b])
                        continue;

                    const double p = (double) data[(size_t) b] * data[(size_t) b];

                    if (p > floorPower)
                        alias += p;
                }

                return (alias > 0.0 && fundamental > 0.0)
                         ? 10.0 * std::log10 (alias / fundamental)
                         : -200.0;
            };

            const int total = fftSize * 2;
            std::vector<float> naive, viaChannel;
            naive.reserve ((size_t) total);
            viaChannel.reserve ((size_t) total);

            // The naive reference: exactly the expression this stage used before.
            const float norm = 1.0f / std::tanh (drive);
            double phase = 0.0;
            const double inc = juce::MathConstants<double>::twoPi * freq / sr;

            for (int i = 0; i < total; ++i)
            {
                const auto x = (float) (0.5 * std::sin (phase));
                naive.push_back (std::tanh (x * drive) * norm);
                phase += inc;
            }

            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);
            dsp.updateCC (80, 127);

            juce::AudioBuffer<float> buffer (2, blockSize);
            phase = 0.0;

            for (int done = 0; done < total; done += blockSize)
            {
                fillSine (buffer, blockSize, freq, phase);
                dsp.process (buffer, blockSize);

                for (int i = 0; i < blockSize; ++i)
                    viaChannel.push_back (buffer.getSample (0, i));
            }

            const double naiveDb = aliasToSignalDb (naive);
            const double adaaDb  = aliasToSignalDb (viaChannel);

            // std::cout rather than logMessage: this target installs no juce
            // Logger, so logMessage would silently go nowhere, and this number is
            // the whole justification for the change.
            std::cout << "    alias/signal: naive " << juce::String (naiveDb, 2)
                      << " dB, ADAA " << juce::String (adaaDb, 2)
                      << " dB, improvement " << juce::String (naiveDb - adaaDb, 2)
                      << " dB" << std::endl;

            expect (naiveDb > -20.0,
                    "the naive shaper should alias badly here, measured "
                    + juce::String (naiveDb, 2) + " dB");
            expect (adaaDb < naiveDb - 3.0,
                    "ADAA should improve on it, measured " + juce::String (adaaDb, 2)
                    + " dB against " + juce::String (naiveDb, 2) + " dB");
        }

        beginTest ("the chain stays finite across every parameter at maximum");
        {
            ChannelDSP dsp;
            dsp.prepare (sr, blockSize);

            for (int cc : { 11, 71, 74, 80, 91, 92, 93, 94, 95 })
                dsp.updateCC (cc, 127);

            juce::AudioBuffer<float> buffer (2, blockSize);
            double phase = 0.0;
            bool allFinite = true;
            float peak = 0.0f;

            for (int b = 0; b < 400; ++b)
            {
                fillSine (buffer, blockSize, 3000.0, phase, 0.8f);
                dsp.process (buffer, blockSize);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                    {
                        const float v = buffer.getSample (ch, i);
                        allFinite = allFinite && std::isfinite (v);
                        if (std::isfinite (v))
                            peak = juce::jmax (peak, std::abs (v));
                    }
            }

            expect (allFinite, "every sample is finite with all effects at maximum");
            expect (peak < 8.0f, "output stays bounded, peak was " + juce::String (peak));
        }

        beginTest ("block size does not change the result");
        {
            // The smoothers and the delay carry state across blocks, so the output
            // must depend on the audio, not on how the host happens to chop it up.
            auto renderAt = [] (int size)
            {
                ChannelDSP dsp;
                dsp.prepare (sr, size);
                dsp.updateCC (94, 90);
                dsp.updateCC (80, 100);

                juce::AudioBuffer<float> buffer (2, size);
                std::vector<float> out;
                double phase = 0.0;

                const int total = 48000;

                for (int done = 0; done < total; done += size)
                {
                    const int count = juce::jmin (size, total - done);
                    fillSine (buffer, count, 500.0, phase, 0.6f);
                    dsp.process (buffer, count);

                    for (int i = 0; i < count; ++i)
                        out.push_back (buffer.getSample (0, i));
                }

                return out;
            };

            const auto a = renderAt (64);
            const auto b = renderAt (256);

            expectEquals ((int) a.size(), (int) b.size());

            float worst = 0.0f;
            for (size_t i = 0; i < juce::jmin (a.size(), b.size()); ++i)
                worst = juce::jmax (worst, std::abs (a[i] - b[i]));

            expect (worst < 1.0e-4f,
                    "64 and 256 sample blocks agree to " + juce::String (worst));
        }
    }
};

static ChannelDspTest channelDspTest;
