/*
  ==============================================================================

    AudioHandler.cpp
    Created: 30 May 2026
    Author:  Antigravity

  ==============================================================================
*/

#include "AudioHandler.h"

namespace ChannelLayout
{
    constexpr int leftChannel      = 0;
    constexpr int rightChannel     = 1;
    constexpr int bufferStart      = 0;   // sample offset where a block begins
    constexpr int firstMidiChannel = 1;   // MIDI channels are 1..16; the per-channel arrays are 0..15
    constexpr int noOutputChannels = 0;
    constexpr int firstChannelBit  = 1;   // bit for channel index 0 in the no-SFZ channel mask
    constexpr int noChannels       = 0;

    // Room reserved off the audio thread, so a block's MIDI never allocates there. A dense block (a chord with
    // pedal and controllers on several parts) is a few dozen events; past this the buffer would grow once.
    constexpr int reservedEventsPerBlock   = 1024;
    constexpr int reservedEventsPerChannel = 256;
    constexpr int noSfzCheckIntervalMs     = 250;   // how soon "no SFZ on channel …" is reported (message thread)
    constexpr int noLoadsPending   = 0;
}

namespace ControllerNumber   // MIDI CC numbers this engine reacts to
{
    constexpr int volume     = 7;
    constexpr int pan        = 10;
    constexpr int expression = 11;
    constexpr int resonance  = 71;
    constexpr int cutoff     = 74;
    constexpr int distortion = 80;
    constexpr int reverb     = 91;
    constexpr int tremolo    = 92;
    constexpr int chorus     = 93;
    constexpr int delay      = 94;
    constexpr int randomMod  = 95;
}

namespace EffectSettings
{
    constexpr float ccMaximum             = 127.0f;   // a CC value of 127 is the full setting
    constexpr int   filterResonanceNeutralCC = 0;

    // Filter: a lowpass at the top of hearing with a Butterworth Q (1 / sqrt 2) is no filter at all.
    constexpr float openCutoffHz          = 20000.0f;
    constexpr float butterworthQ          = 0.707f;
    constexpr float lowestResonanceQ      = 0.5f;     // CC71 sweeps Q from here...
    constexpr float resonanceQRange       = 9.5f;     // ...up to 10
    constexpr float lowestCutoffHz        = 100.0f;   // CC74 sweeps 100 Hz * 200^value = 100 Hz..20 kHz
    constexpr float cutoffSweepRatio      = 200.0f;
    constexpr float lowestAudibleHz       = 20.0f;
    constexpr float highestAudibleHz      = 20000.0f;
    constexpr float filterOn              = 1.0f;
    constexpr float filterOff             = 0.0f;

    constexpr float reverbRoomSize        = 0.5f;
    constexpr float reverbDamping         = 0.5f;
    constexpr float reverbWetOff          = 0.0f;
    constexpr float reverbDryFull         = 1.0f;
    constexpr float reverbFullWidth       = 1.0f;
    constexpr float reverbMaxWet          = 0.85f;

    constexpr double longestDelaySeconds  = 0.5;
    constexpr double initialDelaySeconds  = 0.25;
    constexpr double shortestDelaySeconds = 0.05;     // CC94 sets the time from here...
    constexpr double delayTimeRangeSeconds = 0.35;    // ...up to 0.4 s
    constexpr float  delayMaxMix          = 0.8f;
    constexpr double delayGlidePerSample  = 0.5;      // the echo bends an octave at most and never plays backwards

    constexpr float tremoloRateHz         = 5.0f;
    constexpr float fullLevel             = 1.0f;
    constexpr float halfDepth             = 0.5f;     // the LFO swings between full level and 1 - depth

    constexpr float neutralDrive          = 1.0f;
    constexpr float driveRange            = 15.0f;    // CC80 sweeps the drive from 1 to 16
    constexpr float noModulation          = 0.0f;
    constexpr float unityGain             = 1.0f;
    constexpr float centrePan             = 0.5f;

    // Random modulation: a one-pole low-pass on white noise (keep this much of the last value).
    constexpr float randomModRetain       = 0.998f;
    constexpr float randomModNewNoise     = 0.002f;

    // Smoothing time constants (ms) - see the comment in prepare() for why each differs.
    constexpr double expressionSmoothingMs = 8.0;
    constexpr double toneSmoothingMs       = 20.0;
    constexpr double mixSmoothingMs        = 25.0;

