---
type: concept
title: "Bar-Synced Section Switching"
complexity: intermediate
domain: arranger
aliases:
  - "bar-quantized switching"
status: developing
created: 2026-06-06
updated: 2026-06-06
tags:
  - concept
  - arranger
related:
  - "[[ArrangerSectionSequencer]]"
  - "[[Section Switch Flow]]"
---

# Bar-Synced Section Switching

## Definition
Section changes (user-queued switches and one-shot completions) take effect **only on bar boundaries** — never mid-bar. This is what makes arranger transitions feel musical on a real keyboard.

## Why it matters here
When you tap Fill/Variation/Ending, the change is *queued*, not applied instantly. The [[ArrangerSectionSequencer]] walks **global bar lines** during `advance`:

```
nextBoundary = (floor(pos / beatsPerBar) + 1) * beatsPerBar
```

At each boundary `applyBoundary` runs with this priority:
1. **Queued user switch wins** (last `queue()` before the boundary wins). A Fill entered from a Variation/Break stores `returnIndex` so it can fall back.
2. **One-shot completion** for non-`Loop` sections, once `boundaryAbs - activeStartAbs ≥ lengthBars * beatsPerBar`:
   - `Stop` (Ending) → request stop.
   - `FallThrough` (Intro/Fill) → jump to the saved `returnIndex`.

Because boundaries are absolute (measured from beat 0) and `activeStartAbs` records when the current section began, a multi-bar section completes only after **its own** length, while switches still align to the global grid.

## Edge cases
- Queuing several times before a boundary: only the last target survives (`pendingIndex` is overwritten).
- Switching into a section mid-loop forces [[Loop-Seam Note-Off]] so the old section's notes don't hang.

## Related
[[Section Switch Flow]] shows the end-to-end path; [[ArrangerSectionSequencer]] is the implementation.
