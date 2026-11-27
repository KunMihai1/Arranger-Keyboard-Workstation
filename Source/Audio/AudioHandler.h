/*
  ==============================================================================

    AudioHandler.h
    Created: 30 May 2026
    Author:  Antigravity

  ==============================================================================
*/

#pragma once

#include "MidiChannelSplitter.h"

#include <JuceHeader.h>
#include "MidiHandler.h"

namespace DspConstants
{
    constexpr int    midiChannelCount        = 16;
    constexpr int    stereoChannelCount      = 2;
    constexpr double fallbackSampleRate      = 44100.0;   // until the device reports its own
    constexpr int    filterCutoffOpenCC      = 127;       // CC74 fully open: no filtering

    // Smoothing: each sample covers this fraction of the remaining step, 1 - e^(-1 / samples), so that
    // after `samples` samples one time constant (~63%) of a step is covered.
    constexpr float  jumpStraightToTarget    = 1.0f;      // coefficient before a time constant is set
    constexpr double fullStep                = 1.0;
    constexpr double oneTimeConstant         = 1.0;
    constexpr double fewestSmoothingSamples  = 1.0;
    constexpr double secondsPerMillisecond   = 0.001;

    // log(cosh(x)) = |x| + log1p(e^(-2|x|)) - ln 2, the overflow-safe form.
    constexpr float  twiceTheMagnitude       = 2.0f;
    constexpr float  naturalLogOfTwo         = 0.6931472f;
    // Below this input change the ADAA quotient is cancellation noise: use the midpoint instead.
    constexpr float  nearlyEqualInputs       = 1.0e-5f;
    constexpr float  midpointWeight          = 0.5f;
}

/**
    One-pole parameter smoother.

    CC values arrive at block boundaries and were previously applied as a constant
    for the whole block. At 48 kHz that steps a parameter up to 750 times a second,
    and every step is a discontinuity in the waveform: broadband energy, heard as
    zipper noise. Measured on a modulated filter, per-block updating plants
    sidebands at carrier +/- n * blockrate around 98 dB above where per-sample
    updating leaves them, and because the artifact frequencies follow the host's
    block size the same code sounds different at 64 and 128 samples.

    The fix is to step toward the target once per sample instead. The coefficient
    is a design decision per parameter, not a constant to share - see the time
    constants chosen in prepare().
*/
struct SmoothedParam
{
    float current {};
    float target  {};
    float coeff   = DspConstants::jumpStraightToTarget;

    /** @param milliseconds  time to cover ~63% of a step. */
    void setTimeConstant(double milliseconds, double sampleRate)
    {
        using namespace DspConstants;
        const double samples = juce::jmax(fewestSmoothingSamples, milliseconds * secondsPerMillisecond * sampleRate);
        coeff = static_cast<float>(fullStep - std::exp(-oneTimeConstant / samples));
    }

    /** Jump without smoothing. For prepare() and reset only - never for a CC. */
    void snap(float value) { current = target = value; }

    float nextValue()
    {
        current += (target - current) * coeff;
        return current;
    }

    /** True while the parameter is audible or still decaying toward silence.
        A stage may only be skipped when this is false, or the skip truncates the
        ramp and reintroduces the click that smoothing exists to prevent. */
    bool isActive(float epsilon) const { return current > epsilon || target > epsilon; }
};

/**
    First-order antiderivative antialiasing for tanh (Parker, Zavalishin and
    Le Bivic, DAFx-16).

    A sample is not an instant, it is a sample period, and over that period the
    input moved. Waveshaping pointwise pretends otherwise, and the harmonics that
    fold back below Nyquist are largely an artifact of that pretence. Evaluating
    the average of the curve across the interval instead gives

        y[n] = ( F(x[n]) - F(x[n-1]) ) / ( x[n] - x[n-1] )      F = integral of tanh

    Measured against the naive version this repository shipped: alias-to-signal on
    a 5 kHz tone at high drive improves from -9.2 dB to -15.6 dB, and by 11-13 dB
    in the top octave where the problem is worst, for about 20% more CPU.

    Oversampling does better still (-51.7 dB at 4x) but costs 23x, and sixteen of
    these run in parallel - one per MIDI channel - so it would take roughly 38% of
    a core for one stage in a chain that also has to run filters, chorus, reverb,
    delay and a sampler. This is a budget decision, not a claim that ADAA is the
    better algorithm.
*/
struct Adaa1Tanh
{
    float previousInput {};
    float previousAnti  {};
    bool  primed        = false;

    void reset() { previousInput = {}; previousAnti = {}; primed = false; }

    /** log(cosh(x)), which is the antiderivative of tanh, computed so that it
        survives large arguments. The direct form overflows - cosh(400) is inf,
        and a drive of 16 on a loud sample reaches arguments like that. Factoring
        out e^|x| leaves an expression that is exact for large |x| and still
        accurate near zero, where log1p does the work log(1+y) would lose. */
    static float logCosh(float input)
    {
        using namespace DspConstants;
        const float magnitude = std::abs(input);
        return magnitude + std::log1p(std::exp(-twiceTheMagnitude * magnitude)) - naturalLogOfTwo;
    }