    // Below these a stage is inaudible, so it is skipped (once its ramp has settled).
    constexpr float filterAudibleMix      = 0.0005f;
    constexpr float effectAudibleLevel    = 0.005f;
    constexpr float driveAudibleAbove     = 1.005f;
    constexpr float nearlyUnity           = 0.999f;
}

namespace VoiceAllocation
{
    constexpr int voicesPerChannel = 16;
    constexpr int  everyChannel   = 0;       // juce::Synthesiser::allNotesOff: 0 = all MIDI channels
    constexpr bool cutWithoutTail = false;   // allowTailOff = false: stop at once
}

//==============================================================================
// ChannelDSP

void ChannelDSP::prepare(double newSampleRate, int blockSize)
{
    using namespace EffectSettings;
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = newSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels      = DspConstants::stereoChannelCount;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(openCutoffHz);
    filter.setResonance(butterworthQ);

    reverbParams          = juce::Reverb::Parameters{};
    reverbParams.roomSize = reverbRoomSize;
    reverbParams.damping  = reverbDamping;
    reverbParams.wetLevel = reverbWetOff;
    reverbParams.dryLevel = reverbDryFull;
    reverbParams.width    = reverbFullWidth;
    reverb.setParameters(reverbParams);
    reverb.setSampleRate(newSampleRate);
    reverb.reset();

    chorus.prepare(newSampleRate);   // silent until CC93 opens it

    const int maxDelaySamples = static_cast<int>(newSampleRate * longestDelaySeconds);
    delayLineL.prepare(maxDelaySamples);
    delayLineR.prepare(maxDelaySamples);
    tremoloPhase       = {};
    tremoloPhaseInc    = juce::MathConstants<float>::twoPi * tremoloRateHz / static_cast<float>(newSampleRate);
    randomModSmoothed  = {};
    filterCutoffCC     = DspConstants::filterCutoffOpenCC;
    filterResonanceCC  = filterResonanceNeutralCC;

    // Smoothing time constants. These are chosen per parameter rather than shared,
    // because what counts as "fast enough to feel responsive" and "slow enough not
    // to click" differs by what the parameter does.
    //
    //   expression   a volume pedal has to feel immediate, and gain is the
    //                parameter least prone to artifacts, so it gets the fastest
    //   filter       fast enough to follow a sweep, slow enough that a jumped CC
    //                does not step the cutoff audibly
    //   drive        changes timbre rather than level; a slower ramp reads as
    //                intentional rather than as a glitch
    //   mixes/depths dry-to-wet transitions are the most exposed, so slowest
    expression          .setTimeConstant(expressionSmoothingMs, newSampleRate);
    filterCutoffLog2    .setTimeConstant(toneSmoothingMs, newSampleRate);
    filterResonance     .setTimeConstant(toneSmoothingMs, newSampleRate);
    filterMix           .setTimeConstant(toneSmoothingMs, newSampleRate);
    distortionDrive     .setTimeConstant(toneSmoothingMs, newSampleRate);
    distortionNormFactor.setTimeConstant(toneSmoothingMs, newSampleRate);
    tremoloDepth        .setTimeConstant(mixSmoothingMs, newSampleRate);
    randomModDepth      .setTimeConstant(mixSmoothingMs, newSampleRate);
    delayMix            .setTimeConstant(mixSmoothingMs, newSampleRate);

    // prepare() is not a parameter change, so these start where they belong
    // rather than ramping up from silence on the first block.
    expression          .snap(unityGain);
    filterCutoffLog2    .snap(std::log2(openCutoffHz));
    filterResonance     .snap(butterworthQ);
    filterMix           .snap(filterOff);
    distortionDrive     .snap(neutralDrive);
    distortionNormFactor.snap(unityGain / std::tanh(neutralDrive));
    tremoloDepth        .snap(noModulation);
    randomModDepth      .snap(noModulation);
    delayMix            .snap(noModulation);
    delayTimeSamples = delayTimeTarget = newSampleRate * initialDelaySeconds;

    for (auto& adaa : distortionAdaa)
        adaa.reset();
}

