/*
  ==============================================================================

    CustomTableContainer.cpp
    Created: 17 Jul 2025 10:33:04pm
    Author:  Kisuke

  ==============================================================================
*/

#include "CustomTableContainer.h"

namespace
{
    namespace Text
    {
        const juce::String notesTitlePrefix    = "Notes - ";
        const juce::String noteColumn          = "Note";
        const juce::String timeColumn          = "Time";
        const juce::String velocityColumn      = "Velocity";
        const juce::String channelColumn       = "Channel";
        const juce::String changeButton        = "Change";
        const juce::String applyButton         = "Apply";
        const juce::String everythingToDefault = "Everything to default";
        const juce::String notesToDefault      = "Notes to default";
        const juce::String timeStampsToDefault = "Timestamps to default";
        const juce::String velocitiesToDefault = "Velocities to default";
        const juce::String undoSettings        = "Undo settings";
        const juce::String modifyTimeStamps    = "Modify time stamps";
        const juce::String modifyVelocities    = "Modify velocities";
        const juce::String modifySettings      = "Modify settings";
        const juce::String disclaimer          = "Note: This view shows the data used in the app only.\nYour original files remain unchanged.";
        const juce::String confirmTitle        = "Confirm Action";
        const juce::String askTrackProperties  = "What would you like to do with the track properties?";
        const juce::String askNotes            = "What would you like to do with the notes?";
        const juce::String askTimeStamps       = "What would you like to do with the time stamps?";
        const juce::String askVelocities       = "What would you like to do with the velocities?";
    }

    constexpr int   firstRow            = 0;
    constexpr int   singleRow           = 1;     // a SparseSet range [row, row + 1) is one row
    constexpr int   noteColumnWidth     = 100;
    constexpr int   timeColumnWidth     = 120;
    constexpr int   velocityColumnWidth = 100;
    constexpr int   channelColumnWidth  = 80;
    constexpr float disclaimerFontHeight = 12.0f;
    constexpr int   labelHeight         = 40;
    constexpr int   buttonsControlBarHeight = 30;
    constexpr int   controlBars         = 2;     // the reset bar and the modify bar
    constexpr int   horizontalPadding   = 10;
    constexpr int   comboWidth          = 150;
    constexpr int   buttonWidth         = 50;
    constexpr int   centringDivisor     = 2;
    constexpr int   lowestVelocity      = 0;
    constexpr int   highestVelocity     = 127;
}



