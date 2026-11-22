#include "ArrangerSectionSequencer.h"
#include "ArrangerTime.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int    firstSection = 0;
    constexpr int    noNumber     = 0;     // getTrailingIntValue() of a name without a number
    constexpr double oneBar       = 1.0;
    constexpr double noBeats      = 0.0;
    constexpr int    oneSection   = 1;
}

void ArrangerSectionSequencer::setStyle (const ArrangerStyle& style)
{
    sections.clear();
    for (const auto& section : style.sections)
        sections.push_back ({ section.id, section.name, section.type,
                              juce::jmax (ArrangerStyleDefaults::minimumLengthBars, section.lengthBars), section.afterComplete });

    beatsPerBar = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);
    reset();
}

int ArrangerSectionSequencer::firstVariationIndex() const
{
    for (int sectionIndex {}; sectionIndex < (int) sections.size(); ++sectionIndex)
        if (sections[sectionIndex].type == ArrangerSectionType::Variation)
            return sectionIndex;
    return sections.empty() ? ArrangerStyleDefaults::noIndex : firstSection;
}

int ArrangerSectionSequencer::lastSectionIndex() const
{
    return (int) sections.size() - oneSection;
}

int ArrangerSectionSequencer::findSection (ArrangerSectionType type, const juce::String& name) const
{
    // Live buttons send labels like "Var 2" / "Break"; editor sections are "Variation 2" / "Break 1".
    // Match by type + trailing number so the prefixes don't have to be identical; fall back to the
    // first section of the type (e.g. a single Break addressed as "Break").
    const int wantNum = name.getTrailingIntValue();   // "Var 2" -> 2, "Break" -> 0
    int firstOfType = ArrangerStyleDefaults::noIndex, exactName = ArrangerStyleDefaults::noIndex, numMatch = ArrangerStyleDefaults::noIndex;
    for (int sectionIndex {}; sectionIndex < (int) sections.size(); ++sectionIndex)
    {
        if (sections[sectionIndex].type != type) continue;
        if (firstOfType == ArrangerStyleDefaults::noIndex) firstOfType = sectionIndex;
        if (sections[sectionIndex].name == name) exactName = sectionIndex;
        if (wantNum > noNumber && sections[sectionIndex].name.getTrailingIntValue() == wantNum) numMatch = sectionIndex;
    }
    if (exactName != ArrangerStyleDefaults::noIndex) return exactName;
    if (numMatch  != ArrangerStyleDefaults::noIndex) return numMatch;
    return firstOfType;
}

void ArrangerSectionSequencer::reset()
{
    activeIndex    = juce::jmax (firstSection, firstVariationIndex());
    returnIndex    = activeIndex;
    activeStartAbs = ArrangerStyleDefaults::sectionStartBeat;
    pendingIndex   = ArrangerStyleDefaults::noIndex;
    stopped        = false;
    fillReturnOverride = ArrangerStyleDefaults::noIndex;
}

void ArrangerSectionSequencer::startAt (int sectionIndex)
{
    if (sections.empty()) { reset(); return; }
    activeIndex    = juce::jlimit (firstSection, lastSectionIndex(), sectionIndex);
    returnIndex    = juce::jmax (firstSection, firstVariationIndex());
    activeStartAbs = ArrangerStyleDefaults::sectionStartBeat;
    pendingIndex   = ArrangerStyleDefaults::noIndex;
    stopped        = false;
    fillReturnOverride = ArrangerStyleDefaults::noIndex;
}