void ChannelDSP::updateCC(int ccNumber, int value)
{
    using namespace EffectSettings;
    const float norm = value / ccMaximum;

    // Every case below sets a *target*. Nothing here touches a value the audio
    // loop reads directly; process() walks each parameter toward its target one
    // sample at a time.
    switch (ccNumber)
    {
        case ControllerNumber::expression:
            expression.target = norm;
            break;

        case ControllerNumber::resonance: // → filter Q
            filterResonanceCC = value;
            filterResonance.target = lowestResonanceQ + norm * resonanceQRange;
            break;

        case ControllerNumber::cutoff: // brightness → LP filter cutoff (100 Hz – 20 kHz, log)
        {
            filterCutoffCC = value;
            const float hz = lowestCutoffHz * std::pow(cutoffSweepRatio, norm);
            filterCutoffLog2.target = std::log2(juce::jlimit(lowestAudibleHz, highestAudibleHz, hz));
            break;
        }

        case ControllerNumber::distortion: // drive
        {
            const float drive = neutralDrive + norm * driveRange;
            distortionDrive.target = drive;

            // The normalisation factor is smoothed alongside the drive rather than
            // recomputed from the smoothed drive per sample. Both are monotonic in
            // the same CC, so the two ramps stay consistent with each other, and
            // this avoids a second tanh and a divide in the inner loop.
            distortionNormFactor.target = unityGain / std::tanh(drive);
            break;
        }

        case ControllerNumber::reverb: // wet amount
            reverbMix = norm * reverbMaxWet;
            reverbParams.wetLevel = reverbMix;
            reverb.setParameters(reverbParams);
            break;

        case ControllerNumber::tremolo: // depth
            tremoloDepth.target = norm;
            break;

        case ControllerNumber::chorus: // mix
            chorus.setMix(norm);   // glides there inside the chorus
            break;

        case ControllerNumber::delay: // mix + time
            delayMix.target = norm * delayMaxMix;
            // The time glides there (process) and the lines are read between samples: no jump of the read head.
            delayTimeTarget = sampleRate * (shortestDelaySeconds + norm * delayTimeRangeSeconds);
            break;

        case ControllerNumber::randomMod: // depth
            randomModDepth.target = norm;
            break;

        // CC1 (vibrato): passed through to sfzero — works if the SFZ file defines modwheel
        // CC73/75/72 (attack/decay/release): sfzero reads these from the SFZ region at load
        //   time, not from CC, so they have no effect here
        // CC76 (filterTrack): key-tracking concept doesn't map to post-render DSP
        default: break;
    }
}

