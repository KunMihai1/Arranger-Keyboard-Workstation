#include "SFZLibraryUI.h"
#include "Stage/StageAlert.h"
#include "PersistedKeys.h"
#include "UI/Voices/VoicesPageLayout.h"

namespace SfzLibraryLayout
{
    // Style picker (Import mappings)
    constexpr int   pickerRowHeight     = 28;
    constexpr int   pickerWidth         = 320;
    constexpr int   pickerHeight        = 360;
    constexpr int   pickerMargin        = 6;
    constexpr int   pickerHeaderHeight  = 22;
    constexpr int   pickerHeaderGap     = 4;
    constexpr int   pickerPromptHeight  = 34;
    constexpr int   pickerPromptInsetX  = 8;
    constexpr int   pickerPromptInsetY  = 6;
    constexpr float pickerPromptFontSize = 13.0f;
    constexpr float pickerRowFontSize   = 15.0f;
    constexpr int   pickerRowTextInset  = 8;
    constexpr int   textFromTop         = 0;
    const juce::Colour pickerBackground { 0xff2a2a2a };

    // Instrument chooser (Assign to style track)
    constexpr int instrumentChooserWidth  = 300;
    constexpr int instrumentChooserHeight = 400;

    constexpr size_t oneInstrument = 1;   // more and the row says "N instruments"
}

namespace SfzLibraryStrings
{
    // A style without an id stores its mappings under this id in SFZLibrary.json: keep the text.
    const juce::String defaultStyleId   = "style_default";

    const juce::String pickerPrompt     = "Click a style to import its mappings:";
    const juce::String title            = "SFZ library";
    const juce::String instrumentsSuffix = " instruments";

    const juce::String noFileTitle      = "No file selected";
    const juce::String noFileMessage    = "Please select an SFZ file from the list first.";
    const juce::String assignedTitle    = "Assigned";
    const juce::String assignedToStyle  = "SFZ assigned to style: ";
    const juce::String assignedInstrument = "\nInstrument: ";
    const juce::String nothingTitle     = "Nothing to import";
    const juce::String nothingMessage   = "No other styles with mappings found.";
    const juce::String doneTitle        = "Done";
    const juce::String importedInto     = "Mappings imported into: ";

    const juce::String chooseFileTitle  = "Select SFZ File";
    const juce::String sfzFilePattern   = "*.sfz";

    const juce::String openBracket      = " (";
    const juce::String closeBracket     = ")";
}

//==============================================================================
// Inline style picker used by the Import Config button

namespace
{

class StyleChooserComponent : public juce::Component,
                               public juce::ListBoxModel
{
public:
    std::function<void(const juce::String& styleId)> onStyleChosen;

    StyleChooserComponent(std::vector<std::pair<juce::String, juce::String>> styles)
        : styles(std::move(styles))
    {
        using namespace SfzLibraryLayout;
        listBox.setModel(this);
        listBox.setRowHeight(pickerRowHeight);
        addAndMakeVisible(listBox);
        setSize(pickerWidth, pickerHeight);
    }

    ~StyleChooserComponent() override { listBox.setModel(nullptr); }

    void resized() override
    {
        using namespace SfzLibraryLayout;
        auto area = getLocalBounds().reduced(pickerMargin);
        headerLabel.setBounds(area.removeFromTop(pickerHeaderHeight));
        area.removeFromTop(pickerHeaderGap);
        listBox.setBounds(area);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace SfzLibraryLayout;
        g.fillAll(pickerBackground);
        g.setColour(juce::Colours::lightgrey);
        g.setFont(pickerPromptFontSize);
        g.drawText(SfzLibraryStrings::pickerPrompt,
                   getLocalBounds().removeFromTop(pickerPromptHeight).reduced(pickerPromptInsetX, pickerPromptInsetY),
                   juce::Justification::centredLeft, true);
    }

    int getNumRows() override { return (int)styles.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        using namespace SfzLibraryLayout;
        if (selected) g.fillAll(juce::Colours::coral);
        g.setColour(selected ? juce::Colours::black : juce::Colours::peachpuff);
        g.setFont(pickerRowFontSize);
        if (row < (int)styles.size())
            g.drawText(styles[row].second, pickerRowTextInset, textFromTop, width - pickerRowTextInset - pickerRowTextInset,
                       height, juce::Justification::centredLeft, true);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= int {} && row < (int)styles.size() && onStyleChosen)
        {
            onStyleChosen(styles[row].first);
            if (auto* callOutBox = findParentComponentOfClass<juce::CallOutBox>())
                callOutBox->dismiss();
        }
    }

private:
    std::vector<std::pair<juce::String, juce::String>> styles;
    juce::ListBox listBox;
    juce::Label   headerLabel;
};

} // namespace

