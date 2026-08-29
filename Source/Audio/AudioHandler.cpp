/*
  ==============================================================================

    AudioHandler.cpp
    Created: 30 May 2026
    Author:  Antigravity

  ==============================================================================
*/

#include "AudioHandler.h"

//==============================================================================
// ChannelDSP

void ChannelDSP::prepare(double sr, int blockSize)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels      = 2;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(20000.0f);
    filter.setResonance(0.707f);

    reverbParams          = juce::Reverb::Parameters{};
    reverbParams.roomSize = 0.5f;
    reverbParams.damping  = 0.5f;
    reverbParams.wetLevel = 0.0f;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width    = 1.0f;
    reverb.setParameters(reverbParams);
    reverb.setSampleRate(sr);
    reverb.reset();

    chorus.prepare(spec);
    chorus.setRate(1.5f);
    chorus.setDepth(0.3f);
    chorus.setCentreDelay(15.0f);
    chorus.setFeedback(0.1f);
    chorus.setMix(0.0f);

    const int maxDelaySamples = static_cast<int>(sr * 0.5);
    delayLineL.assign(maxDelaySamples, 0.0f);
    delayLineR.assign(maxDelaySamples, 0.0f);
    delayWritePos      = 0;
    delayReadOffset    = static_cast<int>(sr * 0.25);
    tremoloPhase       = 0.0f;
    tremoloPhaseInc    = juce::MathConstants<float>::twoPi * 5.0f / static_cast<float>(sr);
    randomModSmoothed  = 0.0f;
    filterCutoffCC     = 127;
    filterResonanceCC  = 0;

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
    expression          .setTimeConstant( 8.0, sr);
    filterCutoffLog2    .setTimeConstant(20.0, sr);
    filterResonance     .setTimeConstant(20.0, sr);
    filterMix           .setTimeConstant(20.0, sr);
    distortionDrive     .setTimeConstant(20.0, sr);
    distortionNormFactor.setTimeConstant(20.0, sr);
    tremoloDepth        .setTimeConstant(25.0, sr);
    randomModDepth      .setTimeConstant(25.0, sr);
    delayMix            .setTimeConstant(25.0, sr);

    // prepare() is not a parameter change, so these start where they belong
    // rather than ramping up from silence on the first block.
    expression          .snap(1.0f);
    filterCutoffLog2    .snap(std::log2(20000.0f));
    filterResonance     .snap(0.707f);
    filterMix           .snap(0.0f);
    distortionDrive     .snap(1.0f);
    distortionNormFactor.snap(1.0f / std::tanh(1.0f));
    tremoloDepth        .snap(0.0f);
    randomModDepth      .snap(0.0f);
    delayMix            .snap(0.0f);

    for (auto& adaa : distortionAdaa)
        adaa.reset();
}

