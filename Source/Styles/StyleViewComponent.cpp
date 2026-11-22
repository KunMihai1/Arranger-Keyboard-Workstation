#include "displayGUI.h"
#include "PersistedKeys.h"

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
    }

    constexpr float labelFontHeight = 18.0f;
    constexpr int   labelInset      = 10;
    constexpr int   labelExtraWidth = 10;
    constexpr int   labelExtraHeight = 6;
}

StyleViewComponent::StyleViewComponent(const juce::String& styleName)
{
    label.setFont(juce::Font(labelFontHeight, juce::Font::bold));
    label.setText(styleName, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, AppColours::accent3);
    label.setInterceptsMouseClicks(true, false);
    label.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addAndMakeVisible(label);

    label.addMouseListener(this, false);

}

void StyleViewComponent::resized()
{
    auto font = label.getFont();
    int textWidth = juce::GlyphArrangement::getStringWidthInt(font, label.getText());
    int textHeight = static_cast<int>(font.getHeight());

    label.setBounds(labelInset, labelInset, textWidth + labelExtraWidth, textHeight + labelExtraHeight);
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
