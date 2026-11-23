/*
  ==============================================================================

    StageMetrics.h

    Reference sizes of the Stage panel, in CSS px of the board `StageRevised`
    (docs/superpowers/specs/assets/2026-11-23-ui-redesign/) at the reference
    window 1200 x 645. The app multiplies every length by StageScale's factor
    (spec 2026-11-23 §4.1).

  ==============================================================================
*/

#pragma once

namespace StageMetrics
{
    // --- Reference window and the regions stacked in it (top to bottom) ---
    inline constexpr int   referenceWidth  = 1200;
    inline constexpr int   referenceHeight = 645;
    inline constexpr float referenceScale  = 1.0f;

    inline constexpr int panelHeight    = 124;
    inline constexpr int feltHeight     = 4;
    inline constexpr int noteAreaHeight = 365;
    inline constexpr int keysHeight     = 140;
    inline constexpr int slipHeight     = 12;

    // --- Panel: three columns 1fr | centre | 1fr, padding vertical x horizontal ---
    inline constexpr int panelCentreColumnWidth = 600;
    inline constexpr int panelPaddingVertical   = 7;
    inline constexpr int panelPaddingHorizontal = 20;

    // --- Screen (centre) ---
    inline constexpr int screenWidth  = 368;
    inline constexpr int screenHeight = 106;

    // --- Buttons ---
    inline constexpr int buttonWidth        = 26;   // section and transport buttons
    inline constexpr int buttonHeight       = 14;
    inline constexpr int screenButtonWidth  = 40;   // page buttons beside the screen
    inline constexpr int screenButtonHeight = 17;
    inline constexpr int ledDiameter        = 5;
    inline constexpr int ledToButtonGap     = 3;    // LED sits left of the button
    inline constexpr int buttonToLabelGap   = 3;    // label centred under the button
    inline constexpr int buttonDropEdge     = 2;    // the dark edge under a cap (1 while pressed)
    inline constexpr int keyComponentWidth  = 46;   // a key button's component: wide enough for "AUTO FILL"

    // --- Labels (CSS font-size / letter-spacing em / line-height) ---
    inline constexpr float keyLabelSize        = 9.0f;
    inline constexpr float keyLabelSpacing     = 0.08f;
    inline constexpr int   keyLabelLineHeight  = 10;
    inline constexpr float silkLabelSize       = 10.0f;   // screen buttons, toggles, knob labels
    inline constexpr float silkLabelSpacing    = 0.12f;
    inline constexpr int   silkLabelLineHeight = 12;

    // --- Screen buttons and LED toggles ---
    inline constexpr int screenButtonGap = 6;   // label - cap - LED
    inline constexpr int toggleGap       = 6;   // LED - label
    inline constexpr int toggleHeight    = 14;

    // --- Left third: section buttons ---
    inline constexpr int sectionGridColumns = 6;
    inline constexpr int sectionGridRows    = 3;
    inline constexpr int sectionRowGap      = 10;

    // --- Right third: transport grid, rule, Quick FX ---
    inline constexpr int transportColumns     = 3;
    inline constexpr int transportColumnWidth = 44;
    inline constexpr int transportRowGap      = 12;
    inline constexpr int rightThirdGap        = 8;
    inline constexpr int rightRuleHeight      = 96;
    inline constexpr int quickFxWidth         = 130;
    inline constexpr int quickKnobCellWidth   = 62;

    // --- Knobs: ring with ticks, knurled skirt, cap, pointer; label under the ring ---
    inline constexpr int quickKnobRing          = 52;
    inline constexpr int quickKnobSkirt         = 36;
    inline constexpr int quickKnobCap           = 24;
    inline constexpr int quickKnobTickLength    = 4;
    inline constexpr int quickKnobPointerTop    = 4;
    inline constexpr int quickKnobPointerLength = 10;
    inline constexpr int quickKnobShadowOffset  = 3;
    inline constexpr int quickKnobShadowBlur    = 4;
    inline constexpr int quickKnobSegmentDegrees = 6;
    inline constexpr int quickKnobLabelGap      = 1;

    inline constexpr int effectsKnobCellWidth     = 96;
    inline constexpr int effectsKnobRing          = 72;
    inline constexpr int effectsKnobSkirt         = 50;
    inline constexpr int effectsKnobCap           = 34;
    inline constexpr int effectsKnobTickLength    = 5;
    inline constexpr int effectsKnobPointerTop    = 5;
    inline constexpr int effectsKnobPointerLength = 13;
    inline constexpr int effectsKnobShadowOffset  = 4;
    inline constexpr int effectsKnobShadowBlur    = 6;
    inline constexpr int effectsKnobSegmentDegrees = 5;
    inline constexpr int effectsKnobLabelGap      = 2;

    inline constexpr int knobTickIntervals  = 10;   // 11 ticks from -135 to +135 degrees
    inline constexpr int knobTickWidth      = 2;
    inline constexpr int knobPointerWidth   = 2;
    inline constexpr float knobSweepDegrees = 135.0f;   // each side of 12 o'clock
}
