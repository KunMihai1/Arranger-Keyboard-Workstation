#include "VoicesInstrumentPanel.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace PanelText
    {
        const juce::String rightHand   = "Right hand";
        const juce::String leftHand    = "Left hand";
        const juce::String bothHands   = "Both hands";
        const juce::String noHand      = "No hand";
        const juce::String rightTag    = "R";
        const juce::String leftTag     = "L";
        const juce::String tagGap      = " ";
        const juce::String instrument  = "Instrument";
        const char* const  headingSeparatorUtf8 = " \xc2\xb7 ";   // " · "
        const char* const  searchHintUtf8       = "Search instruments\xe2\x80\xa6";
    }

    constexpr int noRow = -1;
}

//==============================================================================
namespace VoicesModel
{
    juce::String editedHands (const HandVoices& hands)
    {
        if (hands.leftLit && hands.rightLit)
            return PanelText::bothHands;
        if (hands.rightLit)
            return PanelText::rightHand;
        if (hands.leftLit)
            return PanelText::leftHand;
        return PanelText::noHand;
    }

    juce::String handTags (int program, const HandVoices& hands)
    {
        juce::StringArray tags;
        if (hands.leftProgram == program)
            tags.add (PanelText::leftTag);
        if (hands.rightProgram == program)
            tags.add (PanelText::rightTag);
        return tags.joinIntoString (PanelText::tagGap);
    }

    std::optional<int> editedProgram (const HandVoices& hands)
    {
        if (hands.rightLit)
            return hands.rightProgram;
        if (hands.leftLit)
            return hands.leftProgram;
        return {};
    }

    size_t openingGroup (const HandVoices& hands)
    {
        return InstrumentCatalog::groupOf (editedProgram (hands).value_or (hands.rightProgram)).value_or (size_t {});
    }
}

//==============================================================================
struct VoicesInstrumentPanel::GroupRows : public juce::ListBoxModel
{
    explicit GroupRows (VoicesInstrumentPanel& ownerPanel) : owner (ownerPanel) {}

    int getNumRows() override { return static_cast<int> (InstrumentCatalog::groups().size()); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        const auto& groups = InstrumentCatalog::groups();
        if (! juce::isPositiveAndBelow (rowNumber, groups.size()))
            return;
        const bool selected = owner.getSelectedGroup() == static_cast<size_t> (rowNumber);   // none while a search shows
        StageLcd::paintRow (g, { width, height }, owner.scale, groups[static_cast<size_t> (rowNumber)].name, {}, selected);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        if (juce::isPositiveAndBelow (rowNumber, InstrumentCatalog::groups().size()))
            owner.chooseGroup (static_cast<size_t> (rowNumber));
    }

    VoicesInstrumentPanel& owner;
};

struct VoicesInstrumentPanel::InstrumentRows : public juce::ListBoxModel
{
    explicit InstrumentRows (VoicesInstrumentPanel& ownerPanel) : owner (ownerPanel) {}

    int getNumRows() override { return static_cast<int> (owner.shown.size()); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        if (! juce::isPositiveAndBelow (rowNumber, owner.shown.size()))
            return;
        const auto& instrument = owner.shown[static_cast<size_t> (rowNumber)];
        const bool selected = VoicesModel::editedProgram (owner.hands) == std::optional<int> (instrument.program);
        StageLcd::paintRow (g, { width, height }, owner.scale, instrument.name, VoicesModel::handTags (instrument.program, owner.hands),
                            selected);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        if (juce::isPositiveAndBelow (rowNumber, owner.shown.size()))
            owner.chooseShownInstrument (static_cast<size_t> (rowNumber));
    }

    VoicesInstrumentPanel& owner;
};

//==============================================================================
VoicesInstrumentPanel::VoicesInstrumentPanel()
    : search (juce::String::fromUTF8 (PanelText::searchHintUtf8)),
      groupRows (std::make_unique<GroupRows> (*this)),
      instrumentRows (std::make_unique<InstrumentRows> (*this))
{
    addAndMakeVisible (search);
    search.onTextChange = [this] { refreshShown(); };
    search.onEscapeKey  = [this] { setSearchText ({}); };

    for (auto* list : { &groupList, &instrumentList })
    {
        list->setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list->setOutlineThickness (int {});
        list->getViewport()->setScrollBarsShown (false, false, true, false);   // scrolls by wheel, as the LCD lists
        addAndMakeVisible (*list);
    }
    groupList.setModel (groupRows.get());
    instrumentList.setModel (instrumentRows.get());
    refreshShown();
}

