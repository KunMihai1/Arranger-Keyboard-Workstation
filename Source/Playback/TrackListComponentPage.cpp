/*
  ==============================================================================

    TrackListComponentPage.cpp

    The track picker page's look (board PageTracks): the folder / track list as
    an LCD list with glyphs, the details column, the board's buttons, and the
    sort button stepping today's three sort choices. The folder and track logic
    (dialogs, file import, removal) stays in TrackListComponent.cpp and
    TrackListComponentFolders.cpp. Split to keep files under the cap (ADR-003).

  ==============================================================================
*/

#include "displayGUI.h"
#include "TrackListComponentNames.h"
#include "UI/Stage/StageMetrics.h"
#include "UI/Stage/StageFonts.h"
#include "Common/AppColours.h"

using namespace TrackListNames;

namespace
{
    constexpr float halves = 2.0f;
    constexpr int   firstProgram = 0;
    constexpr int   lastProgram  = 127;   // General MIDI programs
}

void TrackListComponent::setScale(float newScale)
{
    scale = newScale;
    for (auto* button : { &addButtonFolder, &removeButtonFolder, &renameButtonFolder, &openFolderButton, &backButton,
                          &addButton, &removeButton, &renameButton, &useButton, &sortButton })
        button->setScale(scale);
    sortLabel.setScale(scale);
    listBox.setRowHeight(juce::roundToInt(static_cast<float>(pickerRowHeight) * scale));
    resized();
    repaint();
}

void TrackListComponent::setSlotNumber(int oneBasedSlot)
{
    slotNumber = oneBasedSlot;
    useButton.setButtonText(Text::useInSlot + juce::String(slotNumber));
}

juce::String TrackListComponent::getUseButtonText() const
{
    return useButton.getButtonText();
}

void TrackListComponent::selectRow(int row)
{
    listBox.selectRow(row);
    repaint();
}

void TrackListComponent::selectedRowsChanged(int)
{
    repaint();   // the details column follows the selection
}

std::vector<juce::Component*> TrackListComponent::getTitleActions()
{
    return { &sortLabel, &sortButton };
}

void TrackListComponent::cycleSort()
{
    const int next = sortComboBox->getSelectedId() == SortIds::byName      ? SortIds::newestFirst
                   : sortComboBox->getSelectedId() == SortIds::newestFirst ? SortIds::oldestFirst
                                                                           : SortIds::byName;
    sortComboBox->setSelectedId(next, juce::sendNotificationSync);   // a new id: comboBoxChanged sorts the open folder
}

juce::String TrackListComponent::sortCaption() const
{
    const int id = sortComboBox->getSelectedId();
    const auto& caption = id == SortIds::newestFirst ? Text::sortNewest : id == SortIds::oldestFirst ? Text::sortOldest : Text::sortName;
    return caption.toUpperCase();
}

std::vector<TrackListComponent::PickerDetail> TrackListComponent::details() const
{
    const int row = listBox.getSelectedRow();
    if (viewMode == ViewMode::FolderView)
    {
        if (! juce::isPositiveAndBelow(row, groupedTrackKeys->size()))
            return {};
        const auto& name = (*groupedTrackKeys)[static_cast<size_t>(row)];
        int melodic {}, percussion {};
        if (auto found = groupedTracks->find(name); found != groupedTracks->end())
            for (const auto& track : found->second)
                ++(track.type == TrackType::Percussion ? percussion : melodic);
        return { { Text::folderKey, name }, { Text::tracksKey, juce::String(melodic + percussion) },
                 { Text::melodicKey, juce::String(melodic) }, { Text::percussionKey, juce::String(percussion) } };
    }

    if (! juce::isPositiveAndBelow(row, availableTracks->size()))
        return {};
    const auto& track = (*availableTracks)[static_cast<size_t>(row)];
    const bool knownProgram = track.instrumentAssociated >= firstProgram && track.instrumentAssociated <= lastProgram;
    const juce::String instrument = knownProgram ? juce::String(juce::MidiMessage::getGMInstrumentName(track.instrumentAssociated))
                                                 : juce::String::fromUTF8(Text::noValueUtf8);
    return { { Text::trackKey, track.getDisplayName() }, { Text::instrumentKey, instrument },
             { Text::typeKey, track.type == TrackType::Percussion ? Text::percussionKey : Text::melodicKey },
             { Text::folderKey, currentFolderName } };
}

void TrackListComponent::addTrackRenameEditor(juce::AlertWindow& window, const juce::String& currentName)
{
    window.addTextEditor(Text::trackNameField, currentName);
}

juce::TextEditor* TrackListComponent::trackRenameEditor(juce::AlertWindow& window)
{
    return window.getTextEditor(Text::trackNameField);
}

