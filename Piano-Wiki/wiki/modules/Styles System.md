---
type: module
title: "Styles System"
path: "Source/Styles/ (StyleViewComponent, StylesListComponent, SectionsComponent, SectionGroupComponent, CurrentStyleComponent, StyleSection, styleSettingsEntry)"
status: active
language: cpp
purpose: "Arranger styles and their sections (Intro/Variation/Fill/Ending) — data and UI"
depends_on:
  - "[[Track Playback]]"
used_by:
  - "[[Arranger Engine]]"
  - "[[Keyboard & Main UI]]"
created: 2026-06-06
updated: 2026-11-24
tags:
  - module
  - styles
related:
  - "[[Arranger Engine]]"
sources: []
---

# Styles System

## Purpose
Manages arranger *styles* (named collections of accompaniment tracks) and the *section* buttons that select which part of a style plays (Intro, Variation, Fill, Break, Ending).

## Key files
- `StyleViewComponent.h/.cpp` — one style as an LCD row of the styles list (the open one inverted with ◂). Click opens it, right-click renames/deletes (`onStyleClicked`, `onStyleRenamed`, `onStyleRemoveComponent`).
- `StylesListComponent.h/.cpp` — the styles list of the Style page: heading, LCD rows, **Add / Rename / Delete** (Rename / Delete act on the open style through its row's own dialogs; DEFAULT Style: disabled). The dialogs are `StageDialog`s owned by the list (2026-11-25); outcomes go to the stage screen message through `onInfo`.
- `InstrumentPicker.h/.cpp` — the "Choose instrument · slot N" sub-page (2026-11-25): LCD search, the 16 GM families as a 2 × 8 grid, the family's 8 programs (current one marked ◂); typing searches all 128. `GmInstruments::families / familyOf / search` are pure (test `InstrumentPicker`).
- `SectionsComponent.h/.cpp` — `StyleSectionComponent`: lays out the section buttons grouped via `SectionGroupComponent`, tracks the last-clicked/active button, applies activation colours, and wires each button to a callback (the callback map drives which engine action fires).
- `SectionGroupComponent.h/.cpp` — a group of related section buttons (mutually-exclusive activation within a group).
- `StyleSection.h`, `styleSettingsEntry.h` — section/style data types and persisted settings.

## How it works
A style is chosen in the styles list. The `StyleSectionComponent` renders the section buttons; clicking one runs its registered callback, which (in arranger mode) calls into the [[Arranger Engine]] (`queueSection` / `selectStartSection`) or (in classic mode) sets the active `StyleSection` on [[Track Playback]]. Active-button colouring reflects the current/queued section.

## The Style page (UI redesign phase 4a, 2026-11-24)
No tabs any more (`MyTabbedComponent` / `TemporaryTabs` removed). `Display` is the Styles page's content and shows one
view at a time (`StyleViewRouter`, `DisplayViews.cpp`): the **style** (styles list + the open style's 8 `Track` strips,
`stylePageLayout`: column 230, gap 24), the **track picker** (`TrackListComponent`, "Choose track · slot N"), the
**notes page** (`TableContainer`, owned by `Display`, declared last so it dies before `CurrentStyleComponent`) and
**style settings** (today's `PlayBackSettingsComponent`). Exit / Esc go back to the style first; closing the page
(`MainComponent::closeStagePage` → `Display::resetView`) destroys the sub-view. `MainComponent::syncStylesPage` copies
the view's title, subtitle, row gap and title-row actions to the `StagePageFrame`.

- Old controls kept as hidden state holders: the play-mode combo (All tracks / Solo go through `setPlayModeSolo` →
  `comboBoxChanged`, so an active section configuration is still cleared), Start / Stop / tempo slider / beat bar
  (the panel drives them), the picker's sort combo, the notes page's reset / modify combos.
- Picking an instrument keeps the track's name (owner 2026-11-23); the instrument button shows
  `Track::instrumentCaption()` (GM name / Drums / As recorded / —). The button opens the **instrument picker**
  (`StyleView::instruments`, `Display::showInstrumentPicker`); a pick returns to the style through `callAsync`.
- **The chosen instrument is what plays** (2026-11-25, test `TrackInstrumentPlayback`): `startPlaying` re-runs
  `applyBPMchangeBeforePlayback`, which takes the strip's instrument and drops the MIDI file's own program changes on
  that track; an arranger config gets the strip's instrument as well as its volume (`applyLiveTrackSettings`); a pick
  reloads the SFZ (`onTrackInstrumentChosen` → `loadSfzForCurrentStyle`). Before, the program baked at style load and
  the file's own program changes won, and the arranger copied volumes only.
- Board values: `docs/superpowers/specs/assets/2026-11-23-ui-redesign/` (`StageRevised` openPage=styles,
  `PageTracks`, `PageNotes`); local only, see [[Owner Backlog]].

## Connects to
- Used by: [[Arranger Engine]] (section switching), [[Keyboard & Main UI]] (button callbacks wired in `MainComponent`).
- Depends on: [[Track Playback]] (`StyleSection`, `TrackEntry`).

## Notes / gotchas
> [!key-insight] Buttons are callback-driven
> `SectionsComponent` doesn't know about the engine directly — it invokes a `std::function` from the callback map. The wiring decides whether a click drives the classic player or the arranger (see commits "wire section buttons to engine").
