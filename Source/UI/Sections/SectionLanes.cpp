#include "SectionLanes.h"
#include <deque>
#include <map>

namespace
{
    constexpr int   firstBar        = 1;
    constexpr int   oneBar          = 1;
    constexpr int   noLanes         = 0;
    constexpr int   smallestWidth   = 1;
    constexpr float middle          = 0.5f;   // a one-pitch track sits mid-lane
    constexpr float fullTravel      = 1.0f;
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

    std::vector<LaneNote> laneNotes (const std::vector<TimedBeatEvent>& events)
    {
        std::vector<LaneNote> notes;
        std::map<std::pair<int, int>, std::deque<size_t>> sounding;   // (channel, pitch) -> its unreleased notes, oldest first
        double lastBeat {};
        for (const auto& event : events)
        {
            lastBeat = juce::jmax (lastBeat, event.beats);
            const auto key = std::make_pair (event.message.getChannel(), event.message.getNoteNumber());
            if (event.message.isNoteOn())
            {
                sounding[key].push_back (notes.size());
                notes.push_back ({ event.beats, {}, event.message.getNoteNumber() });
            }
            else if (event.message.isNoteOff (true))   // velocity-0 note-ons count as releases
            {
                auto& open = sounding[key];
                if (open.empty())
                    continue;
                auto& note = notes[open.front()];
                note.lengthBeats = juce::jmax (double {}, event.beats - note.startBeat);
                open.pop_front();
            }
        }
        for (auto& [key, open] : sounding)
            for (const auto index : open)
                notes[index].lengthBeats = juce::jmax (double {}, lastBeat - notes[index].startBeat);   // held to the end

        std::stable_sort (notes.begin(), notes.end(), [] (const LaneNote& left, const LaneNote& right) { return left.startBeat < right.startBeat; });
        return notes;
    }

    PitchRange pitchRange (const std::vector<LaneNote>& notes)
    {
        if (notes.empty())
            return {};
        PitchRange range { notes.front().pitch, notes.front().pitch };
        for (const auto& note : notes)
        {
            range.low  = juce::jmin (range.low, note.pitch);
            range.high = juce::jmax (range.high, note.pitch);
        }
        return range;
    }

    float noteTop (int pitch, PitchRange range, juce::Rectangle<int> lane, float scale)
    {
        using namespace SectionLaneMetrics;
        const float bandTop = static_cast<float> (lane.getY()) + noteInsetY * scale;
        const float travel  = static_cast<float> (noteHeight - noteThickness) * scale;   // from the top note to the bottom one
        if (range.high <= range.low)
            return bandTop + travel * middle;
        const float height = static_cast<float> (juce::jlimit (range.low, range.high, pitch) - range.low) / static_cast<float> (range.high - range.low);
        return bandTop + travel * (fullTravel - height);
    }

    bool isStrongBarLine (int bar) { return (bar - firstBar) % SectionLaneMetrics::strongLineEvery == int {}; }
    bool showsBarNumber (int bar)  { return (bar - firstBar) % SectionLaneMetrics::numberEvery == int {}; }

    int labelStart (juce::Range<int> window, const std::vector<juce::Range<int>>& above)
    {
        int start = window.getStart();
        for (bool moved = true; moved && start < window.getEnd();)
        {
            moved = false;
            for (const auto& cover : above)
                if (cover.contains (start))
                {
                    start = cover.getEnd();   // past this cover; look again, another may begin there
                    moved = true;
                }
        }
        return start;
    }

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
