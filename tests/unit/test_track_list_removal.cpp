#include <JuceHeader.h>
#include "TrackListComponent.h"

namespace TrackListRemovalTestValues
{
    const juce::String testName     = "TrackListComponent (removing tracks and folders)";
    const juce::String testCategory = "Unit";

    const juce::String removingATrackFiresOnRemoveTrackAfterThe = "Removing a track fires onRemoveTrack after the folder no longer holds it";
    const juce::String removingSeveralSelectedTracksAtOnceRemoves = "Removing several selected tracks at once removes exactly those";
    const juce::String removingAFolderFiresOnRemoveMultipleTracksAfterThe = "Removing a folder fires onRemoveMultipleTracks after the folder is gone";
    const juce::String removingATrackAfterItsFolderWas = "Removing a track after its folder was renamed removes it from that folder";

    const juce::String folderA = "A";
    const juce::String folderB = "B";
    const juce::String oldFolderName = "Old";
    const juce::String trackA0 = "a0";
    const juce::String trackA1 = "a1";
    const juce::String trackA2 = "a2";
    const juce::String trackA3 = "a3";
    const juce::String trackA4 = "a4";
    const juce::String trackB0 = "b0";
    const juce::String trackB1 = "b1";
    const juce::String trackB2 = "b2";
    constexpr int listWidth = 400;
    constexpr int listHeight = 300;
    constexpr int firstFolderRow = 0;
    constexpr int secondFolderRow = 1;
    constexpr int rowAfterSecondFolder = 2;
    constexpr int nextRow = 1;
    constexpr float noOrientation = 0.0f;
    constexpr float noRotation = 0.0f;
    constexpr float noTilt = 0.0f;
    constexpr int doubleClickCount = 2;
    constexpr size_t twoTracks = 2;
    constexpr size_t threeTracks = 3;
    constexpr size_t fiveTracks = 5;
    constexpr size_t oneFolderKey = 1;
    constexpr size_t noFolder = 0;
    const juce::String mapEntryMismatch = ": map entry points at a different track";
    const juce::String callbackBeforeTrackRemoved = "callback fired while the folder still held the track";
    const juce::String onRemoveTrackNotCalled = "onRemoveTrack not called";
    const juce::String threeLeftInA = "folder A should have 3 tracks left";
    const juce::String twoLeftInA = "folder A should have 2 tracks left";
    const juce::String mapHoldsFiveRemaining = "rebuilt map should hold the 5 remaining tracks";
    const juce::String afterRemovingA1 = "after removing a1";
    const juce::String bothRemovedReported = "both removed tracks should be reported";
    const juce::String removedStillInFolder = "a removed track is still in the folder";
    const juce::String openListShowsThree = "the open-folder list should show 3 tracks";
    const juce::StringArray keptTracks { trackA0, trackA2, trackA4 };
    const juce::String keptTrackMissing = "kept track missing: ";
    const juce::String afterRemovingA1AndA3 = "after removing a1 and a3";
    const juce::String callbackBeforeFolderRemoved = "callback fired while folder B still existed";
    const juce::String onRemoveMultipleNotCalled = "onRemoveMultipleTracks not called";
    const juce::String reportsThreeFromB = "should report folder B's 3 tracks";
    const juce::String folderBGone = "folder B should be gone";
    const juce::String mapHoldsOnlyA = "rebuilt map should hold only folder A's 2 tracks";
    const juce::String trackStillInRenamedFolder = "track still in its (renamed) folder";
    const juce::String phantomOldFolder = "removal created a phantom folder under the old name";
}

using namespace TrackListRemovalTestValues;

/*
    Removing tracks/folders must leave the library in a state the shared UUID -> TrackEntry* map can
    be rebuilt from (Display rebuilds it inside onRemoveTrack / onRemoveMultipleTracks).

    Bugs (reproduced live 2026-11-22): after removing a folder track, a style slot played another
    track - the deque shifted and the map kept the old addresses. Removing a folder fired its callback
    while the folder still existed, so a rebuild there would keep pointers to entries about to be
    destroyed. And after a folder rename each entry keeps its old folderName, so removal looked in a
    folder that no longer exists (creating it, then erase(end()) - undefined behaviour).

    removeTracks()/removeFolders() are what the Remove buttons run once the user confirms; they don't
    touch the files on disk (the dialog callback saves), so these tests never write to %APPDATA%.
*/
class TrackListRemovalTest : public juce::UnitTest
{
public:
    TrackListRemovalTest() : juce::UnitTest (TrackListRemovalTestValues::testName, TrackListRemovalTestValues::testCategory) {}

