# Arranger Phase 6 — Synchro Start (done) + Count-In (6b) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement Part B task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Workstation transport feel — **Synchro Start** (groove begins on the first chord) and **Count-In** (metronome bars before the groove).

**Architecture:** Both are engine-side gates on the real-time timer thread, lock-free atomics, no debounce — consistent with the Phase-5 latency rule. Pure helpers carry the testable logic; the timer callback stays a thin guard.

**Tech Stack:** C++17, JUCE, JUCE `UnitTest` (Arranger category → `--run-tests --arranger-tests`).

## Global Constraints

- **No commits without user approval** — "Commit" steps mean `git add` only.
- **Latency:** gates are single atomic checks on the timer thread; triggers are atomic flips from the input thread. No locks, no allocation, no time-based debounce.
- **Korg-faithful:** Synchro Start begins on the first recognised chord; Count-In plays N bars of metronome then the groove. With both on: chord trigger → count-in → groove.
- **No regression:** with both features off, Start behaves exactly as today.

### Build & test (every task)
Build Debug x64 (close the app first — PDB lock); run `Project Synth2.exe --run-tests --arranger-tests`; confirm `TOTAL | ... | failures: 0`. **The `--run-tests` block in `Source/Main.cpp` must be uncommented** (it drifts — verify).

---

## Part A — Synchro Start ✅ DONE (recorded for completeness)

Implemented and tested this session (Arranger 473/0). Files: `ArrangerEngine.{h,cpp}`, `MidiHandler` (via existing chord path), `SettingsWindow.{h,cpp}`, `MainComponent.cpp`, `Display.{h,cpp}`, `CurrentStyleComponent.{h,cpp}`, `tests/unit/test_arranger_engine.cpp`.

- [x] Engine state: `synchroStartEnabled` (setting) + `std::atomic<bool> synchroArmed`; `setSynchroStartEnabled`, `isSynchroArmed`/`armSynchro` (test seams).
- [x] `start()` arms when enabled; `hiResTimerCallback` holds at the downbeat while armed (keeps `lastNowSeconds` fresh → clean start); `setActiveChord` releases on the first **valid** chord; `stop`/`haltAudio` clear it.
- [x] Composes with the Phase-5 held-chord seed (a chord held at Start fires immediately).
- [x] Settings "Synchro Start" toggle (`"SynchroStart"`) wired Settings→MainComponent→Display→CurrentStyleComponent→engine.
- [x] Tests: trigger fires only on a valid chord; stop clears the gate.

Design detail: `docs/superpowers/specs/2026-06-25-arranger-phase6-synchro-start-design.md`.

---

## Part B — Count-In (Phase 6b) ⬜ TO DO

**Goal:** When Count-In is enabled, pressing Start plays **one bar** of metronome clicks (accent on beat 1), then the groove begins from beat 0 of the chosen start section.

**Korg defaults taken (no user needed):** 1 bar; clicks on channel 10 (drums); beat 1 = an accented click, other beats = a softer click; GM side-stick (note 37) as the click voice (audible on the SFZ drum kit and on external GM). Bar count is fixed at 1 for the first cut (a "Count-In bars" setting is a later nicety).

**Architecture decision:** engine-level **pre-roll**, not a synthesised `CountIn` section. The engine counts down a count-in window before advancing the section schedulers — contained, like Synchro Start, and avoids wiring new sequencer rules. (The `CountIn` enum stays a reserved label; we do not route it through the sequencer here.) Interaction: if Synchro Start is also on, the count-in begins **after** the chord releases the synchro gate.

**Pure helper (testable without the timer):** `ArrangerCountIn` — given `beatsPerBar`, the count-in length in beats, and a `[fromBeat, toBeat)` window, returns the click events (channel/note/velocity) whose beat falls in the window. Beat 0 → accented; others → soft. This isolates the "which clicks fire when" logic from the timer.

**Files:**
- Create: `Source/Arranger/ArrangerCountIn.h` (pure helper) — or fold into `ArrangerTime` if trivial.
- Modify: `Source/Arranger/ArrangerEngine.{h,cpp}` (count-in phase + timer integration).
- Modify: `Source/UI/SettingsWindow.{h,cpp}`, `Source/UI/MainComponent.cpp`, `Source/UI/Display.{h,cpp}`, `Source/Styles/CurrentStyleComponent.{h,cpp}` (toggle + wiring, mirror Synchro Start).
- Test: `tests/unit/test_arranger_count_in.cpp` (new, Arranger category) + a count-in case in `test_arranger_engine.cpp`.

**Interfaces:**
- Produces: `struct ClickEvent { int channel; int note; int velocity; double beat; };`
- Produces: `std::vector<ClickEvent> ArrangerCountIn::clicksInWindow(double fromBeats, double toBeats, double beatsPerBar);`
- Produces: `ArrangerEngine::setCountInEnabled(bool)`, `bool isCountingIn() const` (test seam).

### Task 1: Pure count-in click helper (TDD)

