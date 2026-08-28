---
type: overview
title: "Project Overview"
created: 2026-06-06
updated: 2026-06-06
tags:
  - meta
  - overview
status: developing
---

# Piano-App — Project Overview

> [!gap] Seeded from directory scan, not yet ingested
> This overview was sketched from the `Source/` file listing at scaffold time. Ingest the README / code to flesh it out.

**Piano-App** is a JUCE (C++) arranger-keyboard / digital piano application. Build config lives in `Arranger Workstation.jucer`; all code is under `Source/`.

## Subsystems (seed map)

| Area | Key files | Notes |
|------|-----------|-------|
| **Arranger Engine** | `Source/Arranger/ArrangerEngine`, `ArrangerScheduler`, `ArrangerSectionSequencer`, `ArrangerPatternBuilder`, `ArrangerTime` | Multi-section playback engine, bar-synced switching. Active dev area. |
| **MIDI Handling** | `MidiHandler`, `MidiRecordPlayer`, `MidiDevicesDB`, `keyListener` | Input, recording, device management. |
| **Audio / SFZ** | `AudioHandler`, `SFZlibrary`, `SFZLibraryUI` | Sound generation, SFZ instrument library. |
| **Track Playback** | `TrackPlayer`, `Track`, `TrackListComponent`, `MidiNotesTableModel` | Multi-track sequencing and display. |
| **Styles System** | `StyleViewComponent`, `StylesListComponent`, `SectionsComponent`, `SectionGroupComponent` | Arranger styles, sections (Intro/Variation/Fill/Ending). |
| **Keyboard / UI** | `KeyboardUI`, `MainComponent`, `displayGUI`, `NoteLayer`, `CustomBeatBar` | On-screen keyboard and main interface. |
| **Backend / Auth** | `SupabaseClient`, `LoginComponent`, `ValidatorUI`, `PlaytimeTracker` | Supabase integration, login, usage tracking. |

## Folder structure (refactored 2026-06-06)

`Source/` is grouped by subsystem, mirroring these modules:

```
Source/
├── Arranger/   → [[Arranger Engine]] (+ sub-pages)
├── Midi/       → [[MIDI Handling]]
├── Audio/      → [[Audio & SFZ Playback]]
├── Playback/   → [[Track Playback]]
├── Styles/     → [[Styles System]]
├── Backend/    → [[Supabase Backend]]
├── UI/         → [[Keyboard & Main UI]] (+ all view components)
├── Common/     → shared: IOHelper, HelperFunctions, AppColours, interfaces
└── Main.cpp    → app entry point (root)
```

Includes resolve via Projucer header-search-paths (one entry per folder), so bare
`#include "Foo.h"` keeps working across folders.

## Current focus
Arranger engine phase 1 — see [[log]] and branch `arranger-engine-phase1`.
