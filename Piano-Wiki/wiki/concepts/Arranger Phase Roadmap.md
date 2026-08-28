---
type: concept
title: "Arranger Phase Roadmap"
complexity: basic
domain: arranger
aliases:
  - "arranger phases"
status: developing
created: 2026-06-06
updated: 2026-08-15
tags:
  - concept
  - arranger
  - roadmap
related:
  - "[[Arranger Engine]]"
  - "[[ArrangerModel]]"
  - "[[Chord Recognition & Transposition]]"
  - "[[Chord-Aware NTT Transposition]]"
---

# Arranger Phase Roadmap

The arranger is built in phases. The data model ([[ArrangerModel]], schema v2) already reserves fields for later phases, so they're inert today rather than absent.

> [!warning] "done (uncommitted)" below is historical wording
> Phases 5, 6, 6b, chord-zone mute and the editor/preview hardening were **committed** on 2026-06-29 (squashed into `3ffd434`) and **merged to `main`** via PR #8 (`6f2ba81`). Only Phase 7a remains unmerged.

| Phase | Scope | Status (2026-08-15) |
|-------|-------|---------------------|
| **1** | Beat-based single-section playback; pure scheduler + engine loop; MIDI-out + SFZ inject | ✅ done |
| **2** | Multi-section transport with **real rules** (Intro-once, Fill-once, Ending-once-then-stop) via bar-synced switching; demo Intro/Fill/Ending sliced from the one loop | ✅ done — merged to `main` |
| **3** | **Distinct per-section audio / self-contained styles** — author `.style` files (schema v4) where each section is a bar-window over one recording; visual timeline editor; load/play distinct sections | ✅ done — baseline `d56b503`. See [[Arranger Style Authoring]] |
| **4** | **Chord/key transposition** — `PartKind` (Fixed/Acc/Bass) + style-level original chord drive emit-time transposition of the accompaniment to the held chord; split-zone/full-keyboard chord input; bass inversion; chord memory | ✅ done — branch `arranger-engine-phase4-chords`. See [[Chord Recognition & Transposition]] · [[Chord-Aware NTT Transposition]] |
| **5** | **Chord recognition modes** — `ChordMode` (Fingered / Single-Finger / Full-Keyboard); Korg one-finger recognition; Full-Keyboard re-tuned for bass-driven stability (no debounce); 3-way Settings selector | ✅ done (uncommitted). Spec `2026-06-25-arranger-phase5-chord-recognition-modes-design.md`. See [[Chord Recognition & Transposition]] |
| **6** | **Synchro Start** — when enabled, Start arms the engine and the groove begins on the first recognised chord (a chord held at Start fires it at once, via the Phase-5 seed). Settings toggle. | ✅ done (uncommitted). Spec `2026-06-25-arranger-phase6-synchro-start-design.md` |
| **6b** | **Count-In** — one bar of metronome clicks (GM side-stick, accent on beat 1) before the groove; engine pre-roll after the Synchro gate; Settings toggle | ✅ done (uncommitted) |
| **—** | **Chord-zone mute** (pulled forward from backlog) — when on AND the arranger is engaged, the chord-recognition keys drive the accompaniment WITHOUT sounding their raw notes (Korg-style); only note-onsets gated (offs always pass → no stuck notes); Settings toggle `"ChordZoneMute"` | ✅ done (uncommitted) |
| **—** | **Editor/preview hardening** — preview always starts (bypasses Synchro/Count-In on the shared engine); idle playhead parks at the first section; mid-play style swap is now thread-safe + drone-free + clean-phase | ✅ done (uncommitted) |
| **7a** | **Real NTT engine** — per-track `NttType` (No-Transpose / Parallel / Chord / Fixed); role-remap of *every* chord tone + scale-snap for passing tones; curated minor-scale selector (Dorian default); data-driven scale table; master "Chord Transposition On/Off" bypass; per-track selector + Settings controls; schema bump + migration | ✅ **implemented & committed** — `614b4ec` on branch `arranger-phase7a-ntt-engine` (30 files; new `Source/Arranger/NttScales.h` + `ArrangerEnums.h`, schema v5 + migration). Automated suites green. ⚠️ **NOT merged to `main`**, and the **manual smoke test is unsigned (0/9)** — `docs/superpowers/test-plans/2026-06-26-arranger-phase7a-ntt-testing.md`. Spec `docs/superpowers/specs/2026-06-26-arranger-phase7a-ntt-engine-design.md` |
| **7b** | **Chord Variation tables** (Korg Layer 1) — record up to 6 voicings per section + chord→CV map + editor UI | ⬜ planned (separate spec, builds on 7a) |
| **8** | **Yamaha importer** (`.sty`/`.prs`) — SMF + CASM chunk → `.style`, sets `NttType` per channel (consumes 7a) | ⬜ planned |
| **9** | **Korg importer** (`.sty`/`.set`) — proprietary binary container | ⬜ planned |

## Deferred / not-yet-implemented backlog (updated 2026-08-15)
- **NTT depth → Phase 7a, now implemented** (`614b4ec`, unmerged). The real engine (multiple table types beyond root-shift+third-flip, scale-aware snapping) shipped. **User-editable NTT/scale tables for every quality** remain deferred (7a ships only a *curated* minor-scale selector).
- **7b / 8 / 9 have no specs yet** — they are named on this roadmap but nothing is written under `docs/superpowers/specs/`.
- **Chord-variation patterns per chord type → Phase 7b** (Korg "Chord Variation" Layer 1).
- **Yamaha/Korg binary importer → Phase 8 (Yamaha) / Phase 9 (Korg).** User wants these as the next phase *after* the NTT engine they feed.
- **Single-Finger slash/bass voicings** — still deferred.
- **Separate preview engine** (audition in the editor without stopping live playback) — still deferred.
- **Chord-zone mute** — ✅ DONE (pulled forward this session; see table above).
- **Permanently out of scope:** DAW-style audio-clip timeline, crossfades, time-stretch, audio-file playback.

## What "demo" means in Phase 2
`buildDemoMultiSectionStyle` ([[ArrangerPatternBuilder]]) derives the four sections by slicing whole bars out of the single Variation loop and rebasing to beat 0 — Intro = first bar, Fill = last bar, Ending = last few bars. They're deliberately short so the transport behaviour is **audible**, standing in until Phase 3 supplies genuine per-section content. (Commit `c1ac203` shortened them for exactly this reason.)

## Related
[[Arranger Engine]] (runtime), [[ArrangerModel]] (the reserved fields), [[ADR-001 Pure Schedulers and Parallel Engine]] (why this can grow safely).
