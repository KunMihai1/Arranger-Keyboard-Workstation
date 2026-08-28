---
type: concept
title: "Monotonic Beat Playhead"
complexity: intermediate
domain: arranger
aliases:
  - "beat accumulator"
status: developing
created: 2026-06-06
updated: 2026-06-06
tags:
  - concept
  - arranger
related:
  - "[[Arranger Engine]]"
  - "[[ArrangerTime]]"
---

# Monotonic Beat Playhead

## Definition
A single, ever-increasing `playheadBeats` value that represents musical position in quarter-note beats since `start()`. It never wraps — loop wrapping is handled downstream by each [[ArrangerScheduler]].

## Why it matters here
The [[Arranger Engine]] timer doesn't advance by a fixed beat step. Each ~10 ms tick it measures the **wall-clock delta** since the previous tick and converts it to beats at the current BPM:

```
delta   = now - lastNow                       // seconds
beats   = ArrangerTime::secondsToBeats(delta, currentBpm)
window  = [playheadBeats, playheadBeats + beats)
playheadBeats += beats
```

Consequences of accumulating deltas (rather than counting ticks or beats):
- A **BPM change mid-playback** changes the rate *going forward* only — the musical position doesn't jump, because past beats are already banked in `playheadBeats`.
- Timer jitter averages out: the window is exactly the real elapsed time, so no drift accumulates.
- Everything downstream ([[ArrangerScheduler]], [[ArrangerSectionSequencer]]) takes a `(from, to)` beat window, so they stay pure and clock-agnostic.

## Related
[[ArrangerTime]] does the seconds↔beats math. The playhead feeds `onElapsedBeats` for the beat bar in [[Keyboard & Main UI]].
