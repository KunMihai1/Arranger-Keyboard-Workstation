#include "SectionJumpPads.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"
#include "UI/Stage/StageWidgets.h"

namespace
{
    const juce::String jumpText = "Jump";

    // Board px (PageSections .pad / .pad-key / .led).
    constexpr float labelGap      = 8.0f;    // "Jump" to the first pad, and pad to pad
    constexpr float padWidth      = 46.0f;
    constexpr float keyHeight     = 20.0f;
    constexpr float keyDrop       = 2.0f;    // box-shadow 0 2px 0 #0d0e0f
    constexpr float keyCorner     = 3.0f;
    constexpr float keyBorder     = 1.0f;
    constexpr float ledSize       = 5.0f;
    constexpr float columnGap     = 3.0f;    // LED, key, name
    constexpr float nameTextSize  = 9.0f;
    constexpr float labelTextSize = 10.0f;
    constexpr float labelSpacing  = 0.1f;
    constexpr bool  mediumFace    = false;
    constexpr float half          = 0.5f;
}

SectionJumpPads::SectionJumpPads()
{
    setInterceptsMouseClicks (true, false);
}

void SectionJumpPads::setPads (const juce::StringArray& newNames)
{
    names = newNames;
    if (! juce::isPositiveAndBelow (playingIndex, names.size()))
        playingIndex = SectionJumpPadValues::noPad;
    repaint();
}

void SectionJumpPads::setPlayingIndex (int index)
{
    const int lit = juce::isPositiveAndBelow (index, names.size()) ? index : SectionJumpPadValues::noPad;
    if (lit == playingIndex)
        return;
    playingIndex = lit;
    repaint();
}

void SectionJumpPads::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

float SectionJumpPads::padsLeft() const
{
    const auto font = StageFonts::silkscreen (scaled (labelTextSize), mediumFace, labelSpacing);
    return juce::GlyphArrangement::getStringWidth (font, jumpText.toUpperCase()) + scaled (labelGap);
}

juce::Rectangle<int> SectionJumpPads::padBounds (int padIndex) const
{
    if (! juce::isPositiveAndBelow (padIndex, names.size()))
        return {};
    const float left = padsLeft() + static_cast<float> (padIndex) * scaled (padWidth + labelGap);
    return juce::Rectangle<float> (left, scaled (ledSize + columnGap), scaled (padWidth), scaled (keyHeight + keyDrop)).toNearestInt();
}

void SectionJumpPads::clickAt (juce::Point<int> position)
{
    for (int padIndex {}; padIndex < names.size(); ++padIndex)
    {
        // The whole column (LED, key, name) is the pad, as the board's button.
        const auto column = padBounds (padIndex).withY (int {}).withHeight (getHeight());
        if (column.contains (position))
        {
            if (onPadClicked)
                onPadClicked (padIndex);
            return;
        }
    }
}

void SectionJumpPads::mouseDown (const juce::MouseEvent& event)
{
    clickAt (event.getPosition());
}

void SectionJumpPads::paint (juce::Graphics& g)
{
    const auto firstKey = juce::Rectangle<float> (padsLeft(), scaled (ledSize + columnGap), scaled (padWidth), scaled (keyHeight));
    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (scaled (labelTextSize), mediumFace, labelSpacing));
    g.drawText (jumpText.toUpperCase(), juce::Rectangle<float> (float {}, firstKey.getY(), padsLeft(), firstKey.getHeight()),
                juce::Justification::centredLeft, false);

    for (int padIndex {}; padIndex < names.size(); ++padIndex)
    {
        const auto key = padBounds (padIndex).toFloat().withHeight (scaled (keyHeight));
        if (key.getX() > static_cast<float> (getWidth()))
            break;   // more sections than room: the rest are reached on the lanes

        const auto led = juce::Rectangle<float> (scaled (ledSize), scaled (ledSize)).withCentre ({ key.getCentreX(), scaled (ledSize) * half });
        StageDrawing::drawLed (g, led, padIndex == playingIndex, StagePalette::ledGreen, scale);

        const float corner = scaled (keyCorner);
        g.setColour (StagePalette::rubberDropEdge);
        g.fillRoundedRectangle (key.translated (float {}, scaled (keyDrop)), corner);
        g.setGradientFill (juce::ColourGradient::vertical (StagePalette::rubberTop, key.getY(), StagePalette::rubberBottom, key.getBottom()));
        g.fillRoundedRectangle (key, corner);
        g.setColour (StagePalette::rubberBorder);
        g.drawRoundedRectangle (key.reduced (scaled (keyBorder) * half), corner, scaled (keyBorder));

        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (StageFonts::silkscreen (scaled (nameTextSize), mediumFace, labelSpacing));
        const auto nameArea = juce::Rectangle<float> (key.getX() - scaled (labelGap) * half, key.getBottom() + scaled (keyDrop + columnGap),
                                                      key.getWidth() + scaled (labelGap), static_cast<float> (getHeight()) - key.getBottom());
        g.drawText (names[padIndex].toUpperCase(), nameArea, juce::Justification::centredTop, true);
    }
}
