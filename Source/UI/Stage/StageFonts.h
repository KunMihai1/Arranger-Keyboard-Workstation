/*
  ==============================================================================

    StageFonts.h

    The two typefaces of the Stage panel (spec 2026-11-23 §3.2), embedded in
    BinaryData so the panel looks the same on every machine:
      - silkscreen: Barlow Semi Condensed (Medium for dim labels, SemiBold for labels)
      - lcd:        Share Tech Mono (screen and value text)

    Sizes are the board's CSS values (already multiplied by the Stage scale):
    cssPixelSize = CSS font-size (JUCE point height), letterSpacingEm = CSS
    letter-spacing in em. Each typeface is created once, on first use.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace StageFonts
{
    /** Silkscreen label font; semiBold picks the SemiBold face, otherwise Medium. */
    juce::Font silkscreen (float cssPixelSize, bool semiBold, float letterSpacingEm);

    /** LCD font for the screen and value read-outs. */
    juce::Font lcd (float cssPixelSize);
}
