/*
  ==============================================================================

    StageFonts.h

    The two typefaces of the Stage panel (spec 2026-11-23 §3.2), embedded in
    BinaryData so the panel looks the same on every machine:
      - silkscreen: Barlow Semi Condensed (Medium for dim labels, SemiBold for labels)
      - lcd:        Share Tech Mono (screen and value text)

    Each typeface is created once, on first use.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace StageFonts
{
    /** Silkscreen label font; semiBold picks the SemiBold face, otherwise Medium. */
    juce::Font silkscreen (float height, bool semiBold);

    /** LCD font for the screen and value read-outs. */
    juce::Font lcd (float height);
}
