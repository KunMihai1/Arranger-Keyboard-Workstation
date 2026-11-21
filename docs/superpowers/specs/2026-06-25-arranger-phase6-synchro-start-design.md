# Arranger Phase 6 — Synchro Start (+ Count-In follow-on)

**Date:** 2026-06-25
**Branch context:** continues `arranger-engine-phase4-chords` (Phase 5 chord-recognition modes complete)
**Status:** Design + plan ready (not yet implemented)
**Reference target:** Korg Pa-series "Synchro Start".

---

## 1. Goal

**Synchro Start:** when armed, pressing Start does *not* begin the groove immediately — the arranger
waits and starts the moment the player plays the first recognised chord. This is the classic
workstation feel: arm, then "the band comes in" exactly when you hit your first chord. Composes
naturally with the Phase-5 held-chord seed (a chord already held at Start fires it immediately).

**Count-In** (follow-on, "Phase 6b"): an optional N-bar metronome count before the groove starts.
`ArrangerSectionType::CountIn` already exists as a stub. Deferred to keep Phase 6 tight and testable.

## 2. Constraints (unchanged from Phase 5)

- Latency: the gate lives on the timer thread as a single atomic check; the trigger (first chord)
  is an atomic flip set from the input thread. No locks, no allocation, no debounce.
- No regression: classic Start (Synchro off) behaves exactly as today.

## 3. Design — engine-side arming gate

State on `ArrangerEngine`:
- `bool synchroStartEnabled = false;` — the user's setting (message thread).
- `std::atomic<bool> synchroArmed { false };` — true while waiting for the first chord.

Behaviour:
- `setSynchroStartEnabled(bool)` — stores the setting.
- `start()` — unchanged setup, then `synchroArmed.store (synchroStartEnabled);` before `startTimer(10)`.
- `hiResTimerCallback()` — after `if (! playing.load()) return;`, compute `now`, then:
  ```cpp
  if (synchroArmed.load()) { lastNowSeconds = now; return; }  // hold at playhead 0, keep clock fresh
  ```
  Keeping `lastNowSeconds` fresh means the first post-trigger tick has a ~10ms delta, so the groove
  begins cleanly from beat 0 with no time jump.
- `setActiveChord(played)` — after writing the mailbox, fire the trigger:
  ```cpp
  if (played.isValid() && synchroArmed.load())
      synchroArmed.store (false);   // first recognised chord starts the groove
  ```
- `stop()` / `haltAudio()` — `synchroArmed.store (false)` (don't leave it armed across a stop).

**Composition with the held-chord seed (Phase 5):** `CurrentStyleComponent::seedHeldChordIntoArranger()`
runs right after `start()`. If a chord is already held, it calls `setActiveChord(validChord)` → clears
`synchroArmed` → the groove starts immediately on that chord. If nothing is held, it stays armed until
the player plays one. This is exactly Korg Synchro Start.

## 4. UI

Add a **"Synchro Start"** toggle to the Settings panel next to Chord Memory / Bass Inversion
(property `"SynchroStart"`, default off). Callback `onSynchroStartChanged(bool)` →
`CurrentStyleComponent::setSynchroStartEnabled` → `ArrangerEngine::setSynchroStartEnabled`. Apply the
stored value at startup like the other chord settings.

## 5. Testability

The "don't advance while armed" guard is in `hiResTimerCallback` (timer thread) and is a 2-line check.
The **trigger logic** is unit-testable without starting the real timer via an accessor:
- Add `bool isSynchroArmed() const { return synchroArmed.load(); }` (test seam, like `peekPendingStartIndex`).
- Add `void armSynchroForTest() { synchroArmed.store (true); }` *(or)* test through `setSynchroStartEnabled`
  + a lightweight `armSynchro()` that sets the flag without `startTimer` — so tests never spin a real
  `HighResolutionTimer` (the existing engine tests deliberately avoid `start()`).

Tests (`tests/unit/test_arranger_engine.cpp`, Arranger category):
- arm → `isSynchroArmed()` true.
- `setActiveChord(Cmaj valid)` → `isSynchroArmed()` false (trigger fires).
- arm → `setActiveChord(invalid {})` → still armed (only a *valid* chord triggers).
- `haltAudio`/`stop` clears armed.

## 6. Implementation plan (bite-sized)

1. **Engine state + trigger (TDD).** Add `synchroStartEnabled`, `synchroArmed`, `setSynchroStartEnabled`,
   `isSynchroArmed`, lightweight `armSynchro()` test seam. Update `setActiveChord` to flip the trigger,
   and `stop()`/`haltAudio()` to clear it. Write the 4 trigger tests above. Build + `--arranger-tests`.
2. **Timer gate.** Add the `synchroArmed` early-return in `hiResTimerCallback` + set `synchroArmed` in
   `start()`. Build (logic covered by Task 1's trigger tests; the gate is a 2-line guard). Manual smoke:
   arm, Start with hands off → silent; play a chord → groove comes in on that chord.
3. **UI + wiring.** Settings "Synchro Start" toggle (property `"SynchroStart"`) +
   `onSynchroStartChanged` → `Display` → `CurrentStyleComponent::setSynchroStartEnabled` →
   engine; startup apply. Build + full suite + manual smoke (incl. held-chord-at-Start fires immediately).
4. **Verify + stage** (commit deferred for user review).

## 7. Out of scope (Phase 6)
- Count-In / metronome content (Phase 6b — needs click content + the `CountIn` section wired into the
  sequencer's start path).
- Synchro Stop (stop on chord release) — a small follow-on once Synchro Start is in.
- NTT depth / chord variations — separate harmony-side phase.