- [ ] **Step 1: Failing test** — `tests/unit/test_arranger_count_in.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerCountIn.h"
class ArrangerCountInTest : public juce::UnitTest {
public:
    ArrangerCountInTest() : juce::UnitTest("ArrangerCountIn","Arranger") {}
    void runTest() override {
        beginTest("one 4/4 bar emits 4 clicks, beat 0 accented");
        auto c = ArrangerCountIn::clicksInWindow(0.0, 4.0, 4.0);
        expectEquals((int)c.size(), 4);
        expect(c[0].beat == 0.0 && c[0].velocity > c[1].velocity);   // accent on 1
        for (auto& e : c) { expectEquals(e.channel, 10); expectEquals(e.note, 37); }
        beginTest("window is half-open [from,to): no duplicate at the seam");
        expectEquals((int)ArrangerCountIn::clicksInWindow(0.0, 2.0, 4.0).size(), 2);
        expectEquals((int)ArrangerCountIn::clicksInWindow(2.0, 4.0, 4.0).size(), 2);
    }
};
static ArrangerCountInTest arrangerCountInTest;
```
- [ ] **Step 2: Register the test file** in the Projucer project (so it compiles into the app) and build — expect FAIL (no `ArrangerCountIn.h`). *(If Projucer resave adds the file, build twice — see build/test memory.)*
- [ ] **Step 3: Implement `ArrangerCountIn.h`:**
```cpp
#pragma once
#include <vector>
#include <cmath>
struct ClickEvent { int channel; int note; int velocity; double beat; };
struct ArrangerCountIn {
    // One bar of metronome clicks: a click on each integer beat in [fromBeats, toBeats).
    // Beat 0 of the bar is accented. Channel 10 / GM side-stick (37).
    static std::vector<ClickEvent> clicksInWindow(double fromBeats, double toBeats, double beatsPerBar) {
        std::vector<ClickEvent> out;
        const int first = (int) std::ceil(fromBeats - 1e-9);
        for (int b = first; b < (int) std::ceil(toBeats - 1e-9); ++b) {
            if (b < 0 || b >= (int) std::lround(beatsPerBar)) continue;
            const int vel = (b == 0) ? 110 : 70;
            out.push_back({ 10, 37, vel, (double) b });
        }
        return out;
    }
};
```
- [ ] **Step 4: Build & run `--arranger-tests`** — expect the `ArrangerCountIn` test green.
- [ ] **Step 5: Stage** (`git add` the new files; commit deferred).

### Task 2: Engine count-in phase (pre-roll before the section)

- [ ] **Step 1: Failing test** in `test_arranger_engine.cpp` — drive the engine through a count-in via a renderRange-style seam (add `bool isCountingIn() const`):
```cpp
beginTest("count-in: section output is suppressed until the count-in bar elapses");
// enable count-in, start (armed countIn), renderRange across [0,4): expect click events on ch10
// note 37 and NO section notes (the makeStyle note 60); renderRange across [4,4.25): expect note 60 on.
```
  (Mirror the existing direct-`renderRange` test style; add a thin `renderCountIn`/`isCountingIn` seam so this is timer-free.)
- [ ] **Step 2: Build & run** — expect FAIL.
- [ ] **Step 3: Implement** the count-in phase on `ArrangerEngine`:
  - State: `bool countInEnabled = false;` `std::atomic<bool> countingIn { false };` `double countInPlayhead = 0.0;` `double countInLengthBeats = 0.0;` (= `beatsPerBar`).
  - `start()`: if `countInEnabled`, set `countingIn = true`, `countInPlayhead = 0`, `countInLengthBeats = timeSigNum`.
  - In `hiResTimerCallback`, after the Synchro gate: `if (countingIn.load()) { advance the count-in window, dispatch ArrangerCountIn::clicksInWindow(...) as note-ons (ch10 note 37), and when countInPlayhead >= countInLengthBeats, clear countingIn and reset lastNowSeconds so the section starts clean at beat 0; return; }`.
  - With **Synchro Start** on too: only enter the count-in **after** `synchroArmed` clears (the chord starts the count-in). Order in the callback: synchro gate → count-in gate → section.
  - `stop`/`haltAudio`: `countingIn = false`.
- [ ] **Step 4: Build & run** — expect PASS.
- [ ] **Step 5: Stage** (commit deferred).

### Task 3: UI toggle + wiring (mirror Synchro Start)

- [ ] **Step 1:** Settings "Count-In" toggle (`SettingsWindow.{h,cpp}`), property `"CountIn"`, callback `onCountInChanged(bool)`. Lay it out under "Synchro Start"; bump the panel height one row.
- [ ] **Step 2:** `MainComponent.cpp` — wire `onCountInChanged` → `Display::setArrangerCountIn`; apply stored value at startup.
- [ ] **Step 3:** `Display.{h,cpp}` — `setArrangerCountIn(bool)` + remembered member, re-applied on style-component creation.
- [ ] **Step 4:** `CurrentStyleComponent.{h,cpp}` — `setCountInEnabled(bool)` → `arrangerEngine->setCountInEnabled`.
- [ ] **Step 5: Build, run full suite, manual smoke** — enable Count-In, press Start → hear one bar of clicks (accent on 1), then the groove. With Synchro Start also on → chord → count-in → groove.
- [ ] **Step 6: Stage** (commit deferred — user review).

## Out of scope (Phase 6)
- Configurable count-in bar count / click voice selection (fixed at 1 bar, GM side-stick for now).
- Synchro Stop (stop on chord release) — small follow-on.
- NTT depth / chord variations — separate harmony-side phase.

## Self-Review
- Synchro Start (Part A): implemented + tested; recorded here. ✓
- Count-In (Part B): pure helper (Task 1) → engine pre-roll (Task 2) → UI (Task 3); each task independently testable/buildable. ✓
- Latency: count-in logic is a pure helper + a thin timer gate; no debounce, no locks. ✓
- Types: `ClickEvent` / `clicksInWindow` / `setCountInEnabled` / `isCountingIn` consistent across tasks. ✓
- Placeholder scan: test bodies and the helper are concrete; the engine-seam test (Task 2 Step 1) is described with the exact assertions to encode against the chosen `isCountingIn`/`renderCountIn` seam (decide the seam name when wiring, matching the existing `renderRange` test pattern). ✓
