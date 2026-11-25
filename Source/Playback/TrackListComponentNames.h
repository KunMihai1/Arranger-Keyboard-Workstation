#pragma once

/** Names shared by TrackListComponent's translation units (TrackListComponent.cpp and
    TrackListComponentFolders.cpp): dialog text, sort ids and layout sizes. */
namespace TrackListNames
{
    namespace Text
    {
        inline const juce::String chooseMidiFile      = "Select a MIDI file to add";
        inline const juce::String midiFilePattern     = "*.mid";
        inline const juce::String couldNotOpen        = "Could not open the selected file";
        inline const juce::String couldNotRead        = "Could not read the selected file";
        inline const juce::String tracksAddedOf       = " of ";
        inline const juce::String tracksAddedMiddle   = " tracks added, ";
        inline const juce::String tracksAlreadyListed = " already in the list";
        inline const juce::String removeTracksTitle   = "Remove Track(s)";
        inline const juce::String selectTrackToRemove = "Please select at least one track to remove";
        inline const juce::String sureRemoveTracks    = "Are you sure you want to remove the selected track(s)?";
        inline const juce::String removeButton        = "Remove";
        inline const juce::String cancelButton        = "Cancel";
        inline const juce::String okButton            = "OK";
        inline const juce::String renameTrackLower    = "Rename track";
        inline const juce::String selectOneTrack      = "Please select one track to rename";
        inline const juce::String selectOnlyOneTrack  = "Please select only one track to rename";
        inline const juce::String trackNameLabel      = "Track name";
        inline const juce::String trackNameTaken      = "A track with this name already exists.";
        inline const juce::String folderNameLabel     = "Folder name";
        inline const juce::String createFolderTitle   = "Create Folder";
        inline const juce::String emptyFolderName     = "Folder name cannot be empty.";
        inline const juce::String folderNameTaken     = "A folder with this name already exists.";
        inline const juce::String removeFoldersTitle  = "Remove Folder(s)";
        inline const juce::String selectFolderToRemove = "Please select at least one folder to remove";
        inline const juce::String sureRemoveFolders   = "Are you sure you want to remove the selected folder(s)?";
        inline const juce::String renameFolderLower   = "Rename folder";
        inline const juce::String selectOneFolder     = "Please select one folder to rename";
        inline const juce::String selectOnlyOneFolder = "Please select only one folder to rename";
        inline const juce::String instrumentOpen      = " (";
        inline const juce::String instrumentClose     = ")";
        inline const juce::String unnamedTrack        = "Unnamed Track";
        inline const juce::String backButton          = "Back";
        inline const juce::String addButton           = "Add";
        inline const juce::String renameButton        = "Rename";
        inline const juce::String sortByName          = "Sort by Name(ascending)";
        inline const juce::String sortNewestFirst     = "Sort by Last Modified (Newest)";
        inline const juce::String sortOldestFirst     = "Sort by Last Modified (Oldest)";
        // The track picker page (board PageTracks)
        inline const char* const  backUtf8            = "\xe2\x97\x82 Back";           // U+25C2 + "Back"
        inline const char* const  ellipsisUtf8        = "\xe2\x80\xa6";               // U+2026
        inline const char* const  sortArrowsUtf8      = " \xe2\x97\x82 \xe2\x96\xb8";   // " < >" as U+25C2 U+25B8
        inline const char* const  pathSeparatorUtf8   = " \xe2\x80\xba ";             // " > " as U+203A
        inline const juce::String useInSlot           = "Use in slot ";
        inline const juce::String openFolder          = "Open folder";
        inline const juce::String addFolder           = "Add folder";
        inline const juce::String renameShort         = "Rename";
        inline const juce::String addFromMidi         = "Add from MIDI file";
        inline const juce::String removeShort         = "Remove";
        inline const juce::String sort                = "Sort";
        inline const juce::String sortName            = "Name";
        inline const juce::String sortNewest          = "Newest";
        inline const juce::String sortOldest          = "Oldest";
        inline const juce::String folders             = "Folders";
        inline const juce::String folderHeading       = "Folder";
        inline const juce::String trackHeading        = "Track";
        inline const juce::String doubleClickOpen     = "Double-click to open";
        inline const juce::String doubleClickUse      = "Double-click to use the track";
        inline const juce::String tracksSuffix        = " tracks";
        inline const juce::String folderKey           = "Folder";
        inline const juce::String tracksKey           = "Tracks";
        inline const juce::String melodicKey          = "Melodic";
        inline const juce::String percussionKey       = "Percussion";
        inline const juce::String trackKey            = "Track";
        inline const juce::String instrumentKey       = "Instrument";
        inline const juce::String typeKey             = "Type";
        inline const juce::String melodicMark         = "MEL";
        inline const juce::String percussionMark      = "PERC";
        inline const char* const  noValueUtf8         = "\xe2\x80\x94";               // U+2014
    }

    namespace SortIds
    {
        inline constexpr int byName      = 1;
        inline constexpr int newestFirst = 2;
        inline constexpr int oldestFirst = 3;
    }

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

    // The track picker page (board PageTracks, scale 1): list column 620, gap 22, details column the rest.
    inline constexpr int listColumnWidth   = 620;
    inline constexpr int columnGap         = 22;
    inline constexpr int headingToList     = 6;
    inline constexpr int headingGap        = 8;     // Back - path - rule - hint
    inline constexpr int detailHeadingGap  = 8;
    inline constexpr int pickerRowHeight   = 24;
    inline constexpr int glyphLeft         = 10;
    inline constexpr int glyphWidth        = 14;
    inline constexpr int glyphHeight       = 10;
    inline constexpr int glyphTab          = 6;     // the folder glyph's tab
    inline constexpr int glyphTabHeight    = 3;
    inline constexpr float glyphStroke     = 1.5f;
    inline constexpr int noteGlyphSize     = 8;
    inline constexpr int rowNameLeft       = 34;
    inline constexpr int detailPaddingTop  = 6;
    inline constexpr int panelPaddings     = 2;     // padding above and below the details
    inline constexpr int detailPaddingSide = 12;
    inline constexpr int detailRowHeight   = 22;
    inline constexpr int boardDetailRows   = 4;     // the panel keeps room for four rows (folder or track)
    inline constexpr float detailTextSize  = 13.0f;
    inline constexpr float dashLength      = 2.0f;
    inline constexpr float detailRuleAlpha = 0.3f;
    inline constexpr int bigButtonHeight   = 30;
    inline constexpr float bigButtonText   = 12.0f;
    inline constexpr int buttonGap         = 6;
    inline constexpr int firstSlot         = 1;
}
