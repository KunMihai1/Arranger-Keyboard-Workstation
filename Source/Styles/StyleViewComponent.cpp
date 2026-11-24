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
        const juce::String confirmRemoveTitle = "Confirm Remove";
        const juce::String removePromptStart  = "Are you sure you want to remove ' ";
        const juce::String removePromptEnd    = "' ?";
        const juce::String removeButton       = "Remove";
        const juce::String removedTitle       = "Remove style";
        const juce::String removed            = "Style removed succesfully!";
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
    auto* window = new juce::AlertWindow{ StyleDialogText::nameDialogTitle, StyleDialogText::nameDialogPrompt, juce::AlertWindow::NoIcon };

    window->addTextEditor(StyleDialogText::nameEditor, label.getText(), StyleDialogText::nameEditorLabel);

    window->addButton(StyleDialogText::okButton, StyleDialogText::dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(StyleDialogText::cancelButton, StyleDialogText::dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup{ window };
            if (result != StyleDialogText::dialogOk)
                return;

            juce::String oldName = label.getText();
            juce::String theNewName = window->getTextEditor(StyleDialogText::nameEditor)->getText().trim();

            if (theNewName.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::renameStyleTitle, Text::emptyName);
                return;
            }

            if (isInListNames)
            {
                if (isInListNames(theNewName))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::renameStyleTitle, Text::nameExists);
                    return;
                }
            }

            setNameLabel(theNewName);

            if (onStyleRenamed)
                onStyleRenamed(oldName, theNewName);


        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(StyleDialogText::nameEditor))
                editor->grabKeyboardFocus();
        });
}

void StyleViewComponent::removeStyle()
{
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        Text::confirmRemoveTitle,
        Text::removePromptStart + label.getText() + Text::removePromptEnd,
        Text::removeButton,
        StyleDialogText::cancelButton,
        this,
        juce::ModalCallbackFunction::create(
            [this](int result)
            {
                if (result == StyleDialogText::dialogCancel) // Cancel
                    return;

                if (onStyleRemoveComponent)
                    onStyleRemoveComponent(label.getText());

                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::removedTitle, Text::removed);

            }
        )
    );
}
