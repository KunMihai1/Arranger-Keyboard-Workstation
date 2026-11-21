---
type: decision
title: "ADR-003: 800-Line File Cap, Split by Responsibility Into Extra Translation Units"
status: active
date: 2026-11-21
context: "The owner set a hard maximum of 800 lines per source/test file; five sources and two test files were over it"
created: 2026-11-21
updated: 2026-11-21
tags:
  - decision
  - refactor
  - ci
related:
  - "[[ADR-002 Defer MainComponent Display Refactor]]"
  - "[[Keyboard & Main UI]]"
---

# ADR-003: 800-Line File Cap, Split by Responsibility Into Extra Translation Units

**Supersedes** the "leave the files as-is" part of [[ADR-002 Defer MainComponent Display Refactor]].
ADR-002's other guidance still stands: no event bus, and any *architectural* extraction (sub-owners,
DI) happens incrementally, only when a change forces it.

## Context
The owner set a rule: no file over 800 lines. On 2026-11-21 these were over it:
`MainComponent.cpp` 3083, `CurrentStyleComponent.cpp` 1375, `Display.cpp` 1174,
`MidiHandler.cpp` 1114, `CustomTableContainer.cpp` 899, `test_midi_handler.cpp` 1156,
`test_io_helper.cpp` 825.

## Decision
- **Hard cap: 800 lines per `.cpp`/`.h` under `Source/` and `tests/`.** CI fails the build otherwise
  (first step of `.github/workflows/ci.yml`).
- **Split by moving member-function definitions verbatim into extra translation units of the same
  class**, grouped by responsibility. Class declarations, ownership and wiring are untouched:
  - `MainComponent.cpp` (core: ctor/dtor, paint, layout, input, devices check, login) +
    `MainComponentInit.cpp` (widget `*Init()`), `MainComponentActions.cpp` (`toggle*`, `*OnClick`),
    `MainComponentEffects.cpp` (effects/CC settings, chord library), `MainComponentOverlays.cpp`
    (ESC menu, loading label, arranger overlays), `MainComponentPlayback.cpp` (devices, SFZ,
    instruments, recording, section handlers).
  - `CurrentStyleComponent.cpp` + `CurrentStyleComponentTracks.cpp` (tracks, BPM, JSON) +
    `CurrentStyleComponentUI.cpp` (input handlers, section buttons, layout).
  - `Display.cpp` + `DisplayStyleStore.cpp` (style JSON persistence, track removal across styles).
  - `MidiHandler.cpp` → `MidiDevice.cpp` holds the `MidiDevice` class, which shared the file.
  - `CustomTableContainer.cpp` + `CustomTableContainerModify.cpp` (modify/apply dialogs).
  - Tests: one class per file where a file held several; `MidiHandlerTest`'s keyboard/program
    groups became `MidiHandlerKeyboardTest`.
- Verified mechanically: a script checked the multiset of code lines is identical before and after
  (only the replicated `#include` lines differ), and every suite's assertion total is unchanged
  (unit 748, arranger 705, integration 137).

## Alternatives considered
- **Real decomposition (sub-owners / DI, as ADR-002 sketches)** — rejected *for this change*: it alters
  ownership and wiring in the highest-blast-radius files with no bug or feature forcing it. It stays
  the route when a concrete need appears (ADR-002's triggers).
- **Raise the cap or exempt `MainComponent`** — rejected: the owner's rule is the requirement.

## Consequences
- **Positive:** every file is reviewable on its own; files now say what they hold; CI keeps it that way.
- **Negative:** one class spans several `.cpp` files — find definitions by class name, not by file.
  New `Source/` files must be added to **both** `.jucer` projects.
