/*
  ==============================================================================

    AppColours.h
    Created: 6 Apr 2026
    Author:  Kisuke

    Shared colour palette used across all themed windows
    (SoundEffectWindow, MIDIWindow, etc.)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace AppColours
{
    // --- Backgrounds ---
    static const juce::Colour background     { 0xff0f0f1a };   // very dark blue-black
    static const juce::Colour stylePanelTab  { 0xff0a0f0a };   // style panel tabs (RGB 10, 15, 10)
    static const juce::Colour settingsPanelFill   { 0xaa32645f };   // settings side panel (RGBA 50, 100, 95, 170)
    static const juce::Colour settingsPanelBorder { 0xc8a9a9a9 };   // its border (RGBA 169, 169, 169, 200)
    static const juce::Colour panelBg        { 0xff1a1a2e };   // dark indigo panel
    static const juce::Colour cardBg         { 0xff16213e };   // slightly lighter card

    // --- Text ---
    static const juce::Colour titleText      { 0xffe0e0ff };   // soft white-lavender
    static const juce::Colour subtitleText   { 0xff8888aa };   // muted label

    // --- Accents ---
    static const juce::Colour accent1        { 0xffff6f3c };   // warm orange
    static const juce::Colour accent2        { 0xff845ec2 };   // purple
    static const juce::Colour accent3        { 0xff00c9a7 };   // teal
    static const juce::Colour accent4        { 0xff4b7bec };   // blue
    static const juce::Colour accent5        { 0xfff9ca24 };   // golden yellow
    static const juce::Colour accent6        { 0xfffc5c65 };   // coral red
    static const juce::Colour accent7        { 0xff26de81 };   // emerald green
    static const juce::Colour accent8        { 0xffa55eea };   // violet

    // --- UI elements ---
    static const juce::Colour knobTrack      { 0xff2a2a3e };   // dark arc / slider track background
    static const juce::Colour separator      { 0xff2e2e44 };   // subtle divider
    static const juce::Colour rowLabel        { 0xff5e5e7e };   // row section label

    // --- Arranger configuration list ---
    static const juce::Colour configListBackground { 0xff181818 };   // near-black panel
    static const juce::Colour configSelectedRow    { 0xff2a6cc4 };   // clear blue highlight
    static const juce::Colour configActiveRow      { 0xff203a2b };   // subtle green wash
    static const juce::Colour configActiveMark     { 0xff5fd08a };   // check mark + "active" tag

    // --- Arranger timeline: one colour per section type ---
    static const juce::Colour sectionIntro     { 0xff4fc1ff };   // sky blue
    static const juce::Colour sectionVariation { 0xff6a9955 };   // moss green
    static const juce::Colour sectionFill      { 0xffdcdcaa };   // pale yellow
    static const juce::Colour sectionBreak     { 0xffc586c0 };   // orchid
    static const juce::Colour sectionEnding    { 0xffd16969 };   // brick red
    static const juce::Colour sectionCountIn   { 0xff808080 };   // grey
    static const juce::Colour timelineBackground { 0xff1e1e1e };
    static const juce::Colour timelineBarLine    { 0xff3a3a3a };
    static const juce::Colour timelineBarNumber  { 0xff777777 };
    static const juce::Colour styleEditorBackground { 0xff121212 };
    static const juce::Colour styleEditorBorder     { 0xff2a2a2a };

    // --- Sustain LED ---
    static const juce::Colour sustainOn      { 0xff00e676 };   // green LED on
    static const juce::Colour sustainOff     { 0xff3a3a4a };   // dim LED off
}

/** The Stage panel redesign (spec 2026-11-23 §3.1). Values are exact; alpha is the
    spec's percentage x 255, rounded. */
namespace StagePalette
{
    // --- Surfaces ---
    static const juce::Colour panelTop         { 0xff2a2b2e };
    static const juce::Colour panelBottom      { 0xff242528 };
    static const juce::Colour panelBottomEdge  { 0xff0b0b0c };
    static const juce::Colour pageTop          { 0xff26272a };
    static const juce::Colour pageBottom       { 0xff212225 };