TableContainer::TableContainer(juce::MidiMessageSequence& seq, const juce::String& displayName, int channel, std::unordered_map<int, MidiChangeInfo>& map, bool shouldDisable): changesMap{map}, originalSequence{seq}
{
    juce::String displayN = Text::notesTitlePrefix + displayName;
    model = std::make_unique<MidiNotesTableModel>(seq, channel, map);
    table = std::make_unique<juce::TableListBox>(displayN, nullptr);

    model->onUpdate = [this](int rowNumber)
    {
        table->repaintRow(rowNumber);
        if (updateToFile)
            updateToFile();
    };

    model->refreshData = [this]()
    {
        table->updateContent();
    };

    model->onRequestSelectRow = [this](int row)
    {
        if (row >= firstRow && row < model->getNumRows())
        {
            auto mods = juce::ModifierKeys::getCurrentModifiers();

            if (mods.isShiftDown())
            {
                int anchor = (lastSelectedRow >= firstRow) ? lastSelectedRow : row;
                int start = std::min(anchor, row);
                int end = std::max(anchor, row);

                // Copy current selection to an array
                juce::Array<int> selectionArray;
                auto selectionSet = table->getSelectedRows();

                for (int selectionIndex {}; selectionIndex < selectionSet.size(); ++selectionIndex)
                    selectionArray.add(selectionSet[selectionIndex]);

                // Add the new range to the array if not already selected
                for (int rowIndex = start; rowIndex <= end; ++rowIndex)
                    if (!selectionArray.contains(rowIndex))
                        selectionArray.add(rowIndex);

                // Create new SparseSet for selection
                juce::SparseSet<int> newSelection;
                for (auto selectedRow : selectionArray)
                    newSelection.addRange({ selectedRow, selectedRow + singleRow });  // add single row as range [row, row+1)

                // Set the updated selection
                table->setSelectedRows(newSelection);

                lastSelectedRow = row;
            }
            else if (mods.isCtrlDown())
            {
                juce::Array<int> selectionArray;
                auto selectionSet = table->getSelectedRows();

                for (int selectionIndex {}; selectionIndex < selectionSet.size(); selectionIndex++)
                {
                    selectionArray.add(selectionSet[selectionIndex]);
                }

                bool isSelected;

                if (selectionSet.contains(row))
                    isSelected = true;
                else isSelected = false;

                if (isSelected)
                {
                    selectionArray.removeFirstMatchingValue(row);
                }
                else {
                    selectionArray.add(row);
                }

                juce::SparseSet<int> newSelection;
                for (auto selectedRow : selectionArray)
                    newSelection.addRange({ selectedRow, selectedRow + singleRow });

                table->setSelectedRows(newSelection);

                lastSelectedRow = row;
            }
            else
            {
                // Normal single selection, deselect others
                table->selectRow(row, false);
                lastSelectedRow = row;
            }
        }
    };

    model->onMidPlayRepaint = [this](int row)
    {
        if (row >= firstRow && row < model->getNumRows())
        {
            table->deselectAllRows();

            table->selectRow(row);

            table->scrollToEnsureRowIsOnscreen(row);

            table->repaint();
        }
        else table->deselectAllRows();
    };

    model->getCurrentBpm = [this]()
    {
        return getCurrentBPMstyle();
    };

    model->validForErase = [this](MidiChangeInfo& info)
    {
        return validForErase(info);
    };

    table->setModel(model.get());

    table->setMultipleSelectionEnabled(true);

    table->getHeader().addColumn(Text::noteColumn, NoteTableColumns::note, noteColumnWidth);
    table->getHeader().addColumn(Text::timeColumn, NoteTableColumns::time, timeColumnWidth);
    table->getHeader().addColumn(Text::velocityColumn, NoteTableColumns::velocity, velocityColumnWidth);
    table->getHeader().addColumn(Text::channelColumn, NoteTableColumns::channel, channelColumnWidth);

    addAndMakeVisible(table.get());

    changeToOriginalMultipleButton = std::make_unique<juce::TextButton>(Text::changeButton);
    changeToOriginalMultipleButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    if (shouldDisable)
        changeToOriginalMultipleButton->setEnabled(false);

    changeToOriginalMultipleButton->onClick = [this]()
    {
        int selectedID = actionToOriginalCB->getSelectedId();

        if (selectedID == ResetActionIds::everything)
        {
            changeAllUI();
        }
        else if (selectedID == ResetActionIds::notes)
        {
            changeNotesUI();
        }
        else if (selectedID == ResetActionIds::timeStamps)
        {
            changeTimeStampsUI();
        }
        else if (selectedID == ResetActionIds::velocities)
        {
            changeVelocitiesUI();
        }

    };
    addAndMakeVisible(changeToOriginalMultipleButton.get());

    actionToOriginalCB = std::make_unique<juce::ComboBox>();

    actionToOriginalCB->addItem(Text::everythingToDefault, ResetActionIds::everything);
    actionToOriginalCB->addItem(Text::notesToDefault, ResetActionIds::notes);
    actionToOriginalCB->addItem(Text::timeStampsToDefault, ResetActionIds::timeStamps);
    actionToOriginalCB->addItem(Text::velocitiesToDefault, ResetActionIds::velocities);

    actionToOriginalCB->setSelectedId(ResetActionIds::everything);

    addAndMakeVisible(actionToOriginalCB.get());

    infoLabelChange = std::make_unique<juce::Label>();
    infoLabelChange->setText(Text::undoSettings, juce::dontSendNotification);
    infoLabelChange->setJustificationType(juce::Justification::centred);

    addAndMakeVisible(infoLabelChange.get());

    modifyMultipleButton = std::make_unique<juce::TextButton>(Text::applyButton);
    modifyMultipleButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    if (shouldDisable)
        modifyMultipleButton->setEnabled(false);

    modifyMultipleButton->onClick = [this]()
    {
        int selectedID = actionModifyCB->getSelectedId();
        if (selectedID == ModifyActionIds::timeStamps)
        {
            modifyTimeStampsUI();
        }
        else if (selectedID == ModifyActionIds::velocities)
        {
            modifyVelocitiesUI();
        }
    };

    addAndMakeVisible(modifyMultipleButton.get());

    actionModifyCB = std::make_unique<juce::ComboBox>();

    actionModifyCB->addItem(Text::modifyTimeStamps,ModifyActionIds::timeStamps);
    actionModifyCB->addItem(Text::modifyVelocities, ModifyActionIds::velocities);
    
    actionModifyCB->setSelectedId(ModifyActionIds::timeStamps);

    addAndMakeVisible(actionModifyCB.get());

    infoLabelModify = std::make_unique<juce::Label>();
    infoLabelModify->setText(Text::modifySettings, juce::dontSendNotification);
    infoLabelModify->setJustificationType(juce::Justification::centred);

    addAndMakeVisible(infoLabelModify.get());

    disclaimerLabel = std::make_unique<juce::Label>();
    disclaimerLabel->setText(Text::disclaimer, juce::dontSendNotification);
    disclaimerLabel->setJustificationType(juce::Justification::centred);
    disclaimerLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    disclaimerLabel->setFont(juce::Font(disclaimerFontHeight));
    addAndMakeVisible(disclaimerLabel.get());
    table->setBounds(getLocalBounds());
    table->updateContent();
}

