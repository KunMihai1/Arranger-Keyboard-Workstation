#include "MainComponent.h"
#include "AppInfo.h"


namespace
{
    juce::String makeDeviceSettingsKey(const juce::String& VID, const juce::String& PID)
    {
        juce::String deviceKey = VID + "_" + PID;
        if (deviceKey == "_") deviceKey = "default";
        return deviceKey;
    }
}

void MainComponent::applySettingsToChannel(const SoundSettings& s, int channel)
{
    MIDIDevice.setReverb(s.reverb, channel);
    MIDIDevice.setVolume(s.volume, channel);

    MIDIDevice.setBrightness(s.brightness, channel);
    MIDIDevice.setChorus(s.chorus, channel);
    MIDIDevice.setExpression(s.expression, channel);
    MIDIDevice.setResonance(s.resonance, channel);
    MIDIDevice.setSustainToggle(s.sustainToggle, channel);

    MIDIDevice.setAttack(s.attack, channel);
    MIDIDevice.setDecay(s.decay, channel);
    MIDIDevice.setRelease(s.release, channel);
    MIDIDevice.setVibrato(s.vibrato, channel);

    MIDIDevice.setDelay(s.delay, channel);
    MIDIDevice.setPan(s.pan, channel);

    MIDIDevice.setDistortion(s.distortion, channel);
    MIDIDevice.setFilterTrack(s.filterTrack, channel);
    MIDIDevice.setTremolo(s.tremolo, channel);
    MIDIDevice.setRandomMod(s.randomMod, channel);
}

juce::String MainComponent::applyEffectCC(int ccNumber, int value, int channel)
{
    switch (ccNumber)
    {
        case 74: MIDIDevice.setBrightness(value, channel);  return "brightness";
        case 11: MIDIDevice.setExpression(value, channel);  return "expression";
        case 93: MIDIDevice.setChorus(value, channel);      return "chorus";
        case 71: MIDIDevice.setResonance(value, channel);   return "resonance";
        case 64: MIDIDevice.setSustainToggle(value, channel); return "sustainToggle";

        case 1:  MIDIDevice.setVibrato(value, channel);     return "vibrato";
        case 73: MIDIDevice.setAttack(value, channel);      return "attack";
        case 75: MIDIDevice.setDecay(value, channel);       return "decay";
        case 72: MIDIDevice.setRelease(value, channel);     return "release";

        case 7:  MIDIDevice.setVolume(value, channel);      return "volume";
        case 91: MIDIDevice.setReverb(value, channel);      return "reverb";
        case 94: MIDIDevice.setDelay(value, channel);       return "delay";
        case 10: MIDIDevice.setPan(value, channel);         return "pan";

        case 80: MIDIDevice.setDistortion(value, channel);  return "distortion";
        case 76: MIDIDevice.setFilterTrack(value, channel); return "filterTrack";
        case 92: MIDIDevice.setTremolo(value, channel);     return "tremolo";
        case 95: MIDIDevice.setRandomMod(value, channel);   return "randomMod";

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
    return styleID + "." + makeDeviceSettingsKey(VID, PID);
}

void MainComponent::loadEffectsFromFile(const juce::String& styleID)
{
    SoundSettings firstChannel, secondChannel;
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    const juce::String settingsKey = styleID + "." + makeDeviceSettingsKey(VID, PID);

    auto it = styleSettingsMap.find(settingsKey);
    if (it != styleSettingsMap.end())
    {
        applySettingsToChannel(it->second.firstHand, 1);
        applySettingsToChannel(it->second.secondHand, 16);
        return;
    }

    SoundSettings first = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, 1);
    SoundSettings second = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, 16);

    styleSettingsMap[settingsKey] = StyleSettings{ first, second };

    applySettingsToChannel(first, 1);
    applySettingsToChannel(second, 16);
}

void MainComponent::loadSettings(const juce::String& styleID)
{
    if (propertiesFile)
    {

        loadEffectsFromFile(styleID);

        int leftHandInstrumentNumber = propertiesFile->getIntValue(styleID + "_leftInstrumentNumber", 0);
        int rightHandInstrumentNumber = propertiesFile->getIntValue(styleID + "_rightInstrumentNumber", 0);


        this->midiHandler.setProgramNumber(leftHandInstrumentNumber, "left");
        this->midiHandler.setProgramNumber(rightHandInstrumentNumber, "right");
    }
}

void MainComponent::playChordOnClick(const Chord& c)
{
    auto notes = ChordHelper::getNotesForChord(c);

    for (auto note : notes)
    {
        midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, note, (juce::uint8)100));

        juce::Timer::callAfterDelay(500, [this, note]() {
            midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, note));
        });
    }
}

