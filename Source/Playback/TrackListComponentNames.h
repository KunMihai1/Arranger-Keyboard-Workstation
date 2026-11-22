#pragma once

/** Names shared by TrackListComponent's translation units (TrackListComponent.cpp and
    TrackListComponentFolders.cpp): dialog text, sort ids and layout sizes. */
namespace TrackListNames
{
    namespace Text
    {
        inline const juce::String chooseMidiFile      = "Select a MIDI file to add";
        inline const juce::String midiFilePattern     = "*.mid";
        inline const juce::String failedParsing       = "Failed parsing";
        inline const juce::String couldNotOpen        = "Could not open the selected file";
        inline const juce::String couldNotRead        = "Could not read the selected file";
        inline const juce::String addTracksTitle      = "Add Tracks";
        inline const juce::String fileHas             = "Selected MIDI file has ";
        inline const juce::String tracksLine          = " tracks.\n";
        inline const juce::String tracksAdded         = " tracks added.\n";
        inline const juce::String tracksAlreadyListed = " tracks were already in the list.";
        inline const juce::String removeTracksTitle   = "Remove Track(s)";
        inline const juce::String selectTrackToRemove = "Please select at least one track to remove";
        inline const juce::String confirmRemove       = "Confirm Remove";
        inline const juce::String sureRemoveTracks    = "Are you sure you want to remove the selected track(s)?";
        inline const juce::String removeButton        = "Remove";
        inline const juce::String cancelButton        = "Cancel";
        inline const juce::String okButton            = "OK";
        inline const juce::String renameTrackTitle    = "Rename Track";
        inline const juce::String renameTrackLower    = "Rename track";
        inline const juce::String selectOneTrack      = "Please select one track to rename";
        inline const juce::String selectOnlyOneTrack  = "Please select only one track to rename";
        inline const juce::String newTrackName        = "Enter a new name for the track:";
        inline const juce::String trackNameField      = "trackName";
        inline const juce::String trackNameTaken      = "A track with this name already exists.";
        inline const juce::String folderNameField     = "folderName";
        inline const juce::String createFolderTitle   = "Create Folder";
        inline const juce::String newFolderName       = "Enter a name for the new folder:";
        inline const juce::String invalidName         = "Invalid Name";
        inline const juce::String emptyFolderName     = "Folder name cannot be empty.";
        inline const juce::String duplicateName       = "Duplicate Name";
        inline const juce::String folderNameTaken     = "A folder with this name already exists.";
        inline const juce::String removeFoldersTitle  = "Remove Folder(s)";
        inline const juce::String selectFolderToRemove = "Please select at least one folder to remove";
        inline const juce::String sureRemoveFolders   = "Are you sure you want to remove the selected folder(s)?";
        inline const juce::String renameFolderTitle   = "Rename Folder";
        inline const juce::String renameFolderLower   = "Rename folder";
        inline const juce::String selectOneFolder     = "Please select one folder to rename";
        inline const juce::String selectOnlyOneFolder = "Please select only one folder to rename";
        inline const juce::String folderNamePrompt    = "Enter a name for the folder:";
        inline const juce::String instrumentOpen      = " (";
        inline const juce::String instrumentClose     = ")";
        inline const juce::String unnamedTrack        = "Unnamed Track";
        inline const juce::String backButton          = "Back";
        inline const juce::String addButton           = "Add";
        inline const juce::String renameButton        = "Rename";
        inline const juce::String sortByName          = "Sort by Name(ascending)";
        inline const juce::String sortNewestFirst     = "Sort by Last Modified (Newest)";
        inline const juce::String sortOldestFirst     = "Sort by Last Modified (Oldest)";
    }

    namespace SortIds
    {
        inline constexpr int byName      = 1;
        inline constexpr int newestFirst = 2;
        inline constexpr int oldestFirst = 3;
    }

    inline constexpr int dialogOk         = 1;   // AlertWindow button results
    inline constexpr int dialogCancel     = 0;
    inline constexpr int doubleClick      = 2;
    inline constexpr int oneRow           = 1;
    inline constexpr int instrumentNameMetaEvent = 0x04;
    inline constexpr float selectedRowAlpha = 0.35f;
    inline constexpr int rowTextInset     = 5;
    inline constexpr int rowTextSpare     = 10;
    inline constexpr int rowTop           = 0;
    inline constexpr int rowLeft          = 0;
    inline constexpr int rowBorder        = 1;
    inline constexpr int controlBarHeight = 36;
    inline constexpr int backButtonWidth  = 60;
    inline constexpr int sortBoxWidth     = 120;
    inline constexpr int trackButtonWidth = 80;
}
