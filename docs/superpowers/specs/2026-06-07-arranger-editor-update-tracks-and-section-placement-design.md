# Arranger editor: "Update Tracks" + section placement at scroll position

Date: 2026-06-07
Branch: arranger-engine-phase1
Status: approved (verbal), pending implementation

## Context

The arranger style editor (`ArrangerStyleEditor`) holds two independent pieces of
state:

- `sourceTracks` (`std::vector<SourceTrackFile>`) — the recorded MIDI notes.
- `windows` (`std::vector<SectionWindow>`) — the section layout (Intro/Variation/
  Fill/Break/Ending), expressed as **track-agnostic bar-ranges** (`startBar`,
  `lengthBars`, `type`, `name`, `afterComplete`).

Because sections are bar-windows over the whole recording, the notes can be
replaced while keeping the section layout intact.

## Feature A — "Update Tracks" button

Goal: replace an existing configuration's recorded notes with whatever is
currently recorded in the app, **keep the sections exactly as-is**, and
**overwrite** the saved `.style` after a confirmation.

Decisions:
- Source of new tracks: the **current recording in the app** (same source used
  when creating a new configuration).
- Result: **overwrite the loaded config** (NOT a copy), gated by a confirmation
  popup. (User explicitly revised from an earlier "save as copy" idea.)
- Sections are **kept verbatim** — never deleted, renumbered, or rescaled. If the
  new recording is shorter than where a section sits, that section simply has no
  notes in those bars.

Flow:
1. New `Update Tracks` `TextButton` in the editor button row, near `Save`.
   Enabled only when editing an existing saved config (`loadedFile` is set);
   a brand-new unsaved config has nothing to overwrite.
2. On click → confirmation `AlertWindow`:
   "Replace the tracks of '<name>' with the current recording and overwrite the
   saved configuration? Your sections (intros/variations/fills) stay as they are."
3. Cancel → no change.
4. Confirm →
   - Obtain current tracks via a new editor callback
     `std::function<std::vector<SourceTrackFile>()> onRequestCurrentTracks`,
     wired by the host (`CurrentStyleComponent`) to
     `ArrangerSourceBuilder::fromTrackEntries(collectSelectedTracks(), referenceBpm)`.
   - If empty → info popup "No recorded tracks to import." and stop.
   - Replace `sourceTracks`; keep `windows`; `recomputeTotalBars()`;
     `timeline.setTotalBars()`, `layoutTimeline()`, `rebuildPreview()`.
   - Write `toStyleFile()` to `loadedFile` (same name + same `styleId`), then fire
     `onSaved(loadedFile)` so the host refreshes the list and reloads the active
     config.

## Feature B — new section appears at the current scroll position

Goal: adding a section drops it into the part of the timeline currently in view,
instead of always at bar 1.

Change: in `addSectionOfType`, replace `w.startBar = 1;` with a bar derived from
the timeline viewport's horizontal scroll position
(`timelineViewport.getViewPositionX() / kPixelsPerBar`, 1-based), clamped to
`[1, totalBars]`. Clamp `lengthBars` so the section does not run past `totalBars`.

## Out of scope

- No rescaling/stretching of sections to a new recording length.
- No track-by-track merge UI; tracks are replaced wholesale.
- No "save as copy" variant (overwrite-with-confirmation only).
