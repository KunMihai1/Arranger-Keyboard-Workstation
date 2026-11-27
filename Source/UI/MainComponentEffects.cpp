#include "MainComponent.h"
#include "AppInfo.h"


namespace
{
    constexpr int defaultProgram      = 0;
    constexpr int chordPreviewVelocity = 100;
    constexpr int chordPreviewMs      = 500;    // how long a clicked chord sounds
    const juce::String seventhType    = "7";    // "C7", not "C 7"
    const juce::String nameSeparator  = " ";    // "C Major"

    juce::String makeDeviceSettingsKey(const juce::String& VID, const juce::String& PID)
    {
        juce::String deviceKey = VID + DeviceSettingsKeys::vidPidSeparator + PID;
        if (deviceKey == DeviceSettingsKeys::noDeviceKey) deviceKey = DeviceSettingsKeys::defaultDeviceKey;
        return deviceKey;
    }
}

void MainComponent::applySettingsToChannel(const SoundSettings& settings, int channel)
{
    MIDIDevice.setReverb(settings.reverb, channel);
    MIDIDevice.setVolume(settings.volume, channel);

    MIDIDevice.setBrightness(settings.brightness, channel);
    MIDIDevice.setChorus(settings.chorus, channel);
    MIDIDevice.setExpression(settings.expression, channel);
    MIDIDevice.setResonance(settings.resonance, channel);
    MIDIDevice.setSustainToggle(settings.sustainToggle, channel);

    MIDIDevice.setAttack(settings.attack, channel);
    MIDIDevice.setDecay(settings.decay, channel);
    MIDIDevice.setRelease(settings.release, channel);
    MIDIDevice.setVibrato(settings.vibrato, channel);

    MIDIDevice.setDelay(settings.delay, channel);
    MIDIDevice.setPan(settings.pan, channel);

    MIDIDevice.setDistortion(settings.distortion, channel);
    MIDIDevice.setFilterTrack(settings.filterTrack, channel);
    MIDIDevice.setTremolo(settings.tremolo, channel);
    MIDIDevice.setRandomMod(settings.randomMod, channel);
}

juce::String MainComponent::applyEffectCC(int ccNumber, int value, int channel)
{
    switch (ccNumber)
    {
        case EffectControllers::brightness: MIDIDevice.setBrightness(value, channel); return EffectSettingsKeys::brightness;
        case EffectControllers::expression: MIDIDevice.setExpression(value, channel); return EffectSettingsKeys::expression;
        case EffectControllers::chorus: MIDIDevice.setChorus(value, channel); return EffectSettingsKeys::chorus;
        case EffectControllers::resonance: MIDIDevice.setResonance(value, channel); return EffectSettingsKeys::resonance;
        case EffectControllers::sustain: MIDIDevice.setSustainToggle(value, channel); return EffectSettingsKeys::sustainToggle;

        case EffectControllers::vibrato: MIDIDevice.setVibrato(value, channel); return EffectSettingsKeys::vibrato;
        case EffectControllers::attack: MIDIDevice.setAttack(value, channel); return EffectSettingsKeys::attack;
        case EffectControllers::decay: MIDIDevice.setDecay(value, channel); return EffectSettingsKeys::decay;
        case EffectControllers::release: MIDIDevice.setRelease(value, channel); return EffectSettingsKeys::release;

        case EffectControllers::volume: MIDIDevice.setVolume(value, channel); return EffectSettingsKeys::volume;
        case EffectControllers::reverb: MIDIDevice.setReverb(value, channel); return EffectSettingsKeys::reverb;
        case EffectControllers::delay: MIDIDevice.setDelay(value, channel); return EffectSettingsKeys::delay;
        case EffectControllers::pan: MIDIDevice.setPan(value, channel); return EffectSettingsKeys::pan;

        case EffectControllers::distortion: MIDIDevice.setDistortion(value, channel); return EffectSettingsKeys::distortion;
        case EffectControllers::filterTrack: MIDIDevice.setFilterTrack(value, channel); return EffectSettingsKeys::filterTrack;
        case EffectControllers::tremolo: MIDIDevice.setTremolo(value, channel); return EffectSettingsKeys::tremolo;
        case EffectControllers::randomMod: MIDIDevice.setRandomMod(value, channel); return EffectSettingsKeys::randomMod;

        default: return {};
    }
}

void MainComponent::getCurrentEffectDeviceIds(juce::String& VID, juce::String& PID)
{
    if (display)
    {
        VID = display->getVID();
        PID = display->getPID();
        return;
    }

    VID = MIDIDevice.getVID();
    PID = MIDIDevice.getPID();
}

juce::String MainComponent::getCurrentEffectSettingsKey(const juce::String& styleID)
{
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    return styleID + DeviceSettingsKeys::prefixSeparator + makeDeviceSettingsKey(VID, PID);
}

