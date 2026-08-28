#include "displayGUI.h"

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
    listBox.updateContent();
    addAndMakeVisible(addButtonFolder);
    addAndMakeVisible(removeButtonFolder);
    addAndMakeVisible(renameButtonFolder);

    addButtonFolder.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    removeButtonFolder.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    renameButtonFolder.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addButtonFolder.setLookAndFeel(&laf);
    addButtonFolder.setColour(juce::TextButton::buttonColourId, AppColours::accent7);

    removeButtonFolder.setLookAndFeel(&laf);
    removeButtonFolder.setColour(juce::TextButton::buttonColourId, AppColours::accent6);

    renameButtonFolder.setLookAndFeel(&laf);
    renameButtonFolder.setColour(juce::TextButton::buttonColourId, AppColours::accent4);

    addButtonFolder.onClick = [this] {
        addToFolderList();
    };

    removeButtonFolder.onClick = [this] {
        removeFromFolderList();
    };

    renameButtonFolder.onClick = [this] {
        renameFromFolderList();
    };
}

void TrackListComponent::resized()
{
    auto area = getLocalBounds();

    int controlBarHeight = 36;
    auto controlBarArea = area.removeFromTop(controlBarHeight);

    const int backWidth = 100;
    const int smallButtonWidth = 100;
    const int comboWidth = 150;
    const int spacing = 5;

    if (addButtonFolder.isVisible())
        addButtonFolder.setBounds(controlBarArea.removeFromLeft(smallButtonWidth).reduced(spacing));

    if (removeButtonFolder.isVisible())
        removeButtonFolder.setBounds(controlBarArea.removeFromLeft(smallButtonWidth).reduced(spacing));

    if (renameButtonFolder.isVisible())
        renameButtonFolder.setBounds(controlBarArea.removeFromLeft(smallButtonWidth).reduced(spacing));

    if (backButton)
        backButton->setBounds(controlBarArea.removeFromLeft(60).reduced(spacing));

    if (sortComboBox)
        sortComboBox->setBounds(controlBarArea.removeFromLeft(120).reduced(spacing));

    if (addButton)
        addButton->setBounds(controlBarArea.removeFromLeft(80).reduced(spacing));

    if (removeButton)
        removeButton->setBounds(controlBarArea.removeFromLeft(80).reduced(spacing));

    if (renameButton)
        renameButton->setBounds(controlBarArea.removeFromLeft(80).reduced(spacing));

    listBox.setBounds(area);
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
        if (id == 1)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.getDisplayName().toLowerCase() < second.getDisplayName().toLowerCase();
                });
        }
        else if (id == 2)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.file.getLastModificationTime() > second.file.getLastModificationTime();
                });
        }
        else if (id == 3)
        {
            std::sort(availableTracks->begin(), availableTracks->end(), [](const TrackEntry& first, const TrackEntry& second)
                {
                    return first.file.getLastModificationTime() < second.file.getLastModificationTime();
                });
        }

        listBox.updateContent();
        listBox.repaint();
    }
}

void TrackListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(AppColours::accent2.withAlpha(0.35f));
    else
        g.fillAll(AppColours::panelBg);

    g.setColour(AppColours::titleText);

    juce::String label;

    if (viewMode == ViewMode::TrackView)
    {
        if (juce::isPositiveAndBelow(rowNumber, availableTracks->size()))
            label = (*availableTracks)[rowNumber].getDisplayName();
    }
    else if (viewMode == ViewMode::FolderView)
    {
        if (juce::isPositiveAndBelow(rowNumber, groupedTrackKeys->size()))
            label = (*groupedTrackKeys)[rowNumber];
    }

    g.drawText(label, 5, 0, width - 10, height, juce::Justification::centredLeft);

    g.setColour(AppColours::separator);
    g.drawRect(0, 0, width, height, 1);
}

