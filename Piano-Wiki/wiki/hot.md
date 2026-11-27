---
type: meta
title: "Hot Cache"
updated: 2026-11-27T18:00:00
tags:
  - meta
  - hot
status: evergreen
---

# Recent Context

## Last Updated
2026-11-27. Merged Recorder (#75); **Sections v2** on `feat/ui-sections-lanes`: list + editor as the Style page's
Sections view (lanes with notes by pitch, inspector, jump pads). **Owner: pause after this merge.** Next pages
then: keys, Dust, finishes, sign-in. Owner sometimes uses this machine's screen: screenshot first. Injected keys
don't play notes. `docs/superpowers/` is **local only**. Detail in [[log]].

## Working rules (from `AGENTS.md`)
Spec → failing test → code. Root cause and a measurement before any fix; only proven problems;
minimal diff. **No AI attribution** in commits or PRs; no secrets, local files stay git-ignored, never read the
owner's env. UI redesign: commit / push / PR / merge without asking (owner, 2026-11-24); other work asks first.
Before finishing: Sonnet reviewer, then verify each of its claims yourself.

## Key Facts
- JUCE 8.0.13 **sample-based** arranger keyboard (SFZ, no oscillators). [[Keyboard & Main UI]]
  (`MainComponent`) wires everything; [[Arranger Engine]] and [[Track Playback]] run on
  `HighResolutionTimer` threads, not the message thread.
- **No file over 800 lines** (CI). Big classes span several `.cpp`s (`MainComponentStage.cpp`, …).
  [[ADR-003 800-Line File Cap Split By Responsibility]].
- **`NoteLayer` draws on the GL render thread without the message lock**; motion in pure
  `NoteField`. Hiding it detaches the GL context. [[GL Overlay Rendering]].
- Tests live in `ci/Tests.jucer`; exit code = failures, also check the `TOTAL` line.
  Current: **unit 5235, arranger 770, integration 130 (+7 skipped), display 12** (`sparksAreVisible` flaky).
- Visual checks: screenshot/click the app's window by process id (a VS panel can share its title); boards: `specs/assets/…/render/render.sh`.

## Build / Test
- App `Builds\VisualStudio2022\Arranger Workstation.sln`, Debug x64; close the app first (PDB lock).
- Projucer detaches: `Start-Process -Wait`; quote `"Arranger Workstation.jucer"` (space in name).
- Header layout change → `/t:Rebuild`. New `Source/` file → add to **both** `.jucer`s.
- Git Bash `sed -i` drops CRLF in `.jucer` files — edit them with Python.
- Precompiled header + root `Directory.Build.*`: one-file rebuild ~2.6 s, clean ~30 s; editing
  `Source/PrecompiledHeader.h` needs a Projucer re-save. [[Dependencies Index]]
- CI caches compiled JUCE objects (filled by runs on `main`; key hashes `ci/JuceLibraryCode`).

## Active Threads / Next
Full task list with states: [[Owner Backlog]].
- 🟡 **~100 `juce::AlertWindow`s left** (0.5–1 s each in Debug, balanced text layout): move to `StageDialog`.
- 🟡 **UI redesign** (spec `docs/superpowers/specs/2026-11-23-ui-redesign-design.md` + part 2): panel and
  pages Style, Effects, Settings, Voices, Chord helper, Esc menu, Recorder, Sections done. Owner: same look on every resolution; pages centred like the
  panel; agent checks the running app **1:1** against the boards before finishing.
- 🟡 **Delay read offset still jumps** (CC94 → `delayReadOffset` moved instantly; needs fractional
  read). Replacement `DelayLine` exists in the sibling `va-effects` repo.
- 🟡 **Swap `juce::dsp::Chorus` (CC93) for the BBD model** from `va-effects`; budget = 16 `ChannelDSP`s.
- 🟡 **Missing-SFZ popup (`b5765e9`) never smoke-tested**; **Phase 7a manual sign-off 0/9**.
- 🟡 **Track library bug**: tracks from a folder's 2nd+ MIDI file reload as the first file's (see [[Owner Backlog]]).
- 🟢 OpenGL crash parked (unreproducible); tripwire logs to `Desktop/piano_gl_errors.log`.