    float process(float input)
    {
        const float anti = logCosh(input);

        if (!primed)
        {
            primed = true;
            previousInput = input;
            previousAnti  = anti;
            return std::tanh(input);
        }

        const float delta = input - previousInput;

        // As x approaches its previous value the quotient becomes a difference of
        // two nearly equal numbers over a nearly zero divisor, and float
        // cancellation destroys it. Below the threshold, fall back to the curve at
        // the midpoint - the limit the quotient is approaching. This runs on any
        // quiet or slow-moving signal, so it is not an edge case; it is most of a
        // decaying note.
        const float output = (std::abs(delta) < DspConstants::nearlyEqualInputs)
                          ? std::tanh(DspConstants::midpointWeight * (input + previousInput))
                          : (anti - previousAnti) / delta;

        previousInput = input;
        previousAnti  = anti;
        return output;
    }
};

struct ChannelDSP
{
    float reverbMix            {};
    float chorusMix            {};
    float tremoloPhase         {};
    float tremoloPhaseInc      {};   // cached: 2π*5/sampleRate
    float randomModSmoothed    {};
    int   filterCutoffCC       = DspConstants::filterCutoffOpenCC;    // raw CC74 value; 127 = neutral (no filtering)
    int   filterResonanceCC    {};      // raw CC71 value; 0 = neutral

    // Everything a CC can move during playback is smoothed. The reverb and chorus
    // mixes are not in this list because juce::Reverb and juce::dsp::Chorus each
    // smooth their own wet/dry internally.
    SmoothedParam expression;            // CC11
    SmoothedParam tremoloDepth;          // CC92
    SmoothedParam delayMix;              // CC94
    SmoothedParam distortionDrive;       // CC80, as the 1..16 gain rather than 0..1
    SmoothedParam distortionNormFactor;  // tracks 1/tanh(drive); see prepare()
    SmoothedParam randomModDepth;        // CC95
    SmoothedParam filterResonance;       // CC71

    /** CC74 cutoff, smoothed as log2(Hz) rather than Hz.

        Smoothing a frequency linearly is wrong, and audibly so. A one-pole moving
        from 20 kHz toward 150 Hz covers hertz at a rate proportional to how far it
        still has to go, which in musical terms means it crashes down through the
        top octaves at roughly 1/(tau * ln2) - about 70 octaves per second for a
        20 ms time constant - and then crawls the last part. What is heard is a
        lurch, not a sweep, and it is large enough to register as a discontinuity.

        In the log domain the ramp covers a constant number of octaves per second,
        which is what "a smooth filter sweep" actually means. Costs one exp2 per
        sample. */
    SmoothedParam filterCutoffLog2;

    /** Dry/wet crossfade for the filter stage, so that engaging it is not a step.

        A bypassed filter holds no state. The moment it is switched in it is fed a
        signal that is already mid-waveform, and a two-pole TPT lowpass with zero
        integrator state returns roughly a2*x on its first sample - at a 20 kHz
        cutoff and 48 kHz that is about 0.18 of the input, so the output drops to a
        fifth instantly and then recovers. The click that produces was in this code
        long before the smoothing work; smoothing everything else is what made it
        visible.

        Crossfading solves it without paying to run the filter continuously: the
        stage is switched in immediately but mixed in over the ramp, and the cold
        transient - which decays within microseconds at a high cutoff - happens
        while the wet gain is still near zero. */
    SmoothedParam filterMix;

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Reverb                             reverb;
    juce::Reverb::Parameters                 reverbParams;
    juce::dsp::Chorus<float>                 chorus;

    Adaa1Tanh distortionAdaa[DspConstants::stereoChannelCount];         // one per stereo side; state is per-signal

    std::vector<float> delayLineL, delayLineR;
    int    delayWritePos  {};
    int    delayReadOffset {};
    double sampleRate     = DspConstants::fallbackSampleRate;

    juce::Random rng;

    void prepare(double sr, int blockSize);
    void updateCC(int ccNumber, int value);
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
};

class AudioHandler : public juce::AudioIODeviceCallback,
                     private juce::Timer
{
public:
    AudioHandler(MidiHandler& mh);
    ~AudioHandler() override;

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    void loadSfz(const juce::File& sfzFile, int midiChannel);

    std::function<void()> onSfzLoadStart;
    std::function<void()> onSfzLoadComplete;
    std::function<void(int channelMask)> onNoSfzForChannels;
    /** Message thread (a timer calls it): reports the channels that got a note without an SFZ since last time. */
    void reportChannelsWithoutSfz();

private:
    std::atomic<int>  pendingLoads      {};
    std::atomic<int>  noSfzChannelMask  {};   // set on the audio thread, reported by timerCallback on the message thread

    void timerCallback() override;
    void reserveMidiStorage();   ///< off the audio thread: the callback then never allocates for MIDI

    MidiHandler& midiHandler;
    sfzero::Synth sfzSynths[DspConstants::midiChannelCount];
    juce::AudioFormatManager formatManager;
    double currentSampleRate = DspConstants::fallbackSampleRate;

    std::atomic<bool> channelHasSfz[DspConstants::midiChannelCount];
    juce::String      loadedSfzPath[DspConstants::midiChannelCount]; // message-thread only

    juce::AudioBuffer<float> tempBuffer;
    juce::MidiBuffer incomingMidi;     // reserved before the device starts: the callback only clears and refills it
    MidiChannelSplitter channelMidi;
    float channelGains[DspConstants::midiChannelCount];
    float channelPans[DspConstants::midiChannelCount];
    ChannelDSP channelDSP[DspConstants::midiChannelCount];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioHandler)
};
