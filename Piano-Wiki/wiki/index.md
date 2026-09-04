---
type: meta
title: "Master Index"
created: 2026-06-06
updated: 2026-09-04
tags:
  - meta
  - index
status: developing
---

# Piano-App Wiki — Master Index

The master catalog of every page in this wiki. Update on every ingest or new page.

> [!key-insight] Read order for cheap context
> Start with [[hot]] (~500 words, recent context). Then this index. Then the relevant `_index.md`. Only then drill into individual pages.

## Meta
- [[overview]] — executive summary of the whole project
- [[hot]] — hot cache, recent context
- [[log]] — chronological record of operations

## Modules — `wiki/modules/`
See [[modules/_index|Modules Index]]. One page per major subsystem.

- [[Arranger Engine]] — real-time multi-section playback + Phase 4 chord transposition — **deep-dive** ⭐
  - sub-pages: [[ArrangerSectionSequencer]] · [[ArrangerScheduler]] · [[ArrangerPatternBuilder]] · [[ArrangerModel]] · [[ArrangerTime]]
- [[Chord Recognition & Transposition]] — Phase 4: detect the held chord and re-pitch the accompaniment (NTT)
- [[MIDI Handling]] — MIDI I/O, devices, routing, record/playback
- [[Audio & SFZ Playback]] — 16-channel SFZ synths + per-channel DSP
- [[Track Playback]] — classic multi-track sequencer (`MultipleTrackPlayer`)
- [[Styles System]] — styles + section buttons (Intro/Variation/Fill/Ending)
- [[Arranger Style Authoring]] — author/store/browse/live-trigger `.style` configs (editor, timeline, Auto Fill)
- [[Keyboard & Main UI]] — `MainComponent` wiring hub + on-screen keyboard
- [[Supabase Backend]] — cloud auth, device registration, playtime

## Components — `wiki/components/`
See [[components/_index|Components Index]]. Reusable UI / functional components.

## Decisions — `wiki/decisions/`
See [[decisions/_index|Decisions Index]]. Architecture Decision Records (ADRs).
- [[ADR-001 Pure Schedulers and Parallel Engine]]
- [[ADR-002 Defer MainComponent Display Refactor]] — MainComponent/Display flagged as large integration files; refactor deferred, DI preferred over event bus if it happens

## Dependencies — `wiki/dependencies/`
See [[dependencies/_index|Dependencies Index]]. External libraries, versions, risk.

## Flows — `wiki/flows/`
See [[flows/_index|Flows Index]]. Data flows, signal paths, request paths.
- [[Section Switch Flow]]

## Concepts — `wiki/concepts/`
See [[concepts/_index|Concepts Index]]. DSP / synthesis / MIDI domain knowledge.
- [[Monotonic Beat Playhead]] · [[Bar-Synced Section Switching]] · [[Loop-Seam Note-Off]] · [[Arranger Phase Roadmap]] · [[GL Overlay Rendering]] · [[Chord-Aware NTT Transposition]]
