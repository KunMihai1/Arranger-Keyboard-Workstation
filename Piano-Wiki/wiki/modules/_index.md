---
type: meta
title: "Modules Index"
created: 2026-06-06
updated: 2026-06-25
tags:
  - meta
  - index
  - module
status: developing
---

# Modules Index

One page per major subsystem. Use the `module` template.

| Module | Folder | Status |
|--------|--------|--------|
| [[Arranger Engine]] | `Source/Arranger/` | active (phase 4) — **deep-dive** |
| [[Chord Recognition & Transposition]] | `Source/Arranger/` | active (phase 4) |
| [[MIDI Handling]] | `Source/Midi/` | active |
| [[Audio & SFZ Playback]] | `Source/Audio/` | active |
| [[Track Playback]] | `Source/Playback/` | active |
| [[Styles System]] | `Source/Styles/` | active |
| [[Arranger Style Authoring]] | `Source/Arranger/` + `Source/Styles/` | active (phase 3) |
| [[Keyboard & Main UI]] | `Source/UI/` | active |
| [[Supabase Backend]] | `Source/Backend/` | active |

> Shared utilities (IOHelper, HelperFunctions, AppColours, interfaces) live in `Source/Common/`. `Main.cpp` stays at `Source/` root. Folder layout mirrors these modules (refactored 2026-06-06).

### Arranger Engine — sub-pages (deep dive)
| Page | Role |
|------|------|
| [[Arranger Engine]] | Real-time timer loop + hub |
| [[ArrangerSectionSequencer]] | Bar-synced section state machine (pure) |
| [[ArrangerScheduler]] | Per-section loop scheduler (pure) |
| [[ArrangerPatternBuilder]] | Builds styles from `TrackEntry` data |
| [[ArrangerModel]] | Data model (style/section/track/event) |
| [[ArrangerTime]] | Seconds↔beats / bar math |

## Dependency sketch
- [[Keyboard & Main UI]] owns and wires everything.
- [[Arranger Engine]] and [[Track Playback]] are parallel playback engines, both feeding [[Audio & SFZ Playback]] via inject callbacks and sharing MIDI-out from [[MIDI Handling]].
- [[Styles System]] section buttons drive whichever engine is active.
- [[Supabase Backend]] handles auth + device/playtime, tied to [[MIDI Handling]] device IDs.
