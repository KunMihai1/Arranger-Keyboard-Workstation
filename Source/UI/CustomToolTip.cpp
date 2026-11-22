/*
  ==============================================================================

    CustomToolTip.cpp
    Created: 9 Jul 2025 6:27:48pm
    Author:  Kisuke

  ==============================================================================
*/

#include "CustomToolTip.h"

namespace
{
    constexpr float fontHeight         = 14.0f;
    constexpr int   horizontalInset    = 10;
    constexpr int   verticalInset      = 5;
    constexpr int   insetSides         = 2;      // both sides of each axis
    constexpr int   widthSafety        = 4;
    constexpr int   heightSafety       = 2;
    constexpr float backgroundAlpha    = 0.8f;
    constexpr float cornerRadius       = 5.0f;
}

CustomToolTip::CustomToolTip(const juce::String& text)
{
    label.setFont(fontHeight);
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);

    updateSizeForText(text);
}

void CustomToolTip::updateSizeForText(const juce::String& text)
{
    const int textWidth  = juce::GlyphArrangement::getStringWidthInt(label.getFont(), text);
    const int textHeight = (int) label.getFont().getHeight();

    // resized() lays the label out with getLocalBounds().reduced(hInset, vInset), so the tooltip must
    // be that much larger than the text or the inset would shrink the label below the string width and
    // clip an edge glyph. The small extra guards against getStringWidthInt under-measuring.
    setSize(textWidth + horizontalInset * insetSides + widthSafety, textHeight + verticalInset * insetSides + heightSafety);
}

void CustomToolTip::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black.withAlpha(backgroundAlpha));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerRadius);
}

void CustomToolTip::setNewText(const juce::String& newText)
{
    label.setText(newText, juce::dontSendNotification);
    updateSizeForText(newText);   // re-fit so a longer string doesn't clip
}

void CustomToolTip::resized()
{
    label.setBounds(getLocalBounds().reduced(horizontalInset, verticalInset));
}
