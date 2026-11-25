#include "displayGUI.h"
#include "PersistedKeys.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace Text
    {
        const juce::String addStyleTitle       = "Add style";
        const juce::String nameEmpty           = "Name can't be empty!";
        const juce::String nameExists          = "Name already exists!";
        const juce::String styleAddedStart     = "Style '";
        const juce::String styleAddedEnd       = "' added";
        const juce::String fallbackStylePrefix = "Style ";
    }

    // Board StageRevised openPage=styles: heading, list 8 below it, buttons 8 below the list, 6 apart.
    constexpr int sectionGap    = 8;
    constexpr int buttonGap     = 6;
    constexpr int panelPaddings = 2;   // the list's padding above and below its rows
}

StylesListComponent::StylesListComponent(std::vector<juce::String> stylesNamesOut, std::function<void(const juce::String&)> onStyleClicked, int widthSize) : stylesNames{stylesNamesOut}, onStyleClicked{onStyleClicked}
{
    addAndMakeVisible(addButton);
    addAndMakeVisible(renameButton);
    addAndMakeVisible(deleteButton);

    addButton.onClick = [this]()
    {
        addNewStyle();
    };
    renameButton.onClick = [this]()
    {
        if (auto* row = openRow())
            row->changeNameLabel();
    };
    deleteButton.onClick = [this]()
    {
        if (auto* row = openRow())
            row->removeStyle();
    };

    this->widthSize = widthSize;

    styleItemsContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(styleItemsContainer.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    populate();
    refreshRows();
}

void StylesListComponent::resized()
{
    using namespace StageMetrics;
    this->widthSize = getWidth();
    auto scaled = [this] (int boardLength) { return juce::roundToInt(static_cast<float>(boardLength) * scale); };

    // Heading, then the LCD list as tall as its rows (up to the room left above the buttons), then the buttons.
    const auto listTop    = scaled(headingHeight + sectionGap);
    const auto rowsHeight = allStyles.size() * scaled(lcdRowHeight) + scaled(lcdListPadding) * panelPaddings;
    const auto maxHeight  = getHeight() - listTop - scaled(sectionGap + segHeight);
    listBounds = { int {}, listTop, getWidth(), juce::jmax(int {}, juce::jmin(rowsHeight, maxHeight)) };
    viewport.setBounds(listBounds.reduced(int {}, scaled(lcdListPadding)));

    float buttonX {};   // exact widths: rounding each one would drift along the row
    const auto buttonsTop = static_cast<float>(listBounds.getBottom() + scaled(sectionGap));
    for (auto* button : { &addButton, &renameButton, &deleteButton })
    {
        button->setScale(scale);
        button->setBounds(juce::Rectangle<float>(buttonX, buttonsTop, button->getExactWidth(), static_cast<float>(button->getPreferredHeight())).toNearestInt());
        buttonX += button->getExactWidth() + static_cast<float>(buttonGap) * scale;
    }

    layoutStyles();
}

void StylesListComponent::paint(juce::Graphics& g)
{
    StageLcd::paintHeading(g, getLocalBounds().toFloat().withHeight(static_cast<float>(StageMetrics::headingHeight) * scale),
                           scale, StylesListText::heading, {});
    StageLcd::paintPanel(g, listBounds.toFloat(), scale);
}

void StylesListComponent::setOpenStyle(const juce::String& name)
{
    openStyle = name;
    refreshRows();
}

juce::String StylesListComponent::getOpenStyle() const { return openStyle; }

void StylesListComponent::setScale(float newScale)
{
    scale = newScale;
    for (auto* row : allStyles)
        row->setScale(scale);
    resized();
    repaint();
}

juce::Rectangle<int> StylesListComponent::getListBounds() const { return listBounds; }
SegButton& StylesListComponent::getAddButton()    { return addButton; }
SegButton& StylesListComponent::getRenameButton() { return renameButton; }
SegButton& StylesListComponent::getDeleteButton() { return deleteButton; }

StyleViewComponent* StylesListComponent::openRow() const
{
    for (auto* row : allStyles)
        if (row->getNameLabel() == openStyle)
            return row;
    return nullptr;
}

void StylesListComponent::refreshRows()
{
    for (auto* row : allStyles)
    {
        row->setScale(scale);
        row->setOpen(row->getNameLabel() == openStyle);
    }
    // DEFAULT Style cannot be renamed or deleted (as its right-click menu says).
    const bool editable = openRow() != nullptr && openStyle != StyleNames::defaultStyle;
    renameButton.setEnabled(editable);
    deleteButton.setEnabled(editable);
}

void StylesListComponent::setWidthSize(const int newWidth)
{
    this->widthSize = newWidth;
}

void StylesListComponent::layoutStyles()
{
    // One LCD row per style, top to bottom.
    const auto rowHeight = juce::roundToInt(static_cast<float>(StageMetrics::lcdRowHeight) * scale);
    const auto rowWidth  = viewport.getMaximumVisibleWidth();
    int rowTop {};
    for (auto* style : allStyles)
    {
        style->setBounds(int {}, rowTop, rowWidth, rowHeight);
        rowTop += rowHeight;
    }
    styleItemsContainer->setSize(rowWidth, rowTop);
}

void StylesListComponent::repopulate()
{
    stylesNames.clear();
    for (auto* styleView : allStyles)
        stylesNames.push_back(styleView->getNameLabel());

    populate();
    refreshRows();
    resized();
}

void StylesListComponent::addNewStyle()
{
    openDialog({ Text::addStyleTitle, StyleDialogText::nameLabel, StyleDialogText::okButton, true, {} },
        [this](const juce::String& newName)
        {
            if (newName.isEmpty())
            {
                showInfo(Text::nameEmpty);
                return;
            }

            for (auto& name : stylesNames)
            {
                if (name.trim() == newName)
                {
                    showInfo(Text::nameExists);
                    return;
                }
            }

            auto* newStyle = new StyleViewComponent{ newName };

            juce::String currentName = newName;

            allCallBacks(newStyle,currentName);

            if (onStyleAdd)
                onStyleAdd(newName);

            stylesNames.push_back(newName);
            styleItemsContainer->addAndMakeVisible(newStyle);
            allStyles.add(newStyle);
            refreshRows();
            resized(); // instead of repopulating, we just layout the styles again ( repopulating would mean creating all the lambdas again and so on )

            showInfo(Text::styleAddedStart + newName + Text::styleAddedEnd);
        });
}

StageDialog* StylesListComponent::getOpenDialog() const
{
    return dialog != nullptr && dialog->isVisible() ? dialog.get() : nullptr;
}

void StylesListComponent::openDialog(StageDialogOptions options, StageDialog::OnConfirm onConfirm)
{
    dialog = std::make_unique<StageDialog>(std::move(options), std::move(onConfirm));
    dialog->showOver(*this, scale);
}

void StylesListComponent::showInfo(const juce::String& text)
{
    if (onInfo)
        onInfo(text);
}

void StylesListComponent::allCallBacks(StyleViewComponent* newStyle, const juce::String& currentName)
{
    newStyle->onStyleClicked = this->onStyleClicked;
    newStyle->openDialog = [this](StageDialogOptions options, StageDialog::OnConfirm onConfirm)
    {
        openDialog(std::move(options), std::move(onConfirm));
    };
    newStyle->onInfo = [this](const juce::String& text) { showInfo(text); };

    newStyle->onStyleRenamed = [this](const juce::String& oldName, const juce::String& newName)
    {
        if (oldName == openStyle)
            openStyle = newName;   // the open style keeps its mark under its new name
        onStyleRename(oldName, newName);
        refreshRows();
    };

    newStyle->onStyleRemoveComponent = [this](const juce::String& name)
    {
        removeStyleLocally(name);
    };

    newStyle->isInListNames = [this, currentName](const juce::String& newName)
    {
        bool isIn = false;
        for (auto& nameInList : stylesNames)
        {
            if (nameInList == newName && newName != currentName)
            {
                isIn = true;
                break;
            }
        }
        return isIn;
    };
}

void StylesListComponent::removeStyleLocally(const juce::String& name)
{
    for (int styleIndex {}; styleIndex < allStyles.size(); ++styleIndex)
    {
        if (allStyles[styleIndex]->getNameLabel() == name)
        {
            auto* styleView = allStyles[styleIndex];
            styleItemsContainer->removeChildComponent(styleView);
            allStyles.remove(styleIndex);
            break;
        }
    }

    stylesNames.clear();
    for (auto* styleView : allStyles)
        stylesNames.push_back(styleView->getNameLabel());

    if (name == openStyle)
        openStyle = {};

    if (onStyleRemove)
        onStyleRemove(name);

    styleItemsContainer->removeAllChildren();
    allStyles.clear();
    populate();
    refreshRows();
    resized();
}

void StylesListComponent::addStyleLocally(const juce::String& newName)
{
    stylesNames.push_back(newName);

    styleItemsContainer->removeAllChildren();
    allStyles.clear();
    populate();
    refreshRows();
    resized();
}

void StylesListComponent::rebuildStyleNames()
{
    stylesNames.clear();
    for (auto& style : allStyles)
    {
        juce::String name = style->getNameLabel();
        stylesNames.push_back(name);
    }
}


void StylesListComponent::populate()
{
    styleItemsContainer->removeAllChildren();
    allStyles.clear();
    for (int styleIndex {}; styleIndex < stylesNames.size(); styleIndex++)
    {
        juce::String name;
        if (styleIndex < stylesNames.size())
            name = stylesNames[styleIndex];
        else name = Text::fallbackStylePrefix + juce::String(styleIndex);
        auto* newStyle = new StyleViewComponent{ name };

        juce::String currentName = name;

        allCallBacks(newStyle, currentName);

        styleItemsContainer->addAndMakeVisible(newStyle);
        allStyles.add(newStyle);
    }
}