//==============================================================================
juce::String sfzAssignmentText (const SFZLibraryData& data, const juce::String& styleId, const juce::String& entryId)
{
    std::vector<int> programs;
    const auto style = data.styleMappings.find (styleId);
    if (style != data.styleMappings.end())
        for (const auto& [program, assignedEntry] : style->second)
            if (assignedEntry == entryId)
                programs.push_back (program);

    if (programs.empty())
        return {};
    if (programs.size() > SfzLibraryLayout::oneInstrument)
        return juce::String (static_cast<int> (programs.size())) + SfzLibraryStrings::instrumentsSuffix;
    const auto& names = getGMInstrumentNames();
    return juce::isPositiveAndBelow (programs.front(), names.size()) ? names[programs.front()] : juce::String (programs.front());
}

//==============================================================================
// SFZLibraryUI

SFZLibraryUI::SFZLibraryUI(SFZLibraryManager& managerRef) : manager(managerRef)
{
    instrumentList = getGMInstrumentNames();

    libraryList.setModel(this);
    libraryList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    libraryList.setOutlineThickness(int {});
    libraryList.getViewport()->setScrollBarsShown(false, false, true, false);   // scrolls by wheel, as the LCD lists
    addAndMakeVisible(libraryList);

    for (auto* button : { &addFileButton, &assignButton, &importConfigButton })
        addAndMakeVisible(*button);
    addFileButton.onClick      = [this] { addFileButtonClicked(); };
    assignButton.onClick       = [this] { assignButtonClicked(); };
    importConfigButton.onClick = [this] { importConfigButtonClicked(); };
    setScale(scale);
}

SFZLibraryUI::~SFZLibraryUI()
{
    libraryList.setModel(nullptr);
}

void SFZLibraryUI::setScale (float newScale)
{
    scale = newScale;
    for (auto* button : { &addFileButton, &assignButton, &importConfigButton })
        button->setScale(scale);
    libraryList.setRowHeight(juce::roundToInt(static_cast<float>(StageMetrics::lcdRowHeight) * scale));
    resized();
    repaint();
}

void SFZLibraryUI::refresh()
{
    libraryList.updateContent();
    libraryList.repaint();
}

juce::String SFZLibraryUI::currentStyleId() const
{
    juce::String styleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
    return styleId.isEmpty() ? SfzLibraryStrings::defaultStyleId : styleId;
}

void SFZLibraryUI::paint(juce::Graphics& g)
{
    const auto rects = sfzColumnLayout(getLocalBounds(), scale);
    StageLcd::paintHeading(g, rects.heading.toFloat(), scale, SfzLibraryStrings::title, {});
    StageLcd::paintPanel(g, rects.list.toFloat(), scale);
}

void SFZLibraryUI::resized()
{
    const auto rects = sfzColumnLayout(getLocalBounds(), scale);
    libraryList.setBounds(rects.list.reduced(int {}, juce::roundToInt(static_cast<float>(StageMetrics::lcdListPadding) * scale)));
    // Adjacent seg buttons, 6 apart, as the board (exact widths: the row does not drift)
    auto left = static_cast<float>(rects.buttonRow.getX());
    for (auto* button : { &addFileButton, &assignButton, &importConfigButton })
    {
        button->setBounds(juce::Rectangle<float>(left, static_cast<float>(rects.buttonRow.getY()), button->getExactWidth(),
                                                 static_cast<float>(rects.buttonRow.getHeight())).toNearestInt());
        left += button->getExactWidth() + static_cast<float>(VoicesPageMetrics::buttonGap) * scale;
    }
}

int SFZLibraryUI::getNumRows()
{
    return static_cast<int>(manager.getEntries().size());
}

void SFZLibraryUI::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto& entries = manager.getEntries();
    if (! juce::isPositiveAndBelow(rowNumber, entries.size()))
        return;
    const auto& entry = entries[static_cast<size_t>(rowNumber)];
    const auto fileName = entry.sfzPath.isNotEmpty() ? entry.getFile().getFileName() : entry.name;
    StageLcd::paintRow(g, { width, height }, scale, fileName, sfzAssignmentText(manager.getData(), currentStyleId(), entry.id), rowIsSelected);
}

