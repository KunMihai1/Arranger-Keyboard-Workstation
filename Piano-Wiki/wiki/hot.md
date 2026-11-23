---
type: meta
title: "Hot Cache"
updated: 2026-11-23T19:00:00
tags:
  - meta
  - hot
status: evergreen
---

# Recent Context

## Last Updated
2026-11-23. PRs #19–#58: bug fixes, arranger stress tests, faster builds; names cleanup finished.
**UI redesign phase 1 (Stage panel) in progress on `feat/ui-stage-panel`:** Tasks 1-3 (fonts, palette/scaling, models) done; panel
re-laid out by the owner (transport on the right, 2 quick knobs). Then the TODO file. Detail in [[log]].

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
  Current: **unit 1171, arranger 770, integration 130 (+7 skipped)**; integration runs in CI too.
## Build / Test
- App `Builds\VisualStudio2022\Arranger Workstation.sln`, Debug x64; close the app first (PDB lock).
- Projucer detaches: `Start-Process -Wait`; quote `"Arranger Workstation.jucer"` (space in name).
- Header layout change → `/t:Rebuild`. New `Source/` file → add to **both** `.jucer`s.
- Git Bash `sed -i` drops CRLF in `.jucer` files — edit them with Python.
- **Precompiled header**, only the JUCE modules in use, 64-bit tools and one JUCE `/MP` batch (root
  `Directory.Build.*`): one-file rebuild ~2.6 s, clean ~30 s. Editing `Source/PrecompiledHeader.h` needs a Projucer re-save. [[Dependencies Index]]
- CI caches compiled JUCE objects (filled by runs on `main`; key hashes `ci/JuceLibraryCode`).

## Active Threads / Next
Full task list with states: [[Owner Backlog]].
- 🔴 **Use-after-free on deleting the open style** — `callAsync([this]…)` stop callbacks outlive the
  engine/player. Reproduced live 2026-11-23 (Debug crash) and in the no-flag test run. Fix (liveness
  token) awaits owner sign-off: `docs/superpowers/specs/2026-11-22-stop-callback-use-after-free-design.md`.
- 🟡 **UI redesign** ("Stage" panel, pages, real keys/knobs, new Dust, six finishes, Remember me):
  spec `docs/superpowers/specs/2026-11-23-ui-redesign-design.md` + part 2, plan
  `docs/superpowers/plans/2026-11-23-ui-redesign-phase1-panel.md` (Tasks 1-3 done, next Task 4 look-and-feel + widgets). Owner: same look on every resolution (uniform scale, board centred). Owner's conditions:
  behaviour integrated first; agent checks the running app **1:1** against the boards before finishing.
- ✅ **Arranger hard testing** done. Owner: style switch while playing applies on the next Start; any
  section may start; restore-down stays work-area sized.
- 🟡 **Delay read offset still jumps** (CC94 → `delayReadOffset` moved instantly; needs fractional
  read). Replacement `DelayLine` exists in the sibling `va-effects` repo.
- 🟡 **Swap `juce::dsp::Chorus` (CC93) for the BBD model** from `va-effects`; budget = 16 `ChannelDSP`s.
- 🟡 **Missing-SFZ popup (`b5765e9`) never smoke-tested**; **Phase 7a manual sign-off 0/9**.
- 🟢 OpenGL crash parked (unreproducible); tripwire logs to `Desktop/piano_gl_errors.log`.