void TrackListComponent::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (event.getNumberOfClicks() == 2)
    {
        if (viewMode == ViewMode::TrackView && juce::isPositiveAndBelow(row, availableTracks->size()))
        {
            if (trackChosenCallBack)
                trackChosenCallBack(row);
        }
        else if (viewMode == ViewMode::FolderView)
        {
            if (juce::isPositiveAndBelow(row, groupedTrackKeys->size()))
            {
                juce::String folderKey = (*groupedTrackKeys)[row];
                currentFolderName = folderKey;
                if (groupedTracks->find(folderKey) != groupedTracks->end())
                {
                    *availableTracks = (*groupedTracks)[folderKey];
                    initializeTracksFromList();
                    resized();


                    listBox.updateContent();
                    listBox.repaint();
                }
            }
        }
    }
    else
    {

    }
}

void TrackListComponent::addToTrackList()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select a MIDI file to add", juce::File{}, "*.mid");

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
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Failed parsing",
                        "Could not open the selected file");
                    return;
                }

                juce::MidiFile midiFile;
                if (!midiFile.readFrom(inputStream))
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Failed parsing",
                        "Could not read the selected file");
                    return;
                }

                double originalBpm = TrackIOHelper::getOriginalBpmFromFile(midiFile);
                TrackIOHelper::convertTicksToSeconds(midiFile, originalBpm);

                int totalTracks = midiFile.getNumTracks();
                int addedTracks = 0;
                int duplicateTracks = 0;

                for (int i = 0; i < totalTracks; ++i)
                {
                    bool duplicate = false;
                    for (const auto& trackEntry : *availableTracks)
                    {
                        if (trackEntry.file == file && trackEntry.trackIndex == i)
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

                    auto* trackSequence = midiFile.getTrack(i);
                    juce::String displayName = extractDisplayNameFromTrack(*trackSequence);

                    TrackEntry newEntry;
                    newEntry.file = file;
                    newEntry.trackIndex = i;
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

                auto jsonFile = IOHelper::getFile("myTracks.json");

                TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

                int id = sortComboBox->getSelectedId();
                sortComboBox->setSelectedId(id);
                comboBoxChanged(sortComboBox.get());

                notifyAddingToTrackList();

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Add Tracks",
                    "Selected MIDI file has " + juce::String(totalTracks) + " tracks.\n"
                    + juce::String(addedTracks) + " tracks added.\n"
                    + juce::String(duplicateTracks) + " tracks were already in the list."
                );
            }
        }
    );

}

void TrackListComponent::removeFromTrackList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Remove Track(s)", "Please select at least one track to remove");
        return;
    }



    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        "Confirm Remove",
        "Are you sure you want to remove the selected track(s)?",
        "Remove",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create(
            [this, selectedRows](int result)
            {
                if (result == 0) // Cancel
                    return;


                for (int i = selectedRows.size() - 1; i >= 0; --i)
                {
                    int rowIndex = selectedRows[i];
                    auto& track = (*availableTracks)[rowIndex];
                    juce::Uuid uuid = track.getUniqueID();
                    juce::String folderName = track.folderName;

                    auto& folderVector = (*groupedTracks)[folderName];
                    folderVector.erase(std::remove_if(folderVector.begin(), folderVector.end(), [uuid](const TrackEntry& tr)
                        {
                            return uuid == tr.getUniqueID();

                        }));
                    availableTracks->erase(availableTracks->begin() + rowIndex);

                    if (onRemoveTrack) onRemoveTrack(uuid);
                }

                listBox.deselectAllRows();
                listBox.updateContent();
                listBox.repaint();

                auto appDataFolder = IOHelper::getAppDataFolder();

                auto jsonFile = IOHelper::getFile("myTracks.json");
                TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
            }
        )
    );
}