void ChannelDSP::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // tempBuffer is always prepared with 2 channels
    using namespace EffectSettings;
    using namespace ChannelLayout;
    float* left  = buffer.getWritePointer(leftChannel);
    float* right = buffer.getWritePointer(rightChannel);

    // --- LP filter (CC74 brightness + CC71 resonance) ---
    // Runs while the CCs are away from neutral OR while a ramp is still settling;
    // stopping the moment the CC returns to neutral would cut the ramp off and
    // reintroduce the step this smoothing exists to remove.
    filterMix.target = (filterCutoffCC < DspConstants::filterCutoffOpenCC || filterResonanceCC > filterResonanceNeutralCC) ? filterOn : filterOff;

    if (filterMix.isActive(filterAudibleMix))
    {
        // Per-sample rather than filter.process() on a block. The coefficients are
        // recomputed every sample so that a moving cutoff is a continuous sweep
        // rather than one step per block. A TPT filter is the structure that makes
        // this safe: its state is a pair of integrator outputs in signal units,
        // which stay meaningful when the coefficients change underneath them. A
        // biquad's state is delayed signal that belongs to the coefficients that
        // produced it, and modulating one that way diverges.
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            filter.setCutoffFrequency(std::exp2(filterCutoffLog2.nextValue()));
            filter.setResonance(filterResonance.nextValue());

            const float dryL = left[sampleIndex];
            const float dryR = right[sampleIndex];
            const float wetL = filter.processSample(leftChannel, dryL);
            const float wetR = filter.processSample(rightChannel, dryR);
            const float mix  = filterMix.nextValue();

            left[sampleIndex]  = dryL + mix * (wetL - dryL);
            right[sampleIndex] = dryR + mix * (wetR - dryR);
        }

        filter.snapToZero();
    }
    else
    {
        // Fully bypassed. Drop the filter state rather than letting it go stale;
        // the crossfade above is what makes starting cold inaudible next time.
        filter.reset();

        // Keep the ramps tracking their targets even while bypassed, so re-engaging
        // starts from where the parameter actually is.
        filterCutoffLog2.current = filterCutoffLog2.target;
        filterResonance.current  = filterResonance.target;
    }

    // --- Distortion (CC80), antialiased ---
    if (distortionDrive.isActive(driveAudibleAbove) || distortionNormFactor.current < nearlyUnity)
    {
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            const float drive = distortionDrive.nextValue();
            const float norm  = distortionNormFactor.nextValue();

            left[sampleIndex]  = distortionAdaa[leftChannel].process(left[sampleIndex]  * drive) * norm;
            right[sampleIndex] = distortionAdaa[rightChannel].process(right[sampleIndex] * drive) * norm;
        }
    }
    else
    {
        // Bypassed: drop the ADAA history so the next engagement primes from its
        // own first sample rather than differencing against a stale one.
        for (auto& adaa : distortionAdaa)
            adaa.reset();

        distortionDrive.current      = distortionDrive.target;
        distortionNormFactor.current = distortionNormFactor.target;
    }

    // --- Chorus (CC93) ---
    // Runs while heard or still gliding to silence (stopping at the threshold would cut the ramp: a click).
    if (chorus.isActive())
        chorus.process(left, right, numSamples);

    // --- Reverb (CC91) ---
    if (reverbMix > effectAudibleLevel)
        reverb.processStereo(left, right, numSamples);

    // --- Tremolo (CC92) — 5 Hz amplitude LFO ---
    if (tremoloDepth.isActive(effectAudibleLevel))
    {
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            const float depth = tremoloDepth.nextValue();
            const float mod = fullLevel - depth * halfDepth * (fullLevel + std::sin(tremoloPhase));
            left[sampleIndex]  *= mod;
            right[sampleIndex] *= mod;
            tremoloPhase += tremoloPhaseInc;
            if (tremoloPhase >= juce::MathConstants<float>::twoPi)
                tremoloPhase -= juce::MathConstants<float>::twoPi;
        }
    }
    else
    {
        tremoloDepth.current = tremoloDepth.target;
    }

    // --- Delay (CC94) ---
    // The delay line is written unconditionally. Previously the writes lived
    // inside the "is the delay audible" test, so turning CC94 down froze the
    // buffer holding whatever was playing at that moment, and turning it back up
    // replayed up to half a second of stale audio at full mix. A delay line is a
    // record of recent history; it has to stay a record whether or not anyone is
    // currently listening to it.
    if (delayLineL.isPrepared())
    {
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            delayLineL.write(left[sampleIndex]);
            delayLineR.write(right[sampleIndex]);

            // `time` samples behind the sample just written; between samples when the time is gliding.
            delayTimeSamples += juce::jlimit(-delayGlidePerSample, delayGlidePerSample, delayTimeTarget - delayTimeSamples);
            const float delayedL = delayLineL.read(delayTimeSamples);
            const float delayedR = delayLineR.read(delayTimeSamples);

            const float mix = delayMix.nextValue();
            left[sampleIndex]  += mix * delayedL;
            right[sampleIndex] += mix * delayedR;
        }
    }

    // --- Random Mod (CC95) — smoothed noise amplitude flutter ---
    if (randomModDepth.isActive(effectAudibleLevel))
    {
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            randomModSmoothed = randomModRetain * randomModSmoothed + randomModNewNoise * rng.nextFloat();
            const float mod = fullLevel - randomModDepth.nextValue() * randomModSmoothed;
            left[sampleIndex]  *= mod;
            right[sampleIndex] *= mod;
        }
    }
    else
    {
        randomModDepth.current = randomModDepth.target;
    }

    // --- Expression (CC11) ---
    // applyGain() with a single value would step once per block; the per-sample
    // ramp is the whole point. Runs whenever the gain is below unity or still on
    // its way back to it.
    if (expression.current < nearlyUnity || expression.target < nearlyUnity)
    {
        for (int sampleIndex {}; sampleIndex < numSamples; ++sampleIndex)
        {
            const float gain = expression.nextValue();
            left[sampleIndex]  *= gain;
            right[sampleIndex] *= gain;
        }
    }
}

//==============================================================================
// AudioHandler

