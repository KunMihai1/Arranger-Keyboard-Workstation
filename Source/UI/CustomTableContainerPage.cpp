/*
  ==============================================================================

    CustomTableContainerPage.cpp

    The notes page's look (board PageNotes): the note table on the LCD with a
    "#" column and velocity bars, and the board's buttons in place of the two
    old choice boxes. "Whole track" = today's Apply with its two choices,
    "Back to recorded" = today's Change with its four; each button sets the old
    choice and runs the old action. Owner (2026-11-23): no "Edit selected note"
    block and no Undo - notes are edited by double-clicking them, as before.
    Split from CustomTableContainer.cpp to keep it under the cap (ADR-003).

  ==============================================================================
*/

#include "CustomTableContainer.h"
#include "UI/Stage/StageFonts.h"
#include "Common/AppColours.h"

namespace
{
    namespace PageText
    {
        const juce::String notes          = "Notes";
        const juce::String timeStamps     = "Time stamps";
        const juce::String velocities     = "Velocities";
        const juce::String everything     = "Everything";
        const char* const  shiftUtf8      = "Shift time stamps\xe2\x80\xa6";   // + U+2026
        const char* const  velocitiesUtf8 = "Set velocities\xe2\x80\xa6";
        const juce::String wholeTrack     = "Whole track";
        const juce::String backToRecorded = "Back to recorded";
        const juce::String editHint       = "Double-click a note to change it";
    }

    // Board PageNotes at scale 1, relative to the page content (28, 178).
    constexpr int   tableWidth        = 700;
    constexpr int   columnGap         = 22;
    constexpr int   tablePadding      = 4;
    constexpr int   headerHeight      = 22;
    constexpr int   rowHeight         = 21;
    constexpr int   indexColumnWidth  = 66;     // 12 padding + 44 + 10 gap
    constexpr int   noteColumnWidth   = 100;    // 90 + 10
    constexpr int   timeColumnWidth   = 120;    // 110 + 10
    constexpr int   channelColumnWidth = 92;    // 80 + 12 padding
    constexpr int   indexTextInset    = 12;
    constexpr float headerTextSize    = 11.0f;
    constexpr float headerRuleAlpha   = 0.45f;
    constexpr int   wholeTrackTop     = 56;     // the "Whole track" heading
    constexpr int   backToRecordedTop = 112;
    constexpr int   headingToButtons  = 20;
    constexpr int   buttonGap         = 6;
    constexpr int   disclaimerLines   = 2;
    constexpr bool  medium            = false;
}

void TableContainer::NotesTableLook::drawTableHeaderBackground (juce::Graphics& g, juce::TableHeaderComponent& header)
{
    g.setColour (StagePalette::lcdInk.withAlpha (headerRuleAlpha));
    g.fillRect (juce::Rectangle<float> (float {}, static_cast<float> (header.getHeight()) - scale,
                                        static_cast<float> (header.getWidth()), scale));
}

void TableContainer::NotesTableLook::drawTableHeaderColumn (juce::Graphics& g, juce::TableHeaderComponent&, const juce::String& columnName,
                                                            int columnId, int width, int height, bool, bool, int)
{
    const auto inset = columnId == NoteTableColumns::index ? static_cast<float> (indexTextInset) * scale : float {};
    g.setColour (StagePalette::lcdInk);
    g.setFont (StageFonts::lcd (headerTextSize * scale));
    g.drawText (columnName.toUpperCase(), juce::Rectangle<float> (inset, float {}, static_cast<float> (width) - inset, static_cast<float> (height)),
                juce::Justification::centredLeft, false);
}

std::vector<TableContainer::ActionButton> TableContainer::resetButtons()
{
    return { { PageText::notes, ResetActionIds::notes }, { PageText::timeStamps, ResetActionIds::timeStamps },
             { PageText::velocities, ResetActionIds::velocities }, { PageText::everything, ResetActionIds::everything } };
}

std::vector<TableContainer::ActionButton> TableContainer::modifyButtons()
{
    return { { juce::String::fromUTF8 (PageText::shiftUtf8), ModifyActionIds::timeStamps },
             { juce::String::fromUTF8 (PageText::velocitiesUtf8), ModifyActionIds::velocities } };
}

int TableContainer::getNumNotes()
{
    return model ? model->getNumRows() : int {};
}

