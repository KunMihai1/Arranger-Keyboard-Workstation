---
type: concept
title: "Loop-Seam Note-Off"
complexity: intermediate
domain: arranger
aliases:
  - "hanging-note prevention"
  - "note flushing"
status: developing
created: 2026-06-06
updated: 2026-08-19
tags:
  - concept
  - arranger
related:
  - "[[ArrangerScheduler]]"
  - "[[Arranger Engine]]"
---

# Loop-Seam Note-Off

## Definition
The rule that **no note is left sounding across a boundary**. Whenever a loop wraps or a section switches, every still-active note gets an explicit `noteOff` first.

## Why it matters here
A looped pattern can end with a note still held (its `noteOff` lives at or beyond the loop end). Without intervention it would sustain forever or double-trigger on the next pass. Two mechanisms handle this:

### 1. Automatic seam close (same section)
Inside [[ArrangerScheduler]]`::advance`, active notes are tracked by `(channel, noteNumber)` (`noteOn` vel>0 inserts, `noteOff`/vel0 erases). When a slice reaches the loop wrap, the scheduler emits `noteOff` for everything still active **at the seam** and clears the set.

### 2. Manual flush (section switch)
The automatic close only fires on a wrap of the *same* loop. When the [[Arranger Engine]] switches sections mid-loop, it explicitly calls `outgoing.flushActiveNotes()` (emit `noteOff` for all active, then clear) and `reset()`s both the outgoing and incoming schedulers so the new section starts clean at its bar 0.

```
if (seg.sectionIndex != currentSchedulerIndex) {
    flushActiveNotes() on outgoing → dispatch noteOffs
    reset() outgoing; reset() incoming
}
```

As a final safety net, `haltAudio()` sends `allNotesOff` on channels 1–16 on stop/Ending.

## Gotcha: the synthetic note-off's tags are not load-bearing
Both mechanisms stamp each synthetic `noteOff` with the originating note's `PartKind`/`NttType`.
**No production code reads them back.** [[Arranger Engine]]`::dispatchEmitted` transposes only
*note-ons*; note-offs are resolved through its own `activePlayedNote` map — `(channel, ORIGINAL
pitch) → sounding pitch` — so an off always closes the exact pitch that was sent, even if the chord
changed in between. Re-transposing offs at seam time would break precisely that case. See
[[ArrangerScheduler]] for the full reasoning and removal guidance.

## Related
Implementation: [[ArrangerScheduler]]. Triggered during [[Section Switch Flow]].
