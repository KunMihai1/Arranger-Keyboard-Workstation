#include "displayGUI.h"
#include "PersistedKeys.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace Text
    {
        const juce::String cannotChange       = "Can't change";
        const juce::String renameItem         = "Rename";
        const juce::String deleteItem         = "Delete";
        const juce::String renameStyleTitle   = "Rename style";
        const juce::String emptyName          = "You can't put an empty name!";
        const juce::String nameExists         = "The name already exists!";
        const juce::String removeStyleTitle   = "Remove style";
        const juce::String removePromptStart  = "Remove the style '";
        const juce::String removePromptEnd    = "'?";
        const juce::String removeButton       = "Remove";
        const juce::String removed            = "Style removed";
        const char* const  openMarkUtf8       = "\xe2\x97\x82";   // U+25C2: the open style's arrow
    }
}

StyleViewComponent::StyleViewComponent(const juce::String& styleName)
{
    // The label fills the row (the whole row takes clicks); the row's LCD background is painted behind it.
    label.setText(styleName, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, StageLcd::textColour(false));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setInterceptsMouseClicks(true, false);
    label.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addAndMakeVisible(label);

    label.addMouseListener(this, false);

}

void StyleViewComponent::resized()
{
    const auto padding = juce::roundToInt(static_cast<float>(StageMetrics::lcdRowPaddingSide) * scale);
    label.setFont(StageLcd::rowFont(scale));
    label.setBorderSize({ {}, padding, {}, padding });
    label.setBounds(getLocalBounds());
}

void StyleViewComponent::setOpen(bool shouldBeOpen)
{
    open = shouldBeOpen;
    label.setColour(juce::Label::textColourId, StageLcd::textColour(open));
    repaint();
}

bool StyleViewComponent::isOpen() const { return open; }

void StyleViewComponent::setScale(float newScale)
{
    scale = newScale;
    resized();
    repaint();
}

void StyleViewComponent::paint(juce::Graphics& g)
{
    StageLcd::paintRow(g, getLocalBounds(), scale, {}, open ? juce::String::fromUTF8(Text::openMarkUtf8) : juce::String(), open);
}

void StyleViewComponent::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &label && onStyleClicked)
    {
        if (event.mods.isLeftButtonDown())
        {
            if(onStyleClicked)
                onStyleClicked(label.getText());
        }
        else if (event.mods.isRightButtonDown())
        {
            juce::PopupMenu menu;
            if (label.getText() == StyleNames::defaultStyle)
            {
                menu.addItem(Text::cannotChange, [this]() {

                    });
            }
            else
            {
                menu.addItem(Text::renameItem, [this]() {
                    changeNameLabel();
                    });

                menu.addItem(Text::deleteItem, [this]() {
                    removeStyle();
                    });


            }
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this));
        }
    }
}

void StyleViewComponent::setNameLabel(const juce::String& name)
{
    label.setText(name, juce::dontSendNotification);
}

juce::String StyleViewComponent::getNameLabel() const
{
    return label.getText();
}

void StyleViewComponent::changeNameLabel()
{
    if (! openDialog)
        return;

    openDialog({ Text::renameStyleTitle, StyleDialogText::nameLabel, StyleDialogText::okButton, true, label.getText() },
        [this](const juce::String& theNewName)
        {
            const juce::String oldName = label.getText();

            if (theNewName.isEmpty())
            {
                if (onInfo)
                    onInfo(Text::emptyName);
                return;
            }

            if (isInListNames && isInListNames(theNewName))
            {
                if (onInfo)
                    onInfo(Text::nameExists);
                return;
            }

            setNameLabel(theNewName);

            if (onStyleRenamed)
                onStyleRenamed(oldName, theNewName);
        });
}

void StyleViewComponent::removeStyle()
{
    if (! openDialog)
        return;

    openDialog({ Text::removeStyleTitle, Text::removePromptStart + label.getText() + Text::removePromptEnd, Text::removeButton },
        [this](const juce::String&)
        {
            const auto name = label.getText();
            const auto info = onInfo;   // the removal deletes this row
            if (onStyleRemoveComponent)
                onStyleRemoveComponent(name);
            if (info)
                info(Text::removed);
        });
}
