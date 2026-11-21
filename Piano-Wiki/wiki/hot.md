---
type: meta
title: "Hot Cache"
updated: 2026-11-21T00:00:00
tags:
  - meta
  - hot
status: evergreen
---

# Recent Context

## Last Updated
2026-11-21. Three PRs landed in order: **docs** (`AGENTS.md`/`CLAUDE.md` rules, `docs/` now
tracked, `docs/ARCHITECTURE.md`), **perf** (GPU note rendering), **refactor** (800-line file cap).
Detail in [[log]].

## Working rules (from `AGENTS.md`)
Spec → failing test → code. Root cause and a measurement before any fix; only proven problems;
minimal diff. Never commit/push without approval; **no AI attribution** in commits or PRs.
Before finishing: Sonnet reviewer, then verify each of its claims yourself.

## Key Facts
- JUCE 8.0.13 **sample-based** arranger keyboard (SFZ, no oscillators). [[Keyboard & Main UI]]
  (`MainComponent`) wires everything; [[Arranger Engine]] and [[Track Playback]] run on
  `HighResolutionTimer` threads, not the message thread.
- **No file over 800 lines** — CI's first step enforces it. Big classes span several `.cpp`s
  (`MainComponentInit/Actions/Effects/Overlays/Playback.cpp`, `DisplayStyleStore.cpp`,
  `MidiDevice.cpp`, …). [[ADR-003 800-Line File Cap Split By Responsibility]].
- **`NoteLayer` draws on the GL render thread without the message lock**; motion in pure
  `NoteField`. Hiding it detaches the GL context. [[GL Overlay Rendering]].
- Tests live in `ci/Tests.jucer`; exit code = failures, also check the `TOTAL` line.
  Current: **unit 748, arranger 705, integration 137** (integration is local-only).

## Build / Test
- App `Builds\VisualStudio2022\Arranger Workstation.sln`, Debug x64; close the app first (PDB lock).
- Projucer detaches: `Start-Process -Wait`; quote `"Arranger Workstation.jucer"` (space in name).
- Header layout change → `/t:Rebuild`. New `Source/` file → add to **both** `.jucer`s.
- Git Bash `sed -i` drops CRLF in `.jucer` files — edit them with Python.

## Active Threads / Next
- 🟡 **Manual check of the new note rendering** in the real app (login needed):
  `docs/superpowers/test-plans/2026-11-21-note-layer-gpu-testing.md`.
- 🟡 **Delay read offset still jumps** (CC94 → `delayReadOffset` moved instantly; needs fractional
  read). Replacement `DelayLine` exists in the sibling `va-effects` repo.
- 🟡 **Swap `juce::dsp::Chorus` (CC93) for the BBD model** from `va-effects`; budget = 16 `ChannelDSP`s.
- 🟡 **Missing-SFZ popup (`b5765e9`) never smoke-tested**; **Phase 7a manual sign-off 0/9**.
- 🟡 **Prune merged branches**: `fix/sfz-integration`, `fix/dsp-quality`, `docs/wiki-sync-main`,
  `arhitecture/add-arhitecture-optimize-big-files` (all contained in `main`).
- 🟢 OpenGL crash parked (unreproducible); tripwire logs to `Desktop/piano_gl_errors.log`.
- 📝 `log.md` has a permanent gap 2026-06-25 → 2026-08-15 (lost to a truncating write).