void ChannelDSP::updateCC(int ccNumber, int value)
{
    const float norm = value / 127.0f;

    // Every case below sets a *target*. Nothing here touches a value the audio
    // loop reads directly; process() walks each parameter toward its target one
    // sample at a time.
    switch (ccNumber)
    {
        case 11: // expression
            expression.target = norm;
            break;

        case 71: // resonance → filter Q
            filterResonanceCC = value;
            filterResonance.target = 0.5f + norm * 9.5f;
            break;

        case 74: // brightness → LP filter cutoff (100 Hz – 20 kHz, log)
        {
            filterCutoffCC = value;
            const float hz = 100.0f * std::pow(200.0f, norm);
            filterCutoffLog2.target = std::log2(juce::jlimit(20.0f, 20000.0f, hz));
            break;
        }

        case 80: // distortion drive
        {
            const float drive = 1.0f + norm * 15.0f;
            distortionDrive.target = drive;

            // The normalisation factor is smoothed alongside the drive rather than
            // recomputed from the smoothed drive per sample. Both are monotonic in
            // the same CC, so the two ramps stay consistent with each other, and
            // this avoids a second tanh and a divide in the inner loop.
            distortionNormFactor.target = 1.0f / std::tanh(drive);
            break;
        }

        case 91: // reverb wet amount
            reverbMix = norm * 0.85f;
            reverbParams.wetLevel = reverbMix;
            reverb.setParameters(reverbParams);
            break;

        case 92: // tremolo depth
            tremoloDepth.target = norm;
            break;

        case 93: // chorus mix
            chorusMix = norm;
            chorus.setMix(norm);
            break;

        case 94: // delay — mix + time
            delayMix.target = norm * 0.8f;
            if (!delayLineL.empty())
            {
                // NOTE: the read offset still jumps. Changing a delay length
                // instantly moves the read head, which is a pitch discontinuity -
                // it clicks. Smoothing it properly requires fractional
                // interpolation on the read position, which is a separate change.
                int offset = static_cast<int>(sampleRate * (0.05 + norm * 0.35));
                delayReadOffset = juce::jlimit(1, static_cast<int>(delayLineL.size()) - 1, offset);
            }
            break;

        case 95: // random mod depth
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
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // --- LP filter (CC74 brightness + CC71 resonance) ---
    // Runs while the CCs are away from neutral OR while a ramp is still settling;
    // stopping the moment the CC returns to neutral would cut the ramp off and
    // reintroduce the step this smoothing exists to remove.
    filterMix.target = (filterCutoffCC < 127 || filterResonanceCC > 0) ? 1.0f : 0.0f;

    if (filterMix.isActive(0.0005f))
    {
        // Per-sample rather than filter.process() on a block. The coefficients are
        // recomputed every sample so that a moving cutoff is a continuous sweep
        // rather than one step per block. A TPT filter is the structure that makes
        // this safe: its state is a pair of integrator outputs in signal units,
        // which stay meaningful when the coefficients change underneath them. A
        // biquad's state is delayed signal that belongs to the coefficients that
        // produced it, and modulating one that way diverges.
        for (int i = 0; i < numSamples; ++i)
        {
            filter.setCutoffFrequency(std::exp2(filterCutoffLog2.nextValue()));
            filter.setResonance(filterResonance.nextValue());

            const float dryL = left[i];
            const float dryR = right[i];
            const float wetL = filter.processSample(0, dryL);
            const float wetR = filter.processSample(1, dryR);
            const float mix  = filterMix.nextValue();

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
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
    if (distortionDrive.isActive(1.005f) || distortionNormFactor.current < 0.999f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float drive = distortionDrive.nextValue();
            const float norm  = distortionNormFactor.nextValue();

            left[i]  = distortionAdaa[0].process(left[i]  * drive) * norm;
            right[i] = distortionAdaa[1].process(right[i] * drive) * norm;
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
    if (chorusMix > 0.005f)
    {
        juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(),
                                           (size_t)buffer.getNumChannels(),
                                           (size_t)numSamples);
        chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    // --- Reverb (CC91) ---
    if (reverbMix > 0.005f)
        reverb.processStereo(left, right, numSamples);

    // --- Tremolo (CC92) — 5 Hz amplitude LFO ---
    if (tremoloDepth.isActive(0.005f))
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float depth = tremoloDepth.nextValue();
            const float mod = 1.0f - depth * 0.5f * (1.0f + std::sin(tremoloPhase));
            left[i]  *= mod;
            right[i] *= mod;
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
    if (!delayLineL.empty())
    {
        const int delaySize = static_cast<int>(delayLineL.size());

        for (int i = 0; i < numSamples; ++i)
        {
            int readPos = delayWritePos - delayReadOffset;
            if (readPos < 0)
                readPos += delaySize;

            // Read before write. The two positions cannot coincide while the read
            // offset is at least one, but reading first is the habit that keeps a
            // feedback path computable if one is ever added here.
            const float delayedL = delayLineL[readPos];
            const float delayedR = delayLineR[readPos];

            delayLineL[delayWritePos] = left[i];
            delayLineR[delayWritePos] = right[i];

            const float mix = delayMix.nextValue();
            left[i]  += mix * delayedL;
            right[i] += mix * delayedR;

            // Compare-and-wrap rather than %. Modulo is an integer division, and
            // this loop now runs on every channel on every block rather than only
            // when the delay is audible, so the two divisions it removes pay for
            // most of the cost the unconditional write adds.
            if (++delayWritePos >= delaySize)
                delayWritePos = 0;
        }
    }

    // --- Random Mod (CC95) — smoothed noise amplitude flutter ---
    if (randomModDepth.isActive(0.005f))
    {
        for (int i = 0; i < numSamples; ++i)
        {
            randomModSmoothed = 0.998f * randomModSmoothed + 0.002f * rng.nextFloat();
            const float mod = 1.0f - randomModDepth.nextValue() * randomModSmoothed;
            left[i]  *= mod;
            right[i] *= mod;
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
    if (expression.current < 0.999f || expression.target < 0.999f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float gain = expression.nextValue();
            left[i]  *= gain;
            right[i] *= gain;
        }
    }
}

//==============================================================================
// AudioHandler

AudioHandler::AudioHandler(MidiHandler& mh) : midiHandler(mh)
{
    formatManager.registerBasicFormats();
    for (int i = 0; i < 16; ++i)
    {
        for (int v = 0; v < 16; ++v)
            sfzSynths[i].addVoice(new sfzero::Voice());
        channelGains[i] = 1.0f;
        channelPans[i]  = 0.5f;
        channelHasSfz[i].store(false, std::memory_order_relaxed);
    }
}

AudioHandler::~AudioHandler()
{
}

void AudioHandler::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    currentSampleRate = device->getCurrentSampleRate();
    const int bufferSize = device->getCurrentBufferSizeSamples();
    tempBuffer.setSize(2, bufferSize, false, true);

    for (int i = 0; i < 16; ++i)
    {
        sfzSynths[i].setCurrentPlaybackSampleRate(currentSampleRate);
        channelDSP[i].prepare(currentSampleRate, bufferSize);
    }
}

void AudioHandler::audioDeviceStopped()
{
}

void AudioHandler::audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                                     float* const* outputChannelData, int numOutputChannels,
                                                     int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);

    for (int i = 0; i < numOutputChannels; ++i)
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);

    if (numOutputChannels == 0)
        return;

    juce::MidiBuffer incomingMidi;
    midiHandler.getNextMidiBlock(incomingMidi, 0, numSamples);

    // Update per-channel state from incoming CCs before rendering;
    // also detect noteOns on channels with no SFZ loaded.
    for (const auto metadata : incomingMidi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isController())
        {
            const int ch  = msg.getChannel() - 1;
            const int cc  = msg.getControllerNumber();
            const int val = msg.getControllerValue();
            if (ch >= 0 && ch < 16)
            {
                if      (cc == 7)  channelGains[ch] = val / 127.0f;
                else if (cc == 10) channelPans[ch]  = val / 127.0f;
                else               channelDSP[ch].updateCC(cc, val);
            }
        }
        else if (msg.isNoteOn() && onNoSfzForChannels)
        {
            const int ch = msg.getChannel() - 1;
            if (ch >= 0 && ch < 16 && !channelHasSfz[ch].load(std::memory_order_relaxed))
            {
                noSfzChannelMask.fetch_or(1 << ch, std::memory_order_relaxed);
                bool expected = false;
                if (noSfzNotifyPending.compare_exchange_strong(expected, true))
                {
                    juce::MessageManager::callAsync([this]() {
                        const int mask = noSfzChannelMask.exchange(0);
                        noSfzNotifyPending.store(false);
                        if (onNoSfzForChannels && mask != 0)
                            onNoSfzForChannels(mask);
                    });
                }
            }
        }
    }

    // tempBuffer is always 2-channel; reuse its allocation if large enough
    tempBuffer.setSize(2, numSamples, false, false, true);

    juce::AudioBuffer<float> mainBuffer(outputChannelData, numOutputChannels, numSamples);

    for (int channel = 1; channel <= 16; ++channel)
    {
        if (!channelHasSfz[channel - 1].load(std::memory_order_acquire))
            continue;

        juce::MidiBuffer channelMidi;
        for (const auto metadata : incomingMidi)
        {
            const auto message = metadata.getMessage();
            if (message.getChannel() == channel)
                channelMidi.addEvent(message, metadata.samplePosition);
        }

        tempBuffer.clear();
        sfzSynths[channel - 1].renderNextBlock(tempBuffer, channelMidi, 0, numSamples);

        channelDSP[channel - 1].process(tempBuffer, numSamples);

        const float gain = channelGains[channel - 1];
        const float pan  = channelPans[channel - 1];

        if (numOutputChannels >= 2)
        {
            const float leftGain  = gain * std::cos(pan * juce::MathConstants<float>::halfPi);
            const float rightGain = gain * std::sin(pan * juce::MathConstants<float>::halfPi);
            mainBuffer.addFrom(0, 0, tempBuffer, 0, 0, numSamples, leftGain);
            mainBuffer.addFrom(1, 0, tempBuffer, 1, 0, numSamples, rightGain);
        }
        else
        {
            mainBuffer.addFrom(0, 0, tempBuffer, 0, 0, numSamples, gain);
        }
    }
}

