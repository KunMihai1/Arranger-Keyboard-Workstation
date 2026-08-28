---
type: module
title: "ArrangerModel"
path: "Source/Arranger/ArrangerModel.h"
status: active
language: cpp
purpose: "Pure data model for arranger styles, sections, tracks and timed events"
depends_on: []
used_by:
  - "[[Arranger Engine]]"
  - "[[ArrangerScheduler]]"
  - "[[ArrangerSectionSequencer]]"
  - "[[ArrangerPatternBuilder]]"
created: 2026-06-06
updated: 2026-06-06
tags:
  - module
  - arranger
related: []
sources: []
---

# ArrangerModel

Header-only data model (`ARRANGER_SCHEMA_VERSION = 2`). Plain structs, no behaviour.

## Hierarchy
`ArrangerStyle` → `ArrangerSection[]` → `ArrangerTrack[]` → `TimedBeatEvent[]`

- **`ArrangerStyle`** — `id`, `name`, `originalTempo` (BPM patterns were authored at), `timeSigNum`/`timeSigDenom`, `sections`.
- **`ArrangerSection`** — one style element / button. `type` (`ArrangerSectionType`), `lengthBars` (its own loop length), `afterComplete` (`ArrangerAfterComplete`), `tracks`.
- **`ArrangerTrack`** — one accompaniment track: `partType` (`ArrangerPartType`), `instrument`, `channel`, `volume`, and `pattern` (the section's loop for this track). `originalChord` is reserved for Phase 4/5.
- **`TimedBeatEvent`** — `{ double beats; juce::MidiMessage message; }`, positioned in `[0, loopLengthBeats)`.

## Enums
- `ArrangerSectionType` — `Intro, Variation, Fill, Break, Ending, CountIn`.
- `ArrangerPartType` — `Drum, Perc, Bass, Acc` (drives transposition in later phases; inert in Phase 1).
- `ArrangerAfterComplete` — `Loop` (forever), `FallThrough` (Intro/Fill → return variation), `Stop` (Ending). This enum is the transport rule the [[ArrangerSectionSequencer]] reads at each bar boundary.

## Connects to
Consumed by every other arranger class. See [[Arranger Engine]] for the runtime picture.
