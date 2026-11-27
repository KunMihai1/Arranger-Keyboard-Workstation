#pragma once

#include <algorithm>
#include <vector>

/*  A delay line read at a real-numbered position (port of va-effects' DelayLine, Lagrange-3 only).

    ChannelDSP's delay used to read at whole samples and move its read head instantly when CC94 changed the time: the
    head jumped, which is a click (wiki: "delay read offset still jumps"). With a fractional read the time can glide
    between samples, so a change becomes a short pitch bend instead of a step. va-effects measured the choices:
    truncation steps, linear interpolation pumps the treble under modulation (-89 dB), the allpass mistunes its
    recursion while the delay moves; Lagrange-3 stayed below its -200 dB analyser floor. Four taps per read.

    prepare() allocates (never on the audio thread); write() and read() do not. */
class FractionalDelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        const int wanted = maxDelaySamples + extraTaps;
        int size = smallestBuffer;
        while (size < wanted)
            size *= growthFactor;
        buffer.assign (static_cast<size_t> (size), silence);
        bufferSize = size;
        mask = size - wrapOffset;
        maxDelay = static_cast<double> (maxDelaySamples);
        reset();
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), silence);
        writePos = {};
    }

    bool isPrepared() const noexcept { return bufferSize > int {}; }

    /** Store one sample and advance (every sample, read or not: the line is a record of recent history). */
    void write (float sample) noexcept
    {
        buffer[static_cast<size_t> (writePos)] = sample;
        writePos = (writePos + nextSample) & mask;
    }

    /** The signal `delaySamples` back from the newest write, between samples by a cubic through four taps.
        Clamped to [shortestDelay, maxDelay] (Lagrange needs one sample nearer the present than the read position). */
    float read (double delaySamples) const noexcept
    {
        delaySamples = std::clamp (delaySamples, shortestDelay, maxDelay);
        const int whole = static_cast<int> (delaySamples);
        const double fraction = delaySamples - static_cast<double> (whole);

        const float nearer  = tap (whole - nextSample);
        const float atWhole = tap (whole);
        const float further = tap (whole + nextSample);
        const float furthest = tap (whole + nextSample + nextSample);

        // Lagrange basis for the nodes -1, 0, 1, 2 (one step apart), evaluated at `fraction`.
        const double at = fraction;
        const double weightNearer   = -at * (at - step) * (at - twoSteps) / outerDenominator;
        const double weightAtWhole  =  (at * at - step) * (at - twoSteps) / innerDenominator;
        const double weightFurther  = -at * (at + step) * (at - twoSteps) / innerDenominator;
        const double weightFurthest =  at * (at * at - step) / outerDenominator;
        return static_cast<float> (weightNearer * nearer + weightAtWhole * atWhole
                                   + weightFurther * further + weightFurthest * furthest);
    }

    static constexpr double shortestDelay = 1.0;   // samples

private:
    float tap (int samplesBack) const noexcept
    {
        // writePos is one past the newest sample; adding bufferSize keeps the index non-negative.
        const int index = (writePos + bufferSize - wrapOffset - samplesBack) & mask;
        return buffer[static_cast<size_t> (index)];
    }

    static constexpr int   extraTaps      = 4;   // the two taps beyond the whole position, plus headroom
    static constexpr int   smallestBuffer = 4;
    static constexpr int   growthFactor   = 2;   // a power of two: wrapping is a mask, not a division
    static constexpr int   wrapOffset     = 1;
    static constexpr int   nextSample     = 1;
    static constexpr float silence        = 0.0f;
    static constexpr double step             = 1.0;   // node spacing
    static constexpr double twoSteps         = 2.0;
    static constexpr double outerDenominator = 6.0;   // the outer nodes' basis is divided by 3!
    static constexpr double innerDenominator = 2.0;

    std::vector<float> buffer;
    int bufferSize {};
    int mask {};
    int writePos {};
    double maxDelay {};
};
