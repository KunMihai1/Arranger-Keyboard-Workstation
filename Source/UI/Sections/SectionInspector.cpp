#include "SectionInspector.h"
#include "SectionEditModel.h"
#include "SectionsPageLayout.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    namespace InspectorText
    {
        const juce::String heading    = "Selected section";
        const juce::String whenItEnds = "When it ends";
        const juce::String loop       = "Loop";
        const juce::String goOn       = "Go on";
        const juce::String stop       = "Stop";
        const juce::String playThis   = "Play this";
        const juce::String remove     = "Remove";
        const juce::String name       = "NAME";
        const juce::String type       = "TYPE";
        const juce::String startBar   = "START BAR";
        const juce::String length     = "LENGTH";
        const char* const dashUtf8      = "\xe2\x80\x94";   // "—"
        const char* const earlierUtf8   = "\xe2\x97\x82";   // "◂"
        const char* const laterUtf8     = "\xe2\x96\xb8";   // "▸"
    }

    // Board px (PageSections "Selected section").
    constexpr int   lcdPaddingX     = 10;
    constexpr int   lcdPaddingY     = 6;
    constexpr int   lcdRowHeight    = 15;
    constexpr int   lcdRowGap       = 4;
    constexpr int   lcdRows         = 4;
    constexpr float lcdTextSize     = 13.0f;
    constexpr int   arrowPaddingX   = 3;     // .arrow padding 0 3px
    constexpr int   playRowMargin   = 4;     // margin-top on Play this / Remove
    constexpr int   nameRow         = 0;
    constexpr int   typeRow         = 1;
    constexpr int   startRow        = 2;
    constexpr int   lengthRow       = 3;
    constexpr int   oneFewer        = 1;
    constexpr int   twice           = 2;

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }

    int rowOf (InspectorArrow arrow)
    {
        switch (arrow)
        {
            case InspectorArrow::typeEarlier:   case InspectorArrow::typeLater:    return typeRow;
            case InspectorArrow::startEarlier:  case InspectorArrow::startLater:   return startRow;
            case InspectorArrow::lengthShorter: case InspectorArrow::lengthLonger: break;
        }
        return lengthRow;
    }

    bool isLater (InspectorArrow arrow)
    {
        return arrow == InspectorArrow::typeLater || arrow == InspectorArrow::startLater || arrow == InspectorArrow::lengthLonger;
    }

    constexpr size_t arrowCount = 6;
    const std::array<InspectorArrow, arrowCount> allArrows { InspectorArrow::typeEarlier, InspectorArrow::typeLater, InspectorArrow::startEarlier,
                                                    InspectorArrow::startLater, InspectorArrow::lengthShorter, InspectorArrow::lengthLonger };
}

SectionInspector::SectionInspector()
    : heading (InspectorText::heading, SilkLabel::Tone::bright), whenItEnds (InspectorText::whenItEnds, SilkLabel::Tone::dim),
      loop (InspectorText::loop, SegButton::Style::seg, true), goOn (InspectorText::goOn, SegButton::Style::seg, true),
      stop (InspectorText::stop, SegButton::Style::seg, true),
      playThis (InspectorText::playThis, SegButton::Style::seg), remove (InspectorText::remove, SegButton::Style::seg)
{
    addAndMakeVisible (heading);
    addAndMakeVisible (whenItEnds);
    for (auto* button : { &loop, &goOn, &stop, &playThis, &remove })
    {
        button->setMouseClickGrabsKeyboardFocus (false);   // the PC keyboard keeps playing after a press
        addAndMakeVisible (button);
    }
    loop.onClick     = [this] { if (onAfterComplete) onAfterComplete (ArrangerAfterComplete::Loop); };
    goOn.onClick     = [this] { if (onAfterComplete) onAfterComplete (ArrangerAfterComplete::FallThrough); };
    stop.onClick     = [this] { if (onAfterComplete) onAfterComplete (ArrangerAfterComplete::Stop); };
    playThis.onClick = [this] { if (onPlayThis) onPlayThis(); };
    remove.onClick   = [this] { if (onRemove) onRemove(); };
    setSection (std::nullopt);
}

void SectionInspector::setSection (std::optional<SectionWindow> window)
{
    section = std::move (window);
    const bool has = section.has_value();
    for (auto* button : { &loop, &goOn, &stop, &playThis, &remove })
        button->setEnabled (has);
    loop.setLedOn (has && section->afterComplete == ArrangerAfterComplete::Loop);
    goOn.setLedOn (has && section->afterComplete == ArrangerAfterComplete::FallThrough);
    stop.setLedOn (has && section->afterComplete == ArrangerAfterComplete::Stop);
    repaint();
}

void SectionInspector::setScale (float newScale)
{
    scale = newScale;
    heading.setScale (scale);
    whenItEnds.setScale (scale);
    for (auto* button : { &loop, &goOn, &stop, &playThis, &remove })
        button->setScale (scale);
    resized();
    repaint();
}

SegButton& SectionInspector::getAfterButton (ArrangerAfterComplete afterComplete)
{
    switch (afterComplete)
    {
        case ArrangerAfterComplete::Loop:        return loop;
        case ArrangerAfterComplete::FallThrough: return goOn;
        case ArrangerAfterComplete::Stop:        break;
    }
    return stop;
}

juce::Font SectionInspector::lcdFont() const { return StageFonts::lcd (lcdTextSize * scale); }