void MainComponent::buildChordLibrary()
{
    myChordLibrary.clear();

    std::vector<juce::String> roots =
    {
        "C","C#","D","D#","E",
        "F","F#","G","G#","A","A#","B"
    };

    std::vector<juce::String> types =
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

            if (type == "7")
                chord.name = root + "7";
            else
                chord.name = root + " " + type;

            myChordLibrary.push_back(chord);
        }
    }

}

void MainComponent::setCallBacksForEffectWindow()
{
    std::function<void(int, int)> saveCallback = [this](int ccNumber, int value)
    {
        int currentChannel = this->soundEffectWindow->getContent().getSelectedChannel();
        juce::String styleID = display->getStyleID();
        juce::String VID, PID;
        getCurrentEffectDeviceIds(VID, PID);
        const juce::String settingsKey = styleID + "." + makeDeviceSettingsKey(VID, PID);

        const juce::String key = applyEffectCC(ccNumber, value, currentChannel);
        if (key.isEmpty())
            return;

        StyleSettings& s = styleSettingsMap[settingsKey];
        if (currentChannel == 1) s.firstHand.setValue(key, value);
        else if (currentChannel == 16) s.secondHand.setValue(key, value);

        EffectSettingsIOHelper::saveSingleEffect(propertiesFile, styleID, VID, PID, currentChannel, key, value);

        if (!playButton.isVisible())
        {
            if (MIDIDevice.isOpenOUT())
                MIDIDevice.sendMidiCC(currentChannel, ccNumber, value);
            if (MIDIDevice.isOpenAudioOUT())
                midiHandler.injectCC(currentChannel, ccNumber, value);
        }
    };


    auto& content = this->soundEffectWindow->getContent();

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
        auto& content = this->soundEffectWindow->getContent();

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
void MainComponent::setInitialValuesFromSettingsFileEffectWindow()
{
    auto& content = soundEffectWindow->getContent();

    content.getBrightnessKnob().setValue(MIDIDevice.getBrightness(1));
    content.getChorusKnob().setValue(MIDIDevice.getChorus(1));
    content.getExpressionKnob().setValue(MIDIDevice.getExpression(1));
    content.getResonanceKnob().setValue(MIDIDevice.getResonance(1));
    content.getSustainToggle().setOn(MIDIDevice.getSustainToggle(1));

    // Line 2 - Envelope & Modulation
    content.getAttackKnob().setValue(MIDIDevice.getAttack(1));
    content.getDecayKnob().setValue(MIDIDevice.getDecay(1));
    content.getReleaseKnob().setValue(MIDIDevice.getRelease(1));
    content.getVibratoKnob().setValue(MIDIDevice.getVibrato(1));

    // Line 3 - Effects & Pan
    content.getVolumeKnob().setValue(MIDIDevice.getVolume(1));
    content.getReverbKnob().setValue(MIDIDevice.getReverb(1));
    content.getDelayKnob().setValue(MIDIDevice.getDelay(1));
    content.getPanKnob().setValue(MIDIDevice.getPan(1));

    // Line 4 - Additional Effects
    content.getDistortionKnob().setValue(MIDIDevice.getDistortion(1));
    content.getFilterTrackKnob().setValue(MIDIDevice.getFilterTrack(1));
    content.getTremoloKnob().setValue(MIDIDevice.getTremolo(1));
    content.getRandomModKnob().setValue(MIDIDevice.getRandomMod(1));
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
    { 74, [&](int ch) { return MIDIDevice.getBrightness(ch); } },
    { 11, [&](int ch) { return MIDIDevice.getExpression(ch); } },
    { 93, [&](int ch) { return MIDIDevice.getChorus(ch); } },
    { 71, [&](int ch) { return MIDIDevice.getResonance(ch); } },
    { 64, [&](int ch) { return MIDIDevice.getSustainToggle(ch) ? 127 : 0; } },

    { 73, [&](int ch) { return MIDIDevice.getAttack(ch); } },
    { 75, [&](int ch) { return MIDIDevice.getDecay(ch); } },
    { 72, [&](int ch) { return MIDIDevice.getRelease(ch); } },
    { 1,  [&](int ch) { return MIDIDevice.getVibrato(ch); } },

    { 94, [&](int ch) { return MIDIDevice.getDelay(ch); } },
    { 10, [&](int ch) { return MIDIDevice.getPan(ch); } },
    { 91, [&](int ch) { return MIDIDevice.getReverb(ch); } },
    { 7,  [&](int ch) { return MIDIDevice.getVolume(ch); } },

    { 80, [&](int ch) { return MIDIDevice.getDistortion(ch); } },
    { 76, [&](int ch) { return MIDIDevice.getFilterTrack(ch); } },
    { 92, [&](int ch) { return MIDIDevice.getTremolo(ch); } },
    { 95, [&](int ch) { return MIDIDevice.getRandomMod(ch); } }
    };

    for (int ch : { 1, 16 })
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
