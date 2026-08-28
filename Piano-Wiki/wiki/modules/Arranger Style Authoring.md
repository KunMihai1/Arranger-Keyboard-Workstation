---
type: module
title: "Arranger Style Authoring"
path: "Source/Arranger/ArrangerStyleEditor.* · ArrangerStyleListComponent.* · ArrangerTimelineComponent.* · ArrangerDefaults.* · Source/Styles/CurrentStyleComponent.*"
status: active
language: cpp
purpose: "Author, store, browse, and live-trigger self-contained .style configurations (sections carved out of one recording)"
depends_on:
  - "[[Arranger Engine]]"
  - "[[ArrangerSectionSequencer]]"
  - "[[ArrangerModel]]"
created: 2026-06-07
updated: 2026-06-25
tags:
  - module
  - arranger
  - ui
related:
  - "[[ArrangerSectionSequencer]]"
  - "[[GL Overlay Rendering]]"
  - "[[Styles System]]"
sources: []
---

# Arranger Style Authoring

How a user creates and uses a self-contained arranger **configuration** (`.style`). Phase 3 baseline landed in commit `d56b503`; the UX described here is the **working-tree (uncommitted)** refinement layer on top (2026-06-07 session).

> [!key-insight] A "config" is windows into ONE recording
> You record a performance once; the editor carves it into named **sections** (bar-windows). A Variation/Intro/Fill/Break/Ending is just a window of type X over the same `sourceTracks`. There is no per-section re-recording (that would be the bigger "separate grooves" rewrite, deferred).

## Pieces
- **`ArrangerStyleEditor`** — full-screen authoring overlay. Seeds from the live recording (`loadRecording`) or an existing file (`loadFromFile`). Holds `sourceTracks` + `windows` (sections). Live preview via a shared [[Arranger Engine]].
- **`ArrangerTimelineComponent`** — bar ruler with draggable/resizable colored section regions; tracks a `selectedIndex`; emits `onWindowsChanged` / `onSectionSelected`. Implements `TooltipClient` → hovering a region shows its name + bar range (e.g. `Intro 1 (bars 1-2)`).
- **`ArrangerStyleListComponent`** — browser of saved `*.style` files: New / Edit / **Delete** / Load / Close, marks the **active** config with a ✓ + green wash.
- **`CurrentStyleComponent`** — host: owns the engine, the dropdown, the browser/editor overlays, and the per-style active-config state.

## Editor controls (top row)
`Add Intro · Add Var · Add Fill · Add Break · Add Ending · Remove · Preview · Stop · Update Tracks · Save · Close`.
- **Add X** → appends a section of that type, auto-numbered **uniquely per type** (`Intro 1`, `Intro 2`, …) via max-existing+1. Loading an old file de-dups names (`renumberSectionsByType`) so legacy "all Intro 1" files become distinguishable. The new section is placed at the **bar currently at the timeline viewport's left edge** (`timeline.barForX(viewport.getViewPositionX())`, clamped) — it lands *in view*, not back at bar 1. (`ArrangerTimelineComponent::barForX` = inverse of `xForBar`, via `ArrangerTimelineGeometry::xToSnappedBar`.)
- **Update Tracks** (enabled only when editing an existing saved config) → replaces the configuration's recorded notes with the **app's current recording while keeping the sections (windows) exactly as-is**, then **overwrites** the same `.style` (same name + id) after an OK/Cancel confirmation. Source = host's `collectSelectedTracks()` (raw `TrackEntry`s; the editor builds events on its background thread). If the new recording is shorter than a section's bars, that section simply has no notes there — sections are never deleted/renumbered/rescaled.
- **Remove** → deletes the selected section (`timeline.getSelectedIndex()`), recomputes bars, clears the stale selection.
- A brand-new config seeds one full-length looping **Variation 1** (`ArrangerDefaults::defaultWindowsForBars`) as the main groove.
- **Save** renames in place: the editor remembers the file it opened (`setSourceFile`); saving under a new name deletes the old file (no duplicate), with an Overwrite/Cancel prompt on a name clash.
- **Preview**, then click a section on the timeline → `onSectionSelected` → `engine.queueSection(type, name)` jumps to that exact section at the next bar (works because names are unique).
- The timeline **playhead arrow is section-relative**: it reads `engine.getActiveSectionIndex()` + `getActiveSectionLocalBeats()` and draws at `window.startBar + (localBeats mod sectionLen)/bpb`, so jumping to the Ending moves the arrow into the Ending's bars (not the global monotonic position). Auto-follow scroll tracks that arrow.

