/*
  ==============================================================================

    StageScale.h

    Splits the main window into the Stage regions (spec 2026-11-23 §4.1):
    panel, felt, note area, keys and key slip, top to bottom. Every fixed
    region is its reference height x scale, scale = min (width / 1200,
    height / 645); the note area takes the rest (never negative).

    The panel's contents (panelContent) are the 1200-wide board scaled by the
    same factor and centred in the panel, so they look the same on any window
    shape: a wider window only adds plain panel at both sides (owner,
    2026-11-23: "the UI should look the same on any resolution").

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct StageRegions
{
    juce::Rectangle<int> panel, felt, noteArea, keys, slip;
    juce::Rectangle<int> panelContent;   // where the panel's controls are laid out
    float scale {};
};

namespace StageScale
{
    StageRegions regionsFor (juce::Rectangle<int> bounds);
}
