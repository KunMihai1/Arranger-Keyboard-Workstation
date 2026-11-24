#include "displayGUI.h"
#include "PersistedKeys.h"
#include "TrackListComponentNames.h"

// Folder view operations of TrackListComponent (split from TrackListComponent.cpp, ADR-003).
using namespace TrackListNames;

void TrackListComponent::addToFolderList()
{
    auto* window = new juce::AlertWindow(Text::createFolderTitle,
        Text::newFolderName,
        juce::AlertWindow::NoIcon);

    window->addTextEditor(Text::folderNameField, "");
    window->addButton(Text::okButton, dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(Text::cancelButton, dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != dialogOk)
                return;

            juce::String folderName = window->getTextEditor(Text::folderNameField)->getText().trim();
            if (folderName.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    Text::invalidName,
                    Text::emptyFolderName);
                return;
            }

            if (groupedTracks->count(folderName) > size_t {})
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    Text::duplicateName,
                    Text::folderNameTaken);
                return;
            }

            groupedTrackKeys->push_back(folderName);
            (*groupedTracks)[folderName] = {};
            currentFolderName = folderName;

            viewMode = ViewMode::FolderView;
            listBox.updateContent();
            listBox.repaint();

            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

        }));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(Text::folderNameField))
                editor->grabKeyboardFocus();
        });
}

void TrackListComponent::removeFromFolderList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::removeFoldersTitle, Text::selectFolderToRemove);
        return;
    }

    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        Text::confirmRemove,
        Text::sureRemoveFolders,
        Text::removeButton,
        Text::cancelButton,
        this,
        juce::ModalCallbackFunction::create(
            [this, selectedRows](int result)
            {
                if (result == dialogCancel)
                    return;

                removeFolders(selectedRows);

                auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
                TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
            }
        )
    );
}

// The library change itself, kept apart from the confirmation dialog and the save to disk.
void TrackListComponent::removeFolders(const juce::SparseSet<int>& selectedRows)
{
    for (int selectionIndex = selectedRows.size() - oneRow; selectionIndex >= int {}; selectionIndex--)
    {
        int rowIndex = selectedRows[selectionIndex];
        juce::String folderName = (*groupedTrackKeys)[rowIndex];

        auto& tracks = (*groupedTracks)[folderName];
        std::vector<juce::Uuid> uuidsToRemove;

        for (auto& track : tracks)
            uuidsToRemove.push_back(track.getUniqueID());

        tracks.clear();

        groupedTracks->erase(folderName);

        auto it = std::find(groupedTrackKeys->begin(), groupedTrackKeys->end(), folderName);
        if (it != groupedTrackKeys->end())
            groupedTrackKeys->erase(it);

        // Only now: the listener rebuilds its UUID -> entry map from the library, which must no
        // longer contain this folder's (destroyed) entries.
        if (onRemoveMultipleTracks)
            onRemoveMultipleTracks(uuidsToRemove);

        listBox.deselectAllRows();
        listBox.updateContent();
        listBox.repaint();
    }
}

void TrackListComponent::renameFromFolderList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::renameFolderTitle, Text::selectOneFolder);
        return;
    }

    if (selectedRows.size() > oneRow)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::renameFolderLower, Text::selectOnlyOneFolder);
        return;
    }

    auto* window = new juce::AlertWindow(Text::renameFolderTitle,
        Text::folderNamePrompt,
        juce::AlertWindow::NoIcon);

    const int selectedIndex = selectedRows[int {}];

    juce::String oldName = (* groupedTrackKeys)[selectedIndex];

    window->addTextEditor(Text::folderNameField, oldName);
    window->addButton(Text::okButton, dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(Text::cancelButton, dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, oldName, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != dialogOk)
                return;


            juce::String newName = window->getTextEditor(Text::folderNameField)->getText().trim();

            if (oldName == newName)
                return;

            if (groupedTracks->find(newName) != groupedTracks->end())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    Text::renameFolderLower, Text::folderNameTaken);
                return;
            }

            for (auto& folder : *groupedTrackKeys)
            {
                if (folder == oldName)
                {
                    folder = newName;
                    break;
                }
            }

            auto it = groupedTracks->find(oldName);
            if (it != groupedTracks->end())
            {
                auto entries = std::move(it->second);
                groupedTracks->erase(it);
                (*groupedTracks)[newName] = std::move(entries);
            }

            listBox.deselectAllRows();
            listBox.updateContent();
            listBox.repaint();

            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(Text::folderNameField))
                editor->grabKeyboardFocus();
        });
}

void TrackListComponent::backToFolderView()
{
    viewMode = ViewMode::FolderView;
    deallocateTracksFromList();
    listBox.deselectAllRows();
    listBox.updateContent();
    listBox.repaint();
    resized();
}