void MainComponent::loadEffectsFromFile(const juce::String& styleID)
{
    SoundSettings firstChannel, secondChannel;
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    const juce::String settingsKey = styleID + DeviceSettingsKeys::prefixSeparator + makeDeviceSettingsKey(VID, PID);

    auto it = styleSettingsMap.find(settingsKey);
    if (it != styleSettingsMap.end())
    {
        applySettingsToChannel(it->second.firstHand, HandChannels::leftHand);
        applySettingsToChannel(it->second.secondHand, HandChannels::rightHand);
        return;
    }

    SoundSettings first = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, HandChannels::leftHand);
    SoundSettings second = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, HandChannels::rightHand);

    styleSettingsMap[settingsKey] = StyleSettings{ first, second };

    applySettingsToChannel(first, HandChannels::leftHand);
    applySettingsToChannel(second, HandChannels::rightHand);
}

void MainComponent::loadSettings(const juce::String& styleID)
{
    if (propertiesFile)
    {

        loadEffectsFromFile(styleID);

        int leftHandInstrumentNumber = propertiesFile->getIntValue(styleID + SettingsFileKeys::leftInstrumentSuffix, defaultProgram);
        int rightHandInstrumentNumber = propertiesFile->getIntValue(styleID + SettingsFileKeys::rightInstrumentSuffix, defaultProgram);


        this->midiHandler.setProgramNumber(leftHandInstrumentNumber, HandChoiceNames::left);
        this->midiHandler.setProgramNumber(rightHandInstrumentNumber, HandChoiceNames::right);
    }
}

void MainComponent::playChordOnClick(const Chord& chord)
{
    auto notes = ChordHelper::getNotesForChord(chord);

    for (auto note : notes)
    {
        midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(HandChannels::leftHand, note, (juce::uint8)chordPreviewVelocity));

        juce::Timer::callAfterDelay(chordPreviewMs, [this, note]() {
            midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(HandChannels::leftHand, note));
        });
    }
}

void MainComponent::buildChordLibrary()
{
    myChordLibrary.clear();

    static const std::vector<juce::String> roots =
    {
        "C","C#","D","D#","E",
        "F","F#","G","G#","A","A#","B"
    };

    static const std::vector<juce::String> types =
    {
        "Major",
        "Minor",
        "7",
        "Augmented",
        "Diminished"
    };

    for (const auto& root : roots)
    {
        for (const auto& type : types)
        {
            Chord chord;

            if (type == seventhType)
                chord.name = root + seventhType;
            else
                chord.name = root + nameSeparator + type;

            myChordLibrary.push_back(chord);
        }
    }

}

void MainComponent::applyAndSaveEffect(int ccNumber, int value, int currentChannel, bool save)
{
    juce::String styleID = display->getStyleID();
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    const juce::String settingsKey = styleID + DeviceSettingsKeys::prefixSeparator + makeDeviceSettingsKey(VID, PID);

    const juce::String key = applyEffectCC(ccNumber, value, currentChannel);
    if (key.isEmpty())
        return;

    StyleSettings& styleSettings = styleSettingsMap[settingsKey];
    if (currentChannel == HandChannels::leftHand) styleSettings.firstHand.setValue(key, value);
    else if (currentChannel == HandChannels::rightHand) styleSettings.secondHand.setValue(key, value);

    if (save)   // a file write per knob step made turning a knob lag (measured 11.6 ms each)
        EffectSettingsIOHelper::saveSingleEffect(propertiesFile, styleID, VID, PID, currentChannel, key, value);

    if (inPlayView)
    {
        if (MIDIDevice.isOpenOUT())
            MIDIDevice.sendMidiCC(currentChannel, ccNumber, value);
        if (MIDIDevice.isOpenAudioOUT())
            midiHandler.injectCC(currentChannel, ccNumber, value);
    }
}