void SFZLibraryUI::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (! event.mods.isPopupMenu() || ! juce::isPositiveAndBelow(row, manager.getEntries().size()))
        return;
    // Remove (no board button): the row's menu, as the Style page's lists.
    juce::PopupMenu menu;
    const auto entryId = manager.getEntries()[static_cast<size_t>(row)].id;
    menu.addItem(SfzLibraryText::removeFile, [this, entryId]
    {
        const auto& entries = manager.getEntries();
        for (size_t entryIndex {}; entryIndex < entries.size(); ++entryIndex)
            if (entries[entryIndex].id == entryId)
            {
                removeEntry(static_cast<int>(entryIndex));
                return;
            }
    });
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&libraryList).withMousePosition());
}

void SFZLibraryUI::assignButtonClicked()
{
    using namespace SfzLibraryStrings;
    int selectedRow = libraryList.getSelectedRow();
    if (selectedRow < int {} || selectedRow >= (int)manager.getEntries().size())
    {
        StageAlert::showMessage (noFileTitle, noFileMessage);
        return;
    }

    const auto& entryId = manager.getEntries()[selectedRow].id;
    const juce::String styleId = currentStyleId();
    juce::String styleName = getCurrentStyleName ? getCurrentStyleName() : StyleNames::defaultStyle;

    auto chooser = std::make_unique<InstrumentChooserComponent>(instrumentList);
    chooser->instrumentSelectedFunction = [this, styleId, styleName, entryId](int instrumentIndex, const juce::String& name)
    {
        manager.assignToStyleInstrument(styleId, instrumentIndex, entryId);
        if (onLibraryChanged) onLibraryChanged();
        libraryList.repaint();

        StageAlert::showMessage (assignedTitle, assignedToStyle + styleName + assignedInstrument + name + openBracket + juce::String(instrumentIndex) + closeBracket);
    };

    chooser->setSize(SfzLibraryLayout::instrumentChooserWidth, SfzLibraryLayout::instrumentChooserHeight);
    juce::CallOutBox::launchAsynchronously(std::move(chooser), assignButton.getScreenBounds(), nullptr);
}

void SFZLibraryUI::addFileButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>(SfzLibraryStrings::chooseFileTitle, juce::File(), SfzLibraryStrings::sfzFilePattern);
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file.existsAsFile())
        {
            manager.addFile(file);
            libraryList.updateContent();
            if (onLibraryChanged) onLibraryChanged();
        }
    });
}

void SFZLibraryUI::removeEntry(int row)
{
    if (row >= int {} && row < (int)manager.getEntries().size())
    {
        manager.removeEntry(manager.getEntries()[static_cast<size_t>(row)].id);
        libraryList.deselectAllRows();
        libraryList.updateContent();
        libraryList.repaint();
        if (onLibraryChanged) onLibraryChanged();
    }
}

void SFZLibraryUI::importConfigButtonClicked()
{
    using namespace SfzLibraryStrings;
    const juce::String currentId = currentStyleId();

    const auto& mappings = manager.getData().styleMappings;

    // Build list of source styles — all that have at least one mapping and aren't the current style
    std::vector<std::pair<juce::String, juce::String>> sourceStyles;
    for (const auto& entry : mappings)
    {
        if (entry.first != currentId && !entry.second.empty())
        {
            juce::String displayName = entry.first;
            if (getStyleNameForId)
            {
                auto name = getStyleNameForId(entry.first);
                if (name.isNotEmpty()) displayName = name;
            }
            sourceStyles.push_back({ entry.first, displayName });
        }
    }

    if (sourceStyles.empty())
    {
        StageAlert::showMessage (nothingTitle, nothingMessage);
        return;
    }

    juce::String currentStyleName = getCurrentStyleName ? getCurrentStyleName() : currentId;

    auto chooser = std::make_unique<StyleChooserComponent>(sourceStyles);
    chooser->onStyleChosen = [this, currentId, currentStyleName](const juce::String& sourceId)
    {
        manager.importMappingsFromStyle(sourceId, currentId);
        if (onLibraryChanged) onLibraryChanged();
        libraryList.repaint();

        StageAlert::showMessage (doneTitle, importedInto + currentStyleName);
    };

    juce::CallOutBox::launchAsynchronously(std::move(chooser), importConfigButton.getScreenBounds(), nullptr);
}
