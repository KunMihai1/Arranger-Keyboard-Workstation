/*
  ==============================================================================

    CustomToolTip.cpp
    Created: 9 Jul 2025 6:27:48pm
    Author:  Kisuke

  ==============================================================================
*/

#include "CustomToolTip.h"
#include "UI/Stage/StageFonts.h"
#include "Common/AppColours.h"

namespace
{
    constexpr float textSize      = 12.0f;
    constexpr float letterSpacing = 0.04f;
    constexpr float cornerRadius  = 3.0f;
    constexpr float borderWidth   = 1.0f;
    constexpr float borderInset   = 0.5f;   // a 1 px stroke centred on the pixel grid
    constexpr bool  mediumWeight  = false;
    constexpr int   halves        = 2;
}

juce::Font StageTooltip::font (float scale)
{
    return StageFonts::silkscreen (textSize * scale, mediumWeight, letterSpacing);
}

juce::Rectangle<int> StageTooltip::sizeFor (const juce::String& text, float scale)
{
    const auto scaledPadding = static_cast<float> (padding) * scale;
    const auto textWidth  = juce::GlyphArrangement::getStringWidth (font (scale), text);
    const auto textHeight = font (scale).getHeight();
    return { static_cast<int> (std::ceil (textWidth + scaledPadding * halves)), static_cast<int> (std::ceil (textHeight + scaledPadding)) };
}

void StageTooltip::paint (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, float scale)
{
    const auto area = bounds.toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, area.getY(), StagePalette::pageBottom, area.getBottom()));
    g.fillRoundedRectangle (area, cornerRadius * scale);
    g.setColour (StagePalette::ruleLine);
    g.drawRoundedRectangle (area.reduced (borderInset), cornerRadius * scale, borderWidth);   // 1 px at every scale

    g.setColour (StagePalette::silkscreenText);
    g.setFont (font (scale));
    g.drawText (text, area.reduced (static_cast<float> (padding) * scale, float {}), juce::Justification::centredLeft, false);
}

juce::Rectangle<int> StageTooltipLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    // As LookAndFeel_V4: below-right of the mouse, kept inside the parent area.
    const auto size = StageTooltip::sizeFor (tipText);
    const auto tipLeft = screenPos.x > parentArea.getCentreX() ? screenPos.x - (size.getWidth() + StageTooltip::padding) : screenPos.x + StageTooltip::padding;
    const auto tipTop  = screenPos.y > parentArea.getCentreY() ? screenPos.y - (size.getHeight() + StageTooltip::padding) : screenPos.y + StageTooltip::padding;
    return juce::Rectangle<int> (tipLeft, tipTop, size.getWidth(), size.getHeight()).constrainedWithin (parentArea);
}

void StageTooltipLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    StageTooltip::paint (g, { width, height }, text);
}

CustomToolTip::CustomToolTip(const juce::String& text, float tipScale) : text (text), scale (tipScale)
{
    setInterceptsMouseClicks (false, false);
    const auto size = StageTooltip::sizeFor (text, scale);
    setSize (size.getWidth(), size.getHeight());
}

void CustomToolTip::paint(juce::Graphics& g)
{
    StageTooltip::paint (g, getLocalBounds(), text, scale);
}

void CustomToolTip::setNewText(const juce::String& newText)
{
    text = newText;
    const auto size = StageTooltip::sizeFor (text, scale);
    setSize (size.getWidth(), size.getHeight());   // re-fit so a longer string doesn't clip
    repaint();
}