    void runTest() override
    {
        removingATrackLeavesARebuildableLibrary();
        removingSeveralTracksAtOnceRemovesExactlyThose();
        removingAFolderFiresItsCallbackAfterTheFolderIsGone();
        removingATrackAfterAFolderRenameRemovesItThere();
    }

private:
    using Folders = std::unordered_map<juce::String, std::deque<TrackEntry>>;
    using UuidMap = std::unordered_map<juce::Uuid, TrackEntry*>;

    struct Library
    {
        std::shared_ptr<std::deque<TrackEntry>> openFolder = std::make_shared<std::deque<TrackEntry>>();
        std::shared_ptr<Folders> folders = std::make_shared<Folders>();
        std::shared_ptr<std::vector<juce::String>> keys = std::make_shared<std::vector<juce::String>>();

        void addFolder (const juce::String& key, const juce::StringArray& names, const juce::String& folderNameInEntries)
        {
            keys->push_back (key);
            auto& tracks = (*folders)[key];
            for (int nameIndex {}; nameIndex < names.size(); ++nameIndex)
            {
                TrackEntry entry;
                entry.trackIndex  = nameIndex;
                entry.displayName = names[nameIndex];
                entry.folderName  = folderNameInEntries;
                entry.uuid        = juce::Uuid();
                tracks.push_back (entry);
            }
        }

        // Same rule as Display::buildTrackUuidMap(): every library entry, by UUID.
        UuidMap buildMap() const
        {
            UuidMap map;
            for (auto& [name, tracks] : *folders)
                for (auto& tr : tracks)
                    map[tr.getUniqueID()] = &tr;
            return map;
        }

        juce::Uuid uuidOf (const juce::String& folder, const juce::String& name) const
        {
            for (auto& tr : folders->at (folder))
                if (tr.displayName == name)
                    return tr.uuid;
            return {};
        }

        bool folderHolds (const juce::String& folder, const juce::Uuid& uuid) const
        {
            auto it = folders->find (folder);
            if (it == folders->end())
                return false;
            return std::any_of (it->second.begin(), it->second.end(),
                                [&] (const TrackEntry& tr) { return tr.uuid == uuid; });
        }
    };

    static void openFolder (TrackListComponent& list, int folderRow)
    {
        // A double-click on a folder row, as the list box delivers it.
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent doubleClick (juce::Desktop::getInstance().getMainMouseSource(), {}, juce::ModifierKeys::leftButtonModifier,
                                            juce::MouseInputSource::defaultPressure, noOrientation, noRotation, noTilt, noTilt,
                                            &list, &list, now, {}, now, doubleClickCount, false);
        list.listBoxItemClicked (folderRow, doubleClick);
    }

    static juce::SparseSet<int> rowOf (const TrackListComponent& list, const juce::String& name)
    {
        juce::SparseSet<int> rows;
        auto& open = list.getAllAvailableTracks();
        for (int row {}; row < (int) open.size(); ++row)
            if (open[(size_t) row].displayName == name)
                rows.addRange ({ row, row + nextRow });
        return rows;
    }

    void expectMapConsistent (const UuidMap& map, const juce::String& context)
    {
        for (auto& [uuid, entry] : map)
            expect (entry != nullptr && entry->uuid == uuid,
                    context + mapEntryMismatch);
    }

    void removingATrackLeavesARebuildableLibrary()
    {
        beginTest (removingATrackFiresOnRemoveTrackAfterThe);

        Library lib;
        lib.addFolder (folderA, { trackA0, trackA1, trackA2, trackA3 }, folderA);
        lib.addFolder (folderB, { trackB0, trackB1 }, folderB);
        UuidMap map = lib.buildMap();

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (listWidth, listHeight);
        openFolder (list, firstFolderRow);

        const auto removed = lib.uuidOf (folderA, trackA1);
        bool fired = false;
        list.onRemoveTrack = [&] (const juce::Uuid& uuid)
        {
            fired = true;
            expect (uuid == removed);
            expect (! lib.folderHolds (folderA, uuid), callbackBeforeTrackRemoved);
            map = lib.buildMap();   // what Display does
        };

        list.removeTracks (rowOf (list, trackA1));

        expect (fired, onRemoveTrackNotCalled);
        expect (lib.folders->at (folderA).size() == threeTracks, threeLeftInA);
        expect (map.size() == fiveTracks, mapHoldsFiveRemaining);
        expectMapConsistent (map, afterRemovingA1);
    }

