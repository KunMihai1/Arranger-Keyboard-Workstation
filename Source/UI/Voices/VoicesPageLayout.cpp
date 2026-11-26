#include "VoicesPageLayout.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    constexpr float halves = 2.0f;

    float scaled (int boardLength, float scale)
    {
        return static_cast<float> (boardLength) * scale;
    }

    /** A list of rows with the LCD list's padding above and below. */
    int listHeight (int rows)
    {
        return StageMetrics::lcdListPadding + rows * StageMetrics::lcdRowHeight + StageMetrics::lcdListPadding;
    }
}

VoicesPageColumns voicesPageColumns (juce::Rectangle<int> content, float scale)
{
    using namespace VoicesPageMetrics;
    const auto area  = content.toFloat();
    const auto gap   = scaled (columnGap, scale);
    const auto first = scaled (instrumentColumnWidth, scale);
    const auto second = scaled (rangeColumnWidth, scale);
    VoicesPageColumns columns;
    columns.instrument = area.withWidth (first).toNearestInt();
    columns.range      = area.withX (area.getX() + first + gap).withWidth (second).toNearestInt();
    columns.sfz        = area.withLeft (area.getX() + first + gap + second + gap).toNearestInt();
    return columns;
}

InstrumentColumnRects instrumentColumnLayout (juce::Rectangle<int> column, float scale)
{
    using namespace VoicesPageMetrics;
    const auto area = column.toFloat();
    auto top = area.getY();
    InstrumentColumnRects rects;
    rects.heading = area.withY (top).withHeight (scaled (StageMetrics::headingHeight, scale)).toNearestInt();
    top += scaled (StageMetrics::headingHeight + blockGap, scale);
    rects.search = area.withY (top).withHeight (scaled (searchHeight, scale)).toNearestInt();
    top += scaled (searchHeight + blockGap, scale);
    const auto lists = area.withY (top).withHeight (scaled (listHeight (voiceListRows), scale));
    rects.groupList = lists.withWidth (scaled (groupListWidth, scale)).toNearestInt();
    rects.instrumentList = lists.withLeft (lists.getX() + scaled (groupListWidth + blockGap, scale)).toNearestInt();
    return rects;
}

RangeColumnRects rangeColumnLayout (juce::Rectangle<int> column, float scale)
{
    using namespace VoicesPageMetrics;
    const auto area = column.toFloat();
    auto top = area.getY();
    auto next = [&top, &area, scale] (int boardHeight)
    {
        const auto row = area.withY (top).withHeight (scaled (boardHeight, scale));
        top += scaled (boardHeight + blockGap, scale);
        return row;
    };

    RangeColumnRects rects;
    rects.heading    = next (StageMetrics::headingHeight).toNearestInt();
    rects.rangeLabel = next (labelHeight).toNearestInt();
    const auto pair  = next (StageMetrics::fieldHeight);
    const auto half  = (pair.getWidth() - scaled (fieldPairGap, scale)) / halves;
    rects.startField = pair.withWidth (half).toNearestInt();
    rects.endField   = pair.withLeft (pair.getRight() - half).toNearestInt();
    rects.leftLabel      = next (labelHeight).toNearestInt();
    rects.leftField      = next (StageMetrics::fieldHeight).toNearestInt();
    rects.rightLabel     = next (labelHeight).toNearestInt();
    rects.rightField     = next (StageMetrics::fieldHeight).toNearestInt();
    rects.transposeLabel = next (labelHeight).toNearestInt();
    rects.transposeField = next (StageMetrics::fieldHeight).toNearestInt();
    return rects;
}

SfzColumnRects sfzColumnLayout (juce::Rectangle<int> column, float scale)
{
    using namespace VoicesPageMetrics;
    const auto area = column.toFloat();
    auto top = area.getY();
    SfzColumnRects rects;
    rects.heading = area.withY (top).withHeight (scaled (StageMetrics::headingHeight, scale)).toNearestInt();
    top += scaled (StageMetrics::headingHeight + blockGap, scale);
    rects.list = area.withY (top).withHeight (scaled (listHeight (sfzListRows), scale)).toNearestInt();
    top += scaled (listHeight (sfzListRows) + blockGap, scale);
    rects.buttonRow = area.withY (top).withHeight (scaled (StageMetrics::segHeight, scale)).toNearestInt();
    return rects;
}
