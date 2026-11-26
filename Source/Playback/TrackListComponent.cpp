#include "displayGUI.h"
#include "PersistedKeys.h"

#include "TrackListComponentNames.h"

using namespace TrackListNames;

TrackListComponent::TrackListComponent(std::shared_ptr<std::deque<TrackEntry>> tracks,
    std::shared_ptr<std::unordered_map<juce::String, std::deque<TrackEntry>>> groupedTracksMap,
    std::shared_ptr<std::vector<juce::String>> groupedKeys,
    std::function<void(int)> onTrackChosen) : availableTracks{ tracks },
    groupedTracks{ groupedTracksMap },
    groupedTrackKeys{ groupedKeys },
    trackChosenCallBack{ onTrackChosen }
{

    addAndMakeVisible(listBox);
    listBox.setMultipleSelectionEnabled(true);
    listBox.setModel(this);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.updateContent();

    const auto ellipsis = juce::String::fromUTF8(Text::ellipsisUtf8);
    addButtonFolder.setButtonText(Text::addFolder + ellipsis);
    renameButtonFolder.setButtonText(Text::renameShort + ellipsis);
    removeButtonFolder.setButtonText(Text::removeShort);
    openFolderButton.setButtonText(Text::openFolder);
    backButton.setButtonText(juce::String::fromUTF8(Text::backUtf8));
    addButton.setButtonText(Text::addFromMidi + ellipsis);
    renameButton.setButtonText(Text::renameShort + ellipsis);
    removeButton.setButtonText(Text::removeShort);
    sortLabel.setText(Text::sort);
    for (auto* big : { &useButton, &openFolderButton })
    {
        big->setBoardHeight(bigButtonHeight);
        big->setTextSize(bigButtonText);
    }

    addButtonFolder.onClick    = [this] { addToFolderList(); };
    removeButtonFolder.onClick = [this] { removeFromFolderList(); };
    renameButtonFolder.onClick = [this] { renameFromFolderList(); };
    openFolderButton.onClick   = [this] { openFolderAt(listBox.getSelectedRow()); };
    backButton.onClick         = [this] { backToFolderView(); };
    addButton.onClick          = [this] { addToTrackList(); };
    removeButton.onClick       = [this] { removeFromTrackList(); };
    renameButton.onClick       = [this] { renameFromTrackList(); };
    useButton.onClick          = [this]
    {
        const int row = listBox.getSelectedRow();
        if (viewMode == ViewMode::TrackView && juce::isPositiveAndBelow(row, availableTracks->size()) && trackChosenCallBack)
            trackChosenCallBack(row);   // as a double-click on the track
    };
    sortButton.onClick         = [this] { cycleSort(); };

    for (auto* control : { &addButtonFolder, &removeButtonFolder, &renameButtonFolder, &openFolderButton, &backButton,
                           &addButton, &removeButton, &renameButton, &useButton })
        addChildComponent(control);

    // Today's three sort choices; the sort button steps through them. Not shown.
    sortComboBox->addItem(Text::sortByName, SortIds::byName);
    sortComboBox->addItem(Text::sortNewestFirst, SortIds::newestFirst);
    sortComboBox->addItem(Text::sortOldestFirst, SortIds::oldestFirst);
    sortComboBox->setSelectedId(SortIds::byName, juce::dontSendNotification);
    sortComboBox->addListener(this);
    addChildComponent(sortComboBox.get());

    setSlotNumber(firstSlot);
    updateViewControls();
}

int TrackListComponent::getNumRows()
{
    if (viewMode == ViewMode::TrackView)
        return (int)availableTracks->size();
    else return (int)groupedTrackKeys->size();
}

void TrackListComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == sortComboBox.get())
    {
        int id = sortComboBox->getSelectedId();
        if (id == SortIds::byName)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.getDisplayName().toLowerCase() < second.getDisplayName().toLowerCase();
                });
        }
        else if (id == SortIds::newestFirst)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.file.getLastModificationTime() > second.file.getLastModificationTime();
                });
        }
        else if (id == SortIds::oldestFirst)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.file.getLastModificationTime() < second.file.getLastModificationTime();
                });
        }

        listBox.updateContent();
        listBox.repaint();
        updateViewControls();
    }
}

