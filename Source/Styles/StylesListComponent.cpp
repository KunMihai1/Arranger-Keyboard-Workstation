#include "displayGUI.h"

namespace
{
    namespace Text
    {
        const juce::String addingStyleTitle    = "Adding a style";
        const juce::String nameEmpty           = "Name can't be empty!";
        const juce::String nameExists          = "Name already exists!";
        const juce::String styleAddedTitle     = "Add style";
        const juce::String styleAddedStart     = "The style '";
        const juce::String styleAddedEnd       = "' has been added!";
        const juce::String fallbackStylePrefix = "Style ";
    }

    constexpr int   controlBarHeight   = 24;
    constexpr int   buttonWidth        = 80;
    constexpr int   buttonHeight       = 20;
    constexpr int   spacing            = 10;
    constexpr int   columns            = 2;
    constexpr int   gapsBetweenColumns = 1;       // two columns, one gap between them
    constexpr int   paddedSides        = 2;       // left and right
    constexpr int   trailingGap        = 1;       // a gap after the last row too
    constexpr int   itemHeight         = 50;
    constexpr int   centringDivisor    = 2;
    constexpr int   leftEdge           = 0;
    constexpr int   topEdge            = 0;
    constexpr int   separatorRowOffset = 1;       // the separator sits on the bar's last pixel row
    constexpr float headerGlow         = 0.05f;
    constexpr float separatorStartX    = 0.0f;
    constexpr float separatorThickness = 1.0f;
}

StylesListComponent::StylesListComponent(std::vector<juce::String> stylesNamesOut, std::function<void(const juce::String&)> onStyleClicked, int widthSize) : stylesNames{stylesNamesOut}, onStyleClicked{onStyleClicked}
{
    addAndMakeVisible(addButton);

    addButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addButton.setLookAndFeel(&laf);
    addButton.setColour(juce::TextButton::buttonColourId, AppColours::accent7);

    addButton.onClick = [this]()
    {
        addNewStyle();
    };

    this->widthSize = widthSize;

    styleItemsContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(styleItemsContainer.get(), false);
    addAndMakeVisible(viewport);

    populate();
}

void StylesListComponent::resized()
{

    this->widthSize = getWidth();

    addButton.setBounds(spacing / centringDivisor, (controlBarHeight - buttonHeight) / centringDivisor, buttonWidth, buttonHeight);
    viewport.setBounds(leftEdge, controlBarHeight, getWidth(), getHeight() - controlBarHeight);

    layoutStyles();
}

void StylesListComponent::paint(juce::Graphics& g)
{
    g.fillAll(AppColours::panelBg);

    juce::ColourGradient gradient(AppColours::cardBg.brighter(headerGlow), leftEdge, topEdge,
                                  AppColours::cardBg, leftEdge, (float)controlBarHeight, false);
    g.setGradientFill(gradient);
    g.fillRect(leftEdge, topEdge, getWidth(), controlBarHeight);

    g.setColour(AppColours::separator);
    g.drawLine(separatorStartX, (float)(controlBarHeight - separatorRowOffset), (float)getWidth(), (float)(controlBarHeight - separatorRowOffset), separatorThickness);
}

void StylesListComponent::setWidthSize(const int newWidth)
{
    this->widthSize = newWidth;
}

void StylesListComponent::layoutStyles()
{
    const int sidePadding = spacing;

    const int totalSpacing = gapsBetweenColumns * spacing + paddedSides * sidePadding;
    const int itemWidth = (getWidth() - totalSpacing) / columns;

    int itemX = sidePadding;
    int itemY = spacing;

    int placedCount {};
    for (auto* style : allStyles)
    {
        style->setBounds(itemX, itemY, itemWidth, itemHeight);
        placedCount++;

        if (placedCount % columns == int{})
        {
            itemX = sidePadding;
            itemY += itemHeight + spacing;
        }
        else
        {
            itemX += itemWidth + spacing;
        }
    }

    int rows = static_cast<int>(std::ceil(allStyles.size() / (float)columns));
    int totalHeight = rows * itemHeight + (rows + trailingGap) * spacing;

    styleItemsContainer->setSize(getWidth(), totalHeight);
}

void StylesListComponent::repopulate()
{
    stylesNames.clear();
    for (auto* styleView : allStyles)
        stylesNames.push_back(styleView->getNameLabel());

    populate();
    resized();
}

void StylesListComponent::addNewStyle()
{
    auto* window = new juce::AlertWindow{ StyleDialogText::nameDialogTitle, StyleDialogText::nameDialogPrompt, juce::AlertWindow::NoIcon };

    window->addTextEditor(StyleDialogText::nameEditor, {}, StyleDialogText::nameEditorLabel);;

    window->addButton(StyleDialogText::okButton, StyleDialogText::dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(StyleDialogText::cancelButton, StyleDialogText::dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, window](int result)
            {
                std::unique_ptr<juce::AlertWindow> cleanup{ window };
                if (result != StyleDialogText::dialogOk)
                    return;

                juce::String newName = window->getTextEditor(StyleDialogText::nameEditor)->getText().trim();

                if (newName.isEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::addingStyleTitle, Text::nameEmpty);
                    return;
                }

                for (auto& name : stylesNames)
                {
                    if (name.trim() == newName)
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::addingStyleTitle, Text::nameExists);
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
                resized(); // instead of repopulating, we just layout the styles again ( repopulating would mean creating all the lambdas again and so on )

                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::styleAddedTitle, Text::styleAddedStart + newName + Text::styleAddedEnd);
            }
        )
    );

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(StyleDialogText::nameEditor))
                editor->grabKeyboardFocus();
        });
}

void StylesListComponent::allCallBacks(StyleViewComponent* newStyle, const juce::String& currentName)
{
    newStyle->onStyleClicked = this->onStyleClicked;

    newStyle->onStyleRenamed = [this](const juce::String& oldName, const juce::String& newName)
    {
        onStyleRename(oldName, newName);
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

    if (onStyleRemove)
        onStyleRemove(name);

    styleItemsContainer->removeAllChildren();
    allStyles.clear();
    populate();
    resized();
}

void StylesListComponent::addStyleLocally(const juce::String& newName)
{
    stylesNames.push_back(newName);

    styleItemsContainer->removeAllChildren();
    allStyles.clear();
    populate();
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