void TrackListComponent::renameFromTrackList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Rename Track", "Please select one track to rename");
        return;
    }

    if (selectedRows.size() > 1)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Rename track", "Please select only one track to rename");
        return;
    }

    auto* window = new juce::AlertWindow("Rename track",
        "Enter a new name for the track:",
        juce::AlertWindow::NoIcon);

    const int selectedIndex = selectedRows[0];

    auto& track = (*availableTracks)[selectedIndex];
    juce::String oldName = track.displayName;
    juce::Uuid trackUuid = track.uuid;

    window->addTextEditor("trackName", oldName);
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, oldName, window, trackUuid](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != 1)
                return;


            juce::String newName = window->getTextEditor("trackName")->getText().trim();

            if (oldName == newName)
                return;

            TrackEntry* track=nullptr;
            for (auto& tr : *availableTracks)
            {
                if (tr.displayName == newName)  // no need for tr.displayName!=oldName cause of the previous check with return
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "Rename track", "A track with this name already exists.");
                    return;
                }
                else if (tr.displayName == oldName)
                    track = &tr;
            }

            if(track!=nullptr)
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

            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile("myTracks.json");
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

            if (onRenameTrackFromList)
                onRenameTrackFromList(trackUuid, newName);

        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor("folderName"))
                editor->grabKeyboardFocus();
        });

}


void TrackListComponent::addToFolderList()
{
    auto* window = new juce::AlertWindow("Create Folder",
        "Enter a name for the new folder:",
        juce::AlertWindow::NoIcon);

    window->addTextEditor("folderName", "");
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != 1)
                return;

            juce::String folderName = window->getTextEditor("folderName")->getText().trim();
            if (folderName.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Invalid Name",
                    "Folder name cannot be empty.");
                return;
            }

            if (groupedTracks->count(folderName) > 0)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Duplicate Name",
                    "A folder with this name already exists.");
                return;
            }

            groupedTrackKeys->push_back(folderName);
            (*groupedTracks)[folderName] = {};
            currentFolderName = folderName;

            viewMode = ViewMode::FolderView;
            listBox.updateContent();
            listBox.repaint();

            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile("myTracks.json");
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

        }));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor("folderName"))
                editor->grabKeyboardFocus();
        });
}

void TrackListComponent::removeFromFolderList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Remove Folder(s)", "Please select at least one folder to remove");
        return;
    }

    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        "Confirm Remove",
        "Are you sure you want to remove the selected folder(s)?",
        "Remove",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create(
            [this, selectedRows](int result)
            {
                if (result == 0) // Cancel
                    return;

                for (int i = selectedRows.size() - 1; i >= 0; i--)
                {
                    int rowIndex = selectedRows[i];
                    juce::String folderName = (*groupedTrackKeys)[rowIndex];

                    auto& tracks = (*groupedTracks)[folderName];
                    std::vector<juce::Uuid> uuidsToRemove;

                    for (auto& track : tracks)
                        uuidsToRemove.push_back(track.getUniqueID());

                    if (onRemoveMultipleTracks)
                        onRemoveMultipleTracks(uuidsToRemove);

                    tracks.clear();

                    groupedTracks->erase(folderName);

                    auto it = std::find(groupedTrackKeys->begin(), groupedTrackKeys->end(), folderName);
                    if (it != groupedTrackKeys->end())
                        groupedTrackKeys->erase(it);

                    listBox.deselectAllRows();
                    listBox.updateContent();
                    listBox.repaint();

                    auto appDataFolder = IOHelper::getAppDataFolder();

                    auto jsonFile = IOHelper::getFile("myTracks.json");
                    TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
                }
            }
        )
    );
}

