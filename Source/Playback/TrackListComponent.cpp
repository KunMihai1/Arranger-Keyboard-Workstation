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
        backButton->setBounds(controlBarArea.removeFromLeft(backButtonWidth).reduced(spacing));

    if (sortComboBox)
        sortComboBox->setBounds(controlBarArea.removeFromLeft(sortBoxWidth).reduced(spacing));

    if (addButton)
        addButton->setBounds(controlBarArea.removeFromLeft(trackButtonWidth).reduced(spacing));

    if (removeButton)
        removeButton->setBounds(controlBarArea.removeFromLeft(trackButtonWidth).reduced(spacing));

    if (renameButton)
        renameButton->setBounds(controlBarArea.removeFromLeft(trackButtonWidth).reduced(spacing));

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
    }
}

void TrackListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(AppColours::accent2.withAlpha(selectedRowAlpha));
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

    g.drawText(label, rowTextInset, rowTop, width - rowTextSpare, height, juce::Justification::centredLeft);

    g.setColour(AppColours::separator);
    g.drawRect(rowLeft, rowTop, width, height, rowBorder);
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
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        Text::failedParsing,
                        Text::couldNotOpen);
                    return;
                }

                juce::MidiFile midiFile;
                if (!midiFile.readFrom(inputStream))
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        Text::failedParsing,
                        Text::couldNotRead);
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

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    Text::addTracksTitle,
                    Text::fileHas + juce::String(totalTracks) + Text::tracksLine
                    + juce::String(addedTracks) + Text::tracksAdded
                    + juce::String(duplicateTracks) + Text::tracksAlreadyListed
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
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::removeTracksTitle, Text::selectTrackToRemove);
        return;
    }



    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        Text::confirmRemove,
        Text::sureRemoveTracks,
        Text::removeButton,
        Text::cancelButton,
        this,
        juce::ModalCallbackFunction::create(
            [this, selectedRows](int result)
            {
                if (result == dialogCancel)
                    return;

                removeTracks(selectedRows);

                auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
                TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
            }
        )
    );
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
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::renameTrackTitle, Text::selectOneTrack);
        return;
    }

    if (selectedRows.size() > oneRow)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::renameTrackLower, Text::selectOnlyOneTrack);
        return;
    }

    auto* window = new juce::AlertWindow(Text::renameTrackLower,
        Text::newTrackName,
        juce::AlertWindow::NoIcon);

    const int selectedIndex = selectedRows[int {}];

    auto& track = (*availableTracks)[selectedIndex];
    juce::String oldName = track.displayName;
    juce::Uuid trackUuid = track.uuid;

    window->addTextEditor(Text::trackNameField, oldName);
    window->addButton(Text::okButton, dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(Text::cancelButton, dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, oldName, window, trackUuid](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup(window);
            if (result != dialogOk)
                return;


            juce::String newName = window->getTextEditor(Text::trackNameField)->getText().trim();

            if (oldName == newName)
                return;

            TrackEntry* track=nullptr;
            for (auto& tr : *availableTracks)
            {
                if (tr.displayName == newName)  // no need for tr.displayName!=oldName cause of the previous check with return
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        Text::renameTrackLower, Text::trackNameTaken);
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

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);

            if (onRenameTrackFromList)
                onRenameTrackFromList(trackUuid, newName);

        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(Text::folderNameField))
                editor->grabKeyboardFocus();
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
    addButtonFolder.setVisible(false);
    removeButtonFolder.setVisible(false);
    renameButtonFolder.setVisible(false);

    backButton = std::make_unique<juce::TextButton>(Text::backButton);
    addButton = std::make_unique<juce::TextButton>(Text::addButton);
    removeButton = std::make_unique<juce::TextButton>(Text::removeButton);
    sortComboBox = std::make_unique<juce::ComboBox>();
    renameButton = std::make_unique<juce::TextButton>(Text::renameButton);

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

    sortComboBox->addItem(Text::sortByName, SortIds::byName);
    sortComboBox->addItem(Text::sortNewestFirst, SortIds::newestFirst);
    sortComboBox->addItem(Text::sortOldestFirst, SortIds::oldestFirst);

    sortComboBox->setSelectedId(SortIds::byName);
}

void TrackListComponent::deallocateTracksFromList()
{
    backButton = nullptr;
    addButton = nullptr;
    removeButton = nullptr;
    sortComboBox = nullptr;
}
