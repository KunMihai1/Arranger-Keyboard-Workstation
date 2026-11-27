#pragma once
#include <JuceHeader.h>
#include "Arranger/ArrangerStyleFile.h"
#include <vector>

/*  The Sections page's inspector edits (board PageSections, "Selected section"; spec 2026-11-27 sections v2):
    the same changes a drag makes, from ◂ ▸ arrows — clamped the way the timeline clamps a drag (at least one
    bar, ending by the bar line after the last bar) — plus the type and "When it ends". Pure: the editor applies
    the result through its usual windows-changed path (rebuild the preview, rest the playhead). */

namespace SectionEditModel
{
    inline constexpr int earlier = -1;   ///< an arrow's direction: ◂
    inline constexpr int later   = 1;    ///< ▸

    /** The types the Add buttons offer, in their order (Count-in is not a section yet: spec). */
    const std::vector<ArrangerSectionType>& editableTypes();

    /** "<Type> n" for a new section of `type`: n = the highest number of that type + 1 (gaps kept). */
    juce::String nextName (const std::vector<SectionWindow>& windows, ArrangerSectionType type);

    /** Each returns true when window `index` changed. */
    bool stepType (std::vector<SectionWindow>& windows, int index, int direction);
    bool stepStart (std::vector<SectionWindow>& windows, int index, int direction, int totalBars);
    bool stepLength (std::vector<SectionWindow>& windows, int index, int direction, int totalBars);
    bool setAfterComplete (std::vector<SectionWindow>& windows, int index, ArrangerAfterComplete afterComplete);

    /** "Var", "Intro", … as the inspector shows a type; "1 bar" / "8 bars" for a length. */
    juce::String typeText (ArrangerSectionType type);
    juce::String lengthText (int lengthBars);
}
