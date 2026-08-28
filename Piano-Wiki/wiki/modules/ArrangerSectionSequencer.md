---
type: module
title: "ArrangerSectionSequencer"
path: "Source/Arranger/ArrangerSectionSequencer.cpp/.h"
status: active
language: cpp
purpose: "Pure state machine deciding which section is active across a beat window, switching only at bar boundaries"
depends_on:
  - "[[ArrangerModel]]"
  - "[[ArrangerTime]]"
used_by:
  - "[[Arranger Engine]]"
created: 2026-06-06
updated: 2026-06-07
tags:
  - module
  - arranger
related:
  - "[[ArrangerScheduler]]"
  - "[[Bar-Synced Section Switching]]"
sources: []
---

# ArrangerSectionSequencer

> [!key-insight] Emits no MIDI
> It only decides *which section* is active over a window and returns `SectionSegment`s. The [[Arranger Engine]] maps those onto per-section [[ArrangerScheduler]]s. Pure and unit-testable.

## Responsibility
The section state machine. Across a monotonic beat window it produces a list of `SectionSegment`s (each: `sectionIndex`, section-local `localFromBeats`/`localToBeats`, `sectionChanged` flag), applying queued user switches and one-shot completions **only at bar boundaries**. Sets `stopRequested` when an `Ending` finishes.

## State
- `activeIndex` — current section; `activeStartAbs` — absolute beat it began on (exposed via `getActiveStartAbs()` so the engine can compute section-local beats for the editor's playhead arrow).
- `returnIndex` — the Variation/Break to fall back to after an Intro/Fill.
- `pendingIndex` — queued target (-1 = none); last `queue()` before the boundary wins.
- `beatsPerBar` — from the style's time signature via [[ArrangerTime]].
- `stopped` — latched true after an Ending.
- `autoFillEnabled` / `fillReturnOverride` — Auto-Fill flag and the variation an auto-fill should land on (-1 = normal fill return).

## API
- `setStyle(style)` — copies section metadata (`lengthBars` clamped ≥ 1), computes `beatsPerBar`, resets.
- `reset()` — back to the first **Variation** (or 0) at beat 0.
- `startAt(index)` — begin on a specific section (e.g. Intro) while keeping `returnIndex` = first Variation.
- `queue(type, name)` — set `pendingIndex` via `findSection`. With **Auto Fill** on, queuing a Variation while a Variation plays instead queues the matching Fill (then lands on the target variation). Returns false if none.
- `setAutoFillEnabled(bool)` — toggles the Auto-Fill-on-variation-switch behavior (driven by the live UI toggle; see [[Arranger Style Authoring]]).
- `advance(from, to)` → `SequencerStep`.

### `findSection(type, name)` — match by type + trailing number
Live performance buttons send labels like `"Var 2"` / `"Break"`, but editor/engine sections are named `"Variation 2"` / `"Break 1"`. `findSection` matches on **type + trailing integer** (so `"Var 2"` → `"Variation 2"`), preferring an exact-name match, then a number match, then the first section of that type (so a single `"Break"` still resolves). This decouples the button labels from the editor's section names — no renaming/migration needed.

## How `advance` works
Walks **global bar lines**:
1. `nextBoundary = (floor(pos / beatsPerBar) + 1) * beatsPerBar` — the next bar line strictly after `pos`.
2. Emits a `SectionSegment` for `[pos, min(to, nextBoundary))`, in section-local beats (`pos - activeStartAbs`).
3. If that segment ended exactly on the bar line, calls `applyBoundary(nextBoundary)`; if the section changed, the *next* segment is flagged `sectionChanged`.

### `applyBoundary` priority
1. **Queued switch wins.** If `pendingIndex` is set, switch to it. If the target is a **Fill or Break** and we're leaving a Variation/Break, save `returnIndex = from` so the one-shot falls back to the variation you were on. For an **auto-fill**, `fillReturnOverride` (= the new variation) takes precedence so the fill lands on the *target* variation, not the one you left.
2. **One-shot completion.** If the active section's `afterComplete` is `Loop`, do nothing. Otherwise, once `boundaryAbs - activeStartAbs ≥ lengthBars * beatsPerBar`:
   - `Stop` (Ending) → latch `stopped`, set `step.stopRequested`.
   - `FallThrough` (Intro/Fill/Break) → jump to `returnIndex`.

### Auto Fill (matches Korg "Auto Fill In")
When `autoFillEnabled` and you queue a Variation switch *from* a Variation, `queue()` inserts the matching Fill (`Fill N` for `Var N`, else any Fill) as a one-bar transition and sets `fillReturnOverride` to the target variation. The Fill plays once, then `FallThrough` lands on the **new** variation. Break is the *manual* one-shot pause (not auto-inserted); it now also returns to the variation it was triggered from.

See [[Bar-Synced Section Switching]] for the timing rationale and [[Arranger Style Authoring]] for the live-button wiring.

## Connects to
- Used by: [[Arranger Engine]] (`renderRange` consumes the segments).
- Model/time: [[ArrangerModel]], [[ArrangerTime]].

## Gotchas
> [!key-insight] Boundaries are global, not section-relative
> Switches land on absolute bar lines (multiples of `beatsPerBar` from beat 0), and `activeStartAbs` records where the current section began so section-local position is `pos - activeStartAbs`. A multi-bar one-shot only completes after its *own* length has elapsed, measured from `activeStartAbs`.
