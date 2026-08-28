---
type: module
title: "ArrangerPatternBuilder"
path: "Source/Arranger/ArrangerPatternBuilder.cpp/.h"
status: active
language: cpp
purpose: "Builds Phase-1/2 ArrangerStyle data (beat-based sections) from recorded TrackEntry tracks"
depends_on:
  - "[[ArrangerModel]]"
  - "[[ArrangerTime]]"
  - "[[Track Playback]]"
used_by:
  - "[[Arranger Engine]]"
created: 2026-06-06
updated: 2026-06-06
tags:
  - module
  - arranger
related:
  - "[[Arranger Phase Roadmap]]"
sources: []
---

# ArrangerPatternBuilder

> [!key-insight] Bridges the classic data into the arranger
> Converts existing [[Track Playback]] `TrackEntry` data (seconds-timestamped `MidiMessageSequence`) into the beat-based [[ArrangerModel]] the engine consumes.

## Responsibility
A free-function namespace (no state) that produces `ArrangerStyle` objects from a set of `TrackEntry` tracks.

## Functions
- `buildBeatEvents(seq, referenceBpm, channel)` — keeps only note-on/off, forces them onto `channel`, converts each timestamp **seconds → beats** with `referenceBpm` (via [[ArrangerTime]]), and returns them sorted.
  > `referenceBpm` **must** be the tempo the sequence is currently scaled to (the live playback tempo), or beats won't line up with the bar grid.
- `buildSingleSectionStyle(tracks, num, denom, referenceBpm)` — one `Variation 1` section. Percussion → **channel 10**, melodic tracks → **channels 2,3,4…**. Each track carries its instrument, volume, and pattern. `lengthBars` = longest track's last beat rounded up to whole bars (`barsForBeats`).
- `buildDemoMultiSectionStyle(...)` — Phase-2 demo: derives four sections from that single loop using a `sliceBars` helper that clones a section, keeps only events inside `[startBeats, startBeats + numBars*bpb)`, and rebases them to beat 0:
  | Section | Slice | afterComplete |
  |---------|-------|---------------|
  | **Intro 1** | first bar | `FallThrough` |
  | **Variation 1** | the full loop | `Loop` |
  | **Fill 1** | last bar | `FallThrough` |
  | **Ending 1** | last `min(4, loopBars)` bars | `Stop` |

  Order is Intro, Variation, Fill, Ending; the engine defaults to the first **Variation**.

## Why the demo slices are short
> [!key-insight] Observability over realism
> The demo Intro/Fill/Ending are deliberately short slices of the Variation so the transport behaviour (intro-once, fill-once, ending-once-then-stop) is *audible* instead of replaying the whole-song loop. Real per-section phrases are [[Arranger Phase Roadmap|Phase 3]]. This is the change behind commit `c1ac203`.

## Connects to
- Used by: [[Arranger Engine]] (`setStyle` consumes the built `ArrangerStyle`).
- Input: [[Track Playback]] `TrackEntry`; time math: [[ArrangerTime]].