    // --- Silkscreen ---
    static const juce::Colour silkscreenText    { 0xffc8cacd };
    static const juce::Colour silkscreenDimText { 0xffa3a5aa };
    static const juce::Colour ruleLine          { 0xff4a4c51 };

    // --- Rubber and lit buttons ---
    static const juce::Colour rubberTop       { 0xff3d3e42 };
    static const juce::Colour rubberBottom    { 0xff2c2d30 };
    static const juce::Colour rubberBorder    { 0xff111214 };
    static const juce::Colour rubberDropEdge  { 0xff0d0e0f };
    static const juce::Colour rubberHighlight { 0x17ffffff };   // white 9 %
    static const juce::Colour litTop          { 0xff56463a };
    static const juce::Colour litBottom       { 0xff3f3127 };
    static const juce::Colour litBorder       { 0xff2a1d14 };
    static const juce::Colour litText         { 0xffffe2cf };

    // --- LEDs (glow = the LED colour at 70 %) ---
    static const juce::Colour ledOff   { 0xff4a2a24 };
    static const juce::Colour ledRed   { 0xffff5a36 };
    static const juce::Colour ledGreen { 0xff5fd35a };
    static constexpr float ledGlowAlpha = 0.7f;

    // --- Screen ---
    static const juce::Colour screenBezel { 0xff101112 };
    static const juce::Colour lcd         { 0xffa7b394 };
    static const juce::Colour lcdInk      { 0xff1b2216 };
    static const juce::Colour lcdInputInk { 0xff3b4630 };   // .lcd-input: its text, a shade lighter than the lists'

    // --- Chord helper (PageChords) ---
    static const juce::Colour chordName     { 0xffe9e6de };
    static const juce::Colour miniKeyBed    { 0xff0b0b0c };
    static const juce::Colour miniWhite     { 0xfff6f4ee };
    static const juce::Colour miniWhiteEdge { 0x2e000000 };   // inset -1px 0 0 rgba(0,0,0,.18)
    static const juce::Colour miniWhiteLit  { 0xffe4a33a };
    static const juce::Colour miniBlack     { 0xff1a1a1c };
    static const juce::Colour miniBlackLit  { 0xffc27d14 };

    // --- Beat lamps ---
    static const juce::Colour lampOff            { 0xff2b2c2f };
    static const juce::Colour lampDownbeatTop    { 0xffff7a66 };
    static const juce::Colour lampDownbeatBottom { 0xffe3361f };
    static const juce::Colour lampDownbeatGlow   { 0x8cff5032 };   // rgba(255, 80, 50, .55)
    static const juce::Colour lampBeatTop        { 0xffffe77a };
    static const juce::Colour lampBeatBottom     { 0xfff0b70c };
    static const juce::Colour lampBeatGlow       { 0x80ffcd28 };   // rgba(255, 205, 40, .5)

    // --- Knobs ---
    static const juce::Colour knobSkirtDark    { 0xff18181a };
    static const juce::Colour knobSkirtLight   { 0xff2b2b2e };
    static const juce::Colour knobCapHighlight { 0xff55565a };
    static const juce::Colour knobCapMid       { 0xff2e2f32 };
    static const juce::Colour knobCapEdge      { 0xff1f2022 };
    static const juce::Colour knobPointer      { 0xfff2f2f0 };
    static const juce::Colour knobTick         { 0xff8d8f94 };

    // --- Felt, note area ---
    static const juce::Colour feltTop            { 0xff5e1a1d };
    static const juce::Colour feltBottom         { 0xff43121a };
    static const juce::Colour noteAreaBackground { 0xff050506 };
    static const juce::Colour octaveGuideLine    { 0xff15161a };
    static const juce::Colour fallingNote        { 0xff4f8f86 };
    static const juce::Colour slipTop            { 0xff1b1c1e };   // key slip under the keys
    static const juce::Colour slipBottom         { 0xff111213 };
    static const juce::Colour slipEdge           { 0xff000000 };