void AudioHandler::loadSfz(const juce::File& sfzFile, int midiChannel)
{
    if (midiChannel < 1 || midiChannel > 16)
        return;

    if (!sfzFile.existsAsFile())
    {
        // No SFZ mapped for this channel — silence it so the previous instrument
        // doesn't keep playing when the new style has no mapping.
        if (channelHasSfz[midiChannel - 1].load(std::memory_order_relaxed)
            || loadedSfzPath[midiChannel - 1].isNotEmpty())
        {
            channelHasSfz[midiChannel - 1].store(false, std::memory_order_release);
            sfzSynths[midiChannel - 1].clearSounds();
            loadedSfzPath[midiChannel - 1] = juce::String();
        }
        return;
    }

    if (sfzFile.getFullPathName() == loadedSfzPath[midiChannel - 1])
        return;
    loadedSfzPath[midiChannel - 1] = sfzFile.getFullPathName();

    ++pendingLoads;
    if (onSfzLoadStart)
        onSfzLoadStart();

    juce::Thread::launch([this, sfzFile, midiChannel]()
    {
        auto* sound = new sfzero::Sound(sfzFile);
        sound->loadRegions();
        sound->loadSamples(&formatManager);
        sfzSynths[midiChannel - 1].clearSounds();
        sfzSynths[midiChannel - 1].addSound(sound);
        channelHasSfz[midiChannel - 1].store(true, std::memory_order_release);

        if (--pendingLoads == 0)
            juce::MessageManager::callAsync([this]() {
                if (onSfzLoadComplete) onSfzLoadComplete();
            });
    });
}