VoicesInstrumentPanel::~VoicesInstrumentPanel()
{
    groupList.setModel (nullptr);
    instrumentList.setModel (nullptr);
}

void VoicesInstrumentPanel::setScale (float newScale)
{
    scale = newScale;
    search.setScale (scale);
    const auto rowHeight = juce::roundToInt (static_cast<float> (StageMetrics::lcdRowHeight) * scale);
    groupList.setRowHeight (rowHeight);
    instrumentList.setRowHeight (rowHeight);
    resized();
    repaint();
}

void VoicesInstrumentPanel::setHands (const HandVoices& newHands)
{
    hands = newHands;
    instrumentList.repaint();
    repaint();
}

void VoicesInstrumentPanel::showOpeningGroup()
{
    selectedGroup = VoicesModel::openingGroup (hands);
    search.setText ({}, juce::dontSendNotification);
    refreshShown();
    if (const auto program = VoicesModel::editedProgram (hands))
        for (size_t shownIndex {}; shownIndex < shown.size(); ++shownIndex)
            if (shown[shownIndex].program == *program)
                instrumentList.scrollToEnsureRowIsOnscreen (static_cast<int> (shownIndex));
}

juce::String VoicesInstrumentPanel::getHeading() const
{
    return PanelText::instrument + juce::String::fromUTF8 (PanelText::headingSeparatorUtf8) + VoicesModel::editedHands (hands);
}

std::optional<size_t> VoicesInstrumentPanel::getSelectedGroup() const
{
    return search.getText().trim().isEmpty() ? selectedGroup : std::optional<size_t> {};
}

const std::vector<CatalogInstrument>& VoicesInstrumentPanel::getShownInstruments() const
{
    return shown;
}

void VoicesInstrumentPanel::setSearchText (const juce::String& text)
{
    search.setText (text, juce::dontSendNotification);
    refreshShown();
}

void VoicesInstrumentPanel::chooseGroup (size_t groupIndex)
{
    selectedGroup = groupIndex;
    setSearchText ({});   // a group shows its own programs, not the matches
}

void VoicesInstrumentPanel::chooseShownInstrument (size_t shownIndex)
{
    if (shownIndex < shown.size() && onInstrumentChosen)
        onInstrumentChosen (shown[shownIndex].program);
}

void VoicesInstrumentPanel::refreshShown()
{
    const auto query = search.getText();
    const auto& groups = InstrumentCatalog::groups();
    if (query.trim().isNotEmpty())
        shown = InstrumentCatalog::search (query);
    else if (selectedGroup && *selectedGroup < groups.size())
        shown = groups[*selectedGroup].instruments;
    else
        shown.clear();
    groupList.repaint();
    instrumentList.updateContent();
    instrumentList.selectRow (noRow);
    instrumentList.repaint();
}

InstrumentColumnRects VoicesInstrumentPanel::layout() const
{
    return instrumentColumnLayout (getLocalBounds(), scale);
}

void VoicesInstrumentPanel::paint (juce::Graphics& g)
{
    const auto rects = layout();
    StageLcd::paintHeading (g, rects.heading.toFloat(), scale, getHeading(), {});
    StageLcd::paintPanel (g, rects.groupList.toFloat(), scale);
    StageLcd::paintPanel (g, rects.instrumentList.toFloat(), scale);
}

void VoicesInstrumentPanel::resized()
{
    const auto rects = layout();
    const auto padding = juce::roundToInt (static_cast<float> (StageMetrics::lcdListPadding) * scale);
    search.setBounds (rects.search);
    groupList.setBounds (rects.groupList.reduced (int {}, padding));
    instrumentList.setBounds (rects.instrumentList.reduced (int {}, padding));
}
