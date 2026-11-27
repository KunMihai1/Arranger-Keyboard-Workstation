#pragma once
#include <JuceHeader.h>
#include <array>

/*  Where the Sections page's parts go (board PageSections, spec 2026-11-27 sections v2): the configurations
    column at the left (list, its buttons, Add section), the lanes in the middle with the jump pads under
    them, the selected section's inspector at the right. Board px × the Stage scale, in the page content's
    coordinates; pure, so it is unit-tested. */

namespace SectionsPageMetrics
{
    inline constexpr int leftColumnWidth   = 170;
    inline constexpr int rightColumnWidth  = 206;
    inline constexpr int columnGap         = 14;
    inline constexpr int headingHeight     = 12;
    inline constexpr int itemGap           = 6;
    inline constexpr int listRowsShown     = 5;    // the list scrolls past five configurations
    inline constexpr int listPaddingY      = 3;
    inline constexpr int buttonHeight      = 22;
    inline constexpr int buttonGap         = 4;
    inline constexpr int addHeadingMargin  = 8;    // "Add section" sits a little apart from the list
    inline constexpr int addColumns        = 2;
    inline constexpr int addSectionCount   = 5;    // Intro, Var, Fill, Break, Ending (no Count-in: spec)
    inline constexpr int padsGap           = 6;    // lanes to the jump pads
    inline constexpr int padsHeight        = 44;   // LED, key, name
}

struct SectionsPageRects
{
    juce::Rectangle<int> leftColumn, configHeading, configList, configButtons, addHeading;
    std::array<juce::Rectangle<int>, SectionsPageMetrics::addSectionCount> addButtons;
    juce::Rectangle<int> centre;       ///< lane headers + lanes, then the jump pads
    juce::Rectangle<int> pads;
    juce::Rectangle<int> rightColumn;  ///< the selected section's inspector
};

/** `lanesHeight`: the lanes' own height (ruler, lanes and the scroll bar), already scaled. */
SectionsPageRects sectionsPageLayout (juce::Rectangle<int> content, float scale, int lanesHeight);
