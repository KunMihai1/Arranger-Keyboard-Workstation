#include "ArrangerScheduler.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
    constexpr int    silentVelocity = 0;     // a note-on at velocity 0 is a note-off
    constexpr size_t firstPosition  = 0;
    constexpr double noLoop         = 0.0;
}

void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats)
{
    setLoop (std::move (events), {}, {}, loopLengthBeats);   // legacy: all Fixed / Chord
}

void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events,
                                 std::vector<PartKind> parts, double loopLengthBeats)
{
    setLoop (std::move (events), std::move (parts), {}, loopLengthBeats);
}

void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events,
                                 std::vector<PartKind> parts, std::vector<NttType> ntts,
                                 double loopLengthBeats)
{
    // Sort events and their parallel part/ntt tags together by beat.
    std::vector<size_t> beatOrder (events.size());
    std::iota (beatOrder.begin(), beatOrder.end(), firstPosition);
    std::stable_sort (beatOrder.begin(), beatOrder.end(),
                      [&] (size_t earlierIndex, size_t laterIndex) { return events[earlierIndex].beats < events[laterIndex].beats; });

    sortedEvents.clear();
    sortedParts.clear();
    sortedNtt.clear();
    sortedEvents.reserve (events.size());
    sortedParts.reserve (events.size());
    sortedNtt.reserve (events.size());
    for (size_t eventIndex : beatOrder)
    {
        sortedEvents.push_back (events[eventIndex]);
        sortedParts.push_back (eventIndex < parts.size() ? parts[eventIndex] : PartKind::Fixed);
        sortedNtt.push_back   (eventIndex < ntts.size()  ? ntts[eventIndex]  : NttType::Chord);
    }

    loopLen = loopLengthBeats;
    activeNotes.clear();
    activeNoteParts.clear();
    activeNoteNtt.clear();
}

void ArrangerScheduler::reset()
{
    activeNotes.clear();
    activeNoteParts.clear();
    activeNoteNtt.clear();
}

std::vector<EmittedEvent> ArrangerScheduler::flushActiveNotes (double atBeats)
{
    std::vector<EmittedEvent> result;
    for (const auto& key : activeNotes)
    {
        const auto partEntry = activeNoteParts.find (key);
        const auto nttEntry  = activeNoteNtt.find (key);
        const PartKind part = (partEntry != activeNoteParts.end()) ? partEntry->second : PartKind::Fixed;
        const NttType  ntt  = (nttEntry  != activeNoteNtt.end())   ? nttEntry->second  : NttType::Chord;
        result.push_back ({ atBeats, juce::MidiMessage::noteOff (key.first, key.second), part, ntt });
    }
    activeNotes.clear();
    activeNoteParts.clear();
    activeNoteNtt.clear();
    return result;
}

void ArrangerScheduler::trackActiveNote (const juce::MidiMessage& message, PartKind part, NttType ntt)
{
    const auto key = std::make_pair (message.getChannel(), message.getNoteNumber());
    if (message.isNoteOn() && message.getVelocity() > silentVelocity)
    {
        activeNotes.insert (key);
        activeNoteParts[key] = part;
        activeNoteNtt[key]   = ntt;
    }
    else if (message.isNoteOff() || (message.isNoteOn() && message.getVelocity() == silentVelocity))
    {
        activeNotes.erase (key);
        activeNoteParts.erase (key);
        activeNoteNtt.erase (key);
    }
}

std::vector<EmittedEvent> ArrangerScheduler::advance (double fromBeats, double toBeats)
{
    std::vector<EmittedEvent> result;
    if (loopLen <= noLoop || toBeats <= fromBeats)
        return result;

    double position = fromBeats;
    while (position < toBeats - BeatTolerance::windowEdge)
    {
        const double iterationIndex    = std::floor (position / loopLen);
        const double iterationStartAbs = iterationIndex * loopLen;
        const double nextWrapAbs       = iterationStartAbs + loopLen;
        const double phaseStart        = position - iterationStartAbs;
        const double segmentEndAbs     = std::min (toBeats, nextWrapAbs);
        const double phaseEnd          = segmentEndAbs - iterationStartAbs;

        for (size_t eventIndex {}; eventIndex < sortedEvents.size(); ++eventIndex)
        {
            const auto& event = sortedEvents[eventIndex];
            if (event.beats >= phaseStart - BeatTolerance::windowEdge && event.beats < phaseEnd - BeatTolerance::windowEdge)
            {
                const PartKind part = sortedParts[eventIndex];
                const NttType  ntt  = sortedNtt[eventIndex];
                result.push_back ({ iterationStartAbs + event.beats, event.message, part, ntt });
                trackActiveNote (event.message, part, ntt);
            }
        }

        // If we actually reached a seam within the window, close hanging notes there.
        if (std::abs (segmentEndAbs - nextWrapAbs) < BeatTolerance::sameBeat && nextWrapAbs <= toBeats + BeatTolerance::windowEdge)
        {
            for (const auto& key : activeNotes)
            {
                const auto partEntry = activeNoteParts.find (key);
                const auto nttEntry  = activeNoteNtt.find (key);
                const PartKind part = (partEntry != activeNoteParts.end()) ? partEntry->second : PartKind::Fixed;
                const NttType  ntt  = (nttEntry  != activeNoteNtt.end())   ? nttEntry->second  : NttType::Chord;
                result.push_back ({ nextWrapAbs, juce::MidiMessage::noteOff (key.first, key.second), part, ntt });
            }
            activeNotes.clear();
            activeNoteParts.clear();
            activeNoteNtt.clear();
        }

        position = segmentEndAbs;
    }

    return result;
}