    // --- Home (board PageHome): the big Settings / Play / Exit keys ---
    static const juce::Colour bigKeyTop          { 0xff45464b };
    static const juce::Colour bigKeyBottom       { 0xff2c2d30 };
    static const juce::Colour bigKeyBorder       { 0xff111214 };
    static const juce::Colour bigKeyDrop         { 0xff0d0e0f };   // 0 4px 0: the key's side
    static const juce::Colour bigKeyShadow       { 0x80000000 };   // 0 6px 10px rgba(0,0,0,.5)
    static const juce::Colour bigKeyShine        { 0x1affffff };   // inset 0 1px 0 rgba(255,255,255,.1)
    static const juce::Colour bigKeyText         { 0xffd9dbde };
    static const juce::Colour playKeyTop         { 0xff5a4638 };
    static const juce::Colour playKeyBottom      { 0xff3f3127 };
    static const juce::Colour playKeyBorder      { 0xff2a1d14 };
    static const juce::Colour playKeyText        { 0xffffe2cf };
    static const juce::Colour linkText           { 0xffe0b25a };   // PageLogin .link ("Not you?")

    // --- Keys (StageRevised .wk / .wk.down / .bk; spec §7 for the pressed black key) ---
    static const juce::Colour keyBed             { 0xff0b0b0c };
    static const juce::Colour whiteKeyTop        { 0xffe9e6de };
    static const juce::Colour whiteKeyUpper      { 0xfff8f7f2 };   // at 12 %
    static const juce::Colour whiteKeyLower      { 0xfffbfaf6 };   // at 80 %
    static const juce::Colour whiteKeyBottom     { 0xffefece4 };
    static const juce::Colour whiteKeyLip        { 0xffd8d3c6 };
    static const juce::Colour whiteKeyEdge       { 0x1a000000 };   // inset -1px 0 0 rgba(0,0,0,.10)
    static const juce::Colour pressedWhiteTop    { 0xffcfcbc1 };
    static const juce::Colour pressedWhiteUpper  { 0xffe6e3da };   // at 14 %
    static const juce::Colour pressedWhiteLower  { 0xffeeebe3 };   // at 85 %
    static const juce::Colour pressedWhiteBottom { 0xffe4e0d6 };
    static const juce::Colour pressedWhiteLip    { 0xffcfc9bb };
    static const juce::Colour pressedWhiteEdge   { 0x24000000 };   // rgba(0,0,0,.14)
    static const juce::Colour pressedKeyShade    { 0x59000000 };   // inset 0 5px 6px -3px rgba(0,0,0,.35)
    static const juce::Colour blackKeyTop        { 0xff141415 };
    static const juce::Colour blackKeyBody       { 0xff1e1e20 };   // at 82 %
    static const juce::Colour blackKeySlopeTop   { 0xff3b3b3f };   // at 84 %: the lit front slope
    static const juce::Colour blackKeySlopeEnd   { 0xff2a2a2d };
    static const juce::Colour blackKeyBevelLeft  { 0xff2c2c2f };
    static const juce::Colour blackKeyBevelRight { 0xff070707 };
    static const juce::Colour blackKeyShadow     { 0x73000000 };   // 1px 3px 4px rgba(0,0,0,.45)
    static const juce::Colour pressedBlackTop    { 0xff0e0e0f };
    static const juce::Colour pressedBlackBody   { 0xff18181a };

