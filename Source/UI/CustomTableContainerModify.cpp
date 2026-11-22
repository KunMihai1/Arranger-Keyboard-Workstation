#include "CustomTableContainer.h"

namespace
{
    namespace Text
    {
        const juce::String confirmTitle          = "Confirm action";
        const juce::String askTimeStamps         = "What would you like to do with the time stamps?";
        const juce::String askVelocities         = "What would you like to do with the velocities?";
        const juce::String modifyTitle           = "Modify";
        const juce::String shiftTimeStamps       = "Shift the time stamps of your notes";
        const juce::String timeStampsSuffix      = " time stamps";
        const juce::String timeStampLabel        = "Time stamp";
        const juce::String enterVelocity         = "Enter a new velocity for your notes";
        const juce::String velocitiesSuffix      = " velocities";
        const juce::String velocityLabel         = "Velocity";
        const juce::String valueSuffix           = " value:";
        const juce::String propertyEditor        = "propertyEditor";
        const juce::String okButton              = "OK";
        const juce::String modifyTimeStampsTitle = "Modify time stamps";
        const juce::String modifyVelocitiesTitle = "Modify velocities";
        const juce::String invalidInput          = "Invalid input value";
        const juce::String invalidValue          = "Invalid value";
        const juce::String currentRowLog         = "current row is: ";
        const juce::String previousLog           = "prev: ";
        const juce::String currentLog            = " current: ";
        const juce::String nextLog               = " next: ";
    }

    constexpr int dialogOk       = 1;   // the value dialog's OK / Cancel results
    constexpr int dialogCancel   = 0;
    constexpr int firstSelection = 0;
}


void TableContainer::modifyTimeStampsUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askTimeStamps,
        Text::askTimeStamps,
        { TableDialog::modifyAll, TableDialog::modifySelected, TableDialog::cancel },
        { TableDialog::modifyAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
                modifyOnlyTimeStamps();
            else if (result == TableDialog::applyToSelected)
                modifyOnlyTimeStampsSelected(table->getSelectedRows());
        },true);
}

void TableContainer::modifyVelocitiesUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askVelocities,
        Text::askVelocities,
        { TableDialog::modifyAll, TableDialog::modifySelected, TableDialog::cancel },
        { TableDialog::modifyAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
            {
                modifyOnlyVelocities();
            }
            else if (result == TableDialog::applyToSelected)
            {
                modifyOnlyVelocitiesSelected(table->getSelectedRows());
            }
        },true);
}

void TableContainer::modifyOnlyTimeStamps()
{
    newPropertyAndApply([this](MidiChangeInfo& info)
        {
            double old = info.newTimeStamp;
            if (std::holds_alternative<double>(editorValue))
                info.newTimeStamp = old+std::get<double>(editorValue);
            else if (std::holds_alternative<int>(editorValue))
                info.newTimeStamp = old+ static_cast<double>(std::get<int>(editorValue));
        });
}

void TableContainer::modifyOnlyTimeStampsSelected(const juce::SparseSet<int>& allSelected)
{
    newPropertyAndApply([this](MidiChangeInfo& info)
        {
            double old= info.newTimeStamp;
            if (std::holds_alternative<double>(editorValue))
                info.newTimeStamp = old + std::get<double>(editorValue);
            else if (std::holds_alternative<int>(editorValue))
                info.newTimeStamp = old + static_cast<double>(std::get<int>(editorValue));

        },&allSelected);
}

void TableContainer::modifyOnlyVelocities()
{
    newPropertyAndApply([this](MidiChangeInfo& info)
        {
            if (std::holds_alternative<int>(editorValue))
                info.newVelocity = std::get<int>(editorValue);
            else if (std::holds_alternative<double>(editorValue))
                info.newVelocity = static_cast<int>(std::get<double>(editorValue));

        },nullptr,true);
}

void TableContainer::modifyOnlyVelocitiesSelected(const juce::SparseSet<int>& allSelected)
{
    newPropertyAndApply([this](MidiChangeInfo& info)
        {
            if (std::holds_alternative<int>(editorValue))
                info.newVelocity = std::get<int>(editorValue);
            else if (std::holds_alternative<double>(editorValue))
                info.newVelocity = static_cast<int>(std::get<double>(editorValue));

        },&allSelected,true);
}