void TrackListComponent::updateViewControls()
{
    const bool tracks = viewMode == ViewMode::TrackView;
    for (auto* folderControl : { &addButtonFolder, &removeButtonFolder, &renameButtonFolder, &openFolderButton })
        folderControl->setVisible(! tracks);
    for (auto* trackControl : { &backButton, &addButton, &removeButton, &renameButton, &useButton })
        trackControl->setVisible(tracks);
    sortButton.setButtonText(sortCaption() + juce::String::fromUTF8(Text::sortArrowsUtf8));
    if (auto* titleRow = sortButton.getParentComponent())   // the page frame places it by its width
        titleRow->resized();
    resized();
    repaint();
}

juce::Rectangle<int> TrackListComponent::listArea() const
{
    // The list column: its heading (as tall as the Back button in the track view), then the LCD list.
    const auto columnWidth = juce::roundToInt(static_cast<float>(listColumnWidth) * scale);
    const auto headingRow  = viewMode == ViewMode::TrackView ? StageMetrics::segHeight : StageMetrics::headingHeight;
    return getLocalBounds().withWidth(juce::jmin(columnWidth, getWidth()))
                           .withTrimmedTop(juce::roundToInt(static_cast<float>(headingRow + headingToList) * scale));
}

juce::Rectangle<int> TrackListComponent::detailArea() const
{
    // The details column's LCD panel: four rows under its heading.
    const auto left = juce::roundToInt(static_cast<float>(listColumnWidth + columnGap) * scale);
    const auto top  = juce::roundToInt(static_cast<float>(StageMetrics::headingHeight + detailHeadingGap) * scale);
    const auto rows = static_cast<int>(details().size());
    const auto height = juce::roundToInt(static_cast<float>(detailPaddingTop * panelPaddings + rows * detailRowHeight) * scale);
    return { left, top, juce::jmax(int {}, getWidth() - left), height };
}

void TrackListComponent::resized()
{
    auto scaled = [this] (int boardLength) { return juce::roundToInt(static_cast<float>(boardLength) * scale); };
    const auto list = listArea();
    listBox.setBounds(list.reduced(int {}, scaled(StageMetrics::lcdListPadding)));

    if (viewMode == ViewMode::TrackView)
        backButton.setBounds(int {}, int {}, backButton.getPreferredWidth(), backButton.getPreferredHeight());

    // Details column: the panel (rows shown even before a selection: four, as the board), the big button, then a row of buttons.
    const auto left  = scaled(listColumnWidth + columnGap);
    const auto width = juce::jmax(int {}, getWidth() - left);
    const auto panelBottom = scaled(StageMetrics::headingHeight + detailHeadingGap + detailPaddingTop * panelPaddings
                                    + boardDetailRows * detailRowHeight);
    auto* big = viewMode == ViewMode::TrackView ? &useButton : &openFolderButton;
    big->setBounds(left, panelBottom + scaled(detailHeadingGap), width, big->getPreferredHeight());

    auto buttonX = static_cast<float>(left);   // exact widths: rounding each one would drift along the row
    const auto buttonsTop = static_cast<float>(big->getBottom() + scaled(buttonGap));
    const auto row = viewMode == ViewMode::TrackView ? std::vector<SegButton*> { &addButton, &renameButton, &removeButton }
                                                     : std::vector<SegButton*> { &addButtonFolder, &renameButtonFolder, &removeButtonFolder };
    for (auto* button : row)
    {
        button->setBounds(juce::Rectangle<float>(buttonX, buttonsTop, button->getExactWidth(), static_cast<float>(button->getPreferredHeight())).toNearestInt());
        buttonX += button->getExactWidth() + static_cast<float>(buttonGap) * scale;
    }
}

