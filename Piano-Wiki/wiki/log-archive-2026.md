---
type: meta
title: "Operations Log Archive 2026"
created: 2026-11-26
updated: 2026-11-26
tags:
  - meta
  - log
status: evergreen
---

# Operations Log Archive (2026)

The oldest entries of [[log]], moved here unchanged when it neared the 800-line cap. Newest at the top.

## 2026-06-08 — Arranger UX batch: start-section pick, async I/O, Update Tracks, live volume (working tree)
Still `arranger-engine-phase1`, **uncommitted** (HEAD `1957c0e`). Six behavioural changes this session; updated [[Arranger Style Authoring]] and [[Arranger Engine]].  *(snapshot truncated here — the rest of this entry was not recoverable)*

## 2026-06-07 — Backlog noted: split tests into a separate project (deferred)
User wants to move the test suite out of the app build **at some point, not now**. Decision captured in [[hot]] under "Deferred / Backlog": Core static lib + App + Tests console project, so the app build stops compiling the 49 test `.cpp`s. Also corrected the stale note that the test runner is "commented out" — it's actually flag-gated via `--run-tests`. No code changed.

## 2026-06-07 (later) — Live section sync, section-relative playhead, engine tests, editor/live separation (working tree)
Still on `arranger-engine-phase1`, **uncommitted**. HEAD unchanged (`3cc1610`). This pass documents the
session's behavioural refinements; updated [[Arranger Engine]], [[Arranger Style Authoring]], [[ArrangerSectionSequencer]].
- **Click-to-jump fixed.** Bare timeline click failed to jump because sub-bar jitter → `mouseDrag` → `onWindowsChanged` → `rebuildPreview`/`setStyle` → `sequencer.reset()` wiped the queued `pendingIndex`. Now a gesture only counts as an edit when a bar value actually changes (`windowsChangedByDrag`). Root-caused via DBG instrumentation + a `getStackBacktrace()` probe; all `[sec]` tracing since removed.
- **Section-relative playhead arrow** in the editor (reads `getActiveSectionIndex()` + new `getActiveSectionLocalBeats()`/`getActiveStartAbs()`), wraps within a looping section's window; auto-follow tracks it.
- **Engine-driven live button highlight** (arranger mode): clicks only route to the engine; `onActiveSectionChanged` lights the actual sounding section (auto-fill: Var2→Fill lights→Var2). New `StyleSectionComponent::setEngineDrivenHighlight` + `setHighlightedButton`; chain `ArrangerEngine→CurrentStyleComponent→Display→MainComponent::highlightArrangerSection`.
- **Beat bar red→yellow on stop**: new `onStoppedItself` (fired from `haltAudio` only when it was playing) drops the UI playing flag; after an Ending, Var 1 re-arms.
- **Auto Fill toggle persists app-wide** (`ArrangerAutoFillEnabled` in app properties).
- **Editor/live are separate modes**: `presentOverlay` stops live playback before showing browser/editor (no inherited stale section / no Ending firing on close).
- **Engine unit tests fixed**: the queued-switch drain moved from `hiResTimerCallback` into `renderRange`, so direct `renderRange` calls (tests) apply queued switches. Documented the `requestLock` thread model (UI→timer mailbox; only the timer thread mutates the sequencer; `callAsync` back to UI).

## 2026-06-07 — Arranger style authoring + live sections + overlay fixes (working tree)
Synced commits since `c1ac203` up to HEAD `d56b503` (source folder reorg, gitignore/test toggle,
Phase 3 Part A codecs/model/IO/channels/builder, and the `d56b503` editor/timeline baseline),
plus this session's **uncommitted working-tree** refinements:
- New page [[Arranger Style Authoring]] — editor, timeline, browser, per-style config, storage,
  section semantics, Auto Fill wiring. New concept [[GL Overlay Rendering]].
- Updated [[ArrangerSectionSequencer]] — `findSection` matches by type + trailing number
  (`Var 2`→`Variation 2`); Fill/Break save `returnIndex`; **Auto Fill** (`fillReturnOverride`).
- Session changes (uncommitted, branch `arranger-engine-phase1`): active config shown only in the
  browser (✓) not the dropdown + per-style stale-state fix + persists to `allStyles.json`;
  `.style` files moved to `Piano Synth2/ArrangerStyles/`; rename-on-save + Delete button + hover
  tooltips + unique per-type section names + Add Var/Break buttons + click-to-jump preview;
  Load no longer auto-plays; loading label hides the GL note layer; ESC menu is now an in-app
  child (hides note layer + keyboard, clears notes on close); Korg semantics confirmed
  (Intro one-shot → Variation loops; Break returns to current variation).

## 2026-06-06 — Source/ reorganized into subsystem folders
- Moved 96 files from flat `Source/` into `Midi/ Audio/ Playback/ Styles/ Backend/ UI/ Common/`
  (mirrors the wiki modules); `Arranger/` already existed; `Main.cpp` stays at root.
- `.jucer` `file=` paths rewritten + `headerPath` extended per folder (bare includes still resolve).
- On branch `refactor/source-folder-structure`; pending Projucer re-save + build verify before commit.
- Updated module pages' `path:`, [[modules/_index]], and [[overview]] folder map.

## 2026-06-06 — Arranger Engine deep dive
- Read all 5 arranger `.cpp` implementations + headers.
- Split the arranger into focused pages: [[Arranger Engine]] (timer loop + hub),
  [[ArrangerScheduler]], [[ArrangerSectionSequencer]], [[ArrangerPatternBuilder]],
  [[ArrangerModel]], [[ArrangerTime]].
- Added flow [[Section Switch Flow]], decision [[ADR-001 Pure Schedulers and Parallel Engine]],
  concepts [[Monotonic Beat Playhead]], [[Bar-Synced Section Switching]],
  [[Loop-Seam Note-Off]], [[Arranger Phase Roadmap]].
- Updated all folder indexes + [[index]].

## 2026-06-06 — Codebase ingested into module pages
- Read the key headers across `Source/` and wrote 7 module pages:
  [[Arranger Engine]], [[MIDI Handling]], [[Audio & SFZ Playback]], [[Track Playback]],
  [[Styles System]], [[Keyboard & Main UI]], [[Supabase Backend]].
- Updated [[modules/_index]] and [[index]] with the real subsystem map + dependency sketch.
- Source of truth: header-level read (interfaces + doc comments), not full `.cpp` bodies.

## 2026-06-06 — Vault scaffolded
- Created Mode B (Repository) wiki structure for the Piano-app JUCE synth.
- Folders: modules, components, decisions, dependencies, flows, concepts, meta.
- Seeded index, overview, hot cache, templates, CSS theming.
- Vault is local-only (gitignored from the Piano-app repo, no remote).
- Branch at scaffold time: `arranger-engine-phase1`.
