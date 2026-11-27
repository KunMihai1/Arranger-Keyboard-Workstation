#include "BbdChorus.h"

#include <algorithm>
#include <cmath>

namespace
{
    namespace Juno   // va-effects BbdChorus defaults: the Juno chorus (MN3009)
    {
        constexpr double stages          = 256.0;
        constexpr double rateHz          = 0.5;
        constexpr double centreDelayMs   = 5.0;
        constexpr double depthMs         = 2.0;
        constexpr double longestDelayMs  = 60.0;
        constexpr double antiAliasHz     = 12000.0;
        constexpr double noiseDb         = -78.0;   // measured MN-series noise floors sit near here
        constexpr double sideSpread      = 0.25;    // the right side's LFO a quarter cycle on: width
    }

    constexpr double pi                   = 3.14159265358979323846;
    constexpr double twoPi                = 6.28318530717958647692;
    constexpr double clockPhases          = 2.0;     // a two-phase clock: one stage per half cycle
    constexpr double guardBand            = 0.45;    // a real reconstruction filter sits below half the clock
    constexpr double butterworthQ         = 0.7071;
    constexpr double millisecondsPerSecond = 1000.0;
    constexpr double secondsPerMillisecond = 0.001;
    constexpr double shortestSeconds      = 1.0e-6;
    constexpr double lowestCutoffHz       = 1.0;
    constexpr double highestCutoffShare   = 0.49;    // of the sample rate
    constexpr double decibelsPerDecade    = 20.0;
    constexpr double decade               = 10.0;
    constexpr double fullCycle            = 1.0;
    constexpr double extraLineSamples     = 2.0;
    constexpr int    filterUpdateInterval = 16;      // samples between moving-filter coefficient updates

    constexpr double compressorAttackMs   = 1.0;
    constexpr double compressorReleaseMs  = 100.0;
    constexpr float  compandReference     = 0.1f;    // ~ -20 dBFS, the rotation point
    constexpr float  compandFloor         = 1.0e-4f; // -80 dBFS: silence is a defined gain, not a gate

    constexpr double mixSmoothingMs       = 25.0;    // as ChannelDSP's other mixes
    constexpr float  audibleMix           = 0.005f;
    constexpr float  unity                = 1.0f;
    constexpr double oneSample            = 1.0;
    constexpr double warmUpMs             = 12.0;    // longer than the longest sweep (7 ms) plus the detectors' attack

    // xorshift32 noise in [-1, 1), seeded per side so a run is reproducible.
    constexpr std::uint32_t noiseSeed      = 0x9e3779b9u;
    constexpr std::uint32_t noiseSeedStep  = 0x85ebca6bu;
    constexpr int           shiftLeftFirst = 13;
    constexpr int           shiftRight     = 17;
    constexpr int           shiftLeftLast  = 5;
    constexpr float         noiseScale     = 4.656612873e-10f;   // 2 / 2^32: a uint32 onto [0, 2)

    constexpr int leftSide  = 0;
    constexpr int rightSide = 1;

    double samplesFor (double milliseconds, double sampleRate) { return milliseconds * secondsPerMillisecond * sampleRate; }
}

//==============================================================================
void BbdChorus::Lowpass::setCutoff (double hz, double sampleRate)
{
    const double cutoff = std::clamp (hz, lowestCutoffHz, sampleRate * highestCutoffShare);
    const double warped = std::tan (pi * cutoff / sampleRate);
    const double inverseQ = unity / butterworthQ;
    const double denominator = unity / (unity + warped * (warped + inverseQ));
    damping = static_cast<float> (inverseQ);
    gain1 = static_cast<float> (denominator);
    gain2 = static_cast<float> (warped * denominator);
    gain3 = static_cast<float> (warped * warped * denominator);
}

float BbdChorus::Lowpass::process (float input)
{
    constexpr float trapezoid = 2.0f;   // the integrators' state update (Zavalishin's TPT)
    const float fromState2 = input - state2;
    const float bandpass = gain1 * state1 + gain2 * fromState2;
    const float lowpass  = state2 + gain2 * state1 + gain3 * fromState2;
    state1 = trapezoid * bandpass - state1;
    state2 = trapezoid * lowpass - state2;
    return lowpass;
}

void BbdChorus::Detector::setTimes (double attackMs, double releaseMs, double sampleRate)
{
    auto coefficient = [sampleRate] (double milliseconds)
    {
        const double samples = std::max (oneSample, samplesFor (milliseconds, sampleRate));
        return static_cast<float> (oneSample - std::exp (-oneSample / samples));
    };
    attack = coefficient (attackMs);
    release = coefficient (releaseMs);
}

float BbdChorus::Detector::process (float input)
{
    const float magnitude = std::abs (input);
    level += (magnitude - level) * (magnitude > level ? attack : release);
    return level;
}

//==============================================================================
double BbdChorus::clockHzForDelayMs (double delayMs)
{
    const double seconds = std::max (shortestSeconds, delayMs * secondsPerMillisecond);
    return Juno::stages / (clockPhases * seconds);
}