void TableContainer::createPageControls()
{
    tableLook = std::make_unique<NotesTableLook>();
    table->getHeader().setLookAndFeel (tableLook.get());
    table->setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    table->setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);

    for (const auto& [caption, actionId] : modifyButtons())
    {
        auto* button = modifyActionButtons.add (new SegButton (caption, SegButton::Style::seg));
        button->onClick = [this, id = actionId]
        {
            actionModifyCB->setSelectedId (id, juce::dontSendNotification);
            if (modifyMultipleButton->onClick)
                modifyMultipleButton->onClick();   // today's Apply with this choice
        };
        addAndMakeVisible (button);
    }

    for (const auto& [caption, actionId] : resetButtons())
    {
        auto* button = resetActionButtons.add (new SegButton (caption, SegButton::Style::seg));
        button->onClick = [this, id = actionId]
        {
            actionToOriginalCB->setSelectedId (id, juce::dontSendNotification);
            if (changeToOriginalMultipleButton->onClick)
                changeToOriginalMultipleButton->onClick();   // today's Change with this choice
        };
        addAndMakeVisible (button);
    }

    // Today's note that the user's files are safe: dim text at the bottom of the right column.
    disclaimerLabel->setColour (juce::Label::textColourId, StagePalette::silkscreenDimText);
    disclaimerLabel->setJustificationType (juce::Justification::bottomLeft);
    disclaimerLabel->setBorderSize ({});
    refreshPageButtons();
    setScale (scale);
}

void TableContainer::refreshPageButtons()
{
    if (modifyMultipleButton == nullptr || changeToOriginalMultipleButton == nullptr)
        return;
    for (auto* button : modifyActionButtons)
        button->setEnabled (modifyMultipleButton->isEnabled());
    for (auto* button : resetActionButtons)
        button->setEnabled (changeToOriginalMultipleButton->isEnabled());
}

void TableContainer::setScale (float newScale)
{
    scale = newScale;
    auto scaled = [this] (int boardLength) { return juce::roundToInt (static_cast<float> (boardLength) * scale); };
    if (model)
        model->setScale (scale);
    if (tableLook)
        tableLook->scale = scale;

    table->setHeaderHeight (scaled (headerHeight));
    table->setRowHeight (scaled (rowHeight));
    auto& header = table->getHeader();
    header.setColumnWidth (NoteTableColumns::index, scaled (indexColumnWidth));
    header.setColumnWidth (NoteTableColumns::note, scaled (noteColumnWidth));
    header.setColumnWidth (NoteTableColumns::time, scaled (timeColumnWidth));
    header.setColumnWidth (NoteTableColumns::channel, scaled (channelColumnWidth));
    header.setColumnWidth (NoteTableColumns::velocity, scaled (tableWidth - indexColumnWidth - noteColumnWidth - timeColumnWidth - channelColumnWidth));

    for (auto* button : modifyActionButtons)
        button->setScale (scale);
    for (auto* button : resetActionButtons)
        button->setScale (scale);
    disclaimerLabel->setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, medium, StageMetrics::silkDimSpacing));

    resized();
    repaint();
    table->updateContent();
    table->repaint();
}

void TableContainer::resized()
{
    if (table == nullptr)
        return;
    auto scaled = [this] (int boardLength) { return juce::roundToInt (static_cast<float> (boardLength) * scale); };

    const auto tableArea = getLocalBounds().withWidth (juce::jmin (scaled (tableWidth), getWidth()));
    table->setBounds (tableArea.reduced (int {}, scaled (tablePadding)));

    const auto right = getLocalBounds().withTrimmedLeft (scaled (tableWidth + columnGap));
    auto placeRow = [&] (juce::OwnedArray<SegButton>& buttons, int top)
    {
        auto buttonX = static_cast<float> (right.getX());   // exact widths: no drift along the row
        for (auto* button : buttons)
        {
            button->setBounds (juce::Rectangle<float> (buttonX, static_cast<float> (top), button->getExactWidth(),
                                                       static_cast<float> (button->getPreferredHeight())).toNearestInt());
            buttonX += button->getExactWidth() + static_cast<float> (buttonGap) * scale;
        }
    };
    placeRow (modifyActionButtons, scaled (wholeTrackTop + headingToButtons));
    placeRow (resetActionButtons, scaled (backToRecordedTop + headingToButtons));

    disclaimerLabel->setBounds (right.withTrimmedTop (right.getHeight() - scaled (StageMetrics::silkLabelLineHeight * disclaimerLines)));
}

void TableContainer::paint (juce::Graphics& g)
{
    auto scaledF = [this] (int boardLength) { return static_cast<float> (boardLength) * scale; };
    const auto bounds = getLocalBounds().toFloat();
    StageLcd::paintPanel (g, bounds.withWidth (juce::jmin (scaledF (tableWidth), bounds.getWidth())), scale);

    const auto right = bounds.withTrimmedLeft (scaledF (tableWidth + columnGap));
    const auto headingHeight = scaledF (StageMetrics::headingHeight);

    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, medium, StageMetrics::silkDimSpacing));
    g.drawText (PageText::editHint.toUpperCase(), right.withHeight (headingHeight), juce::Justification::centredLeft, false);

    StageLcd::paintHeading (g, right.withY (scaledF (wholeTrackTop)).withHeight (headingHeight), scale, PageText::wholeTrack, {});
    StageLcd::paintHeading (g, right.withY (scaledF (backToRecordedTop)).withHeight (headingHeight), scale, PageText::backToRecorded, {});
}
