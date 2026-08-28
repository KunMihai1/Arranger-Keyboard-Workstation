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
updated: 2026-06-06
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
- `StyleViewComponent.h/.cpp` — one style item in a styles list. Click to select, right-click to rename/delete (`onStyleClicked`, `onStyleRenamed`, `onStyleRemoveComponent`).
- `StylesListComponent.h/.cpp` — the scrollable list of styles.
- `SectionsComponent.h/.cpp` — `StyleSectionComponent`: lays out the section buttons grouped via `SectionGroupComponent`, tracks the last-clicked/active button, applies activation colours, and wires each button to a callback (the callback map drives which engine action fires).
- `SectionGroupComponent.h/.cpp` — a group of related section buttons (mutually-exclusive activation within a group).
- `StyleSection.h`, `styleSettingsEntry.h` — section/style data types and persisted settings.

## How it works
A style is chosen in the styles list. The `StyleSectionComponent` renders the section buttons; clicking one runs its registered callback, which (in arranger mode) calls into the [[Arranger Engine]] (`queueSection` / `selectStartSection`) or (in classic mode) sets the active `StyleSection` on [[Track Playback]]. Active-button colouring reflects the current/queued section.

## Connects to
- Used by: [[Arranger Engine]] (section switching), [[Keyboard & Main UI]] (button callbacks wired in `MainComponent`).
- Depends on: [[Track Playback]] (`StyleSection`, `TrackEntry`).

## Notes / gotchas
> [!key-insight] Buttons are callback-driven
> `SectionsComponent` doesn't know about the engine directly — it invokes a `std::function` from the callback map. The wiring decides whether a click drives the classic player or the arranger (see commits "wire section buttons to engine").
