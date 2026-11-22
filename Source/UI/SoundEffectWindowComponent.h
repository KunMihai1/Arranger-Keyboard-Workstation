/*
  ==============================================================================

    SoundEffectWindowComponent.h
    Created: 5 Apr 2026 11:24:18pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
/**
 *  A single rotary knob with a custom arc-style paint, a label underneath,
 *  and a numeric value readout.  Designed for controlling one MIDI CC.
 */
class EffectKnobComponent : public juce::Component
{
public:
    EffectKnobComponent(const juce::String& name, int ccNumber,
                        juce::Colour accentColour);
    ~EffectKnobComponent() override;

    /** Hook: called with (ccNumber, newValue 0-127) whenever the user moves the knob. */
    std::function<void(int ccNumber, int value)> onValueChanged;

    void setValue(double newValue, juce::NotificationType notify = juce::sendNotification);
    double getValue() const;
    int getCCNumber() const { return cc; }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider knob;
    juce::Label  nameLabel;
    juce::Label  valueLabel;
    juce::Colour accent;
    int cc;

    /** Custom LookAndFeel that draws the arc-style rotary knob. */
    class ArcKnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        juce::Colour arcColour;

        void drawRotarySlider(juce::Graphics&, int sliderX, int sliderY, int width,
                              int height, float sliderPos,
                              float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;
    };

    ArcKnobLookAndFeel arcLnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectKnobComponent)
};

//==============================================================================
/**
 *  An LED-style toggle for Sustain Pedal (CC 64).
 */
class SustainToggleComponent : public juce::Component
{
public:
    SustainToggleComponent();

    /** Hook: called with (64, 127) on press, (64, 0) on release. */
    std::function<void(int ccNumber, int value)> onValueChanged;

    bool isOn() const;
    void setOn(bool shouldBeOn);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::ToggleButton toggle;
    juce::Label nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SustainToggleComponent)
};

//==============================================================================
/**
 *  The main content panel that lives inside the DocumentWindow.
 *  Contains channel selector, 4 rotary effect knobs, and a sustain toggle.
 */
class SoundEffectContentComponent : public juce::Component
{
public:
    SoundEffectContentComponent();
    ~SoundEffectContentComponent() override = default;

    /** Hook: called when the user picks a different channel (1 or 16). */
    std::function<void(int channel)> onChannelChanged;

    /** Returns the currently selected channel (1 or 16). */
    int getSelectedChannel() const;

    /** Access individual knobs for external wiring. */
    EffectKnobComponent& getBrightnessKnob()  { return brightnessKnob; }
    EffectKnobComponent& getExpressionKnob()  { return expressionKnob; }
    EffectKnobComponent& getChorusKnob()      { return chorusKnob; }
    EffectKnobComponent& getResonanceKnob()   { return resonanceKnob; }
    SustainToggleComponent& getSustainToggle() { return sustainToggle; }

    // Line 2
    EffectKnobComponent& getAttackKnob()      { return attackKnob; }
    EffectKnobComponent& getDecayKnob()       { return decayKnob; }
    EffectKnobComponent& getReleaseKnob()     { return releaseKnob; }
    EffectKnobComponent& getVibratoKnob()     { return vibratoKnob; }

    // Line 3
    EffectKnobComponent& getReverbKnob()      { return reverbKnob; }
    EffectKnobComponent& getDelayKnob()       { return delayKnob; }
    EffectKnobComponent& getPanKnob()         { return panKnob; }
    EffectKnobComponent& getVolumeKnob()      { return volumeKnob; }

    // Line 4
    EffectKnobComponent& getDistortionKnob()  { return distortionKnob; }
    EffectKnobComponent& getFilterTrackKnob() { return filterTrackKnob; }
    EffectKnobComponent& getTremoloKnob()     { return tremoloKnob; }
    EffectKnobComponent& getRandomModKnob()   { return randomModKnob; }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Label       headerLabel;
    juce::ComboBox    channelSelector;
    juce::Label       channelLabel;
    
    //line 1
    EffectKnobComponent  brightnessKnob;   // CC 74
    EffectKnobComponent  expressionKnob;   // CC 11
    EffectKnobComponent  chorusKnob;       // CC 93
    EffectKnobComponent  resonanceKnob;    // CC 71
    SustainToggleComponent sustainToggle;   // CC 64

    //line 2
    EffectKnobComponent attackKnob;   // CC 73
    EffectKnobComponent decayKnob;    // CC 75
    EffectKnobComponent releaseKnob;  // CC 72
    EffectKnobComponent vibratoKnob;  // CC 1


    // Line 3
    EffectKnobComponent delayKnob;    // CC 94
    EffectKnobComponent panKnob;      // CC 10
    EffectKnobComponent reverbKnob;    // CC 91
    EffectKnobComponent volumeKnob;    // CC 7

    // Line 4
    EffectKnobComponent distortionKnob;   // CC 80
    EffectKnobComponent filterTrackKnob;  // CC 76
    EffectKnobComponent tremoloKnob;      // CC 92
    EffectKnobComponent randomModKnob;    // CC 95

    std::vector<EffectKnobComponent*> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEffectContentComponent)
};

//==============================================================================
/**
 *  Top-level DocumentWindow wrapper.
 */
class SoundEffectWindow : public juce::DocumentWindow,
                          public juce::KeyListener
{
public:
    std::function<void()> onWindowClosed;

    SoundEffectWindow(juce::PropertiesFile* prop=nullptr);
    ~SoundEffectWindow() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;
    void closeButtonPressed() override;

    /** Convenience accessor so callers can wire callbacks on the content. */
    SoundEffectContentComponent& getContent() { return *content; }

private:
    SoundEffectContentComponent* content = nullptr;   // owned by the window via setContentOwned
    juce::PropertiesFile* propertyFile = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEffectWindow)
};