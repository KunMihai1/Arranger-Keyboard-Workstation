#include "InstrumentPicker.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace Text
    {
        const char* const  searchHintUtf8 = "Search instruments\xe2\x80\xa6";   // + U+2026
        const juce::String families    = "Families";
        const juce::String instruments = "Instruments";
        const juce::String foundSuffix = " found";
        const juce::String nothing     = "Nothing found";
        const char* const  currentMarkUtf8 = "\xe2\x97\x82";   // U+25C2: the program the track plays now
    }

    // Board px at scale 1 (the Voices page's search and LCD lists).
    constexpr int searchWidth       = 400;
    constexpr int searchHeight      = 24;
    constexpr int sectionGap        = 12;
    constexpr int headingToList     = 6;
    constexpr int listInset         = 4;
    constexpr int visibleRows       = 8;
    constexpr int familyColumnWidth = 180;
    constexpr int familyColumns     = 2;
    constexpr int listGap           = 24;
    constexpr int insetSides        = 2;     // the list inset above and below / left and right
    constexpr int oneRow            = 1;
    constexpr int noScroll          = 0;

    int scaled (int boardLength, float scale) { return juce::roundToInt (static_cast<float> (boardLength) * scale); }
}

const juce::StringArray& GmInstruments::families()
{
    static const juce::StringArray names { "Piano", "Chromatic Perc.", "Organ", "Guitar", "Bass", "Strings", "Ensemble", "Brass",
                                           "Reed", "Pipe", "Synth Lead", "Synth Pad", "Synth Effects", "Ethnic", "Percussive",
                                           "Sound Effects" };
    return names;
}

int GmInstruments::familyOf (int program)
{
    return juce::jlimit (int {}, families().size() - oneRow, program / programsPerFamily);
}

std::vector<int> GmInstruments::search (const juce::String& text)
{
    std::vector<int> found;
    const auto query = text.trim();
    if (query.isEmpty())
        return found;
    const auto& names = getGMInstrumentNames();
    for (int program {}; program < names.size(); ++program)
        if (names[program].containsIgnoreCase (query))
            found.push_back (program);
    return found;
}

InstrumentPickerRects instrumentPickerLayout (juce::Rectangle<int> content, float scale)
{
    using namespace StageMetrics;
    InstrumentPickerRects rects;
    auto area = content;
    rects.search = area.removeFromTop (scaled (searchHeight, scale)).withWidth (scaled (searchWidth, scale));
    area.removeFromTop (scaled (sectionGap, scale));

    auto headings = area.removeFromTop (scaled (headingHeight, scale));
    area.removeFromTop (scaled (headingToList, scale));
    const auto listHeight = scaled (visibleRows * lcdRowHeight + listInset * insetSides, scale);
    auto lists = area.removeFromTop (listHeight);

    const auto familyWidth = scaled (familyColumnWidth * familyColumns + listInset * insetSides, scale);
    rects.familyHeading = headings.removeFromLeft (familyWidth);
    rects.familyList    = lists.removeFromLeft (familyWidth);
    headings.removeFromLeft (scaled (listGap, scale));
    lists.removeFromLeft (scaled (listGap, scale));
    rects.instrumentHeading = headings;
    rects.instrumentList    = lists;
    return rects;
}

InstrumentPicker::InstrumentPicker (int program)
    : currentProgram (program), searchBox (juce::String::fromUTF8 (Text::searchHintUtf8))
{
    selectedFamily = program >= int {} ? GmInstruments::familyOf (program) : int {};
    searchBox.onTextChange = [this] { refreshShown(); };
    searchBox.onEscapeKey  = [this] { setSearchText ({}); };
    addAndMakeVisible (searchBox);
    refreshShown();
}

void InstrumentPicker::setScale (float newScale)
{
    scale = newScale;
    searchBox.setScale (scale);
    resized();
    repaint();
}

void InstrumentPicker::setSearchText (const juce::String& text)
{
    searchBox.setText (text, false);
    refreshShown();
}

juce::String InstrumentPicker::getSearchText() const { return searchBox.getText(); }

void InstrumentPicker::selectFamily (int family)
{
    selectedFamily = juce::jlimit (int {}, GmInstruments::families().size() - oneRow, family);
    setSearchText ({});
}

