# Arranger Engine — Phase 2 Design Spec (Section State Machine + Transport)

**Date:** 2026-06-05
**Status:** Phase 2 implemented (multi-section engine + sequencer + button wiring; 108 Arranger unit-test assertions passing)
**Author:** Mihai + Claude
**Depends on:** Phase 1 (`arranger-engine-phase1`, merged) — looping engine, `ArrangerModel`, `ArrangerScheduler`, `ArrangerPatternBuilder`, `ArrangerEngine`, Classic/Arranger toggle.
**Parent roadmap:** `2026-06-05-arranger-engine-design.md` §4, Phase 2.

---

## 1. Context & Goal

Phase 1 added an **Arranger mode**: pressing Start loops the current style's tracks
(a single derived section) seamlessly to the existing MIDI-out / SFZ path, behind
a persisted Classic/Arranger toggle. The section buttons (Intro 1-3, Var 1-4,
Fill 1-4, Ending 1-3, Break) already exist in `StyleSectionComponent` and fire
callbacks through `Display::handleXxxDisplay` →
`CurrentStyleComponent::handleXxxCurrentStyle`, but in arranger mode those
callbacks do **nothing** — they only call `trackPlayer->setLastSectionUsed(...)`
for the classic linear player.

**Goal:** make the section buttons drive the arranger like a Korg/Yamaha
workstation: pressing a section queues a **bar-synced** switch, **Intro** plays
once then falls into a Variation, **Ending** plays once then stops, and a **Fill**
plays once then returns to the current variation — all with no hung notes at any
seam. Classic linear playback stays byte-for-byte unchanged behind the toggle.

### The content tension (and why it doesn't block Phase 2)

The roadmap intentionally orders **Phase 2 = state machine** *before*
**Phase 3 = the real Content & Sectioning pipeline**. So Phase 2 has no real,
distinct per-section content to switch between yet — the builder still produces a
single loop.

The resolution is the central design idea of this phase:

> **Intro-plays-once, Ending-plays-once-then-stops, and Fill-plays-once-then-returns
> are *transport behaviors*, not content differences.** Every one of them is
> demonstrable audibly using only the single loop Phase 1 already builds — the
> loop content is identical; only the *transport flag* differs.

The **only** thing that cannot be demonstrated audibly with one loop is two
variations *sounding different* (Var 1 vs Var 2). That is correctly a **content**
concern: it is covered in Phase 2 by **unit tests** against hand-built synthetic
multi-section styles, and given real audio in Phase 3.

This keeps Phase 2 "audible and testable on its own" (the project's per-phase
philosophy), respects the roadmap order, and confines the only throwaway code to a
single builder function.

---

## 2. Scope

### In
- A pure, unit-tested **`ArrangerSectionSequencer`** — the section state machine.
- `ArrangerEngine` upgraded from single-loop to **multi-section** playback.
- **Bar-synced** queued section switching with seam note-offs across the swap.
- **One-shot sections** with auto-transitions: Intro → Variation, Fill → return,
  Ending → stop.
- A builder function producing a small, honest **multi-section demo style** from
  the one available loop.
- Wiring the existing Intro/Var/Fill/Ending/Break buttons to the engine **in
  arranger mode only**.

### Out / deferred (decided with the user)
- **Auto-Fill on variation change** — Phase 3+ (only meaningful with multiple
  distinct variations).
- **Synchro Start** — Phase 4. It most naturally triggers on the first *recognized
  chord*; pulling MIDI-input scanning in now buys little Phase-2 value.
- **Count-In** — no metronome content yet.
- **Real distinct-sounding per-section content** — Phase 3.

---

## 3. Architecture

Three new/changed units plus wiring. Each has one clear job and a tested boundary.

### 3.1 `ArrangerSectionSequencer` (new — the testable core)
Pure, no threading, no I/O — like `ArrangerScheduler`. It owns:
- the section list (each section: `SectionType`, `lengthBars`, and an
  `afterComplete` transport rule),
- the **active** section index,
- the variation to **return to** (for Fill / Intro fall-through),
- a single **pending** request (target section + the press that set it).

Its job is to answer one question, deterministically: given the monotonic beat
window `[fromBeats, toBeats)` and the time signature, **which section is active
across that window, and where do the boundaries fall?** It returns an ordered list
of segments:

```
struct SectionSegment {
    int    sectionIndex;     // which section plays in this sub-window
    double fromBeats;        // absolute beats (sub-window start)
    double toBeats;          // absolute beats (sub-window end)
    bool   startsAtBoundary; // true if a section change happened at fromBeats
};
```

Transitions occur **only at bar boundaries** (`beatsPerBar`). At a boundary the
sequencer:
1. applies a pending request if one exists (and clears it), else
2. checks whether the active section is a one-shot whose `lengthBars` has elapsed
   since it started, and if so applies its `afterComplete` rule
   (Intro/Fill → fall-through variation, Ending → a terminal "stop" sentinel).

The sequencer never emits MIDI. It only partitions time into section segments and
signals a terminal stop. This is what makes it unit-testable without audio.

### 3.2 `ArrangerEngine` changes
- Holds the whole multi-section `ArrangerStyle` and **one `ArrangerScheduler` per
  section** (each scheduler pre-loaded with that section's merged events + loop
  length). Schedulers are cheap and this keeps each section's seam state isolated.
- The timer tick computes the beat window `[from, to)` exactly as today, then asks
  the sequencer for the segments. For each segment it advances **that section's**
  scheduler over the sub-window and dispatches the events. When a segment starts at
  a boundary (a real section change), the *outgoing* section's scheduler is asked
  to flush seam note-offs for its still-sounding notes before the new section's
  scheduler runs — reusing the existing seam machinery so nothing hangs across the
  swap.
- On a terminal "stop" sentinel from the sequencer (Ending finished), the engine
  calls its existing `stop()` path (all-notes-off, reset).
- New public API: `queueSection(ArrangerSectionType type, const juce::String& name)`,
  `getActiveSectionIndex()`, and a `startAt(section)` used by Start-with-Intro.
- A section's scheduler phase is reset to bar 0 when it becomes active, so each
  section always enters on its own downbeat.

### 3.3 `ArrangerPatternBuilder` change
- New `buildDemoMultiSectionStyle(tracks, timeSigNum, timeSigDenom)` returning a
  style with four sections, all derived from the **same** single loop:

  | Section id | Name | `SectionType` | `lengthBars` | `afterComplete` |
  |---|---|---|---|---|
  | `intro_1`   | Intro 1     | Intro     | loop bars | → Variation 1 |
  | `var_1`     | Variation 1 | Variation | loop bars | (loops) |
  | `fill_1`    | Fill 1      | Fill      | 1 bar*    | → return to current variation |
  | `ending_1`  | Ending 1    | Ending    | loop bars | → stop |

  *Fill content = the loop's **last bar** (cheap, recognizable accent); if the loop
  is one bar, Fill = the whole loop. The transport rule is the real deliverable;
  the content slice is the only throwaway part and is replaced in Phase 3.
- The existing `buildSingleSectionStyle` is **kept** (used by its current tests and
  as the trivial fallback).
- To carry `afterComplete`, the **model** (`ArrangerModel.h`) gains an inert-until-now
  field on `ArrangerSection`: an `ArrangerAfterComplete { Loop, FallThrough, Stop }`
  enum (default `Loop`). Schema version bumps to 2.

### 3.4 Wiring (`CurrentStyleComponent`)
- `handleIntro/Var/Fill/Ending/BreakCurrentStyle` each gain an arranger-mode branch:
  when `arrangerModeEnabled`, call `arrangerEngine->queueSection(type, name)` (and,
  if stopped, select-for-start) instead of the classic `setLastSectionUsed`. Classic
  mode keeps its exact current behavior.
- `startPlaying()` already branches on `arrangerModeEnabled` (Phase 1). It switches
  from `buildSingleSectionStyle` to `buildDemoMultiSectionStyle`, and honors a
  "start with intro" selection if the user pressed Intro while stopped.

---

## 4. State machine behavior (normative)

