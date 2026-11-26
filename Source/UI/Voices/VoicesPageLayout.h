#pragma once
#include <JuceHeader.h>

/*  The Voices page (board StageRevised openPage=voices): three columns, each laid out by its own function so the
    block that owns the column (instrument panel, range fields, SFZ library) can lay itself out from its bounds.
    All functions take board px times the Stage scale; rects are in the coordinates of the rect passed in. */

struct VoicesPageColumns
{
    juce::Rectangle<int> instrument, range, sfz;
};

struct InstrumentColumnRects
{
    juce::Rectangle<int> heading, search, groupList, instrumentList;
};

struct RangeColumnRects
{
    juce::Rectangle<int> heading;
    juce::Rectangle<int> rangeLabel, startField, endField;
    juce::Rectangle<int> leftLabel, leftField, rightLabel, rightField;
    juce::Rectangle<int> transposeLabel, transposeField;
};

struct SfzColumnRects
{
    juce::Rectangle<int> heading, list, buttonRow;
};

/** Instrument 400 wide, range 250, the SFZ library the rest; 28 apart. */
VoicesPageColumns voicesPageColumns (juce::Rectangle<int> content, float scale);

/** Heading, search field, then the group list (140) and the instrument list (the rest), 8 apart. */
InstrumentColumnRects instrumentColumnLayout (juce::Rectangle<int> column, float scale);

/** Heading, then label / field pairs: start and end note side by side (6 apart), Left hand, Right hand, Transpose. */
RangeColumnRects rangeColumnLayout (juce::Rectangle<int> column, float scale);

/** Heading, the library list (five rows), the button row. */
SfzColumnRects sfzColumnLayout (juce::Rectangle<int> column, float scale);

namespace VoicesPageMetrics
{
    inline constexpr int instrumentColumnWidth = 400;
    inline constexpr int rangeColumnWidth      = 250;
    inline constexpr int columnGap             = 28;
    inline constexpr int blockGap              = 8;     // heading, search, lists ... (flex gap 8)
    inline constexpr int groupListWidth        = 140;
    inline constexpr int searchHeight          = 24;
    inline constexpr int voiceListRows         = 8;     // every group visible (the board's seven sample rows: 155)
    inline constexpr int sfzListRows           = 5;
    inline constexpr int labelHeight           = 12;
    inline constexpr int fieldPairGap          = 6;     // start / end note fields
    inline constexpr int buttonGap             = 6;     // Load SFZ... / Assign... / Import...
    inline constexpr int fieldPairs            = 2;
}