juce::Rectangle<int> SectionInspector::lcdBounds() const
{
    using namespace SectionsPageMetrics;
    const int top = scaled (headingHeight + itemGap, scale);
    const int height = scaled (twice * lcdPaddingY + lcdRows * lcdRowHeight + (lcdRows - oneFewer) * lcdRowGap, scale);
    return { int {}, top, getWidth(), height };
}

juce::Rectangle<int> SectionInspector::lcdRow (int rowIndex) const
{
    const auto lcd = lcdBounds();
    return { lcd.getX() + scaled (lcdPaddingX, scale), lcd.getY() + scaled (lcdPaddingY + rowIndex * (lcdRowHeight + lcdRowGap), scale),
             lcd.getWidth() - scaled (twice * lcdPaddingX, scale), scaled (lcdRowHeight, scale) };
}

juce::Rectangle<int> SectionInspector::arrowBounds (InspectorArrow arrow) const
{
    if (! section.has_value())
        return {};
    // The value sits right-aligned between its two arrows: ◂ value ▸.
    const auto row = lcdRow (rowOf (arrow));
    const auto font = lcdFont();
    const auto arrowWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, juce::String::fromUTF8 (InspectorText::laterUtf8)))
                          + scaled (twice * arrowPaddingX, scale);
    if (isLater (arrow))
        return row.withLeft (row.getRight() - arrowWidth);

    juce::String value;
    switch (rowOf (arrow))
    {
        case typeRow:  value = SectionEditModel::typeText (section->type); break;
        case startRow: value = juce::String (section->startBar); break;
        default:       value = SectionEditModel::lengthText (section->lengthBars); break;
    }
    const auto valueWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, value));
    return row.withLeft (row.getRight() - arrowWidth - valueWidth - arrowWidth).withWidth (arrowWidth);
}

void SectionInspector::clickAt (juce::Point<int> position)
{
    for (auto arrow : allArrows)
        if (arrowBounds (arrow).contains (position))
        {
            if (onArrow)
                onArrow (arrow);
            return;
        }
}

void SectionInspector::mouseDown (const juce::MouseEvent& event)
{
    clickAt (event.getPosition());
}

void SectionInspector::resized()
{
    using namespace SectionsPageMetrics;
    auto area = getLocalBounds();
    heading.setBounds (area.removeFromTop (scaled (headingHeight, scale)));
    area.setTop (lcdBounds().getBottom() + scaled (itemGap, scale));
    whenItEnds.setBounds (area.removeFromTop (scaled (headingHeight, scale)));
    area.removeFromTop (scaled (itemGap, scale));

    // Loop / Go on / Stop sit side by side (the board's flex row without a gap).
    auto afterRow = area.removeFromTop (scaled (buttonHeight, scale));
    float left = static_cast<float> (afterRow.getX());
    for (auto* button : { &loop, &goOn, &stop })
    {
        button->setBounds (juce::Rectangle<float> (left, static_cast<float> (afterRow.getY()), button->getExactWidth(),
                                                   static_cast<float> (afterRow.getHeight())).toNearestInt());
        left += button->getExactWidth();
    }

    area.removeFromTop (scaled (itemGap + playRowMargin, scale));
    auto playRow = area.removeFromTop (scaled (buttonHeight, scale));
    left = static_cast<float> (playRow.getX());
    for (auto* button : { &playThis, &remove })
    {
        button->setBounds (juce::Rectangle<float> (left, static_cast<float> (playRow.getY()), button->getExactWidth(),
                                                   static_cast<float> (playRow.getHeight())).toNearestInt());
        left += button->getExactWidth() + static_cast<float> (scaled (buttonGap, scale));
    }
}

void SectionInspector::paint (juce::Graphics& g)
{
    StageLcd::paintPanel (g, lcdBounds().toFloat(), scale);
    g.setFont (lcdFont());
    g.setColour (StagePalette::lcdInk);

    const auto dash = juce::String::fromUTF8 (InspectorText::dashUtf8);
    const std::array<juce::String, lcdRows> labels { InspectorText::name, InspectorText::type, InspectorText::startBar, InspectorText::length };
    for (int rowIndex {}; rowIndex < lcdRows; ++rowIndex)
    {
        const auto row = lcdRow (rowIndex);
        g.drawText (labels[(size_t) rowIndex], row, juce::Justification::centredLeft, false);
        if (! section.has_value())
        {
            g.drawText (dash, row, juce::Justification::centredRight, false);
            continue;
        }
        if (rowIndex == nameRow)
        {
            g.drawText (section->name, row, juce::Justification::centredRight, true);
            continue;
        }
        const auto earlierArrow = rowIndex == typeRow ? InspectorArrow::typeEarlier : rowIndex == startRow ? InspectorArrow::startEarlier : InspectorArrow::lengthShorter;
        const auto laterArrow   = rowIndex == typeRow ? InspectorArrow::typeLater   : rowIndex == startRow ? InspectorArrow::startLater   : InspectorArrow::lengthLonger;
        const auto earlierBox = arrowBounds (earlierArrow);
        const auto laterBox   = arrowBounds (laterArrow);
        const auto value = rowIndex == typeRow  ? SectionEditModel::typeText (section->type)
                         : rowIndex == startRow ? juce::String (section->startBar)
                                                : SectionEditModel::lengthText (section->lengthBars);
        g.drawText (juce::String::fromUTF8 (InspectorText::earlierUtf8), earlierBox, juce::Justification::centred, false);
        g.drawText (value, row.withLeft (earlierBox.getRight()).withRight (laterBox.getX()), juce::Justification::centred, false);
        g.drawText (juce::String::fromUTF8 (InspectorText::laterUtf8), laterBox, juce::Justification::centred, false);
    }
}