void InstrumentPicker::chooseRow (int row)
{
    const auto index = firstShownRow + row;
    if (juce::isPositiveAndBelow (index, static_cast<int> (shown.size())) && onChosen)
        onChosen (shown[static_cast<size_t> (index)]);
}

int InstrumentPicker::getSelectedFamily() const { return selectedFamily; }
std::vector<int> InstrumentPicker::getShownPrograms() const { return shown; }

void InstrumentPicker::refreshShown()
{
    firstShownRow = noScroll;
    if (getSearchText().trim().isNotEmpty())
    {
        shown = GmInstruments::search (getSearchText());
    }
    else
    {
        shown.clear();
        for (int offset {}; offset < GmInstruments::programsPerFamily; ++offset)
            shown.push_back (selectedFamily * GmInstruments::programsPerFamily + offset);
    }
    repaint();
}

juce::Rectangle<int> InstrumentPicker::familyCell (const InstrumentPickerRects& rects, int family) const
{
    const auto inner  = rects.familyList.reduced (scaled (listInset, scale));
    const auto column = family / visibleRows;
    const auto row    = family % visibleRows;
    const auto width  = inner.getWidth() / familyColumns;
    const auto height = scaled (StageMetrics::lcdRowHeight, scale);
    return { inner.getX() + column * width, inner.getY() + row * height, width, height };
}

juce::Rectangle<int> InstrumentPicker::instrumentRow (const InstrumentPickerRects& rects, int row) const
{
    const auto inner  = rects.instrumentList.reduced (scaled (listInset, scale));
    const auto height = scaled (StageMetrics::lcdRowHeight, scale);
    return { inner.getX(), inner.getY() + row * height, inner.getWidth(), height };
}

void InstrumentPicker::paint (juce::Graphics& g)
{
    const auto rects = instrumentPickerLayout (getLocalBounds(), scale);
    const bool searching = getSearchText().trim().isNotEmpty();
    const auto& families = GmInstruments::families();
    const auto& names = getGMInstrumentNames();

    StageLcd::paintHeading (g, rects.familyHeading.toFloat(), scale, Text::families, {});
    const auto results = searching ? (shown.empty() ? Text::nothing : juce::String (static_cast<int> (shown.size())) + Text::foundSuffix)
                                   : families[selectedFamily];
    StageLcd::paintHeading (g, rects.instrumentHeading.toFloat(), scale, Text::instruments, results);

    StageLcd::paintPanel (g, rects.familyList.toFloat(), scale);
    for (int family {}; family < families.size(); ++family)
        StageLcd::paintRow (g, familyCell (rects, family), scale, families[family], {}, ! searching && family == selectedFamily);

    StageLcd::paintPanel (g, rects.instrumentList.toFloat(), scale);
    for (int row {}; row < visibleRows; ++row)
    {
        const auto index = firstShownRow + row;
        if (! juce::isPositiveAndBelow (index, static_cast<int> (shown.size())))
            break;
        const auto program = shown[static_cast<size_t> (index)];
        const bool current = program == currentProgram;
        const auto mark = current ? juce::String::fromUTF8 (Text::currentMarkUtf8)
                                  : (searching ? families[GmInstruments::familyOf (program)] : juce::String());
        StageLcd::paintRow (g, instrumentRow (rects, row), scale, names[program], mark, current);
    }
}

void InstrumentPicker::resized()
{
    searchBox.setBounds (instrumentPickerLayout (getLocalBounds(), scale).search);
}

void InstrumentPicker::mouseDown (const juce::MouseEvent& event)
{
    const auto rects = instrumentPickerLayout (getLocalBounds(), scale);
    const auto point = event.getPosition();
    for (int family {}; family < GmInstruments::families().size(); ++family)
        if (familyCell (rects, family).contains (point))
        {
            selectFamily (family);
            return;
        }
    for (int row {}; row < visibleRows; ++row)
        if (instrumentRow (rects, row).contains (point))
        {
            chooseRow (row);
            return;
        }
}

void InstrumentPicker::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const auto lastFirst = juce::jmax (noScroll, static_cast<int> (shown.size()) - visibleRows);
    const auto step = wheel.deltaY > float {} ? -oneRow : (wheel.deltaY < float {} ? oneRow : noScroll);
    firstShownRow = juce::jlimit (noScroll, lastFirst, firstShownRow + step);
    repaint();
}