void TableContainer::resized()
{
    auto area = getLocalBounds();
    const int spacing = 10;
    const int verticalPadding = 5;
    const int infoLabelHeight = 20;

    table->setBounds(area.removeFromTop(area.getHeight() - (labelHeight + buttonsControlBarHeight*controlBars)));

    auto disclaimerArea = area.removeFromBottom(labelHeight);
    auto controlBarChangeArea = area.removeFromBottom(buttonsControlBarHeight);
    auto controlBarModifyArea = area.removeFromBottom(buttonsControlBarHeight);

    auto innerBarChange = controlBarChangeArea.reduced(horizontalPadding, verticalPadding);

    actionToOriginalCB->setBounds(innerBarChange.removeFromLeft(comboWidth));
    innerBarChange.removeFromLeft(spacing);
    changeToOriginalMultipleButton->setBounds(innerBarChange.removeFromLeft(buttonWidth));

    infoLabelChange->setBounds(innerBarChange.withTrimmedTop((innerBarChange.getHeight() - infoLabelHeight) / centringDivisor)
        .withHeight(infoLabelHeight));

    auto innerBarModify = controlBarModifyArea.reduced(horizontalPadding, verticalPadding);

    actionModifyCB->setBounds(innerBarModify.removeFromLeft(comboWidth));
    innerBarModify.removeFromLeft(spacing);
    modifyMultipleButton->setBounds(innerBarModify.removeFromLeft(buttonWidth));

    infoLabelModify->setBounds(innerBarModify.withTrimmedTop((innerBarModify.getHeight() - infoLabelHeight) / centringDivisor)
        .withHeight(infoLabelHeight));

    disclaimerLabel->setBounds(disclaimerArea);
}

void TableContainer::updateObjects()
{
    // Follow the player's actual state rather than flipping: notifications don't pair up one-to-one
    // with play/stop (Start while playing posts one, Stop while stopped posts one).
    if (! isPlayerRunning)
        return;

    const bool editingAllowed = ! isPlayerRunning();

    if (changeToOriginalMultipleButton)
        changeToOriginalMultipleButton->setEnabled(editingAllowed);

    if (modifyMultipleButton)
        modifyMultipleButton->setEnabled(editingAllowed);
}

TableContainer::~TableContainer()
{
    table->setModel(nullptr);

    if (model && removeModelFromListener)
        removeModelFromListener(model.get());

    if (removeContainerFromListeners)
        removeContainerFromListeners();
}

void TableContainer::onlyNotes()
{
    resetPropertyAndApply([](MidiChangeInfo& info)
    {
            info.newNumber = info.oldNumber;
    });
}

void TableContainer::onlyNotesSelected(const juce::SparseSet<int>& allSelected)
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newNumber = info.oldNumber;

        } ,
        &allSelected);
}

void TableContainer::onlyTimeStamps()
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newTimeStamp = info.oldTimeStamp;
        },nullptr,false);
}

void TableContainer::onlyTimeStampsSelected(const juce::SparseSet<int>& allSelected)
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newTimeStamp = info.oldTimeStamp;
        },
        &allSelected,false);
}

void TableContainer::onlyVelocities()
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newVelocity = info.oldVelocity;

        },nullptr,true);
    
}

void TableContainer::onlyVelocitiesSelected(const juce::SparseSet<int>& allSelected)
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newVelocity = info.oldVelocity;

        },
        &allSelected,true);
}

void TableContainer::allProperties()
{
    for (auto& pair : changesMap)
    {
        auto& key = pair.first;
        
        int originalIndex = model->getOriginalIndexFromRow(key);

        auto& info = pair.second;
        info.newNumber = info.oldNumber;
        info.newTimeStamp = info.oldTimeStamp;
        info.newVelocity = info.oldVelocity;

        applyChangeFromSequence(originalIndex, info,false);
    }
    changesMap.clear();
}

void TableContainer::allPropertiesSelected(const juce::SparseSet<int>& allSelected)
{
    resetPropertyAndApply([](MidiChangeInfo& info)
        {
            info.newNumber = info.oldNumber;
            info.newTimeStamp = info.oldTimeStamp;
            info.newVelocity = info.oldVelocity;
        }, 
        &allSelected);
}

bool TableContainer::validForErase(MidiChangeInfo& info)
{
    if (info.newNumber == info.oldNumber && info.newTimeStamp == info.oldTimeStamp && info.newVelocity == info.oldVelocity)
        return true;
    return false;
}

