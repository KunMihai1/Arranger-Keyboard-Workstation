---
type: meta
title: "Owner Backlog"
created: 2026-11-22
updated: 2026-11-22
tags:
  - meta
  - backlog
status: evergreen
---

# Owner Backlog

Every task the owner has given, in the owner's priority order, with its state. **Read this before
choosing the next piece of work** and update it when a task moves. Status checked in the code on
2026-11-22; anything not checked says so. Rules for *how* to work are in `AGENTS.md`, not here.

Legend: ✅ done · 🟡 in progress · ⬜ not started · ❓ needs the owner's decision · 🔒 needs the owner's unlocked screen

## 1. Standing work (do these first)
- 🟡 **Names, not values in every file** (owner's rule, `docs/names-not-values.md`). One subsystem per PR,
  behaviour-identical. Done: Common, Audio, Midi, Arranger (all of `Source/Arranger`). Left (102 files on
  `ci/named-values-allowlist.txt`): rest of Playback → Styles → Backend → UI → tests.
- 🟡 **Faster builds**: a research agent measures and implements build-time improvements on
  `perf/build-time`; the orchestrator then reviews, opens the PR and syncs the wiki.
- ✅ **800-line cap** for source, test and wiki files (CI enforces).
- ✅ **800-line cap for `docs/` too** (owner, 2026-11-22). Seven documents (971–1425 lines) split at their
  headings into two parts each, content unchanged; CI checks `docs/**/*.md`.
- ✅ **Auto-fill plays the fill of the variation you leave** (owner, 2026-11-22: Var 2 → Var 1 plays
  Fill 2). `ArrangerSectionSequencer::queue`; spec `2026-11-22-autofill-outgoing-variation-design.md`.
- ⬜ **CI once per PR** (owner approved 2026-11-22): `push` runs only on `main`; PRs keep their
  `pull_request` run. Do it after the build-time agent's `ci.yml` changes land (same file).
- ✅ **Arranger hard testing** (headless + live, `docs/superpowers/test-plans/2026-11-22-arranger-stress-test-plan.md`).
  Live on 2026-11-22 too: Synchro Start, Count-In and the style editor while playing all work. Not run:
  SFZ instrument / audio output switch while playing (no SFZ library on this machine).
- ✅ **Style tab vanished after Home** (found live 2026-11-22): Settings tab → Home → Play → Styles removed
  the style tab. Temporary tabs now close by name (`TemporaryTabs`); spec
  `2026-11-22-temporary-tabs-after-home-design.md`.

## 1b. UI refinement (owner, 2026-11-22) — after section 1, before the TODO file; owner reviews it first
- **Most important (owner): it must NOT look AI-generated** — a real-music-gear, human-designed look (think
  hardware keyboards, studio gear), not generic gradients/glows/rounded cards.
- ⬜ A refined, quality feel everywhere: the piano keys as realistic as possible,
  real-looking knobs on the Effects page, buttons and the rest of the UI to match; anything that adds
  realism is welcome.
- ⬜ **Piano skins**: decorative designs on the keyboard body (the owner's example was an art-wrapped
  workstation). Original artwork only, no brand names or copied designs.
- Show the owner the direction (mock-ups / screenshots) before building it.

## 2. Open engineering threads (from the wiki)
- ⬜ Delay read offset still jumps on CC94 changes (needs a fractional read; `DelayLine` in `va-effects`).
- ⬜ Swap `juce::dsp::Chorus` (CC93) for the BBD model from `va-effects`.
- ⬜ 🔒 Missing-SFZ popup never smoke-tested; Phase 7a manual sign-off 0/9.
- ⬜ Prune merged branches (`fix/sfz-integration`, `fix/dsp-quality`, `docs/wiki-sync-main`,
  `arhitecture/add-arhitecture-optimize-big-files`).
- ❓ Instrument presets (reverb/chorus/cutoff… per instrument) are never sent — only the program change.
  Owner unsure (2026-11-22); left as is until they decide.
- ⬜ `AudioHandler` allocates a `juce::MidiBuffer` on every audio callback (audio thread must not allocate).

## 3. Owner's TODO file (last priority, owner's words summarised)
| Item | State |
|---|---|
| BPM applied when a style / track loads | ✅ |
| Change BPM while playing | ⬜ locked while the arranger plays (`CurrentStyleComponent.cpp:189`) |
| Real arranger with sections, loop based | ✅ |
| Track playtime, send it for a website TOP list | ✅ sent to the backend; website side unknown |
| Login / logout inside the app, persistent login (JSON in AppData) | ⬜ no logout or saved session in the code |
| Chord recognition, accompaniment follows the key | ✅ |
| Right-click glitch on a style while playing | ⬜ needs a reproduction. Lead: `MyTabbedComponent::mouseDown` "Close Tab" removes a tab without telling `Display` (same state problem as the Home bug) and picks the tab from `x` although the tabs are vertical |
| JSON of last / most used styles | ⬜ |
| Richer save dialog (track name, more fields) | ⬜ |
| Detached threads → thread pool | ⬜ 9 sites: 4 `.detach()` (Login ×2, MainComponent, MainComponentActions), 5 `Thread::launch` (AudioHandler::loadSfz, ArrangerStyleEditor ×2, CurrentStyleComponent ×2) |
| Equalizer | ⬜ |
| SFZ with the playback system per channel | ✅ |
| Check MIDI in/out and audio out changes while playing | ⬜ not verified live (no SFZ library / second device here) |
| SFZ glitch when switching instrument mid-play (load time) | ⬜ not investigated (no SFZ library here) |
| Arranger "sounds different" with SFZ / does it use SFZ | 🟡 doubled output ruled out; found and fixed the SFZ queue backlog (PR #33). Sound on a real SFZ library not checked |
| Arranger beat bar on the beat | ✅ |
| "Notes information" for arranger notes (timestamp based) | ✅ kept as is (explained to the owner: the right-click table of a track's notes) |
| Var 2 → Var 1 plays Fill 1 | ✅ now plays Fill 2, the fill of the variation you leave |
| Arranger starts again after the end | ✅ (owner: "good thing") |

## Owner decisions already taken (don't reopen)
- A style switch while playing takes effect on the next Start; any section may be the start section.
- Restore-down window stays work-area sized; no scaled smaller window ("over engineering").
