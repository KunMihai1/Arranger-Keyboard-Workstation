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
    inline constexpr int panelSideColumnWidth   = 280;   // (1200 - 2 x 20 - 600) / 2
    inline constexpr int panelBottomEdge        = 1;
    inline constexpr int centreColumnGap        = 10;    // screen buttons to screen
    inline constexpr int screenButtonRowGap     = 7;
    inline constexpr int screenButtonsPerSide   = 4;

    // --- Pages over the note area (board .page) ---
    inline constexpr int   pagePaddingTop    = 16;
    inline constexpr int   pagePaddingSide   = 28;
    inline constexpr int   pageRowGap        = 14;    // title row to content
    inline constexpr int   subPageRowGap     = 12;    // the track picker and notes pages (PageTracks, PageNotes)
    inline constexpr float pageTitleSize     = 14.0f;
    inline constexpr float pageTitleSpacing  = 0.2f;
    inline constexpr int   pageTitleRuleGap  = 14;
    inline constexpr int   pageBottomEdge    = 1;

    // --- Screen (centre): bezel, LCD with three text rows, beat lamps under it ---
    inline constexpr int screenWidth  = 368;
    inline constexpr int screenHeight = 106;
    inline constexpr int bezelPaddingTop     = 6;
    inline constexpr int bezelPaddingSide    = 6;
    inline constexpr int bezelPaddingBottom  = 5;
    inline constexpr int bezelGap            = 5;   // LCD to lamp row
    inline constexpr int bezelCornerRadius   = 3;
    inline constexpr int lcdCornerRadius     = 2;
    inline constexpr int lcdPaddingVertical  = 5;
    inline constexpr int lcdPaddingSide      = 12;
    inline constexpr float lcdTopRowSize     = 11.0f;
    inline constexpr float lcdTitleSize      = 28.0f;
    inline constexpr float lcdTempoSize      = 24.0f;
    inline constexpr float lcdBottomRowSize  = 13.0f;
    inline constexpr int lcdBottomRowGap     = 14;
    inline constexpr int lcdTopRowMinGap     = 14;   // a message beside the page name keeps at least this (as the bottom row)
    inline constexpr int tempoPaddingSide    = 2;
    inline constexpr int tempoEditorWidth    = 52;
    inline constexpr int tempoEditorHeight   = 26;
    inline constexpr float tempoEditorSize   = 22.0f;
    inline constexpr int tempoEditorPadding  = 4;
    inline constexpr int tempoDragStep       = 6;   // px of vertical drag per tempo step
    inline constexpr float messageScrollSpeed = 0.06f;   // px per ms (60 px/s) for a message wider than the screen
    inline constexpr int lampHeight          = 8;
    inline constexpr int lampGap             = 4;
    inline constexpr int lampRowInset        = 1;
    inline constexpr int lampCornerRadius    = 2;
    inline constexpr int lampGlowRadius      = 7;
    inline constexpr int defaultBeatsPerBar  = 4;   // lamps shown before a style reports its time signature
    inline constexpr int tempoStepUp         = 1;
    inline constexpr int tempoStepDown       = -1;

    // --- Buttons ---
    inline constexpr int buttonWidth        = 26;   // section and transport buttons
    inline constexpr int buttonHeight       = 14;
    inline constexpr int screenButtonWidth  = 40;   // page buttons beside the screen
    inline constexpr int screenButtonHeight = 17;
    inline constexpr int ledDiameter        = 5;
    inline constexpr int ledGlowRadius      = 3;    // a lit LED's glow reaches this far past it
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
    inline constexpr int quickFxTitleGap      = 8;     // "QUICK FX" to its rule
    inline constexpr int quickFxRowGap        = 6;
    inline constexpr int partGap              = 10;    // "PART", Left, Right
    inline constexpr float partLabelSpacing   = 0.1f;  // .silk-dim letter-spacing
    inline constexpr int ruleWidth            = 1;

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
    inline constexpr int   knobDeadZone     = 5;        // px around the centre where a turn has no direction
    inline constexpr int   knobValueLineHeight = 11;    // effects page: the value under the label
    inline constexpr float knobValueTextSize   = 10.0f;

    // --- Page materials (StageRevised .seg / .inst / .fader / .lcd-list; PageTracks .seg-lit) ---
    inline constexpr int   segHeight          = 22;
    inline constexpr int   segPaddingSide     = 10;
    inline constexpr int   segBorder          = 1;
    inline constexpr int   segLedGap          = 6;     // LED to text
    inline constexpr float segLabelSize       = 10.0f;
    inline constexpr float segLabelSpacing    = 0.1f;
    inline constexpr int   instrumentWidth    = 88;
    inline constexpr int   instrumentHeight   = 22;
    inline constexpr int   instrumentPadding  = 4;
    inline constexpr int   instrumentCorner   = 3;
    inline constexpr int   instrumentDropEdge = 2;
    inline constexpr float instrumentSpacing  = 0.04f;
    inline constexpr float silkDimSpacing     = 0.1f;
    inline constexpr int   dividerHeight      = 20;
    inline constexpr int   faderWidth         = 40;
    inline constexpr int   faderHeight        = 132;
    inline constexpr int   faderSlotLeft      = 18;
    inline constexpr int   faderSlotTop       = 4;
    inline constexpr int   faderSlotWidth     = 4;
    inline constexpr int   faderSlotHeight    = 124;
    inline constexpr int   faderSlotCorner    = 2;
    inline constexpr int   faderScaleLeft     = 4;
    inline constexpr int   faderScaleWidth    = 8;
    inline constexpr int   faderScaleMarks    = 5;
    inline constexpr int   faderScaleStep     = 30;    // marks at 4, 34, 64, 94, 124
    inline constexpr int   faderCapLeft       = 6;
    inline constexpr int   faderCapWidth      = 28;
    inline constexpr int   faderCapHeight     = 16;
    inline constexpr int   faderCapCorner     = 2;
    inline constexpr int   faderCapTravel     = 116;   // cap top runs 116 (value 0) .. 0 (value 127)
    inline constexpr int   faderCapShadowY    = 3;
    inline constexpr int   faderCapShadowBlur = 4;
    inline constexpr int   lcdListCorner      = 2;
    inline constexpr int   lcdListPadding     = 4;     // top and bottom
    inline constexpr int   lcdInsetShadow     = 3;     // inset 0 1px 3px
    inline constexpr int   lcdRowHeight       = 21;    // 14 px mono line + padding 3 / 3
    inline constexpr int   lcdRowPaddingSide  = 10;
    inline constexpr float lcdListTextSize    = 14.0f;
    inline constexpr int   headingGap         = 10;    // section heading: title, rule, dim text
    inline constexpr int   headingHeight      = 12;

    // --- Fields and row toggles (StageRevised .field / .row-toggle, Settings page) ---
    inline constexpr int   fieldHeight        = 24;
    inline constexpr int   fieldPadding       = 8;
    inline constexpr int   fieldCorner        = 2;
    inline constexpr float fieldTextSize      = 13.0f;
    inline constexpr int   fieldArrowWidth    = 22;    // the two arrows at 13 px mono (board 21.33)
    inline constexpr int   rowToggleHeight    = 22;
    inline constexpr int   rowToggleGap       = 8;     // LED to text
    inline constexpr float rowToggleSpacing   = 0.06f;

    // --- Track strip (StageRevised .strip, Style page): top to bottom, gaps 6 ---
    inline constexpr int   stripPadding       = 6;
    inline constexpr int   stripGap           = 6;
    inline constexpr int   stripNumberHeight  = 12;
    inline constexpr int   stripNameWidth     = 84;
    inline constexpr int   stripNameHeight    = 20;
    inline constexpr float stripNameTextSize  = 12.0f;
    inline constexpr int   stripValueHeight   = 11;
    inline constexpr int   stripNotesHeight   = 18;
    inline constexpr int   stripNotesPadding  = 8;
    inline constexpr int   stripBorder        = 1;   // border-left
    inline constexpr int   stripHeight        = 257; // padding, 6 parts and their gaps
    inline constexpr int   tracksHeadingGap   = 6;   // tracks heading to the strips
    inline constexpr int   stylesColumnWidth  = 230; // Style page: styles column, gap, tracks
    inline constexpr int   stylesColumnGap    = 24;
}