void TableContainer::applyChangeFromSequence(int originalIndex, const MidiChangeInfo& info, bool modifyVelocity, bool shouldApplyChanges)
{
    auto* event = originalSequence.getEventPointer(originalIndex);
    if (event == nullptr)
        return;

    double ratio;
    double fromChange {};
    if (shouldApplyChanges) //this is not a good condition
    {
        fromChange = info.oldTimeStamp - info.newTimeStamp;
        ratio = info.newBPMchange / getCurrentBPMstyle();
    }
    else ratio = info.oldBPMchange / getCurrentBPMstyle();
    


    if (event->message.isNoteOn())
    {
        double duration {};
        if (!modifyVelocity && event->noteOffObject != nullptr)
        {
            auto* noteoffEvent = event->noteOffObject;
            duration = noteoffEvent->message.getTimeStamp() - event->message.getTimeStamp();
        }

        juce::MidiMessage newMsg = juce::MidiMessage::noteOn(
            event->message.getChannel(),
            info.newNumber,
            (juce::uint8)juce::jlimit(lowestVelocity, highestVelocity, info.newVelocity)
        );
        bool shouldClamp = false;

        double timeToApplyOn {};

        if (shouldApplyChanges)
        {
            timeToApplyOn = info.oldTimeStamp * ratio - fromChange;
        }
        else timeToApplyOn = info.newTimeStamp * ratio;

        /*
        if (timeToApplyOn < 0)
        {
            timeToApplyOn = 0.0;
            shouldClamp = true;
        }
        */

        newMsg.setTimeStamp(timeToApplyOn);

        event->message = newMsg;

        if (!modifyVelocity && event->noteOffObject != nullptr)
        {
            auto* noteOffEvent = event->noteOffObject;
            juce::MidiMessage offMsg = juce::MidiMessage::noteOff(
                noteOffEvent->message.getChannel(),
                info.newNumber
            );
            

            double offTime = info.newTimeStamp + duration;
                
            double timetoApplyOff {};
            if (shouldApplyChanges)
            {
                offTime = info.oldTimeStamp + duration;
                timetoApplyOff = offTime * ratio - fromChange;
            }
            else timetoApplyOff = offTime * ratio; 

            //if (shouldClamp)
            //    timetoApplyOff = duration; 

            offMsg.setTimeStamp(timetoApplyOff);

            noteOffEvent->message = offMsg;
        }
    }
}

void TableContainer::onConfirmed()
{
    int scrollPos = table->getViewport()->getViewPositionY();

    juce::SparseSet<int> selectedRows = table->getSelectedRows();

    table->setModel(nullptr);
    model->refreshVectorFromSequence(originalSequence);
    table->setModel(model.get());
    table->updateContent();

    table->setSelectedRows(selectedRows);

    table->getViewport()->setViewPosition(table->getViewport()->getViewPositionX(), scrollPos);

    table->repaint();

    if (updateToFile)
        updateToFile();
}

void TableContainer::changeAllUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askTrackProperties,
        Text::askTrackProperties,
        { TableDialog::changeAll, TableDialog::changeSelected, TableDialog::cancel },
        { TableDialog::changeAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
                allProperties();
            else if (result == TableDialog::applyToSelected)
                allPropertiesSelected(table->getSelectedRows());
        });
}

void TableContainer::changeNotesUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askNotes,
        Text::askNotes,
        { TableDialog::changeAll, TableDialog::changeSelected, TableDialog::cancel },
        { TableDialog::changeAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
                onlyNotes();
            else if (result == TableDialog::applyToSelected)
                onlyNotesSelected(table->getSelectedRows());
        });
}

void TableContainer::changeTimeStampsUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askTimeStamps,
        Text::askTimeStamps,
        { TableDialog::changeAll, TableDialog::changeSelected, TableDialog::cancel },
        { TableDialog::changeAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
                onlyTimeStamps();
            else if (result == TableDialog::applyToSelected)
                onlyTimeStampsSelected(table->getSelectedRows());
        });
}

void TableContainer::changeVelocitiesUI()
{
    showModifyChangeDialog(Text::confirmTitle,
        Text::askVelocities,
        Text::askVelocities,
        { TableDialog::changeAll, TableDialog::changeSelected, TableDialog::cancel },
        { TableDialog::changeAll, TableDialog::cancel },
        [this](int result)
        {
            if (result == TableDialog::applyToAll)
                onlyVelocities();
            else if (result == TableDialog::applyToSelected)
                onlyVelocitiesSelected(table->getSelectedRows());
        });
}

bool TableContainer::anySelected()
{
    if (table->getSelectedRows().size())
        return true;

    return false;
}

void TableContainer::addModelAsListenerToTrackPlayer(MultipleTrackPlayer* player)
{
    if (player)
        player->addSubjectTrackPlayerListener(model.get());
}
