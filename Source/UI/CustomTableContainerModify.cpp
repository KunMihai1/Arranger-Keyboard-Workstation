#include "CustomTableContainer.h"


void TableContainer::modifyTimeStampsUI()
{
    showModifyChangeDialog("Confirm action",
        "What would you like to do with the time stamps?",
        "What would you like to do with the time stamps?",
        { "Modify All", "Modify selected", "Cancel" },
        { "Modify All", "Cancel" },
        [this](int result)
        {
            if (result == 1)
                modifyOnlyTimeStamps();
            else if (result == 2)
                modifyOnlyTimeStampsSelected(table->getSelectedRows());
        },true);
}

void TableContainer::modifyVelocitiesUI()
{
    showModifyChangeDialog("Confirm action",
        "What would you like to do with the velocities?",
        "What would you like to do with the velocities?",
        { "Modify All", "Modify selected", "Cancel" },
        { "Modify All", "Cancel" },
        [this](int result)
        {
            if (result == 1)
            {
                modifyOnlyVelocities();
            }
            else if (result == 2)
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
        for (int rangeIndex = 0; rangeIndex < selected->getNumRanges(); ++rangeIndex)
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
        for (int rangeIndex = 0; rangeIndex < selected->getNumRanges(); ++rangeIndex)
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
    auto* e = originalSequence.getEventPointer(originalIndex);
    if (e == nullptr)
        return;

    auto oldMsg = e->message;

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
       juce::String title = "Modify", text, label;
       int selectedID = actionModifyCB->getSelectedId();

       if (selectedID == 1)
       {
           text = "Shift the time stamps of your notes";
           title += " time stamps";
           label = "Time stamp";
       }
       else if (selectedID == 2)
       {
           text = "Enter a new velocity for your notes";
           title += " velocities";
           label = "Velocity";
       }
       label += " value:";

       auto* window = new juce::AlertWindow{ title, text, juce::AlertWindow::NoIcon };

       window->addTextEditor("propertyEditor", "", label);

       window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
       window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

       window->enterModalState(true,
           juce::ModalCallbackFunction::create([this, window, selectedID, title,messageAllSelected,messageNoSelection,buttonsWithSelection,buttonsWithoutSelection,onValidResults](int result)
               {
                   std::unique_ptr<juce::AlertWindow> cleanup{ window };
                   if (result != 1)
                       return;

                   juce::String userValue = window->getTextEditor("propertyEditor")->getText().trim();
                   if (selectedID == 1)
                   {
                       if (!Validator::isValidMidiDoubleString(userValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Modify time stamps", "Invalid input value");
                           return;
                       }

                       double doubleValue = userValue.getDoubleValue();

                       int currentRow = table->getSelectedRows()[0];
                       DBG("current row is: " + juce::String(currentRow));

                       double timeStampPreviousNoteOn=model->getPreviousNoteOnTimeStamp(currentRow);
                       double timeStampCurrentNoteOn = model->getCurrentNoteOnTimeStamp(currentRow);
                       double timeStampNextNoteOn = model->getNextNoteOnTimeStamp(currentRow);

                       DBG("prev: " + juce::String(timeStampPreviousNoteOn) + " current: " + juce::String(timeStampCurrentNoteOn) + " next: " + juce::String(timeStampNextNoteOn));

                       if (timeStampPreviousNoteOn == -1)
                           return;

                       if (!Validator::isValidMidiDoubleValueTimeStamps(doubleValue,timeStampCurrentNoteOn,timeStampPreviousNoteOn,timeStampNextNoteOn))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Modify time stamps", "Invalid value");
                           return;
                       }
                       editorValue = doubleValue;
                   }
                   else if (selectedID == 2)
                   {
                       if (!Validator::isValidMidiIntegerString(userValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Modify velocities", "Invalid input value");
                           return;
                       }

                       int intValue = userValue.getIntValue();

                       if (!Validator::isValidMidiIntValue(intValue))
                       {
                           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Modify velocities", "Invalid value");
                           return;
                       }

                       editorValue = intValue;
                   }

                   auto* alert = new juce::AlertWindow(title, anySelected() ? messageAllSelected : messageNoSelection, juce::AlertWindow::QuestionIcon);

                   auto& buttons = anySelected() ? buttonsWithSelection : buttonsWithoutSelection;

                   for (int i = 0; i < buttons.size(); i++)
                       alert->addButton(buttons[i], i + 1);

                   alert->enterModalState(true,
                       juce::ModalCallbackFunction::create([this, onValidResults](int result)
                           {
                               if (result == 0) return;
                               onValidResults(result);
                               onConfirmed();
                           }),
                       true);
                   
               }));
       juce::MessageManager::callAsync([window]()
           {
               if (auto* editor = window->getTextEditor("propertyEditor"))
                   editor->grabKeyboardFocus();
           });
    }
    else
    {
        auto* alert = new juce::AlertWindow(title, anySelected() ? messageAllSelected : messageNoSelection, juce::AlertWindow::QuestionIcon);

        auto& buttons = anySelected() ? buttonsWithSelection : buttonsWithoutSelection;

        for (int i = 0; i < buttons.size(); i++)
            alert->addButton(buttons[i], i + 1);

        alert->enterModalState(true,
            juce::ModalCallbackFunction::create([this, onValidResults](int result)
                {
                    if (result == 0) return;
                    onValidResults(result);
                    onConfirmed();
                }),
            true);
    }
}