## Section semantics (Korg mapping)
- **Variation** = `Loop` (the groove that repeats). **Intro/Fill/Break** = `FallThrough` one-shots → return to a Variation. **Ending** = `Stop`. See [[ArrangerSectionSequencer]].
- **Auto Fill** (live "Variations" toggle): switching `Var N` while a variation plays auto-inserts `Fill N` then lands on `Var N`. Wiring chain: `SectionGroupComponent.onToggleChanged` → `MainComponent` → `Display::setArrangerAutoFillEnabled` → `CurrentStyleComponent::setArrangerAutoFillEnabled` → `ArrangerEngine::setAutoFillEnabled` → sequencer. State is remembered in `Display` and re-applied when a style tab is (re)created.
- Live buttons send `Var N` / `Break`; sections are named `Variation N` / `Break 1` → resolved by [[ArrangerSectionSequencer]]'s type+trailing-number match.

## Storage
- `.style` files live in **`%APPDATA%/Roaming/Piano Synth2/ArrangerStyles/`** via `IOHelper::getArrangerStylesFolder()` (= `getFile("ArrangerStyles")`). Moved here from the old sibling `Roaming/ArrangerStyles/` so all app data is under one folder.
- The **per-style active selection** is *not* a file — it's a string `"arrangerConfig"` on each style object inside `allStyles.json`. `CurrentStyleComponent::loadJson` restores it (and **clears stale state first**, so each style only ever shows its own config), and activating/clearing a config persists immediately via `anyTrackChanged()`.

## Active-config UX (this session)
- The active config is shown **only in the browser** (✓ + "active"), not as a row in the play-settings dropdown. The dropdown is play-mode + actions only.
- **Load** = make active + close browser + prime the engine — it no longer auto-starts playback (press Start to play).
- Picking "All tracks" / "Solo" clears the active config (back to live tracks) and updates the browser marker.

## Live section-button highlight (engine-driven)
In arranger mode the live `Var/Fill/Break/Intro/Ending` buttons are **driven by the engine's actual active section**, not by the click. `StyleSectionComponent::setEngineDrivenHighlight(true)` makes a click only route to the engine (no self-toggle); `ArrangerEngine::onActiveSectionChanged` → `CurrentStyleComponent::onArrangerSectionChanged` → `Display` → `MainComponent::highlightArrangerSection` lights the matching button (mapped by type + trailing number; only one lit across both groups).
- Press Var 2 with Auto Fill on → **Fill** lights when it starts, then **Var 2** when the fill resolves — 100% in sync with audio.
- Before the first play: nothing lit. After an Ending stops: Ending clears, **Var 1 re-arms** (via `onStoppedItself`/`notifyActiveSection`), and the beat bar's first beat goes back to **yellow** (the UI now learns the engine stopped itself).
- Classic (non-arranger) mode keeps the old click-toggle highlight (`engineDrivenHighlight=false`).
- **Auto Fill toggle persists app-wide**: `MainComponent` writes/reads `ArrangerAutoFillEnabled` in the app properties and re-ticks the toggle on startup (not per style).

## Editor ↔ live playback are separate modes
Editor preview and live performance share the **one** [[Arranger Engine]]. To stop them fighting, `CurrentStyleComponent::presentOverlay` **stops any live playback before showing the browser/editor** (and resets the beat bar). So entering edit is a clean break — it no longer inherits a stale section or fires a queued Ending on close. (Trade-off: opening the editor cuts off live playback; a truly background-playing live engine would need a *separate* preview engine — deferred.)

## Start-section selection (pre-play)
You are **not** forced to start on Variation 1; pick any section before pressing Start.
- The remembered choice lives in the host (`CurrentStyleComponent::pendingStartType` / `pendingStartName`, default `Variation`/`Variation 1`) — **not** the engine (whose `pendingStartIndex` is throwaway; see [[Arranger Engine]]).
- `selectOrQueueSection(type, name)` (every section-button handler routes here): **playing** → `queueSection` (switch at next bar); **stopped** → store as the pending start, `engine.selectStartSection`, and highlight that button via `onArrangerSectionChanged(0, type, name)`.
- `highlightPendingStartSection()` shows + arms the pending start while idle. Called on config load (`applyActiveConfig`, which also resets the pending start to Var 1), on arranger-mode enable, and after Stop / self-stop — so "before Start, Var 1 is lit" holds and the highlight returns to the chosen start after a play/stop cycle.
- `startPlaying` re-applies `engine.selectStartSection(pendingStart…)` **after `setStyle`, before `start()`** (undoing the `stopPlaying(false)` wipe), in both the active-config and demo branches.
- **Sticky:** the choice persists across Start/Stop cycles until you click another section or reload the config (an Intro played once does *not* auto-revert — that variant was offered but not taken).

