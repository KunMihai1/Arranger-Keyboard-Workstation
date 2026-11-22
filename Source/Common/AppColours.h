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
