#include <JuceHeader.h>
#include "TrackListComponent.h"

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
    TrackListRemovalTest() : juce::UnitTest ("TrackListComponent (removing tracks and folders)", "Unit") {}

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
            for (int i = 0; i < names.size(); ++i)
            {
                TrackEntry e;
                e.trackIndex  = i;
                e.displayName = names[i];
                e.folderName  = folderNameInEntries;
                e.uuid        = juce::Uuid();
                tracks.push_back (e);
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
                                            juce::MouseInputSource::defaultPressure, 0.0f, 0.0f, 0.0f, 0.0f,
                                            &list, &list, now, {}, now, 2, false);
        list.listBoxItemClicked (folderRow, doubleClick);
    }

    static juce::SparseSet<int> rowOf (const TrackListComponent& list, const juce::String& name)
    {
        juce::SparseSet<int> rows;
        auto& open = list.getAllAvailableTracks();
        for (int i = 0; i < (int) open.size(); ++i)
            if (open[(size_t) i].displayName == name)
                rows.addRange ({ i, i + 1 });
        return rows;
    }

    void expectMapConsistent (const UuidMap& map, const juce::String& context)
    {
        for (auto& [uuid, entry] : map)
            expect (entry != nullptr && entry->uuid == uuid,
                    context + ": map entry points at a different track");
    }

    void removingATrackLeavesARebuildableLibrary()
    {
        beginTest ("Removing a track fires onRemoveTrack after the folder no longer holds it");

        Library lib;
        lib.addFolder ("A", { "a0", "a1", "a2", "a3" }, "A");
        lib.addFolder ("B", { "b0", "b1" }, "B");
        UuidMap map = lib.buildMap();

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (400, 300);
        openFolder (list, 0);

        const auto removed = lib.uuidOf ("A", "a1");
        bool fired = false;
        list.onRemoveTrack = [&] (const juce::Uuid& uuid)
        {
            fired = true;
            expect (uuid == removed);
            expect (! lib.folderHolds ("A", uuid), "callback fired while the folder still held the track");
            map = lib.buildMap();   // what Display does
        };

        list.removeTracks (rowOf (list, "a1"));

        expect (fired, "onRemoveTrack not called");
        expect (lib.folders->at ("A").size() == 3, "folder A should have 3 tracks left");
        expect (map.size() == 5, "rebuilt map should hold the 5 remaining tracks");
        expectMapConsistent (map, "after removing a1");
    }

    void removingSeveralTracksAtOnceRemovesExactlyThose()
    {
        beginTest ("Removing several selected tracks at once removes exactly those");

        Library lib;
        lib.addFolder ("A", { "a0", "a1", "a2", "a3", "a4" }, "A");

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (400, 300);
        openFolder (list, 0);

        const auto u1 = lib.uuidOf ("A", "a1");
        const auto u3 = lib.uuidOf ("A", "a3");
        std::vector<juce::Uuid> reported;
        UuidMap map;
        list.onRemoveTrack = [&] (const juce::Uuid& uuid) { reported.push_back (uuid); map = lib.buildMap(); };

        auto rows = rowOf (list, "a1");
        rows.addRange (rowOf (list, "a3").getTotalRange());
        list.removeTracks (rows);   // the rows are removed from the last to the first

        expect (reported.size() == 2 && std::find (reported.begin(), reported.end(), u1) != reported.end()
                                     && std::find (reported.begin(), reported.end(), u3) != reported.end(),
                "both removed tracks should be reported");
        expect (! lib.folderHolds ("A", u1) && ! lib.folderHolds ("A", u3), "a removed track is still in the folder");
        expect (lib.folders->at ("A").size() == 3, "folder A should have 3 tracks left");
        expect (list.getAllAvailableTracks().size() == 3, "the open-folder list should show 3 tracks");
        for (auto* name : { "a0", "a2", "a4" })
            expect (! lib.uuidOf ("A", name).isNull(), juce::String ("kept track missing: ") + name);
        expectMapConsistent (map, "after removing a1 and a3");
    }

    void removingAFolderFiresItsCallbackAfterTheFolderIsGone()
    {
        beginTest ("Removing a folder fires onRemoveMultipleTracks after the folder is gone");

        Library lib;
        lib.addFolder ("A", { "a0", "a1" }, "A");
        lib.addFolder ("B", { "b0", "b1", "b2" }, "B");

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (400, 300);

        bool fired = false;
        size_t reported = 0;
        UuidMap map;
        list.onRemoveMultipleTracks = [&] (const std::vector<juce::Uuid>& uuids)
        {
            fired = true;
            reported = uuids.size();
            expect (lib.folders->count ("B") == 0, "callback fired while folder B still existed");
            map = lib.buildMap();   // what Display does
        };

        juce::SparseSet<int> folderB;
        folderB.addRange ({ 1, 2 });
        list.removeFolders (folderB);

        expect (fired, "onRemoveMultipleTracks not called");
        expect (reported == 3, "should report folder B's 3 tracks");
        expect (lib.folders->count ("B") == 0 && lib.keys->size() == 1, "folder B should be gone");
        expect (map.size() == 2, "rebuilt map should hold only folder A's 2 tracks");
    }

    void removingATrackAfterAFolderRenameRemovesItThere()
    {
        beginTest ("Removing a track after its folder was renamed removes it from that folder");

        // What renameFromFolderList() leaves behind: the folder is now keyed "A", but its entries
        // still carry the old folderName "Old".
        Library lib;
        lib.addFolder ("A", { "a0", "a1", "a2" }, "Old");

        TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
        list.setSize (400, 300);
        openFolder (list, 0);

        const auto removed = lib.uuidOf ("A", "a1");
        list.removeTracks (rowOf (list, "a1"));

        expect (! lib.folderHolds ("A", removed), "track still in its (renamed) folder");
        expect (lib.folders->at ("A").size() == 2, "folder A should have 2 tracks left");
        expect (lib.folders->count ("Old") == 0, "removal created a phantom folder under the old name");
    }
};

static TrackListRemovalTest trackListRemovalTest;
