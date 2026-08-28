---
type: module
title: "ArrangerTime"
path: "Source/Arranger/ArrangerTime.cpp/.h"
status: active
language: cpp
purpose: "Pure musical-time conversions shared across the arranger"
depends_on: []
used_by:
  - "[[Arranger Engine]]"
  - "[[ArrangerSectionSequencer]]"
  - "[[ArrangerPatternBuilder]]"
created: 2026-06-06
updated: 2026-06-06
tags:
  - module
  - arranger
related:
  - "[[Monotonic Beat Playhead]]"
sources: []
---

# ArrangerTime

The arranger's small, pure time library. A **beat = one quarter note** everywhere in the arranger.

## Functions
- `secondsToBeats(seconds, bpm)` = `seconds * bpm / 60` (bpm ≤ 0 falls back to 120).
- `beatsToSeconds(beats, bpm)` = `beats * 60 / bpm`.
- `beatsPerBar(num, denom)` = `num * 4 / denom` — quarter-note beats per bar, so 6/8 → 3.0, 4/4 → 4.0. Bad values default to 4.
- `barsForBeats(beats, beatsPerBar)` = `ceil(beats / beatsPerBar)` with a `1e-6` tolerance; 0 → 0.

## Why it exists
Centralising these keeps the [[Arranger Engine]] timer loop, the [[ArrangerSectionSequencer]] bar math, and the [[ArrangerPatternBuilder]] conversions consistent and individually testable. Used by [[Monotonic Beat Playhead]] (the engine accumulates `secondsToBeats(delta, bpm)` each tick).