## Off-thread save / load / update (no UI freeze)
Serializing/parsing a `.style` is heavy (hex-encode every MIDI event + disk I/O on a ~600 KB file), so these run on a background thread (`juce::Thread::launch`) with the heavy data work off the message thread and only `engine.setStyle` + UI updates marshalled back via `MessageManager::callAsync` (guarded by `Component::SafePointer` so a closed editor no-ops). Pattern: **show overlay → heavy work (bg) → apply + hide (msg)**; the `callAsync` IS the "done" signal.
- **Save** & **Update Tracks** (in-editor): an internal full-screen dim+label overlay (`ArrangerStyleEditor::setBusy`, styled like the app's "Preparing style…"), buttons disabled meanwhile.
- **Load** (browser **Load** + **Edit**, parsed before the editor exists): the app overlay via `CurrentStyleComponent::onBusy` → `Display::onArrangerBusy` → `MainComponent::setLoadingOverlayVisible`. (Needed because the editor is parented to the top-level window *above* `MainComponent`, so MainComponent's own overlay can't cover it — hence the editor's internal one for in-editor ops.)
- **No post-save re-parse:** `onSaved(file, builtStyle)` now hands the already-built `ArrangerStyle` back to the host (`applyActiveConfig`) instead of re-reading the file (the editor already holds decoded events), so saving doesn't re-freeze. Refactored: `configureEditorCallbacks()` + `applyActiveConfig()` shared by new/edit/load paths.

## Live volume wins (arranger Start)
When Start plays a saved config, `startPlaying` overlays each live track slider's volume onto a **copy** of `activeArrangerConfig` (`applyLiveTrackVolumes`, matched by track UUID = `ArrangerTrack.id`) before `engine.setStyle`. So the slider you see is what you hear; the saved `.style` is untouched, and tracks with no live match keep their saved volume. (Engine sends each track's `CC7` in `sendInstrumentSetup`.)

## Recorded key / original chord (Phase 4)
The accompaniment transposes *from* the chord the recording was played in — so a config stores its **original chord** at style level. See [[Chord Recognition & Transposition]].
- **Model:** `ArrangerModel` gained `originalRoot` / `originalQuality`; the `.style` schema bumped **v3 → v4**, and v3 files load as **C major**.
- **Authoring:** on load/record the editor **auto-detects** the recorded chord (`autoDetectOriginalChord()` → `ChordDetector::detectKeyFromEvents`) and pre-fills two combos — **`keyRootBox`** (C…B) and **`keyQualityBox`** (maj/min/…). The detect result is editable; if nothing is detectable it falls back to C major. `populateKeyControls()` seeds them.
- **Bass part:** a source track whose display name contains *bass* is tagged `PartKind::Bass` (`ArrangerPatternBuilder` / `ArrangerSourceBuilder`), so it transposes and honours **bass inversion**; drums/perc stay `Fixed` (never transpose).

## Gotchas
- The editor/browser are full-screen overlays parented to the top-level window, but the OpenGL note layer renders *through their middle* (it's a native surface above peer painting). `CurrentStyleComponent::presentOverlay` fires `onAuthoringOverlayVisible(true)` → `Display::onArrangerOverlayVisible` → `MainComponent::setArrangerOverlaySceneHidden(true)` to hide the note layer while an overlay is up (restored on `showStyleList(false)`). See [[GL Overlay Rendering]]. Symptom before the fix: the timeline appeared as two strips (ruler+region-tops up top, region-bottoms at the bottom) with a black band between.
- Auto Fill / matching only works if a `Fill N` (or any Fill) exists in the config; otherwise the switch is direct.
- **Click-to-jump bug (fixed):** a bare click on a region used to fail to jump. Cause: sub-bar mouse jitter fired `mouseDrag`, which made `mouseUp` emit `onWindowsChanged` → `rebuildPreview` → `engine.setStyle` → `sequencer.reset()`, wiping the `pendingIndex` the click had just queued. Fix: `ArrangerTimelineComponent` only treats a gesture as an edit when a bar value **actually changes** (`windowsChangedByDrag`), so a plain click no longer rebuilds the style. Lesson: **any `setStyle`/`rebuildPreview` during playback resets the sequencer** — never trigger it on a click.
