---
type: decision
title: "ADR-002: Defer the MainComponent / Display Refactor, Lean DI Over Event Bus"
status: active
date: 2026-09-04
context: "MainComponent and Display have grown into large integration files; whether/how to refactor them was discussed and deliberately deferred"
created: 2026-09-04
updated: 2026-09-04
tags:
  - decision
  - ui
  - refactor
related:
  - "[[Keyboard & Main UI]]"
  - "[[ADR-001 Pure Schedulers and Parallel Engine]]"
---

# ADR-002: Defer the MainComponent / Display Refactor, Lean DI Over Event Bus When It Happens

## Context
`MainComponent.cpp` (~3,083 lines) and `Display.cpp` (~1,174 lines) are the app's largest
files. `MainComponent` because it's the composition root — it owns nearly every
subsystem directly as a member and wires them together via ~40+ inline `std::function`
callback assignments. `Display` has grown large for similar reasons at a smaller scale.
(`Styles/CurrentStyleComponent.cpp`, ~1,375 lines, has the same shape and deserves the
same scrutiny later — noted here, not otherwise addressed by this ADR.)

Whether to refactor — extract sub-owners, wiring coordinators, dependency injection, an
event bus — was discussed at length.

## Decision
Leave both files as-is for now. If/when a refactor happens:
- **Prefer DI-style decomposition** — small coordinator/sub-owner classes that are
  constructed and handed references (e.g. an object that owns just the chord-settings
  wiring), rather than a wholesale rewrite of ownership or wiring.
- **Do not reach for an event bus / pub-sub** as the fix. Most relationships inside
  `MainComponent` are one call away — subsystem A directly triggering subsystem B
  through a single callback. A bus adds a broker, subscription bookkeeping, and
  untraceable "who's listening" indirection to solve a many-to-many fan-out problem this
  app doesn't have. It only pays for itself where fan-out is genuinely unknown at compile
  time; a 1:1 callback doesn't qualify.
- Any extraction happens **incrementally** — pull out only the one area a change is
  already forcing you into, never as a dedicated refactor pass.

## Alternatives considered
- **Refactor now** — rejected: no concrete pain (no bug traced to this shape, no second
  host window, no need to unit-test subsystems in isolation) to justify the churn and
  regression risk of touching the app's highest-blast-radius files.
- **Event bus across the board** — rejected: trades a bluntly-readable file for
  indirection, without a fan-out problem that would justify the cost.

## Consequences
- **Positive:** no near-term churn or regression risk; the door stays open for a lighter,
  targeted DI-based cleanup later, done piece by piece.
- **Negative / trade-off:** the files keep growing in the meantime — every new subsystem
  or callback continues to land in `MainComponent`/`Display` until the trigger condition
  below is actually met.

## Trigger to revisit
A concrete need shows up — wanting to unit-test the audio+arranger stack without the
GUI, adding a second host window, or genuine concurrent multi-person work on disjoint
areas of `MainComponent`. General file-size discomfort alone is not the trigger.
