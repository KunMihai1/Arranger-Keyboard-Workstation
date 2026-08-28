#include "SFZLibraryUI.h"

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
        listBox.setModel(this);
        listBox.setRowHeight(28);
        addAndMakeVisible(listBox);
        setSize(320, 360);
    }

    ~StyleChooserComponent() override { listBox.setModel(nullptr); }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6);
        headerLabel.setBounds(b.removeFromTop(22));
        b.removeFromTop(4);
        listBox.setBounds(b);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colours::lightgrey);
        g.setFont(13.0f);
        g.drawText("Click a style to import its mappings:", getLocalBounds().removeFromTop(34).reduced(8, 6),
                   juce::Justification::centredLeft, true);
    }

    int getNumRows() override { return (int)styles.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (selected) g.fillAll(juce::Colours::coral);
        g.setColour(selected ? juce::Colours::black : juce::Colours::peachpuff);
        g.setFont(15.0f);
        if (row < (int)styles.size())
            g.drawText(styles[row].second, 8, 0, w - 16, h, juce::Justification::centredLeft, true);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < (int)styles.size() && onStyleChosen)
        {
            onStyleChosen(styles[row].first);
            if (auto* cb = findParentComponentOfClass<juce::CallOutBox>())
                cb->dismiss();
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
        int selectedRow = libraryList.getSelectedRow();
        if (selectedRow < 0 || selectedRow >= (int)manager.getEntries().size())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No file selected", "Please select an SFZ file from the list first.");
            return;
        }

        const auto& entryId = manager.getEntries()[selectedRow].id;
        juce::String styleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
        if (styleId.isEmpty())
            styleId = "style_default";

        juce::String styleName = getCurrentStyleName ? getCurrentStyleName() : juce::String("DEFAULT Style");

        auto chooser = std::make_unique<InstrumentChooserComponent>(instrumentList);
        chooser->instrumentSelectedFunction = [this, styleId, styleName, entryId](int instrumentIndex, const juce::String& name)
        {
            manager.assignToStyleInstrument(styleId, instrumentIndex, entryId);
            if (onLibraryChanged) onLibraryChanged();
            updateStatusLabel();
            libraryList.repaint();

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Assigned", "SFZ assigned to style: " + styleName + "\nInstrument: " + name + " (" + juce::String(instrumentIndex) + ")");
        };

        if (auto* display = findParentComponentOfClass<juce::Component>())
        {
            auto bounds = getScreenBounds();
            chooser->setSize(300, 400);
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
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("SFZ Library", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);

    updateStatusLabel();
}

void SFZLibraryUI::resized()
{
    auto bounds = getLocalBounds();
    closeButton.setBounds(bounds.getRight() - 35, 5, 30, 20);
    bounds.removeFromTop(30);

    statusLabel.setBounds(bounds.removeFromTop(30).reduced(5, 0));

    // Two rows of two buttons each
    auto buttonArea = bounds.removeFromBottom(84).reduced(5);

    auto topRow = buttonArea.removeFromTop(40);
    int btnW = topRow.getWidth() / 2;
    addFileButton.setBounds(topRow.removeFromLeft(btnW).reduced(2));
    removeFileButton.setBounds(topRow.reduced(2));

    auto botRow = buttonArea;
    btnW = botRow.getWidth() / 2;
    assignButton.setBounds(botRow.removeFromLeft(btnW).reduced(2));
    importConfigButton.setBounds(botRow.reduced(2));

    libraryList.setBounds(bounds.reduced(5));
}

int SFZLibraryUI::getNumRows()
{
    return static_cast<int>(manager.getEntries().size());
}

void SFZLibraryUI::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::coral);

    if (rowNumber < manager.getEntries().size())
    {
        const auto& entry = manager.getEntries()[rowNumber];

        juce::String styleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
        if (styleId.isEmpty())
            styleId = "style_default";

        juce::String assignedInsts = "";
        bool isAssigned = false;
        auto styleIt = manager.getData().styleMappings.find(styleId);
        if (styleIt != manager.getData().styleMappings.end())
        {
            for (const auto& pair : styleIt->second) {
                if (pair.second == entry.id) {
                    isAssigned = true;
                    assignedInsts += juce::String(pair.first) + ", ";
                }
            }
        }

        juce::String textToDraw = entry.name + " (" + entry.sfzPath + ")";

        if (isAssigned)
        {
            assignedInsts = assignedInsts.dropLastCharacters(2);
            textToDraw += "   [Assigned: " + assignedInsts + "]";
        }

        if (rowIsSelected)
            g.setColour(juce::Colours::black);
        else if (isAssigned)
            g.setColour(juce::Colours::lightgreen);
        else
            g.setColour(juce::Colours::peachpuff);

        g.drawText(textToDraw, 10, 0, width - 20, height, juce::Justification::centredLeft, true);
    }
}

void SFZLibraryUI::addFileButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select SFZ File", juce::File(), "*.sfz");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
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
    if (selectedRow >= 0 && selectedRow < (int)manager.getEntries().size())
    {
        manager.removeEntry(manager.getEntries()[selectedRow].id);
        libraryList.updateContent();
        if (onLibraryChanged) onLibraryChanged();
        updateStatusLabel();
    }
}

void SFZLibraryUI::importConfigButtonClicked()
{
    juce::String currentStyleId = getCurrentStyleId ? getCurrentStyleId() : juce::String();
    if (currentStyleId.isEmpty()) currentStyleId = "style_default";

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
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Nothing to import", "No other styles with mappings found.");
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

        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Done", "Mappings imported into: " + currentStyleName);
    };

    juce::CallOutBox::launchAsynchronously(std::move(chooser), getScreenBounds(), nullptr);
}

void SFZLibraryUI::updateStatusLabel()
{
    juce::String styleName = getCurrentStyleName ? getCurrentStyleName() : juce::String("DEFAULT Style");
    statusLabel.setText("Current Style: " + styleName, juce::dontSendNotification);
}

void SFZLibraryUI::buildInstrumentList()
{
    instrumentList = getGMInstrumentNames();
}