void TrackListComponent::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (event.getNumberOfClicks() == doubleClick)
    {
        if (viewMode == ViewMode::TrackView && juce::isPositiveAndBelow(row, availableTracks->size()))
        {
            if (trackChosenCallBack)
                trackChosenCallBack(row);
        }
        else if (viewMode == ViewMode::FolderView)
            openFolderAt(row);
    }
    repaint();   // the details column follows the selection
}

void TrackListComponent::openFolderAt(int row)
{
    if (viewMode != ViewMode::FolderView || ! juce::isPositiveAndBelow(row, groupedTrackKeys->size()))
        return;

    juce::String folderKey = (*groupedTrackKeys)[row];
    currentFolderName = folderKey;
    if (groupedTracks->find(folderKey) != groupedTracks->end())
    {
        *availableTracks = (*groupedTracks)[folderKey];
        initializeTracksFromList();
        resized();
        if (onFolderOpened)
            onFolderOpened(folderKey);

        listBox.deselectAllRows();
        listBox.updateContent();
        listBox.repaint();
    }
}

void TrackListComponent::addToTrackList()
{
    auto chooser = std::make_shared<juce::FileChooser>(Text::chooseMidiFile, juce::File{}, Text::midiFilePattern);

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                juce::FileInputStream inputStream(file);
                if (!inputStream.openedOk())
                {
                    showInfo(Text::couldNotOpen);
                    return;
                }

                juce::MidiFile midiFile;
                if (!midiFile.readFrom(inputStream))
                {
                    showInfo(Text::couldNotRead);
                    return;
                }

                double originalBpm = TrackIOHelper::getOriginalBpmFromFile(midiFile);
                TrackIOHelper::convertTicksToSeconds(midiFile, originalBpm);

                int totalTracks = midiFile.getNumTracks();
                int addedTracks {};
                int duplicateTracks {};

                for (int trackIndex {}; trackIndex < totalTracks; ++trackIndex)
                {
                    bool duplicate = false;
                    for (const auto& trackEntry : *availableTracks)
                    {
                        if (trackEntry.file == file && trackEntry.trackIndex == trackIndex)
                        {
                            duplicate = true;
                            break;
                        }
                    }
                    if (duplicate)
                    {
                        ++duplicateTracks;
                        continue;
                    }

                    auto* trackSequence = midiFile.getTrack(trackIndex);
                    juce::String displayName = extractDisplayNameFromTrack(*trackSequence);

                    TrackEntry newEntry;
                    newEntry.file = file;
                    newEntry.trackIndex = trackIndex;
                    newEntry.displayName = displayName;
                    newEntry.originalSequenceTicks = *trackSequence;

                    newEntry.sequence = *trackSequence;

                    newEntry.sequence.updateMatchedPairs();
                    newEntry.originalBPM = originalBpm;

                    newEntry.folderName = currentFolderName;

                    if (TrackIOHelper::foundPercussion(trackSequence))
                        newEntry.type = TrackType::Percussion;
                    else newEntry.type = TrackType::Melodic;

                    newEntry.uuid = TrackEntry::generateUUID();

                    availableTracks->push_back(newEntry);
                    (*groupedTracks)[currentFolderName].push_back(newEntry);

                    TrackEntry* storedPtr = &(*groupedTracks)[currentFolderName].back();

                    if (addToMapOnAdding)
                        addToMapOnAdding(storedPtr);

                    ++addedTracks;
                }

                listBox.updateContent();
                listBox.repaint();
                auto appDataFolder = IOHelper::getAppDataFolder();

                auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);

                TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

                int id = sortComboBox->getSelectedId();
                sortComboBox->setSelectedId(id);
                comboBoxChanged(sortComboBox.get());

                notifyAddingToTrackList();

                showInfo(juce::String(addedTracks) + Text::tracksAddedOf + juce::String(totalTracks)
                         + Text::tracksAddedMiddle + juce::String(duplicateTracks) + Text::tracksAlreadyListed);
            }
        }
    );

}

void TrackListComponent::removeFromTrackList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        showInfo(Text::selectTrackToRemove);
        return;
    }

    openDialog({ Text::removeTracksTitle, Text::sureRemoveTracks, Text::removeButton },
        [this, selectedRows](const juce::String&)
        {
            removeTracks(selectedRows);

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        });
}