void TableContainer::resetPropertyAndApply(const std::function<void(MidiChangeInfo&)>& resetProperty, const juce::SparseSet<int>* selected, bool modifyVelocity)
{
    if (selected!=nullptr)
    {
        for (int rangeIndex {}; rangeIndex < selected->getNumRanges(); ++rangeIndex)
        {
            auto range = selected->getRange(rangeIndex);
            for (int row = range.getStart(); row < range.getEnd(); ++row)
            {
                int originalIndex = model->getOriginalIndexFromRow(row);
                int noteOnIndex = model->getChangesMapIndexFromRow(row);

                auto it = changesMap.find(noteOnIndex);

                if (it != changesMap.end())
                {
                    auto& info = it->second;
                    resetProperty(info);
                    applyChangeFromSequence(originalIndex, info, modifyVelocity);

                    if (validForErase(info))
                        changesMap.erase(it);
                }
            }
        }
    }
    else
    {
        for (auto it = changesMap.begin(); it != changesMap.end(); )
        {
            auto& key = it->first;
            auto& info = it->second;

            int originalIndex = model->getOriginalIndexFromRow(key);

            resetProperty(info);
            applyChangeFromSequence(originalIndex, info, modifyVelocity);

            if (validForErase(info))
                it = changesMap.erase(it);
            else
                ++it;
        }
    }
}

void TableContainer::newPropertyAndApply(const std::function<void(MidiChangeInfo&)>& newProperty, const juce::SparseSet<int>* selected, bool modifyVelocity)
{
    if (selected != nullptr)
    {
        for (int rangeIndex {}; rangeIndex < selected->getNumRanges(); ++rangeIndex)
        {
            auto range = selected->getRange(rangeIndex);
            for (int row = range.getStart(); row < range.getEnd(); ++row)
            {
                int originalIndex = model->getOriginalIndexFromRow(row);
                int noteOnIndex = model->getChangesMapIndexFromRow(row);
                newPropertyHelperFunction(newProperty, selected, modifyVelocity,originalIndex, noteOnIndex);
            }
        }
    }
    else
    {
        for (auto& entry : model->getEvents())
        {
            auto& originalIndex = entry.originalIndex;
            auto& noteOnIndex = entry.indexForChangesMap;
            newPropertyHelperFunction(newProperty, selected, modifyVelocity,originalIndex, noteOnIndex);
        }
    }

}

void TableContainer::newPropertyHelperFunction(const std::function<void(MidiChangeInfo&)>& newProperty, const juce::SparseSet<int>* selected, bool modifyVelocity, int originalIndex, int noteOnIndex)
{
    auto* event = originalSequence.getEventPointer(originalIndex);
    if (event == nullptr)
        return;

    auto oldMsg = event->message;

    auto [it, inserted] = changesMap.try_emplace(noteOnIndex);
    MidiChangeInfo& info = it->second;

    if (inserted)
    {
        
        info.oldNumber = oldMsg.getNoteNumber();
        info.oldTimeStamp = oldMsg.getTimeStamp();
        info.oldVelocity = oldMsg.getVelocity();
        info.oldBPMchange = getCurrentBPMstyle();

        info.newBPMchange = getCurrentBPMstyle();
        info.newNumber = oldMsg.getNoteNumber();
        info.newTimeStamp = oldMsg.getTimeStamp();
        info.newVelocity = oldMsg.getVelocity();
    }

    newProperty(info);

    if (modifyVelocity)
        applyChangeFromSequence(originalIndex, info, true,true);
    else
        applyChangeFromSequence(originalIndex, info,false,true);

    info.newBPMchange = getCurrentBPMstyle();

    if (validForErase(info))
        changesMap.erase(noteOnIndex);
}