    // --- Sections timeline ---
    static const juce::Colour timelineLanes         { 0xff121315 };
    static const juce::Colour timelineBarLine       { 0xff1e2024 };
    static const juce::Colour timelineBarLineStrong { 0xff2c2f34 };   // every 4th bar
    static const juce::Colour sectionIntro          { 0xffc99a3e };
    static const juce::Colour sectionVariation      { 0xff5f8f84 };
    static const juce::Colour sectionFill           { 0xffc8743f };
    static const juce::Colour sectionBreak          { 0xff8a8f99 };
    static const juce::Colour sectionEnding         { 0xffb0534a };
    static const juce::Colour sectionCountIn        { 0xff7a7aa8 };
    static const juce::Colour laneNote              { 0xff6f8f86 };   // the recording's notes in the lanes
    static const juce::Colour laneBarNumber         { 0xff7d8086 };
    static const juce::Colour windowLabel           { 0xfff3efe6 };   // window labels, selected outline, grips
    static const juce::Colour windowLabelShadow     { 0xcc000000 };   // text-shadow 0 1px 1px rgba(0,0,0,.8)
    static const juce::Colour transposeChipBorder   { 0xff3a3c41 };   // the lane header's transpose button
    static constexpr float windowFillAlpha         = 0.22f;   // type colour + 38 (hex)
    static constexpr float windowSelectedFillAlpha = 0.4f;    // type colour + 66 (hex)
    static constexpr float windowSelectedGlowAlpha = 0.35f;   // 0 0 0 1px rgba(243,239,230,.35)
    static constexpr float gripAlpha               = 0.9f;
    static constexpr float playheadGlowAlpha       = 0.35f;   // box-shadow 0 0 4px rgba(255,166,58,.7), spread over 4 px

    // --- Accent ---
    static const juce::Colour tempoAccent { 0xffffa63a };   // tempo and playhead

    // --- Page materials (StageRevised .strip / .fader*) ---
    static const juce::Colour stripBorder     { 0xff34363a };
    static const juce::Colour faderSlot       { 0xff0c0c0d };
    static const juce::Colour faderSlotShade  { 0xcc000000 };   // inset 0 1px 1px rgba(0,0,0,.8)
    static const juce::Colour faderSlotShine  { 0x0fffffff };   // 0 1px 0 rgba(255,255,255,.06)
    static const juce::Colour faderScale      { 0xff6c6e73 };
    static const juce::Colour faderCapTop     { 0xff5b5c60 };
    static const juce::Colour faderCapMid     { 0xff3a3b3e };
    static const juce::Colour faderCapGroove  { 0xff1c1d1f };
    static const juce::Colour faderCapBottom  { 0xff2c2d30 };
    static const juce::Colour faderCapShadow  { 0x99000000 };   // 0 3px 4px rgba(0,0,0,.6)
    static const juce::Colour faderCapShine   { 0x2effffff };   // inset 0 1px 0 rgba(255,255,255,.18)
    static const juce::Colour lcdInset        { 0x80000000 };   // inset 0 1px 3px rgba(0,0,0,.5)
}

/** Panel finishes (board SkinsFinish, redesign B4): the five optional surfaces, their side strips and trims.
    "Clear" colours are a stop's own colour at zero alpha, so a CSS "transparent" fades without going grey. */
namespace FinishPalette
{
    // Rosso burst: red lacquer, maple sides
    static const juce::Colour rossoCentre      { 0xffe2402b };
    static const juce::Colour rossoInner       { 0xffb81e1a };   // 38 %
    static const juce::Colour rossoOuter       { 0xff7a0f12 };   // 72 %
    static const juce::Colour rossoEdge        { 0xff3d0709 };
    static const juce::Colour lacquerShine     { 0x38ffffff };   // rgba(255,255,255,.22)
    static const juce::Colour lacquerShineSoft { 0x0affffff };   // .04
    static const juce::Colour clearWhite       { 0x00ffffff };
    static const juce::Colour clearBlack       { 0x00000000 };
    static const juce::Colour lacquerShadow    { 0x2e000000 };   // rgba(0,0,0,.18)
    static const juce::Colour mapleBase        { 0xffd9b382 };
    static const juce::Colour sideShadeLeft    { 0x40000000 };   // .25
    static const juce::Colour sideShadeMiddle  { 0x14ffffff };   // white .08
    static const juce::Colour sideShadeRight   { 0x4d000000 };   // .3