- **States:** `Stopped`, `Playing(activeIndex)`, with a transient pending request.
- **While playing:** pressing a section button sets the pending request; it applies
  at the **next bar boundary** (Korg's default variation-change quantize). At the
  boundary: flush seam note-offs for the outgoing section, make the target active,
  resume from the target's bar 0.
- **One-shot sections** (`afterComplete != Loop`): when `lengthBars` have elapsed
  since the section became active, the sequencer applies the rule at the next bar
  boundary — Intro/Fill fall through to the saved variation; Ending stops.
- **Fill** remembers the variation that was playing when it was triggered and
  returns to it. **Intro** falls through to Variation 1 (or the selected variation).
- **While stopped:** pressing a section button selects what `Start` begins with
  (pressing **Intro** = start-with-intro). `Start` begins at the selected/active
  section; `Stop` is unchanged (all-notes-off, reset, beat bar to downbeat).
- **Break** is a normal looping (variation-class) section, `afterComplete = Loop`;
  it plays until you switch away. (In the demo style only Intro/Var/Fill/Ending are
  generated; Break is supported by the machine and exercised by unit tests.)
- **Double-press before a boundary:** the later press **replaces** the pending
  request (last-press-wins); no queue depth > 1.

---

## 5. How this sets up Phase 3

The sequencer and engine consume a generic `std::vector<ArrangerSection>` and only
read `type`, `lengthBars`, `afterComplete`, and per-track `pattern`. Phase 3
replaces `buildDemoMultiSectionStyle` with a real importer / authoring tool that
fills each section with genuinely different content. **No sequencer or engine change
is required** — only richer data. The Phase 2 unit tests (which build synthetic
multi-section styles by hand) already prove the machine against arbitrary content.

---

## 6. Testing

**Unit — `ArrangerSectionSequencer`:**
- queued switch applies exactly at the next bar boundary, never mid-bar;
- a window that spans several bars and transitions resolves into ordered segments;
- one-shot Intro auto-advances to the variation after exactly `lengthBars`;
- Fill returns to the variation that was active when it was triggered;
- Ending yields the terminal stop sentinel after its length;
- double-press before a boundary keeps only the last request;
- `Loop` sections never auto-transition.

**Unit — `ArrangerEngine`:**
- a section swap flushes seam note-offs for the outgoing section (no hung notes);
- `queueSection` is reflected by `getActiveSectionIndex()` only **after** the
  boundary, not immediately;
- loop length / active scheduler tracks the active section;
- Ending drives the engine to `stop()` (all-notes-off observed, `isPlaying` false).

**Unit — `ArrangerPatternBuilder`:**
- `buildDemoMultiSectionStyle` yields 4 sections with the correct types,
  `afterComplete` rules, Fill = last-bar slice, and the same channel mapping as
  Phase 1 (perc → 10, melodic → 2,3,4,…).

**Manual smoke (the real proof):**
- Arranger mode on, Start → Variation 1 loops. Press Fill → one fill at the next
  bar, then back to Variation 1. Press Intro-then-Start (from stopped) → intro once
  → Variation 1. Press Ending → ending once → silence. **Zero hung notes** at every
  seam. Classic mode unchanged.

---

## 7. Risks

- **Seam correctness across a section *swap*** (not just a loop wrap) — the primary
  risk, same family as Phase 1. Mitigated by reusing the scheduler's tested
  seam-flush and by the explicit engine test above.
- **Boundary math** (a render window that crosses a bar boundary *and* a section
  change at once) — mitigated by keeping all boundary logic in the pure sequencer
  with half-open intervals and the same epsilon discipline as `ArrangerScheduler`.
- **Schema bump (v1 → v2)** — `afterComplete` defaults to `Loop`, so any persisted
  v1 data and the existing single-section path remain valid.

---

## 8. Files

**New:**
- `Source/Arranger/ArrangerSectionSequencer.h` / `.cpp`
- `tests/unit/test_arranger_section_sequencer.cpp`

**Modified:**
- `Source/Arranger/ArrangerModel.h` — `ArrangerAfterComplete` enum + field; schema → 2.
- `Source/Arranger/ArrangerEngine.h` / `.cpp` — multi-section playback, `queueSection`.
- `Source/Arranger/ArrangerPatternBuilder.h` / `.cpp` — `buildDemoMultiSectionStyle`.
- `tests/unit/test_arranger_engine.cpp`, `tests/unit/test_arranger_pattern_builder.cpp`,
  `tests/unit/test_arranger_model.cpp` — extend.
- `Source/CurrentStyleComponent.cpp` — arranger-mode branch in the section handlers
  and `startPlaying()`.
- `Project Synth2.jucer` — register new files.

---

## 9. Out of scope (whole project — unchanged from parent spec)
- DAW-style audio-clip timeline, crossfades, time-stretch.
- Chord recognition, transposition (NTR/NTT), CV tables — Phases 4-6.
