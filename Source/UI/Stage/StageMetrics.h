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

    // --- Quick knob ---
    inline constexpr int quickKnobRing  = 52;
    inline constexpr int quickKnobSkirt = 36;
    inline constexpr int quickKnobCap   = 24;
}
