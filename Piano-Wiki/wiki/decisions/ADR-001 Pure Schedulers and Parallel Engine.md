---
type: decision
title: "ADR-001: Pure Schedulers and Parallel Arranger Engine"
status: active
date: 2026-06-06
context: "Adding an arranger to an app that already has a working MultipleTrackPlayer"
created: 2026-06-06
updated: 2026-06-06
tags:
  - decision
  - arranger
related:
  - "[[Arranger Engine]]"
  - "[[ArrangerScheduler]]"
  - "[[ArrangerSectionSequencer]]"
---

# ADR-001: Pure Schedulers and Parallel Arranger Engine

## Context
The app already ships a working classic player, [[Track Playback]] (`MultipleTrackPlayer`). The arranger needs multi-section, bar-synced playback with reliable note handling — risky real-time logic — without destabilising what already works.

## Decision
1. **Split the real-time concern from the musical logic.** [[ArrangerScheduler]] (per-section loop) and [[ArrangerSectionSequencer]] (section state machine) are **pure**: no threads, no I/O, no clock. The only impure/threaded class is [[Arranger Engine]] (`HighResolutionTimer`), which just maps the pure outputs to MIDI dispatch.
2. **Run the arranger parallel to the classic player.** `ArrangerEngine` is a separate object; output is re-routed when switching engines. `MultipleTrackPlayer` is untouched.

## Alternatives considered
- **Extend `MultipleTrackPlayer` in place** — rejected: would entangle new section logic with the stable player and risk regressions.
- **One monolithic real-time class** — rejected: untestable; every timing bug would need audio hardware to reproduce.

## Consequences
- **Positive:** scheduler/sequencer are unit-tested without audio (`renderRange`, `advance` are public and deterministic). The classic player stays safe. Real-time risk is confined to one small class.
- **Positive:** clean phase roadmap — pure logic can grow (per-section phrases, transposition) without touching the engine loop. See [[Arranger Phase Roadmap]].
- **Negative / trade-off:** two playback engines to maintain and an output-routing seam to manage on engine switch; some duplicated concepts (timing, channels) between the two.