void TrackListComponent::paint(juce::Graphics& g)
{
    const bool tracks = viewMode == ViewMode::TrackView;
    auto scaledF = [this] (int boardLength) { return static_cast<float>(boardLength) * scale; };

    // List heading: [< Back] FOLDERS > name ---- DOUBLE-CLICK TO ...
    const auto list = listArea().toFloat();
    const auto headingRowHeight = scaledF(tracks ? StageMetrics::segHeight : StageMetrics::headingHeight);
    auto heading = juce::Rectangle<float>(list.getX(), float {}, list.getWidth(), headingRowHeight)
                       .withSizeKeepingCentre(list.getWidth(), scaledF(StageMetrics::headingHeight));
    if (tracks)
        heading = heading.withTrimmedLeft(static_cast<float>(backButton.getRight()) + scaledF(headingGap));
    const auto path = tracks ? Text::folders + juce::String::fromUTF8(Text::pathSeparatorUtf8) + currentFolderName : Text::folders;
    StageLcd::paintHeading(g, heading, scale, path, tracks ? Text::doubleClickUse : Text::doubleClickOpen);
    StageLcd::paintPanel(g, list, scale);

    // Details heading and panel.
    const auto left = scaledF(listColumnWidth + columnGap);
    StageLcd::paintHeading(g, { left, float {}, static_cast<float>(getWidth()) - left, scaledF(StageMetrics::headingHeight) }, scale,
                           tracks ? Text::trackHeading : Text::folderHeading, {});
    const auto panelTop = scaledF(StageMetrics::headingHeight + detailHeadingGap);
    const auto panelBottom = static_cast<float>(openFolderButton.isVisible() ? openFolderButton.getY() : useButton.getY()) - scaledF(detailHeadingGap);
    StageLcd::paintPanel(g, { left, panelTop, static_cast<float>(getWidth()) - left, panelBottom - panelTop }, scale);

    auto rowArea = juce::Rectangle<float>(left, panelTop, static_cast<float>(getWidth()) - left, panelBottom - panelTop)
                       .reduced(scaledF(detailPaddingSide), scaledF(detailPaddingTop));
    g.setFont(StageFonts::lcd(detailTextSize * scale));
    for (const auto& detail : details())
    {
        const auto line = rowArea.removeFromTop(scaledF(detailRowHeight));
        g.setColour(StagePalette::lcdInk);
        g.drawText(detail.label, line, juce::Justification::centredLeft, true);
        g.drawText(detail.value, line, juce::Justification::centredRight, true);
        // .detail border-bottom: 1px dashed rgba(27,34,22,.3)
        g.setColour(StagePalette::lcdInk.withAlpha(detailRuleAlpha));
        const float dashes[] = { dashLength * scale, dashLength * scale };
        g.drawDashedLine({ line.getX(), line.getBottom() - scale / halves, line.getRight(), line.getBottom() - scale / halves },
                         dashes, static_cast<int>(std::size(dashes)), scale);
    }
}

void TrackListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    auto scaledF = [this] (int boardLength) { return static_cast<float>(boardLength) * scale; };
    if (rowIsSelected)
        g.fillAll(StagePalette::lcdInk);

    juce::String name, right;
    const bool tracks = viewMode == ViewMode::TrackView;
    if (tracks && juce::isPositiveAndBelow(rowNumber, availableTracks->size()))
    {
        const auto& track = (*availableTracks)[static_cast<size_t>(rowNumber)];
        name  = track.getDisplayName();
        right = track.type == TrackType::Percussion ? Text::percussionMark : Text::melodicMark;
    }
    else if (! tracks && juce::isPositiveAndBelow(rowNumber, groupedTrackKeys->size()))
    {
        name = (*groupedTrackKeys)[static_cast<size_t>(rowNumber)];
        const auto found = groupedTracks->find(name);
        right = juce::String(found != groupedTracks->end() ? static_cast<int>(found->second.size()) : int {}) + Text::tracksSuffix;
    }

    const auto ink = StageLcd::textColour(rowIsSelected);
    g.setColour(ink);

    // Glyph: a folder outline with its tab, or a note dot (PageTracks .folder-glyph / .note-glyph).
    const auto centreY = static_cast<float>(height) / halves;
    if (tracks)
    {
        const auto dot = scaledF(noteGlyphSize);
        g.fillEllipse(scaledF(glyphLeft) + (scaledF(glyphWidth) - dot) / halves, centreY - dot / halves, dot, dot);
    }
    else
    {
        const auto stroke = glyphStroke * scale;
        const juce::Rectangle<float> body { scaledF(glyphLeft), centreY - scaledF(glyphHeight) / halves + scaledF(glyphTabHeight) / halves,
                                            scaledF(glyphWidth), scaledF(glyphHeight) };
        g.drawRect(body, stroke);
        g.drawRect(juce::Rectangle<float>(body.getX(), body.getY() - scaledF(glyphTabHeight), scaledF(glyphTab), scaledF(glyphTabHeight) + stroke), stroke);
    }

    g.setFont(StageLcd::rowFont(scale));
    const auto textArea = juce::Rectangle<float>(scaledF(rowNameLeft), float {}, static_cast<float>(width) - scaledF(rowNameLeft + StageMetrics::lcdRowPaddingSide),
                                                 static_cast<float>(height));
    g.drawText(right, textArea, juce::Justification::centredRight, false);
    const auto rightWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), right) + scaledF(StageMetrics::lcdRowPaddingSide);
    g.drawText(name, textArea.withTrimmedRight(rightWidth), juce::Justification::centredLeft, true);
}