double BbdChorus::bandwidthHzForDelayMs (double delayMs)
{
    return std::min (Juno::antiAliasHz, clockHzForDelayMs (delayMs) * guardBand);
}

void BbdChorus::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    lfoIncrement = Juno::rateHz / sampleRate;
    noiseAmplitude = static_cast<float> (std::pow (decade, Juno::noiseDb / decibelsPerDecade));
    mixCoefficient = static_cast<float> (oneSample - std::exp (-oneSample / samplesFor (mixSmoothingMs, sampleRate)));

    const int longestDelay = static_cast<int> (std::ceil (samplesFor (Juno::longestDelayMs, sampleRate)) + extraLineSamples);
    for (int sideIndex {}; sideIndex < sideCount; ++sideIndex)
    {
        auto& side = sides[sideIndex];
        side.line.prepare (longestDelay);
        side.antiAlias.setCutoff (Juno::antiAliasHz, sampleRate);   // fixed: the device's input network
        side.compressorLevel.setTimes (compressorAttackMs, compressorReleaseMs, sampleRate);
        side.expanderLevel.setTimes (compressorAttackMs, compressorReleaseMs, sampleRate);
        side.phaseOffset = sideIndex == rightSide ? Juno::sideSpread : double {};
    }
    mixCurrent = mixTarget = {};
    reset();
}

void BbdChorus::reset()
{
    lfoPhase = {};
    samplesUntilFilterUpdate = {};
    for (int sideIndex {}; sideIndex < sideCount; ++sideIndex)
    {
        auto& side = sides[sideIndex];
        side.line.reset();
        side.antiAlias.reset();
        side.reconstruction.reset();
        side.compressorLevel.level = side.expanderLevel.level = compandReference;   // neutral gain, not a 31x cold boost
        side.noiseState = noiseSeed + static_cast<std::uint32_t> (sideIndex) * noiseSeedStep;
    }
}

void BbdChorus::setMix (float newMix)
{
    if (! isActive() && newMix > audibleMix)
    {
        reset();   // coming back from silence: start from a clean line, not from what played before
        warmUpSamples = static_cast<int> (samplesFor (warmUpMs, sampleRate));   // and stay dry while it fills
    }
    mixTarget = std::clamp (newMix, float {}, unity);
}

bool BbdChorus::isActive() const
{
    return mixTarget > audibleMix || mixCurrent > audibleMix || warmUpSamples > int {};
}

float BbdChorus::nextNoise (std::uint32_t& state) const
{
    state ^= state << shiftLeftFirst;
    state ^= state >> shiftRight;
    state ^= state << shiftLeftLast;
    return static_cast<float> (state) * noiseScale - unity;
}

float BbdChorus::processSide (Side& side, float input, double phase, bool updateFilter, float wet)
{
    // Into the device: compress 2:1, band-limit for the sampler the bucket brigade is.
    const float compressGain = unity / std::sqrt (std::max (side.compressorLevel.process (input), compandFloor) / compandReference);
    const float bandLimited = side.antiAlias.process (input * compressGain);

    const double voicePhase = phase + side.phaseOffset;
    const double sweep = std::sin (twoPi * (voicePhase - std::floor (voicePhase)));
    const double delaySamples = samplesFor (Juno::centreDelayMs + Juno::depthMs * sweep, sampleRate);

    side.line.write (bandLimited + noiseAmplitude * nextNoise (side.noiseState));   // the line's own noise floor
    const float delayed = side.line.read (delaySamples);

    // The reconstruction corner follows the delay: a longer delay is a slower clock, so less treble.
    if (updateFilter)
        side.reconstruction.setCutoff (bandwidthHzForDelayMs (delaySamples * millisecondsPerSecond / sampleRate), sampleRate);
    const float reconstructed = side.reconstruction.process (delayed);

    // Out of the device: expand 1:2. Its level left the compressor milliseconds ago, so the pair only cancels in
    // steady state - the compander's breathing, which is part of the sound.
    const float expandGain = std::max (side.expanderLevel.process (reconstructed), compandFloor) / compandReference;
    return (unity - wet) * input + wet * reconstructed * expandGain;
}

void BbdChorus::process (float* left, float* right, int numSamples)
{
    for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
    {
        if (warmUpSamples > int {})
            --warmUpSamples;   // the line fills while the output stays dry
        else
            mixCurrent += (mixTarget - mixCurrent) * mixCoefficient;
        const bool updateFilter = samplesUntilFilterUpdate-- <= int {};
        if (updateFilter)
            samplesUntilFilterUpdate = filterUpdateInterval - static_cast<int> (oneSample);

        left[sampleIndex]  = processSide (sides[leftSide], left[sampleIndex], lfoPhase, updateFilter, mixCurrent);
        right[sampleIndex] = processSide (sides[rightSide], right[sampleIndex], lfoPhase, updateFilter, mixCurrent);

        lfoPhase += lfoIncrement;
        if (lfoPhase >= fullCycle)
            lfoPhase -= fullCycle;
    }
}
