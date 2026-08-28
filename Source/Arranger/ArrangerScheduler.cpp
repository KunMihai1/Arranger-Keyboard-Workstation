#include "ArrangerScheduler.h"
#include <algorithm>
#include <cmath>
#include <numeric>

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
    std::vector<size_t> idx (events.size());
    std::iota (idx.begin(), idx.end(), (size_t) 0);
    std::stable_sort (idx.begin(), idx.end(),
                      [&] (size_t a, size_t b) { return events[a].beats < events[b].beats; });

    sortedEvents.clear();
    sortedParts.clear();
    sortedNtt.clear();
    sortedEvents.reserve (events.size());
    sortedParts.reserve (events.size());
    sortedNtt.reserve (events.size());
    for (size_t i : idx)
    {
        sortedEvents.push_back (events[i]);
        sortedParts.push_back (i < parts.size() ? parts[i] : PartKind::Fixed);
        sortedNtt.push_back   (i < ntts.size()  ? ntts[i]  : NttType::Chord);
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
        const auto it  = activeNoteParts.find (key);
        const auto itN = activeNoteNtt.find (key);
        const PartKind part = (it  != activeNoteParts.end()) ? it->second  : PartKind::Fixed;
        const NttType  ntt  = (itN != activeNoteNtt.end())   ? itN->second : NttType::Chord;
        result.push_back ({ atBeats, juce::MidiMessage::noteOff (key.first, key.second), part, ntt });
    }
    activeNotes.clear();
    activeNoteParts.clear();
    activeNoteNtt.clear();
    return result;
}

void ArrangerScheduler::trackActiveNote (const juce::MidiMessage& m, PartKind part, NttType ntt)
{
    const auto key = std::make_pair (m.getChannel(), m.getNoteNumber());
    if (m.isNoteOn() && m.getVelocity() > 0)
    {
        activeNotes.insert (key);
        activeNoteParts[key] = part;
        activeNoteNtt[key]   = ntt;
    }
    else if (m.isNoteOff() || (m.isNoteOn() && m.getVelocity() == 0))
    {
        activeNotes.erase (key);
        activeNoteParts.erase (key);
        activeNoteNtt.erase (key);
    }
}

std::vector<EmittedEvent> ArrangerScheduler::advance (double fromBeats, double toBeats)
{
    std::vector<EmittedEvent> result;
    if (loopLen <= 0.0 || toBeats <= fromBeats)
        return result;

    double pos = fromBeats;
    while (pos < toBeats - 1e-12)
    {
        const double iterationIndex    = std::floor (pos / loopLen);
        const double iterationStartAbs = iterationIndex * loopLen;
        const double nextWrapAbs       = iterationStartAbs + loopLen;
        const double phaseStart        = pos - iterationStartAbs;
        const double segmentEndAbs     = std::min (toBeats, nextWrapAbs);
        const double phaseEnd          = segmentEndAbs - iterationStartAbs;

        for (size_t i = 0; i < sortedEvents.size(); ++i)
        {
            const auto& ev = sortedEvents[i];
            if (ev.beats >= phaseStart - 1e-12 && ev.beats < phaseEnd - 1e-12)
            {
                const PartKind part = sortedParts[i];
                const NttType  ntt  = sortedNtt[i];
                result.push_back ({ iterationStartAbs + ev.beats, ev.message, part, ntt });
                trackActiveNote (ev.message, part, ntt);
            }
        }

        // If we actually reached a seam within the window, close hanging notes there.
        if (std::abs (segmentEndAbs - nextWrapAbs) < 1e-9 && nextWrapAbs <= toBeats + 1e-12)
        {
            for (const auto& key : activeNotes)
            {
                const auto it  = activeNoteParts.find (key);
                const auto itN = activeNoteNtt.find (key);
                const PartKind part = (it  != activeNoteParts.end()) ? it->second  : PartKind::Fixed;
                const NttType  ntt  = (itN != activeNoteNtt.end())   ? itN->second : NttType::Chord;
                result.push_back ({ nextWrapAbs, juce::MidiMessage::noteOff (key.first, key.second), part, ntt });
            }
            activeNotes.clear();
            activeNoteParts.clear();
            activeNoteNtt.clear();
        }

        pos = segmentEndAbs;
    }

    return result;
}
