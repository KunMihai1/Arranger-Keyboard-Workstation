---
type: module
title: "ArrangerScheduler"
path: "Source/Arranger/ArrangerScheduler.cpp/.h"
status: active
language: cpp
purpose: "Pure per-section loop scheduler: emits a section's MIDI for a beat window and closes notes at every seam"
depends_on:
  - "[[ArrangerModel]]"
used_by:
  - "[[Arranger Engine]]"
created: 2026-06-06
updated: 2026-08-19
tags:
  - module
  - arranger
related:
  - "[[ArrangerSectionSequencer]]"
  - "[[Loop-Seam Note-Off]]"
sources: []
---

# ArrangerScheduler

> [!key-insight] Pure and deterministic
> No threads, no I/O, no clock. Given a loop and a beat window it returns exactly the events to emit. This is what makes the arranger unit-testable — see [[ADR-001 Pure Schedulers and Parallel Engine]].

## Responsibility
Holds **one section's** events (positioned in beats within `[0, loopLen)`) and, on `advance(from, to)`, returns the `EmittedEvent`s for that monotonic window — wrapping the loop as many times as needed and closing any still-sounding notes at each loop seam so nothing hangs.

## API
- `setLoop(events, loopLengthBeats)` — stores events, `stable_sort`s them by beat, clears active-note state.
- `advance(fromBeats, toBeats)` — the core. Returns events with **absolute** beat timestamps.
- `flushActiveNotes(atBeats)` — emit `noteOff` for every currently-sounding note at `atBeats`, then clear. Used by the engine on a mid-loop section switch (where the normal seam-close wouldn't fire).
- `reset()` — forget which notes are sounding (does not touch the loop).

## How `advance` works (the algorithm)
Walks the window in loop-iteration slices:
1. `iterationIndex = floor(pos / loopLen)` → the absolute start of the current loop pass.
2. Computes the section-local `phaseStart`/`phaseEnd` for this slice (clamped to the next wrap or `toBeats`).
3. Emits every sorted event whose beat falls in `[phaseStart, phaseEnd)`, offsetting it back to absolute beats (`iterationStartAbs + ev.beats`), and tracks it as active/inactive via `trackActiveNote`.
4. If the slice actually reached a loop seam (`segmentEndAbs == nextWrapAbs`), emits `noteOff` for all still-active notes at the seam and clears them — see [[Loop-Seam Note-Off]].
5. Advances `pos` and repeats until `toBeats`.

`trackActiveNote` keys active notes by `(channel, noteNumber)`: a `noteOn` with velocity > 0 inserts, a `noteOff` (or `noteOn` vel 0) erases. Floating-point comparisons use `1e-9`/`1e-12` tolerances.

## Connects to
- Used by: [[Arranger Engine]] — one `ArrangerScheduler` per section, indexed by the [[ArrangerSectionSequencer]]'s segments.
- Model: [[ArrangerModel]] (`TimedBeatEvent`).

## Gotchas
> [!key-insight] Two ways notes get closed
> Within a single section, loop seams close notes automatically (step 4). But when the engine *switches* sections mid-loop, it must call `flushActiveNotes()` on the outgoing scheduler manually — the seam logic only fires on a wrap of the *same* loop.

> [!warning] The seam note-off's `part`/`ntt` tags are vestigial — do not build on them
> Both the seam close and `flushActiveNotes()` stamp each synthetic `noteOff` with the originating
> note's `PartKind`/`NttType`, memoised at note-on time in `activeNoteParts` / `activeNoteNtt`
> (`ArrangerScheduler.h:54-55`). **Nothing in production reads them back.**
>
> `ArrangerEngine::dispatchEmitted` reads `e.part`/`e.ntt` only in the *note-on* branch
> (`ArrangerEngine.cpp:200,206`). Its note-off branch ignores both and instead looks up
> `activePlayedNote` — `(channel, ORIGINAL pitch) → sounding pitch`, recorded at the moment the
> note-on was transposed (`ArrangerEngine.cpp:207,213-217`).
>
> That asymmetry is correct, not an oversight. Re-transposing an off at seam time would use the
> **current** chord, so a chord change between note-on and seam would emit an off for a pitch that
> was never sounding — hanging the real note forever. Memoising the played pitch is immune to it.
>
> Consequence: `activeNoteNtt` has **zero** consumers, production or test. `activeNoteParts` has
> exactly one — `tests/unit/test_arranger_scheduler.cpp:114` asserts the seam off carries
> `PartKind::Acc`. Both cost two map insert/erase per note and are harmless at arranger note rates.
> Left in place deliberately: if offs ever need real tags, the plumbing already exists. Deleting
> `activeNoteNtt` is free; deleting `activeNoteParts` also costs that test assertion.
>
> Likely history: `activeNoteParts` arrived in Phase 4 assuming offs would be re-transposed;
> `activeNoteNtt` was added beside it in Phase 7a for symmetry; `activePlayedNote` superseded both
> without either being removed. Verified 2026-08-19 against `e394005`.