bool ArrangerSectionSequencer::queue (ArrangerSectionType type, const juce::String& name)
{
    const int target = findSection (type, name);
    if (target == ArrangerStyleDefaults::noIndex) return false;

    // Auto Fill: when switching from one Variation to another, insert a matching Fill (Fill N for
    // Variation N, else any Fill) as a one-bar transition that then lands on the new variation.
    if (autoFillEnabled
        && type == ArrangerSectionType::Variation
        && target != activeIndex
        && activeIndex >= firstSection && activeIndex < (int) sections.size()
        && sections[activeIndex].type == ArrangerSectionType::Variation)
    {
        const int fill = findSection (ArrangerSectionType::Fill, name);  // matches the trailing number
        if (fill != ArrangerStyleDefaults::noIndex)
        {
            pendingIndex       = fill;
            fillReturnOverride = target;
            return true;
        }
    }

    pendingIndex       = target;
    fillReturnOverride = ArrangerStyleDefaults::noIndex;
    return true;
}

void ArrangerSectionSequencer::applyBoundary (double boundaryAbs, SequencerStep& step)
{
    // 1) A queued user switch always wins.
    if (pendingIndex != ArrangerStyleDefaults::noIndex)
    {
        const int from = activeIndex;
        if (sections[pendingIndex].type == ArrangerSectionType::Fill
            || sections[pendingIndex].type == ArrangerSectionType::Break)
        {
            if (fillReturnOverride != ArrangerStyleDefaults::noIndex)
                returnIndex = fillReturnOverride;   // auto-fill: land on the NEW variation after the fill
            // Otherwise remember the variation to return to after the fill/break one-shot completes.
            else if (sections[from].type == ArrangerSectionType::Variation
                     || sections[from].type == ArrangerSectionType::Break)
                returnIndex = from;
        }
        fillReturnOverride = ArrangerStyleDefaults::noIndex;
        activeIndex    = pendingIndex;
        pendingIndex   = ArrangerStyleDefaults::noIndex;
        activeStartAbs = boundaryAbs;
        return;
    }

    // 2) One-shot completion (Intro/Fill/Ending) when the section's own length has elapsed.
    const auto& active = sections[activeIndex];
    if (active.after == ArrangerAfterComplete::Loop)
        return;

    const double sectionLenBeats = (double) active.lengthBars * beatsPerBar;
    if (boundaryAbs - activeStartAbs < sectionLenBeats - BeatTolerance::wholeBars)
        return; // multi-bar one-shot not finished yet

    if (active.after == ArrangerAfterComplete::Stop)
    {
        stopped = true;
        step.stopRequested = true;
        return;
    }

    // FallThrough: Intro/Fill -> the saved return variation.
    activeIndex    = juce::jlimit (firstSection, lastSectionIndex(), returnIndex);
    activeStartAbs = boundaryAbs;
}

SequencerStep ArrangerSectionSequencer::advance (double fromBeats, double toBeats)
{
    SequencerStep step;
    if (sections.empty() || stopped || toBeats <= fromBeats || beatsPerBar <= noBeats)
        return step;

    double position = fromBeats;
    bool   startChanged = false;

    while (position < toBeats - BeatTolerance::windowEdge)
    {
        // Next global bar line strictly after position.
        double nextBoundary = (std::floor (position / beatsPerBar + BeatTolerance::sameBeat) + oneBar) * beatsPerBar;
        if (nextBoundary <= position + BeatTolerance::sameBeat)
            nextBoundary += beatsPerBar;

        const double segmentEnd = std::min (toBeats, nextBoundary);

        SectionSegment segment;
        segment.sectionIndex   = activeIndex;
        segment.localFromBeats = position - activeStartAbs;
        segment.localToBeats   = segmentEnd - activeStartAbs;
        segment.sectionChanged = startChanged;
        step.segments.push_back (segment);
        startChanged = false;

        position = segmentEnd;

        // Reached a bar boundary inside the window: apply transitions there.
        if (std::abs (segmentEnd - nextBoundary) < BeatTolerance::sameBeat && segmentEnd <= toBeats + BeatTolerance::windowEdge)
        {
            const int before = activeIndex;
            applyBoundary (nextBoundary, step);
            if (step.stopRequested)
                break;
            if (activeIndex != before)
                startChanged = true;
        }
    }

    return step;
}