void TableContainer::showModifyChangeDialog(
    const juce::String& title,
    const juce::String& messageAllSelected,
    const juce::String& messageNoSelection,
    const std::vector<juce::String>& buttonsWithSelection,
    const std::vector<juce::String>& buttonsWithoutSelection,
    std::function<void(int)> onValidResults,
    bool isModify)
{

    if (isModify)
    {
       juce::String title = Text::modifyTitle, text, label;
       int selectedID = actionModifyCB->getSelectedId();

       if (selectedID == ModifyActionIds::timeStamps)
       {
           text = Text::shiftTimeStamps;
           title += Text::timeStampsSuffix;
           label = Text::timeStampLabel;
       }
       else if (selectedID == ModifyActionIds::velocities)
       {
           text = Text::enterVelocity;
           title += Text::velocitiesSuffix;
           label = Text::velocityLabel;
       }
       label += Text::valueSuffix;

       auto* window = new juce::AlertWindow{ title, text, juce::AlertWindow::NoIcon };

       window->addTextEditor(Text::propertyEditor, {}, label);

       window->addButton(Text::okButton, dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
       window->addButton(TableDialog::cancel, dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

       window->enterModalState(true,
           juce::ModalCallbackFunction::create([this, window, selectedID, title,messageAllSelected,messageNoSelection,buttonsWithSelection,buttonsWithoutSelection,onValidResults](int result)
               {
                   std::unique_ptr<juce::AlertWindow> cleanup{ window };
                   if (result != dialogOk)
                       return;

                   juce::String userValue = window->getTextEditor(Text::propertyEditor)->getText().trim();
                   if (selectedID == ModifyActionIds::timeStamps)
                   {
                       if (!Validator::isValidMidiDoubleString(userValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::modifyTimeStampsTitle, Text::invalidInput);
                           return;
                       }

                       double doubleValue = userValue.getDoubleValue();

                       int currentRow = table->getSelectedRows()[firstSelection];
                       DBG(Text::currentRowLog + juce::String(currentRow));

                       double timeStampPreviousNoteOn=model->getPreviousNoteOnTimeStamp(currentRow);
                       double timeStampCurrentNoteOn = model->getCurrentNoteOnTimeStamp(currentRow);
                       double timeStampNextNoteOn = model->getNextNoteOnTimeStamp(currentRow);

                       DBG(Text::previousLog + juce::String(timeStampPreviousNoteOn) + Text::currentLog + juce::String(timeStampCurrentNoteOn) + Text::nextLog + juce::String(timeStampNextNoteOn));

                       if (timeStampPreviousNoteOn == Validator::noTimeStamp)
                           return;

                       if (!Validator::isValidMidiDoubleValueTimeStamps(doubleValue,timeStampCurrentNoteOn,timeStampPreviousNoteOn,timeStampNextNoteOn))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::modifyTimeStampsTitle, Text::invalidValue);
                           return;
                       }
                       editorValue = doubleValue;
                   }
                   else if (selectedID == ModifyActionIds::velocities)
                   {
                       if (!Validator::isValidMidiIntegerString(userValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::modifyVelocitiesTitle, Text::invalidInput);
                           return;
                       }

                       int intValue = userValue.getIntValue();

                       if (!Validator::isValidMidiIntValue(intValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::modifyVelocitiesTitle, Text::invalidValue);
                           return;
                       }

                       editorValue = intValue;
                   }

                   auto* alert = new juce::AlertWindow(title, anySelected() ? messageAllSelected : messageNoSelection, juce::AlertWindow::QuestionIcon);

                   auto& buttons = anySelected() ? buttonsWithSelection : buttonsWithoutSelection;

                   for (int buttonIndex {}; buttonIndex < buttons.size(); buttonIndex++)
                       alert->addButton(buttons[buttonIndex], buttonIndex + TableDialog::firstButtonId);

                   alert->enterModalState(true,
                       juce::ModalCallbackFunction::create([this, onValidResults](int result)
                           {
                               if (result == TableDialog::dismissed) return;
                               onValidResults(result);
                               onConfirmed();
                           }),
                       true);
                   
               }));
       juce::MessageManager::callAsync([window]()
           {
               if (auto* editor = window->getTextEditor(Text::propertyEditor))
                   editor->grabKeyboardFocus();
           });
    }
    else
    {
        auto* alert = new juce::AlertWindow(title, anySelected() ? messageAllSelected : messageNoSelection, juce::AlertWindow::QuestionIcon);

        auto& buttons = anySelected() ? buttonsWithSelection : buttonsWithoutSelection;

        for (int buttonIndex {}; buttonIndex < buttons.size(); buttonIndex++)
            alert->addButton(buttons[buttonIndex], buttonIndex + TableDialog::firstButtonId);

        alert->enterModalState(true,
            juce::ModalCallbackFunction::create([this, onValidResults](int result)
                {
                    if (result == TableDialog::dismissed) return;
                    onValidResults(result);
                    onConfirmed();
                }),
            true);
    }
}
