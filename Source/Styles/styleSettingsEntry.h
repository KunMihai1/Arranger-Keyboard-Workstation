/*
  ==============================================================================

    styleSettingsEntry.h
    Created: 8 Apr 2026 1:08:42am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "PersistedKeys.h"

/** Starting values of a hand's sound settings (MIDI controller values). */
namespace SoundSettingsDefaults
{
    constexpr int controllerCentre = 64;   // neutral setting of a 0..127 controller
    constexpr int effectOff        = 0;
}

struct SoundSettings
{
	int reverb = SoundSettingsDefaults::controllerCentre;
	int volume = SoundSettingsDefaults::controllerCentre;

	int brightness = SoundSettingsDefaults::controllerCentre;
	int chorus = SoundSettingsDefaults::controllerCentre;
	int expression = SoundSettingsDefaults::controllerCentre;
	int resonance = SoundSettingsDefaults::controllerCentre;
	bool sustainToggle = false;

	int attack = SoundSettingsDefaults::controllerCentre;
	int decay = SoundSettingsDefaults::controllerCentre;
	int release = SoundSettingsDefaults::controllerCentre;
	int vibrato = SoundSettingsDefaults::effectOff;

	int delay = SoundSettingsDefaults::controllerCentre;
	int pan = SoundSettingsDefaults::controllerCentre;

	int distortion = SoundSettingsDefaults::effectOff;
	int filterTrack = SoundSettingsDefaults::effectOff;
	int tremolo = SoundSettingsDefaults::effectOff;
	int randomMod = SoundSettingsDefaults::effectOff;

    void setValue(const juce::String& key, int value)
    {
        if (key == EffectSettingsKeys::brightness) brightness = value;
        else if (key == EffectSettingsKeys::expression) expression = value;
        else if (key == EffectSettingsKeys::chorus) chorus = value;
        else if (key == EffectSettingsKeys::resonance) resonance = value;
        else if (key == EffectSettingsKeys::sustainToggle) sustainToggle = value;

        else if (key == EffectSettingsKeys::vibrato) vibrato = value;
        else if (key == EffectSettingsKeys::attack) attack = value;
        else if (key == EffectSettingsKeys::decay) decay = value;
        else if (key == EffectSettingsKeys::release) release = value;

        else if (key == EffectSettingsKeys::volume) volume = value;
        else if (key == EffectSettingsKeys::reverb) reverb = value;
        else if (key == EffectSettingsKeys::delay) delay = value;
        else if (key == EffectSettingsKeys::pan) pan = value;

        else if (key == EffectSettingsKeys::distortion) distortion = value;
        else if (key == EffectSettingsKeys::filterTrack) filterTrack = value;
        else if (key == EffectSettingsKeys::tremolo) tremolo = value;
        else if (key == EffectSettingsKeys::randomMod) randomMod = value;
    }

};

struct StyleSettings {
	SoundSettings firstHand;
	SoundSettings secondHand;
};