void MainComponent::setCallBacksForEffectWindow()
{
    std::function<void(int, int)> saveCallback = [this](int ccNumber, int value)
    {
        const int channel = this->effectsContent->getSelectedChannel();
        applyAndSaveEffect(ccNumber, value, channel);

        // The panel's Quick FX knobs show the same stored volume / reverb for the lit part: follow a change made
        // here (they were only re-read on the next page change).
        const bool quickFxValue = ccNumber == EffectControllers::volume || ccNumber == EffectControllers::reverb;
        if (quickFxValue && stagePanel != nullptr && channel == selectedPartChannel())
            stagePanel->setQuickKnobValues(MIDIDevice.getVolume(channel), MIDIDevice.getReverb(channel));
    };


    auto& content = *this->effectsContent;

    // Line 1 – Tone
    content.getBrightnessKnob().onValueChanged = saveCallback;
    content.getExpressionKnob().onValueChanged = saveCallback;
    content.getChorusKnob().onValueChanged = saveCallback;
    content.getResonanceKnob().onValueChanged = saveCallback;
    content.getSustainToggle().onValueChanged = saveCallback;

    // Line 2 – Envelope
    content.getAttackKnob().onValueChanged   = saveCallback;
    content.getDecayKnob().onValueChanged     = saveCallback;
    content.getReleaseKnob().onValueChanged   = saveCallback;
    content.getVibratoKnob().onValueChanged = saveCallback;

    // Line 3 – Space
    content.getVolumeKnob().onValueChanged = saveCallback;
    content.getReverbKnob().onValueChanged = saveCallback;
    content.getDelayKnob().onValueChanged     = saveCallback;
    content.getPanKnob().onValueChanged       = saveCallback;

    // Line 4 – Modulation
    content.getDistortionKnob().onValueChanged  = saveCallback;
    content.getFilterTrackKnob().onValueChanged = saveCallback;
    content.getTremoloKnob().onValueChanged     = saveCallback;
    content.getRandomModKnob().onValueChanged   = saveCallback;

    content.onChannelChanged = [this](int channel)
    {
        auto& content = *this->effectsContent;

        // Line 1 – Tone
        content.getBrightnessKnob().setValue(MIDIDevice.getBrightness(channel));
        content.getExpressionKnob().setValue(MIDIDevice.getExpression(channel));
        content.getChorusKnob().setValue(MIDIDevice.getChorus(channel));
        content.getResonanceKnob().setValue(MIDIDevice.getResonance(channel));
        content.getSustainToggle().setOn(MIDIDevice.getSustainToggle(channel));

        // Line 2 – Envelope
        content.getAttackKnob().setValue(MIDIDevice.getAttack(channel));
        content.getDecayKnob().setValue(MIDIDevice.getDecay(channel));
        content.getReleaseKnob().setValue(MIDIDevice.getRelease(channel));
        content.getVibratoKnob().setValue(MIDIDevice.getVibrato(channel));

        // Line 3 – Space
        content.getVolumeKnob().setValue(MIDIDevice.getVolume(channel));
        content.getReverbKnob().setValue(MIDIDevice.getReverb(channel));
        content.getDelayKnob().setValue(MIDIDevice.getDelay(channel));
        content.getPanKnob().setValue(MIDIDevice.getPan(channel));

        // Line 4 – Modulation
        content.getDistortionKnob().setValue(MIDIDevice.getDistortion(channel));
        content.getFilterTrackKnob().setValue(MIDIDevice.getFilterTrack(channel));
        content.getTremoloKnob().setValue(MIDIDevice.getTremolo(channel));
        content.getRandomModKnob().setValue(MIDIDevice.getRandomMod(channel));
    };
}
void MainComponent::sendEffectsBeforePlaying()
{
    const bool hasOut   = MIDIDevice.isOpenOUT();
    const bool hasAudio = MIDIDevice.isOpenAudioOUT();

    if (!hasOut && !hasAudio)
        return;

    struct EffectCC
    {
        int ccNumber;
        std::function<int(int)> getter;
    };

    std::vector<EffectCC> effects = {
    { EffectControllers::brightness, [&](int ch) { return MIDIDevice.getBrightness(ch); } },
    { EffectControllers::expression, [&](int ch) { return MIDIDevice.getExpression(ch); } },
    { EffectControllers::chorus, [&](int ch) { return MIDIDevice.getChorus(ch); } },
    { EffectControllers::resonance, [&](int ch) { return MIDIDevice.getResonance(ch); } },
    { EffectControllers::sustain, [&](int ch) { return MIDIDevice.getSustainToggle(ch) ? MidiFacts::highestControllerValue : MidiFacts::lowestControllerValue; } },

    { EffectControllers::attack, [&](int ch) { return MIDIDevice.getAttack(ch); } },
    { EffectControllers::decay, [&](int ch) { return MIDIDevice.getDecay(ch); } },
    { EffectControllers::release, [&](int ch) { return MIDIDevice.getRelease(ch); } },
    { EffectControllers::vibrato, [&](int ch) { return MIDIDevice.getVibrato(ch); } },

    { EffectControllers::delay, [&](int ch) { return MIDIDevice.getDelay(ch); } },
    { EffectControllers::pan, [&](int ch) { return MIDIDevice.getPan(ch); } },
    { EffectControllers::reverb, [&](int ch) { return MIDIDevice.getReverb(ch); } },
    { EffectControllers::volume, [&](int ch) { return MIDIDevice.getVolume(ch); } },

    { EffectControllers::distortion, [&](int ch) { return MIDIDevice.getDistortion(ch); } },
    { EffectControllers::filterTrack, [&](int ch) { return MIDIDevice.getFilterTrack(ch); } },
    { EffectControllers::tremolo, [&](int ch) { return MIDIDevice.getTremolo(ch); } },
    { EffectControllers::randomMod, [&](int ch) { return MIDIDevice.getRandomMod(ch); } }
    };

    for (int ch : { HandChannels::leftHand, HandChannels::rightHand })
    {
        for (auto& effect : effects)
        {
            const int value = effect.getter(ch);
            if (hasOut)
                MIDIDevice.sendMidiCC(ch, effect.ccNumber, value);
            if (hasAudio)
                midiHandler.injectCC(ch, effect.ccNumber, value);
        }
    }
}