AudioHandler::AudioHandler(MidiHandler& mh) : midiHandler(mh)
{
    formatManager.registerBasicFormats();
    for (int channelIndex {}; channelIndex < DspConstants::midiChannelCount; ++channelIndex)
    {
        for (int voiceIndex {}; voiceIndex < VoiceAllocation::voicesPerChannel; ++voiceIndex)
            sfzSynths[channelIndex].addVoice(new sfzero::Voice());
        channelGains[channelIndex] = EffectSettings::unityGain;
        channelPans[channelIndex]  = EffectSettings::centrePan;
        channelHasSfz[channelIndex].store(false, std::memory_order_relaxed);
    }
    reserveMidiStorage();
    startTimer(ChannelLayout::noSfzCheckIntervalMs);
}

void AudioHandler::reserveMidiStorage()
{
    incomingMidi.clear();
    incomingMidi.ensureSize(MidiChannelSplitter::bytesPerShortEvent() * static_cast<size_t>(ChannelLayout::reservedEventsPerBlock));
    channelMidi.prepare(ChannelLayout::reservedEventsPerChannel);
}

void AudioHandler::timerCallback()
{
    reportChannelsWithoutSfz();
}

void AudioHandler::reportChannelsWithoutSfz()
{
    const int mask = noSfzChannelMask.exchange(ChannelLayout::noChannels);
    if (onNoSfzForChannels && mask != ChannelLayout::noChannels)
        onNoSfzForChannels(mask);
}

AudioHandler::~AudioHandler()
{
    constexpr bool letRunningLoadsFinish = false;   // a load cannot be interrupted part way
    constexpr int  waitForeverMs         = -1;
    sfzLoaders.removeAllJobs(letRunningLoadsFinish, waitForeverMs);
    aliveToken.reset();
    stopTimer();
    midiHandler.setInternalSynthListening(false);   // nothing will drain the queue any more
}

void AudioHandler::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    // From now on this synth drains MidiHandler's queue; anything queued before (an External MIDI
    // session) is dropped instead of firing all at once in the first block.
    midiHandler.setInternalSynthListening(true);

    currentSampleRate = device->getCurrentSampleRate();
    const int bufferSize = device->getCurrentBufferSizeSamples();
    tempBuffer.setSize(DspConstants::stereoChannelCount, bufferSize, false, true);
    reserveMidiStorage();

    for (int channelIndex {}; channelIndex < DspConstants::midiChannelCount; ++channelIndex)
    {
        // Note-offs and pedal-ups sent while the synth was stopped were never queued for it, so a note
        // held across the stop would hang now: start from silence (this also lifts the sustain pedal).
        sfzSynths[channelIndex].allNotesOff(VoiceAllocation::everyChannel, VoiceAllocation::cutWithoutTail);
        sfzSynths[channelIndex].setCurrentPlaybackSampleRate(currentSampleRate);
        channelDSP[channelIndex].prepare(currentSampleRate, bufferSize);
    }
}

void AudioHandler::audioDeviceStopped()
{
    midiHandler.setInternalSynthListening(false);   // no audio callback will drain the queue now
}

