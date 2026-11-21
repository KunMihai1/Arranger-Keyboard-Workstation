# Track map points at the wrong track — design

**Date:** 2026-11-22 · **Status:** signed off by the owner 2026-11-22 (option A)

## Problem
`Display::mapUuidToTrack` (`unordered_map<Uuid, TrackEntry*>`, shared by reference with
`CurrentStyleComponent`) is how every style slot finds its MIDI data. Two paths leave its pointers
aimed at the wrong `TrackEntry`, so a slot named "Bass" plays and edits another track.

## Evidence (reproduced live 2026-11-22, see wiki log)
1. **Picker folder switch.** Assign *Bass* from folder A, then only browse into folder B. *Bass*'s
   "Notes information" shows folder B's track. Cause: the picker callback
   (`Display.cpp:356`) stores `&(*availableTracksFromFolder)[i]`. That is a pointer into the picker's
   *copy* of the open folder, which `*availableTracks = (*groupedTracks)[folder]` overwrites on every
   folder open (and the sort combo reorders).
2. **Removing a folder track.** Remove a track stored before *Bass* in the same folder. *Bass* shows
   another track's notes. Cause: `removeFromTrackList` erases from the folder's deque
   (`erase(remove_if)`, which shifts later entries), and nobody rebuilds the map. Removing a folder
   destroys its entries while their map pointers stay.
3. Side issue in the same lines: `erase(remove_if(...))` with one iterator is undefined behaviour
   when the track isn't found. It isn't found after a folder rename, which leaves each entry's
   `folderName` stale (that rename bug is separate).

Edits made through "Notes information" go through the same pointer. After a picker selection they
landed in the copy and were lost.

## Options
- **A: the map only ever points into the library, and is rebuilt after removals (recommended).**
  1. Picker callback: drop the line that re-points the map. The library entry is already mapped
     (startup `buildTrackUuidMap()`, and `addToMapOnAdding` for new tracks).
  2. `onRemoveTrack` / `onRemoveMultipleTracks` in Display: rebuild the map after removing from the
     styles. `CurrentStyleComponent` holds the same map object, so assignment keeps it in sync.
  3. `removeFromFolderList`: fire `onRemoveMultipleTracks` *after* the folder is erased, so the rebuild
     sees the final library (`removeFromTrackList` already fires after its erase).
  4. Use the correct `erase(remove_if(...), end())` idiom.

  About 10 lines across `Display.cpp` and `TrackListComponent.cpp`.
- **B: extract a `TrackLibrary` class** that owns the folders and the map and does every add, remove
  and rename. Fully unit-testable, but a refactor touching Display, TrackListComponent and the
  CurrentStyleComponent constructor. Not forced by this bug.
- **C: no pointers.** Slots look up the library by UUID on every use. Removes the whole class of bug,
  but changes every map reader (about 8 sites). Larger than needed.

## Decision
A, unless the owner prefers otherwise.

## Tests
- **Unit:** `TrackListComponent` with a real two-folder library, the folder opened through the real
  double-click handler. The confirm dialog's callback can't be delivered in the headless runner
  (`runDispatchLoopUntil` needs `JUCE_MODAL_LOOPS_PERMITTED`). So the post-confirm work moves into
  `removeTracks(rows)` / `removeFolders(rows)`, which the dialog callbacks call and which don't save to
  `%APPDATA%` (the callbacks do). Checks:
  - removing a track fires `onRemoveTrack` after the deque is updated;
  - removing a folder fires `onRemoveMultipleTracks` after the folder is gone;
  - a rebuild done inside the callback leaves every map entry pointing at the entry with its own UUID;
  - removing a track whose `folderName` is stale does not crash.
- **Display wiring** (it reads and writes the real `%APPDATA%` files, so it isn't unit-testable):
  re-run the exact live reproduction steps for both bugs. App data is backed up and restored.

## Acceptance
- After browsing another folder, and after removing any other track or folder, every style slot's
  "Notes information" still shows its own track.
- Both suites green. The CI assertion floors are unchanged or raised.
