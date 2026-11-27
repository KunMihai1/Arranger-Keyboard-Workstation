#include "SectionLanes.h"

namespace
{
    constexpr int   firstBar        = 1;
    constexpr int   oneBar          = 1;
    constexpr int   noLanes         = 0;
    constexpr int   noNotes         = 0;
    constexpr int   smallestWidth   = 1;
    constexpr float clear           = 0.0f;
    constexpr float faintestNote    = 0.35f;   // one note in a busy track still shows
    constexpr float fullyLit        = 1.0f;
    constexpr float zoomFactors[]   = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };   // index 3 = today's 56 px

    const char* const labelSeparatorUtf8 = " \xc2\xb7 ";   // " · "
    const juce::String barWord           = " bar";
    const juce::String barsWord          = " bars";
    const juce::String noneText          = "None";
    const juce::String parallelText      = "Parallel";
    const juce::String chordText         = "Chord";
    const juce::String fixedText         = "Fixed";

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }
}

namespace SectionLanes
{
    int laneCount (int trackCount) { return juce::jmax (SectionLaneMetrics::minimumLanes, trackCount); }

    juce::Rectangle<int> laneBounds (int laneIndex, int width, float scale)
    {
        using namespace SectionLaneMetrics;
        const int top    = scaled (rulerHeight + laneIndex * laneHeight, scale);
        const int bottom = scaled (rulerHeight + (laneIndex + oneBar) * laneHeight, scale);   // no gaps between scaled lanes
        return { int {}, top, width, bottom - top };
    }

    int contentHeight (int laneTotal, float scale)
    {
        using namespace SectionLaneMetrics;
        return scaled (rulerHeight + juce::jmax (noLanes, laneTotal) * laneHeight, scale);
    }

    std::vector<int> beatDensity (const std::vector<TimedBeatEvent>& events, int totalBeats)
    {
        std::vector<int> density (static_cast<size_t> (juce::jmax (int {}, totalBeats)), noNotes);
        for (const auto& event : events)
        {
            if (! event.message.isNoteOn())
                continue;
            const int beat = static_cast<int> (std::floor (event.beats));
            if (juce::isPositiveAndBelow (beat, totalBeats))
                ++density[static_cast<size_t> (beat)];
        }
        return density;
    }

    float densityAlpha (int notes, int busiestBeat)
    {
        if (notes <= noNotes || busiestBeat <= noNotes)
            return clear;
        const float share = static_cast<float> (juce::jmin (notes, busiestBeat)) / static_cast<float> (busiestBeat);
        return faintestNote + (fullyLit - faintestNote) * share;
    }

    bool isStrongBarLine (int bar) { return (bar - firstBar) % SectionLaneMetrics::strongLineEvery == int {}; }
    bool showsBarNumber (int bar)  { return (bar - firstBar) % SectionLaneMetrics::numberEvery == int {}; }

    juce::String windowLabel (const juce::String& name, int lengthBars)
    {
        return name + juce::String::fromUTF8 (labelSeparatorUtf8) + juce::String (lengthBars) + (lengthBars == oneBar ? barWord : barsWord);
    }

    NttType nextTransposeMode (NttType mode)
    {
        switch (mode)
        {
            case NttType::NoTranspose: return NttType::Parallel;
            case NttType::Parallel:    return NttType::Chord;
            case NttType::Chord:       return NttType::Fixed;
            case NttType::Fixed:       break;
        }
        return NttType::NoTranspose;
    }

    juce::String transposeModeText (NttType mode)
    {
        switch (mode)
        {
            case NttType::NoTranspose: return noneText;
            case NttType::Parallel:    return parallelText;
            case NttType::Chord:       return chordText;
            case NttType::Fixed:       break;
        }
        return fixedText;
    }

    int zoomLevelCount() { return static_cast<int> (std::size (zoomFactors)); }
    int clampZoomLevel (int level) { return juce::jlimit (int {}, zoomLevelCount() - oneBar, level); }
    float zoomFactor (int level) { return zoomFactors[static_cast<size_t> (clampZoomLevel (level))]; }

    int barWidth (int zoomLevel, float scale)
    {
        return juce::jmax (smallestWidth, juce::roundToInt (static_cast<float> (SectionLaneMetrics::baseBarWidth) * zoomFactor (zoomLevel) * scale));
    }
}
