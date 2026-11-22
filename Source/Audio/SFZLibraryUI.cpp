#include "SFZLibraryUI.h"
#include "PersistedKeys.h"

namespace SfzLibraryLayout
{
    // Style picker (Import Config)
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
    const juce::Colour pickerBackground { 0xff2a2a2a };

    // Instrument chooser (Assign to Style)
    constexpr int instrumentChooserWidth  = 300;
    constexpr int instrumentChooserHeight = 400;

    // Library window
    constexpr float titleFontSize      = 16.0f;
    constexpr int   titleHeight        = 30;
    constexpr int   closeButtonFromRight = 35;
    constexpr int   closeButtonTop     = 5;
    constexpr int   closeButtonWidth   = 30;
    constexpr int   closeButtonHeight  = 20;
    constexpr int   statusHeight       = 30;
    constexpr int   statusInsetX       = 5;
    constexpr int   noInset            = 0;
    constexpr int   buttonAreaHeight   = 84;    // two rows of two buttons
    constexpr int   buttonAreaMargin   = 5;
    constexpr int   buttonRowHeight    = 40;
    constexpr int   buttonsPerRow      = 2;
    constexpr int   buttonGap          = 2;
    constexpr int   listMargin         = 5;
    constexpr int   rowTextInset       = 10;
    constexpr int   textFromTop        = 0;
}

namespace SfzLibraryStrings
{
    // A style without an id stores its mappings under this id in SFZLibrary.json: keep the text.
    const juce::String defaultStyleId   = "style_default";

    const juce::String pickerPrompt     = "Click a style to import its mappings:";
    const juce::String title            = "SFZ Library";
    const juce::String currentStyle     = "Current Style: ";

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
    const juce::String assignedPrefix   = "   [Assigned: ";
    const juce::String assignedSuffix   = "]";
    const juce::String listSeparator    = ", ";
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
// SFZLibraryUI

SFZLibraryUI::SFZLibraryUI(SFZLibraryManager& managerRef) : manager(managerRef)
{
    buildInstrumentList();

    addAndMakeVisible(libraryList);
    libraryList.setModel(this);

    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);

    addAndMakeVisible(addFileButton);
    addFileButton.onClick = [this] { addFileButtonClicked(); };

    addAndMakeVisible(removeFileButton);
    removeFileButton.onClick = [this] { removeFileButtonClicked(); };

    addAndMakeVisible(assignButton);
    assignButton.onClick = [this]
    {
        using namespace SfzLibraryStrings;
        int selectedRow = libraryList.getSelectedRow();
        if (selectedRow < int {} || selectedRow >= (int)manager.getEntries().size())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, noFileTitle, noFileMessage);
            return;
        }

        const auto& entryId = manager.getEntries()[selectedRow].id;
        juce::String styleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
        if (styleId.isEmpty())
            styleId = defaultStyleId;

        juce::String styleName = getCurrentStyleName ? getCurrentStyleName() : StyleNames::defaultStyle;

        auto chooser = std::make_unique<InstrumentChooserComponent>(instrumentList);
        chooser->instrumentSelectedFunction = [this, styleId, styleName, entryId](int instrumentIndex, const juce::String& name)
        {
            manager.assignToStyleInstrument(styleId, instrumentIndex, entryId);
            if (onLibraryChanged) onLibraryChanged();
            updateStatusLabel();
            libraryList.repaint();

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, assignedTitle,
                assignedToStyle + styleName + assignedInstrument + name + openBracket + juce::String(instrumentIndex) + closeBracket);
        };

        if (auto* display = findParentComponentOfClass<juce::Component>())
        {
            auto bounds = getScreenBounds();
            chooser->setSize(SfzLibraryLayout::instrumentChooserWidth, SfzLibraryLayout::instrumentChooserHeight);
            juce::CallOutBox::launchAsynchronously(std::move(chooser), bounds, nullptr);
        }
    };

    addAndMakeVisible(importConfigButton);
    importConfigButton.onClick = [this] { importConfigButtonClicked(); };

    addAndMakeVisible(closeButton);
    closeButton.onClick = [this] {
        if (onClose) onClose();
        else setVisible(false);
    };
}

SFZLibraryUI::~SFZLibraryUI()
{
    libraryList.setModel(nullptr);
}

void SFZLibraryUI::paint(juce::Graphics& g)
{
    using namespace SfzLibraryLayout;
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(titleFontSize);
    g.drawText(SfzLibraryStrings::title, getLocalBounds().removeFromTop(titleHeight), juce::Justification::centred, true);

    updateStatusLabel();
}

