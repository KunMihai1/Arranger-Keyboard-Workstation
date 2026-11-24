/*
  ==============================================================================

    SoundEffectWindowComponent.h
    Created: 5 Apr 2026 11:24:18pm
    Author:  Kisuke

    The Effects page (board StageRevised openPage=effects, spec part 2 P.1):
    four groups (Tone, Envelope, Space, Modulation) of four effects-size knobs,
    each sending its MIDI controller to the selected instrument's channel, and
    the title row's Instrument 1st / 2nd and Sustain. It lives on a page frame
    (MainComponent); it used to be the "Sound Effects" window.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "UI/Stage/StageWidgets.h"
#include "UI/Stage/StagePageWidgets.h"

/** One effect knob: a StageKnob (effects size) with its value; reports on release, as before. */
class EffectKnobComponent : public juce::Component
{
public:
    EffectKnobComponent (const juce::String& name, int ccNumber);

    std::function<void(int ccNumber, int value)> onValueChanged;

    void setValue (double newValue, juce::NotificationType notify = juce::sendNotification);
    double getValue() const;
    int getCCNumber() const { return cc; }
    void setScale (float newScale);
    StageKnob& getKnob() { return knob; }

    void resized() override;

private:
    StageKnob knob;
    int cc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectKnobComponent)
};

/** The sustain pedal (CC64): a seg button with a green LED in the page's title row. */
class SustainToggleComponent : public SegButton
{
public:
    SustainToggleComponent();

    std::function<void(int ccNumber, int value)> onValueChanged;
    bool isOn() const;
    void setOn (bool shouldBeOn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SustainToggleComponent)
};

/** Where the 16 knobs and the 4 group headings go (board, scale 1: two columns 548 wide, gap 48; a group is
    its heading, gap 6, four 96-wide cells spread across the column; the second row of groups 14 below). */
struct EffectsPageRects
{
    static constexpr int groups        = 4;
    static constexpr int knobsPerGroup = 4;
    std::array<juce::Rectangle<int>, groups> headings;
    std::array<juce::Rectangle<int>, groups * knobsPerGroup> knobs;   // group by group, left to right
};
EffectsPageRects effectsPageLayout (juce::Rectangle<int> content, float scale);

class SoundEffectContentComponent : public juce::Component
{
public:
    SoundEffectContentComponent();
    ~SoundEffectContentComponent() override = default;

    std::function<void(int channel)> onChannelChanged;

    int getSelectedChannel() const;
    /** 1st = the left-hand instrument's channel, 2nd = the right hand's (today's choice). */
    void selectSecondInstrument (bool second);

    EffectKnobComponent& getBrightnessKnob()  { return brightnessKnob; }
    EffectKnobComponent& getExpressionKnob()  { return expressionKnob; }
    EffectKnobComponent& getChorusKnob()      { return chorusKnob; }
    EffectKnobComponent& getResonanceKnob()   { return resonanceKnob; }
    SustainToggleComponent& getSustainToggle() { return sustainToggle; }
    EffectKnobComponent& getAttackKnob()      { return attackKnob; }
    EffectKnobComponent& getDecayKnob()       { return decayKnob; }
    EffectKnobComponent& getReleaseKnob()     { return releaseKnob; }
    EffectKnobComponent& getVibratoKnob()     { return vibratoKnob; }
    EffectKnobComponent& getReverbKnob()      { return reverbKnob; }
    EffectKnobComponent& getDelayKnob()       { return delayKnob; }
    EffectKnobComponent& getPanKnob()         { return panKnob; }
    EffectKnobComponent& getVolumeKnob()      { return volumeKnob; }
    EffectKnobComponent& getDistortionKnob()  { return distortionKnob; }
    EffectKnobComponent& getFilterTrackKnob() { return filterTrackKnob; }
    EffectKnobComponent& getTremoloKnob()     { return tremoloKnob; }
    EffectKnobComponent& getRandomModKnob()   { return randomModKnob; }

    /** The page frame's title row: INSTRUMENT, 1st, 2nd, a divider, Sustain. */
    std::vector<juce::Component*> getTitleActions();
    void setScale (float newScale);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshInstrumentLeds();

    juce::ComboBox    channelSelector;   ///< today's choice, kept (hidden) as the state 1st / 2nd set
    SilkLabel         instrumentLabel;
    SegButton         firstButton, secondButton;
    TitleDivider      sustainDivider;

    EffectKnobComponent  brightnessKnob;   // CC 74
    EffectKnobComponent  expressionKnob;   // CC 11
    EffectKnobComponent  chorusKnob;       // CC 93
    EffectKnobComponent  resonanceKnob;    // CC 71
    SustainToggleComponent sustainToggle;  // CC 64

    EffectKnobComponent attackKnob;   // CC 73
    EffectKnobComponent decayKnob;    // CC 75
    EffectKnobComponent releaseKnob;  // CC 72
    EffectKnobComponent vibratoKnob;  // CC 1

    EffectKnobComponent volumeKnob;    // CC 7
    EffectKnobComponent reverbKnob;    // CC 91
    EffectKnobComponent delayKnob;     // CC 94
    EffectKnobComponent panKnob;       // CC 10

    EffectKnobComponent distortionKnob;   // CC 80
    EffectKnobComponent filterTrackKnob;  // CC 76
    EffectKnobComponent tremoloKnob;      // CC 92
    EffectKnobComponent randomModKnob;    // CC 95

    std::vector<EffectKnobComponent*> knobs;   ///< group by group, as the board
    float scale = StageMetrics::referenceScale;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundEffectContentComponent)
};
