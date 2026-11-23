/*
  ==============================================================================

    StageScale.h

    Splits the main window into the Stage regions (spec 2026-11-23 §4.1):
    panel, felt, note area, keys and key slip, top to bottom. Every fixed
    region is its reference height x scale, scale = min (width / 1200,
    height / 645); the note area takes the rest (never negative).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct StageRegions
{
    juce::Rectangle<int> panel, felt, noteArea, keys, slip;
    float scale {};
};

namespace StageScale
{
    StageRegions regionsFor (juce::Rectangle<int> bounds);
}
