#include "SectionsPageLayout.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    constexpr int twice = 2;

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }
}

SectionsPageRects sectionsPageLayout (juce::Rectangle<int> content, float scale, int lanesHeight)
{
    using namespace SectionsPageMetrics;
    SectionsPageRects rects;

    auto area = content;
    rects.leftColumn  = area.removeFromLeft (scaled (leftColumnWidth, scale));
    rects.rightColumn = area.removeFromRight (scaled (rightColumnWidth, scale));
    area.removeFromLeft (scaled (columnGap, scale));
    area.removeFromRight (scaled (columnGap, scale));
    rects.centre = area.withHeight (lanesHeight);
    rects.pads   = area.withTrimmedTop (lanesHeight + scaled (padsGap, scale)).withHeight (scaled (padsHeight, scale));

    auto left = rects.leftColumn;
    rects.configHeading = left.removeFromTop (scaled (headingHeight, scale));
    left.removeFromTop (scaled (itemGap, scale));
    rects.configList = left.removeFromTop (scaled (listRowsShown * StageMetrics::lcdRowHeight + twice * listPaddingY, scale));
    left.removeFromTop (scaled (itemGap, scale));
    rects.configButtons = left.removeFromTop (scaled (buttonHeight, scale));
    left.removeFromTop (scaled (itemGap + addHeadingMargin, scale));
    rects.addHeading = left.removeFromTop (scaled (headingHeight, scale));
    left.removeFromTop (scaled (itemGap, scale));

    // The 2-column grid of Add buttons; widths share the column exactly (the right column ends flush).
    const int gap = scaled (buttonGap, scale);
    const int columnWidth = (rects.leftColumn.getWidth() - gap) / addColumns;
    for (size_t index {}; index < rects.addButtons.size(); ++index)
    {
        const int row    = static_cast<int> (index) / addColumns;
        const int column = static_cast<int> (index) % addColumns;
        const int top    = left.getY() + row * (scaled (buttonHeight, scale) + gap);
        const int buttonLeft = column == int {} ? rects.leftColumn.getX() : rects.leftColumn.getRight() - columnWidth;
        rects.addButtons[index] = { buttonLeft, top, columnWidth, scaled (buttonHeight, scale) };
    }
    return rects;
}