// The library change itself, kept apart from the confirmation dialog and the save to disk.
void TrackListComponent::removeTracks(const juce::SparseSet<int>& selectedRows)
{
    for (int selectionIndex = selectedRows.size() - oneRow; selectionIndex >= int {}; --selectionIndex)
    {
        int rowIndex = selectedRows[selectionIndex];
        auto& track = (*availableTracks)[rowIndex];
        juce::Uuid uuid = track.getUniqueID();

        // The rows belong to the open folder. Don't go by track.folderName: a folder rename leaves
        // it stale, and operator[] would then create that old folder and erase(end()) from it.
        auto folder = groupedTracks->find(currentFolderName);
        if (folder != groupedTracks->end())
        {
            auto& folderVector = folder->second;
            folderVector.erase(std::remove_if(folderVector.begin(), folderVector.end(), [uuid](const TrackEntry& tr)
                {
                    return uuid == tr.getUniqueID();
                }),
                folderVector.end());
        }
        availableTracks->erase(availableTracks->begin() + rowIndex);

        if (onRemoveTrack) onRemoveTrack(uuid);
    }

    listBox.deselectAllRows();
    listBox.updateContent();
    listBox.repaint();
}

void TrackListComponent::renameFromTrackList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        showInfo(Text::selectOneTrack);
        return;
    }

    if (selectedRows.size() > oneRow)
    {
        showInfo(Text::selectOnlyOneTrack);
        return;
    }

    const auto& selected = (*availableTracks)[selectedRows[int {}]];
    const juce::String oldName = selected.displayName;
    const juce::Uuid trackUuid = selected.uuid;

    openDialog({ Text::renameTrackLower, Text::trackNameLabel, Text::okButton, true, oldName },
        [this, oldName, trackUuid](const juce::String& newName)
        {
            if (newName.isEmpty() || oldName == newName)
                return;

            TrackEntry* track = nullptr;
            for (auto& tr : *availableTracks)
            {
                if (tr.displayName == newName)  // no need for tr.displayName!=oldName cause of the previous check with return
                {
                    showInfo(Text::trackNameTaken);
                    return;
                }
                else if (tr.displayName == oldName)
                    track = &tr;
            }

            if (track != nullptr)
                track->displayName = newName;

            auto& tracks = (*groupedTracks)[currentFolderName];

            for (auto& tr : tracks)
            {
                if (tr.displayName == oldName)
                {
                    tr.displayName = newName;
                    break;
                }
            }

            comboBoxChanged(sortComboBox.get());

            listBox.deselectAllRows();
            listBox.updateContent();
            listBox.repaint();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

            if (onRenameTrackFromList)
                onRenameTrackFromList(trackUuid, newName);
        });
}


//the flat list availableTracks doesn't need to be built rn, because we have grouped tracks and that's structured, but for future if
//i wanna change anything, like to have idk, something with all the tracks, i won't need to iterate again through every structured folder name to get all the available tracks

std::deque<TrackEntry>& TrackListComponent::getAllAvailableTracks() const
{
    return *availableTracks;
}

juce::String TrackListComponent::extractDisplayNameFromTrack(const juce::MidiMessageSequence& trackSeq)
{
    juce::String trackName;
    juce::String instrumentName;

    for (int eventIndex {}; eventIndex < trackSeq.getNumEvents(); eventIndex++)
    {
        const auto& event = trackSeq.getEventPointer(eventIndex)->message;

        if (event.isTrackNameEvent())
        {
            trackName = event.getTextFromTextMetaEvent();
        }
        else if (event.isMetaEvent() && event.getMetaEventType() == instrumentNameMetaEvent)
        {
            instrumentName = event.getTextFromTextMetaEvent();
        }
    }

    if (trackName.isNotEmpty() && instrumentName.isNotEmpty())
        return trackName + Text::instrumentOpen + instrumentName + Text::instrumentClose;
    else if (trackName.isNotEmpty())
        return trackName;
    else if (instrumentName.isNotEmpty())
        return instrumentName;

    return Text::unnamedTrack;
}

void TrackListComponent::initializeTracksFromList()
{
    viewMode = ViewMode::TrackView;
    comboBoxChanged(sortComboBox.get());   // the folder opens in the chosen order
    updateViewControls();
}

void TrackListComponent::deallocateTracksFromList()
{
    updateViewControls();
}