    // Champagne: brushed gold, walnut sides
    static const juce::Colour champagneTop     { 0xffe9d6a6 };
    static const juce::Colour champagneMiddle  { 0xffcfb47c };   // 45 %
    static const juce::Colour champagneBottom  { 0xffb89a62 };
    static const juce::Colour champagneSheen   { 0x59fff8e1 };   // rgba(255,248,225,.35)
    static const juce::Colour champagneClear   { 0x00fff8e1 };
    static const juce::Colour walnutBase       { 0xff5b3a24 };
    static const juce::Colour darkInk          { 0xff2b2416 };   // silkscreen on the light gold (7.7:1 on #cfb47c)
    static const juce::Colour darkInkDim       { 0xff5a4a30 };   // the dim silkscreen (4.3:1)
    static const juce::Colour walnutShadeLeft  { 0x4d000000 };   // .3
    static const juce::Colour walnutShadeMiddle { 0x0fffffff };  // white .06
    static const juce::Colour walnutShadeRight { 0x59000000 };   // .35

    // Tobacco burst: flamed maple sunburst, dark sides
    static const juce::Colour tobaccoBase      { 0xffe0a44a };
    static const juce::Colour tobaccoClear     { 0x006e2d0a };
    static const juce::Colour tobaccoBurst     { 0x8c6e2d0a };   // rgba(110,45,10,.55) at 58 %
    static const juce::Colour tobaccoDeep      { 0xf22d1206 };   // rgba(45,18,6,.95) at 85 %
    static const juce::Colour tobaccoEdge      { 0xff1c0b04 };
    static const juce::Colour tobaccoLight     { 0x40fff0d2 };   // rgba(255,240,210,.25)
    static const juce::Colour tobaccoLightClear { 0x00fff0d2 };
    static const juce::Colour tobaccoShadow    { 0x26000000 };   // .15
    static const juce::Colour tobaccoSideOuter { 0xff140c07 };
    static const juce::Colour tobaccoSideMiddle { 0xff2a1a10 };
    static const juce::Colour tobaccoSideInner { 0xff120a06 };

    // Midnight flake: navy metal flake, chrome trim, rosewood sides
    static const juce::Colour midnightCentre   { 0xff2a4a8a };
    static const juce::Colour midnightMiddle   { 0xff16295a };   // 45 %
    static const juce::Colour midnightEdge     { 0xff0a1430 };
    static const juce::Colour midnightShine    { 0x2effffff };   // .18
    static const juce::Colour midnightShadow   { 0x40000000 };   // .25
    static const juce::Colour chromeTop        { 0xfff4f6f8 };
    static const juce::Colour chromeMiddle     { 0xff9aa1a8 };   // 60 %
    static const juce::Colour chromeBottom     { 0xff5d646b };
    static const juce::Colour rosewoodBase     { 0xff4a2219 };

    // Concert black: piano gloss, gold pinstripe
    static const juce::Colour concertTop       { 0xff2a2a2d };
    static const juce::Colour concertMiddle    { 0xff0e0e10 };   // 35 %
    static const juce::Colour concertBottom    { 0xff060607 };
    static const juce::Colour glossShine       { 0x1affffff };   // .10 at 44 %
    static const juce::Colour glossShineSoft   { 0x05ffffff };   // .02 at 50 %
    static const juce::Colour goldClear        { 0x00c9a24e };
    static const juce::Colour goldEdge         { 0xffc9a24e };
    static const juce::Colour goldCentre       { 0xffe8cf8a };
    static const juce::Colour concertSideOuter { 0xff050505 };
    static const juce::Colour concertSideMiddle { 0xff1c1c1e };
    static const juce::Colour concertSideInner { 0xff060606 };
}
