#include "displayGUI.h"
#include "PersistedKeys.h"
#include "TrackListComponentNames.h"

// Folder view operations of TrackListComponent (split from TrackListComponent.cpp, ADR-003).
using namespace TrackListNames;

void TrackListComponent::addToFolderList()
{
    openDialog({ Text::createFolderTitle, Text::folderNameLabel, Text::okButton, true, {} },
        [this](const juce::String& folderName)
        {
            if (folderName.isEmpty())
            {
                showInfo(Text::emptyFolderName);
                return;
            }

            if (groupedTracks->count(folderName) > size_t {})
            {
                showInfo(Text::folderNameTaken);
                return;
            }

            groupedTrackKeys->push_back(folderName);
            (*groupedTracks)[folderName] = {};
            currentFolderName = folderName;

            viewMode = ViewMode::FolderView;
            listBox.updateContent();
            listBox.repaint();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        });
}

void TrackListComponent::removeFromFolderList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        showInfo(Text::selectFolderToRemove);
        return;
    }

    openDialog({ Text::removeFoldersTitle, Text::sureRemoveFolders, Text::removeButton },
        [this, selectedRows](const juce::String&)
        {
            removeFolders(selectedRows);

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        });
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
        showInfo(Text::selectOneFolder);
        return;
    }

    if (selectedRows.size() > oneRow)
    {
        showInfo(Text::selectOnlyOneFolder);
        return;
    }

    const juce::String oldName = (*groupedTrackKeys)[selectedRows[int {}]];

    openDialog({ Text::renameFolderLower, Text::folderNameLabel, Text::okButton, true, oldName },
        [this, oldName](const juce::String& newName)
        {
            if (newName.isEmpty())
            {
                showInfo(Text::emptyFolderName);
                return;
            }

            if (oldName == newName)
                return;

            if (groupedTracks->find(newName) != groupedTracks->end())
            {
                showInfo(Text::folderNameTaken);
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

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        });
}

bool TrackListComponent::openFolderNamed(const juce::String& folder)
{
    const auto found = std::find(groupedTrackKeys->begin(), groupedTrackKeys->end(), folder);
    if (found == groupedTrackKeys->end())
        return false;
    openFolderAt(static_cast<int>(std::distance(groupedTrackKeys->begin(), found)));
    return viewMode == ViewMode::TrackView;
}

void TrackListComponent::backToFolderView()
{
    viewMode = ViewMode::FolderView;
    deallocateTracksFromList();
    if (onFolderOpened)
        onFolderOpened({});   // back at the folder list: the next picker starts here too
    listBox.deselectAllRows();
    listBox.updateContent();
    listBox.repaint();
    resized();
}
