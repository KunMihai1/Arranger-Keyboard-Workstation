#pragma once

#include "FractionalDelayLine.h"
#include <cstdint>

/*  CC93's chorus: a bucket-brigade (BBD) chorus, ported from the owner's va-effects repo (BbdChorus, measured there),
    replacing juce::dsp::Chorus.

    A BBD is an analog shift register clocked between ~10 and 200 kHz, and delay = stages / (2 * clock): a longer
    delay means a slower clock and so less treble. The delayed voice therefore darkens as the LFO lengthens it, in time
    with the sweep; that, the 2:1 / 1:2 compander around the line and the line's own noise floor are what make it sound
    like the hardware rather than a delay with an LFO. Parameters are the Juno chorus's (MN3009, 256 stages; 0.5 Hz,
    5 ms +- 2 ms), with the two sides a quarter cycle apart for width.

    Two changes from va-effects, both for 16 channels' worth of CPU (measured: 191 ns per stereo sample before):
    the moving reconstruction filter's coefficients are updated every 16 samples instead of every sample (the LFO is
    0.5 Hz), and the compander's powers are a square root and a plain ratio (they were pow (x, -0.5) and pow (x, 1)).

    prepare() allocates; process() does not. */
class BbdChorus
{
public:
    void prepare (double sampleRate);
    void reset();

    /** The wet share, 0..1 (CC93); it glides there, so a jump does not click. */
    void setMix (float newMix);
    /** True while the chorus is heard or its mix is still gliding to silence (skipping it then would click). */
    bool isActive() const;

    void process (float* left, float* right, int numSamples);

    /** The model's central arithmetic, exposed for the tests: the clock a 256-stage line needs for a delay, and the
        bandwidth that leaves (the lower of the fixed anti-alias corner and 0.45 x the clock). */
    static double clockHzForDelayMs (double delayMs);
    static double bandwidthHzForDelayMs (double delayMs);

private:
    /** Topology-preserving state-variable lowpass: its state stays valid while the cutoff moves. */
    struct Lowpass
    {
        float damping {}, gain1 {}, gain2 {}, gain3 {};
        float state1 {}, state2 {};
        void setCutoff (double hz, double sampleRate);
        float process (float input);
        void reset() { state1 = state2 = {}; }
    };

    /** One-pole level follower with a fast attack and slow release (the compander's detectors). */
    struct Detector
    {
        float level {}, attack {}, release {};
        void setTimes (double attackMs, double releaseMs, double sampleRate);
        float process (float input);
    };

    struct Side
    {
        FractionalDelayLine line;
        Lowpass antiAlias, reconstruction;
        Detector compressorLevel, expanderLevel;
        std::uint32_t noiseState {};
        double phaseOffset {};
    };

    float processSide (Side& side, float input, double lfoPhase, bool updateFilter, float wet);
    float nextNoise (std::uint32_t& state) const;

    static constexpr int sideCount = 2;   // left, right
    Side sides[sideCount];
    double sampleRate {};
    double lfoPhase {};
    double lfoIncrement {};
    float noiseAmplitude {};
    float mixCurrent {}, mixTarget {}, mixCoefficient {};
    int samplesUntilFilterUpdate {};
    int warmUpSamples {};   ///< after re-opening: the line fills and the compander settles before the mix moves
};