void AudioHandler::audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                                     float* const* outputChannelData, int numOutputChannels,
                                                     int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);
    // Filters and level followers fed exact silence decay into denormals, which x86 computes far slower
    // (measured: the chorus alone went from 111 to 152 ns per stereo sample on silence). JUCE's device
    // layer does not flush them, so the callback does.
    const juce::ScopedNoDenormals noDenormals;

    using namespace ChannelLayout;
    for (int outputIndex {}; outputIndex < numOutputChannels; ++outputIndex)
        if (outputChannelData[outputIndex] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[outputIndex], numSamples);

    if (numOutputChannels == noOutputChannels)
        return;

    incomingMidi.clear();   // keeps its reserved storage: no allocation here
    midiHandler.getNextMidiBlock(incomingMidi, bufferStart, numSamples);

    // Update per-channel state from incoming CCs before rendering;
    // also detect noteOns on channels with no SFZ loaded.
    for (const auto metadata : incomingMidi)
    {
        if (metadata.numBytes > MidiChannelSplitter::largestShortMessage)
            continue;   // SysEx is not for the synths; copying it into a MidiMessage would allocate
        const auto msg = metadata.getMessage();
        if (msg.isController())
        {
            const int channelIndex = msg.getChannel() - firstMidiChannel;
            const int controller   = msg.getControllerNumber();
            const int value        = msg.getControllerValue();
            if (channelIndex >= int {} && channelIndex < DspConstants::midiChannelCount)
            {
                if      (controller == ControllerNumber::volume) channelGains[channelIndex] = value / EffectSettings::ccMaximum;
                else if (controller == ControllerNumber::pan)    channelPans[channelIndex]  = value / EffectSettings::ccMaximum;
                else                                             channelDSP[channelIndex].updateCC(controller, value);
            }
        }
        else if (msg.isNoteOn() && onNoSfzForChannels)
        {
            const int channelIndex = msg.getChannel() - firstMidiChannel;
            if (channelIndex >= int {} && channelIndex < DspConstants::midiChannelCount
                && !channelHasSfz[channelIndex].load(std::memory_order_relaxed))
            {
                // Only an atomic here: posting a message from the audio thread allocates (timerCallback reports it).
                noSfzChannelMask.fetch_or(firstChannelBit << channelIndex, std::memory_order_relaxed);
            }
        }
    }

    // tempBuffer is always 2-channel; reuse its allocation if large enough
    tempBuffer.setSize(DspConstants::stereoChannelCount, numSamples, false, false, true);

    juce::AudioBuffer<float> mainBuffer(outputChannelData, numOutputChannels, numSamples);

    channelMidi.split(incomingMidi);   // one pass, into buffers reserved in audioDeviceAboutToStart

    for (int channel = firstMidiChannel; channel <= DspConstants::midiChannelCount; ++channel)
    {
        if (!channelHasSfz[channel - firstMidiChannel].load(std::memory_order_acquire))
            continue;

        tempBuffer.clear();
        sfzSynths[channel - firstMidiChannel].renderNextBlock(tempBuffer, channelMidi.channel(channel), bufferStart, numSamples);

        channelDSP[channel - firstMidiChannel].process(tempBuffer, numSamples);

        const float gain = channelGains[channel - firstMidiChannel];
        const float pan  = channelPans[channel - firstMidiChannel];

        if (numOutputChannels >= DspConstants::stereoChannelCount)
        {
            const float leftGain  = gain * std::cos(pan * juce::MathConstants<float>::halfPi);
            const float rightGain = gain * std::sin(pan * juce::MathConstants<float>::halfPi);
            mainBuffer.addFrom(leftChannel, bufferStart, tempBuffer, leftChannel, bufferStart, numSamples, leftGain);
            mainBuffer.addFrom(rightChannel, bufferStart, tempBuffer, rightChannel, bufferStart, numSamples, rightGain);
        }
        else
        {
            mainBuffer.addFrom(leftChannel, bufferStart, tempBuffer, leftChannel, bufferStart, numSamples, gain);
        }
    }
}

void AudioHandler::loadSfz(const juce::File& sfzFile, int midiChannel)
{
    using namespace ChannelLayout;
    if (midiChannel < firstMidiChannel || midiChannel > DspConstants::midiChannelCount)
        return;

    if (!sfzFile.existsAsFile())
    {
        // No SFZ mapped for this channel — silence it so the previous instrument
        // doesn't keep playing when the new style has no mapping.
        if (channelHasSfz[midiChannel - firstMidiChannel].load(std::memory_order_relaxed)
            || loadedSfzPath[midiChannel - firstMidiChannel].isNotEmpty())
        {
            channelHasSfz[midiChannel - firstMidiChannel].store(false, std::memory_order_release);
            sfzSynths[midiChannel - firstMidiChannel].clearSounds();
            loadedSfzPath[midiChannel - firstMidiChannel] = juce::String();
        }
        return;
    }

    if (sfzFile.getFullPathName() == loadedSfzPath[midiChannel - firstMidiChannel])
        return;
    loadedSfzPath[midiChannel - firstMidiChannel] = sfzFile.getFullPathName();

    ++pendingLoads;
    if (onSfzLoadStart)
        onSfzLoadStart();

    sfzLoaders.addJob([this, sfzFile, midiChannel, alive = std::weak_ptr<bool>(aliveToken)]()
    {
        auto* sound = new sfzero::Sound(sfzFile);
        sound->loadRegions();
        sound->loadSamples(&formatManager);
        sfzSynths[midiChannel - firstMidiChannel].clearSounds();
        sfzSynths[midiChannel - firstMidiChannel].addSound(sound);
        channelHasSfz[midiChannel - firstMidiChannel].store(true, std::memory_order_release);

        if (--pendingLoads == noLoadsPending)
            juce::MessageManager::callAsync([this, alive]() {
                if (alive.expired()) return;   // the handler went before this message arrived
                if (onSfzLoadComplete) onSfzLoadComplete();
            });
    });
}
