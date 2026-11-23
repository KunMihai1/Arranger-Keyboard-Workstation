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