void TrackListComponent::renameFromFolderList()
{
    juce::SparseSet<int> selectedRows = listBox.getSelectedRows();

    if (selectedRows.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Rename Folder", "Please select one folder to rename");
        return;
    }

    if (selectedRows.size() > 1)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Rename folder", "Please select only one folder to rename");
        return;
    }

    auto* window = new juce::AlertWindow("Rename Folder",
        "Enter a name for the folder:",
        juce::AlertWindow::NoIcon);

    const int selectedIndex = selectedRows[0];

    juce::String oldName = (* groupedTrackKeys)[selectedIndex];

    window->addTextEditor("folderName", oldName);
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, oldName, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != 1)
                return;


            juce::String newName = window->getTextEditor("folderName")->getText().trim();

            if (oldName == newName)
                return;

            if (groupedTracks->find(newName) != groupedTracks->end())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Rename folder", "A folder with this name already exists.");
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

            auto jsonFile = IOHelper::getFile("myTracks.json");
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor("folderName"))
                editor->grabKeyboardFocus();
        });
}

void TrackListComponent::backToFolderView()
{
    viewMode = ViewMode::FolderView;
    deallocateTracksFromList();
    addButtonFolder.setVisible(true);
    removeButtonFolder.setVisible(true);
    renameButtonFolder.setVisible(true);
    listBox.updateContent();
    listBox.repaint();
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

    for (int i = 0; i < trackSeq.getNumEvents(); i++)
    {
        const auto& event = trackSeq.getEventPointer(i)->message;

        if (event.isTrackNameEvent())
        {
            trackName = event.getTextFromTextMetaEvent();
        }
        else if (event.isMetaEvent() && event.getMetaEventType() == 0x04)
        {
            instrumentName = event.getTextFromTextMetaEvent();
        }
    }

    if (trackName.isNotEmpty() && instrumentName.isNotEmpty())
        return trackName + " (" + instrumentName + ")";
    else if (trackName.isNotEmpty())
        return trackName;
    else if (instrumentName.isNotEmpty())
        return instrumentName;

    return "Unnamed Track";
}

void TrackListComponent::initializeTracksFromList()
{
    viewMode = ViewMode::TrackView;
    addButtonFolder.setVisible(false);
    removeButtonFolder.setVisible(false);
    renameButtonFolder.setVisible(false);

    backButton = std::make_unique<juce::TextButton>("Back");
    addButton = std::make_unique<juce::TextButton>("Add");
    removeButton = std::make_unique<juce::TextButton>("Remove");
    sortComboBox = std::make_unique<juce::ComboBox>();
    renameButton = std::make_unique<juce::TextButton>("Rename");

    sortComboBox->addListener(this);

    backButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    removeButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    renameButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    backButton->setLookAndFeel(&laf);
    backButton->setColour(juce::TextButton::buttonColourId, AppColours::accent2);

    addButton->setLookAndFeel(&laf);
    addButton->setColour(juce::TextButton::buttonColourId, AppColours::accent7);

    removeButton->setLookAndFeel(&laf);
    removeButton->setColour(juce::TextButton::buttonColourId, AppColours::accent6);

    renameButton->setLookAndFeel(&laf);
    renameButton->setColour(juce::TextButton::buttonColourId, AppColours::accent4);

    backButton->onClick = [this]() {
        backToFolderView();
    };

    addButton->onClick = [this]() {
        addToTrackList();
    };

    removeButton->onClick = [this]() {
        removeFromTrackList();
    };

    renameButton->onClick = [this]()
    {
        renameFromTrackList();
    };

    addAndMakeVisible(backButton.get());
    addAndMakeVisible(addButton.get());
    addAndMakeVisible(removeButton.get());
    addAndMakeVisible(sortComboBox.get());
    addAndMakeVisible(renameButton.get());

    sortComboBox->addItem("Sort by Name(ascending)", 1);
    sortComboBox->addItem("Sort by Last Modified (Newest)", 2);
    sortComboBox->addItem("Sort by Last Modified (Oldest)", 3);

    sortComboBox->setSelectedId(1);
}

void TrackListComponent::deallocateTracksFromList()
{
    backButton = nullptr;
    addButton = nullptr;
    removeButton = nullptr;
    sortComboBox = nullptr;
}