    void removingSeveralTracksAtOnceRemovesExactlyThose()
    {
        beginTest (removingSeveralSelectedTracksAtOnceRemoves);

        Library lib;
        lib.addFolder (folderA, { trackA0, trackA1, trackA2, trackA3, trackA4 }, folderA);

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (listWidth, listHeight);
        openFolder (list, firstFolderRow);

        const auto u1 = lib.uuidOf (folderA, trackA1);
        const auto u3 = lib.uuidOf (folderA, trackA3);
        std::vector<juce::Uuid> reported;
        UuidMap map;
        list.onRemoveTrack = [&] (const juce::Uuid& uuid) { reported.push_back (uuid); map = lib.buildMap(); };

        auto rows = rowOf (list, trackA1);
        rows.addRange (rowOf (list, trackA3).getTotalRange());
        list.removeTracks (rows);   // the rows are removed from the last to the first

        expect (reported.size() == twoTracks && std::find (reported.begin(), reported.end(), u1) != reported.end()
                                     && std::find (reported.begin(), reported.end(), u3) != reported.end(),
                bothRemovedReported);
        expect (! lib.folderHolds (folderA, u1) && ! lib.folderHolds (folderA, u3), removedStillInFolder);
        expect (lib.folders->at (folderA).size() == threeTracks, threeLeftInA);
        expect (list.getAllAvailableTracks().size() == threeTracks, openListShowsThree);
        for (const auto& name : keptTracks)
            expect (! lib.uuidOf (folderA, name).isNull(), keptTrackMissing + name);
        expectMapConsistent (map, afterRemovingA1AndA3);
    }

    void removingAFolderFiresItsCallbackAfterTheFolderIsGone()
    {
        beginTest (removingAFolderFiresOnRemoveMultipleTracksAfterThe);

        Library lib;
        lib.addFolder (folderA, { trackA0, trackA1 }, folderA);
        lib.addFolder (folderB, { trackB0, trackB1, trackB2 }, folderB);

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (listWidth, listHeight);

        bool fired = false;
        size_t reported {};
        UuidMap map;
        list.onRemoveMultipleTracks = [&] (const std::vector<juce::Uuid>& uuids)
        {
            fired = true;
            reported = uuids.size();
            expect (lib.folders->count (folderB) == noFolder, callbackBeforeFolderRemoved);
            map = lib.buildMap();   // what Display does
        };

        juce::SparseSet<int> folderBRows;
        folderBRows.addRange ({ secondFolderRow, rowAfterSecondFolder });
        list.removeFolders (folderBRows);

        expect (fired, onRemoveMultipleNotCalled);
        expect (reported == threeTracks, reportsThreeFromB);
        expect (lib.folders->count (folderB) == noFolder && lib.keys->size() == oneFolderKey, folderBGone);
        expect (map.size() == twoTracks, mapHoldsOnlyA);
    }

    void removingATrackAfterAFolderRenameRemovesItThere()
    {
        beginTest (removingATrackAfterItsFolderWas);

        // What renameFromFolderList() leaves behind: the folder is now keyed "A", but its entries
        // still carry the old folderName "Old".
        Library lib;
        lib.addFolder (folderA, { trackA0, trackA1, trackA2 }, oldFolderName);

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (listWidth, listHeight);
        openFolder (list, firstFolderRow);

        const auto removed = lib.uuidOf (folderA, trackA1);
        list.removeTracks (rowOf (list, trackA1));

        expect (! lib.folderHolds (folderA, removed), trackStillInRenamedFolder);
        expect (lib.folders->at (folderA).size() == twoTracks, twoLeftInA);
        expect (lib.folders->count (oldFolderName) == noFolder, phantomOldFolder);
    }
};

static TrackListRemovalTest trackListRemovalTest;
