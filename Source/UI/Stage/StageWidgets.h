/*
  ==============================================================================

    StageWidgets.h

    The Stage panel's hardware materials (spec 2026-11-23 §3.3), each drawing
    itself at the Stage scale set with setScale():

      RubberButton  rubber cap + LED + silkscreen label
        - key:       LED left of a 26 x 14 cap, label (1-2 lines) centred under it;
                     the component is centred on the cap, only the cap takes clicks
        - screen:    label, 40 x 17 cap, LED in a row (mirrored with LabelSide::right)
        - ledToggle: LED + label, no cap; the whole row takes clicks
      StageKnob     knurled rotary knob (quick 52 / effects 72) with its label under it

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"

class RubberButton : public juce::Button
{
public:
    enum class Style     { key, screen, ledToggle };
    enum class LedColour { red, green };
    enum class LabelTone { bright, dim };     // silkscreen (SemiBold) or dim silkscreen (Medium)
    enum class LabelSide { left, right };     // screen style: which side of the cap the label is on

    RubberButton (const juce::String& label, Style style, LedColour ledColour);

    void setLedOn (bool shouldBeOn);
    bool isLedOn() const;

    void setScale (float newScale);
    void setLabelTone (LabelTone newTone);
    void setLabelSide (LabelSide newSide);

    Style        getStyle() const;
    LedColour    getLedColour() const;
    juce::String getLabel() const;
    int          getLabelLineCount() const;

    /** Size the component needs at the current scale. */
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    juce::Rectangle<float> getCapBounds() const;
    juce::Rectangle<float> getLedBounds() const;
    juce::Rectangle<float> getLabelBounds() const;

    bool hitTest (int xPosition, int yPosition) override;

protected:
    void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override;

private:
    juce::Font labelFont() const;
    float labelWidth() const;
    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;

    juce::String label;
    juce::StringArray labelLines;
    Style style;
    LedColour ledColour;
    LabelTone labelTone { LabelTone::bright };
    LabelSide labelSide { LabelSide::left };
    bool ledOn {};
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RubberButton)
};

class StageKnob : public juce::Slider
{
public:
    enum class Size { quick, effects };

    StageKnob (const juce::String& label, Size size);

    void setScale (float newScale);
    Size getSize() const;
    juce::String getLabel() const;

    int getPreferredWidth() const;
    int getPreferredHeight() const;

    /** The ring (ticks + knob) inside the component. */
    juce::Rectangle<float> getRingBounds() const;

    void paint (juce::Graphics& g) override;

private:
    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;

    juce::String label;
    Size size;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageKnob)
};