void SFZLibraryUI::resized()
{
    using namespace SfzLibraryLayout;
    auto bounds = getLocalBounds();
    closeButton.setBounds(bounds.getRight() - closeButtonFromRight, closeButtonTop, closeButtonWidth, closeButtonHeight);
    bounds.removeFromTop(titleHeight);

    statusLabel.setBounds(bounds.removeFromTop(statusHeight).reduced(statusInsetX, noInset));

    // Two rows of two buttons each
    auto buttonArea = bounds.removeFromBottom(buttonAreaHeight).reduced(buttonAreaMargin);

    auto topRow = buttonArea.removeFromTop(buttonRowHeight);
    int buttonWidth = topRow.getWidth() / buttonsPerRow;
    addFileButton.setBounds(topRow.removeFromLeft(buttonWidth).reduced(buttonGap));
    removeFileButton.setBounds(topRow.reduced(buttonGap));

    auto bottomRow = buttonArea;
    buttonWidth = bottomRow.getWidth() / buttonsPerRow;
    assignButton.setBounds(bottomRow.removeFromLeft(buttonWidth).reduced(buttonGap));
    importConfigButton.setBounds(bottomRow.reduced(buttonGap));

    libraryList.setBounds(bounds.reduced(listMargin));
}

int SFZLibraryUI::getNumRows()
{
    return static_cast<int>(manager.getEntries().size());
}

void SFZLibraryUI::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    using namespace SfzLibraryStrings;
    if (rowIsSelected)
        g.fillAll(juce::Colours::coral);

    if (rowNumber < manager.getEntries().size())
    {
        const auto& entry = manager.getEntries()[rowNumber];

        juce::String styleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
        if (styleId.isEmpty())
            styleId = defaultStyleId;

        juce::String assignedInsts;
        bool isAssigned = false;
        auto styleIt = manager.getData().styleMappings.find(styleId);
        if (styleIt != manager.getData().styleMappings.end())
        {
            for (const auto& pair : styleIt->second) {
                if (pair.second == entry.id) {
                    isAssigned = true;
                    assignedInsts += juce::String(pair.first) + listSeparator;
                }
            }
        }

        juce::String textToDraw = entry.name + openBracket + entry.sfzPath + closeBracket;

        if (isAssigned)
        {
            assignedInsts = assignedInsts.dropLastCharacters(listSeparator.length());
            textToDraw += assignedPrefix + assignedInsts + assignedSuffix;
        }

        if (rowIsSelected)
            g.setColour(juce::Colours::black);
        else if (isAssigned)
            g.setColour(juce::Colours::lightgreen);
        else
            g.setColour(juce::Colours::peachpuff);

        using namespace SfzLibraryLayout;
        g.drawText(textToDraw, rowTextInset, textFromTop, width - rowTextInset - rowTextInset, height,
                   juce::Justification::centredLeft, true);
    }
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

void SFZLibraryUI::removeFileButtonClicked()
{
    int selectedRow = libraryList.getSelectedRow();
    if (selectedRow >= int {} && selectedRow < (int)manager.getEntries().size())
    {
        manager.removeEntry(manager.getEntries()[selectedRow].id);
        libraryList.updateContent();
        if (onLibraryChanged) onLibraryChanged();
        updateStatusLabel();
    }
}

void SFZLibraryUI::importConfigButtonClicked()
{
    using namespace SfzLibraryStrings;
    juce::String currentStyleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
    if (currentStyleId.isEmpty()) currentStyleId = defaultStyleId;

    const auto& mappings = manager.getData().styleMappings;

    // Build list of source styles — all that have at least one mapping and aren't the current style
    std::vector<std::pair<juce::String, juce::String>> sourceStyles;
    for (const auto& entry : mappings)
    {
        if (entry.first != currentStyleId && !entry.second.empty())
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
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, nothingTitle, nothingMessage);
        return;
    }

    juce::String currentStyleName = getCurrentStyleName ? getCurrentStyleName() : currentStyleId;

    auto chooser = std::make_unique<StyleChooserComponent>(sourceStyles);
    chooser->onStyleChosen = [this, currentStyleId, currentStyleName](const juce::String& sourceId)
    {
        manager.importMappingsFromStyle(sourceId, currentStyleId);
        if (onLibraryChanged) onLibraryChanged();
        libraryList.repaint();
        updateStatusLabel();

        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, doneTitle, importedInto + currentStyleName);
    };

    juce::CallOutBox::launchAsynchronously(std::move(chooser), getScreenBounds(), nullptr);
}

void SFZLibraryUI::updateStatusLabel()
{
    using namespace SfzLibraryStrings;
    juce::String styleName = getCurrentStyleName ? getCurrentStyleName() : StyleNames::defaultStyle;
    statusLabel.setText(currentStyle + styleName, juce::dontSendNotification);
}

void SFZLibraryUI::buildInstrumentList()
{
    instrumentList = getGMInstrumentNames();
}
