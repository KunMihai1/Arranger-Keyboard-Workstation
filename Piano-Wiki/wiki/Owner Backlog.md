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
- ✅ **Names, not values in every file** (owner's rule, `docs/names-not-values.md`). One subsystem per PR,
  behaviour-identical. **Done 2026-11-22:** all of `Source/` (PRs #44–#51) and all 41 test files (#52–#56);
  `ci/named-values-allowlist.txt` is empty, so CI now rejects any new bare value anywhere. The last
  `"DEFAULT Style"` copy (`SFZLibraryUI`) uses `StyleNames::defaultStyle`.
- ✅ **Faster builds** (research agent, 2026-11-22): 64-bit compiler/linker + one batch for the JUCE
  module files. Local: clean build 44 → 30 s, header edit 7.1 → 5.5 s. On GitHub's runners the warm
  build step barely moved (41–47 s → 40–41 s, within noise).
  Spec `2026-11-22-build-time-design.md`.
- ✅ **800-line cap** for source, test and wiki files (CI enforces).
- ✅ **800-line cap for `docs/` too** (owner, 2026-11-22). Seven documents (971–1425 lines) split at their
  headings into two parts each, content unchanged; CI checks `docs/**/*.md`.
- ✅ **Auto-fill plays the fill of the variation you leave** (owner, 2026-11-22: Var 2 → Var 1 plays
  Fill 2). `ArrangerSectionSequencer::queue`; spec `2026-11-22-autofill-outgoing-variation-design.md`.
- ✅ **CI once per PR** (owner approved 2026-11-22): `push` runs only on `main`; PRs keep their
  `pull_request` run, so each PR commit is built once instead of twice.
- ✅ **Arranger hard testing** (headless + live, `docs/superpowers/test-plans/2026-11-22-arranger-stress-test-plan.md`).
  Live on 2026-11-22 too: Synchro Start, Count-In and the style editor while playing all work. Not run:
  SFZ instrument / audio output switch while playing (no SFZ library on this machine).
- ✅ **Style tab vanished after Home** (found live 2026-11-22): Settings tab → Home → Play → Styles removed
  the style tab. Temporary tabs now close by name (`TemporaryTabs`); spec
  `2026-11-22-temporary-tabs-after-home-design.md`.

## 1b. UI refinement (owner, 2026-11-22) — after section 1, before the TODO file; owner reviews it first
- **Most important (owner): it must NOT look AI-generated** — a real-music-gear, human-designed look (think
  hardware keyboards, studio gear), not generic gradients/glows/rounded cards.
- ✅ **Phase 1, the Stage panel — merged 2026-11-23 (PR #61, `908891f`):** spec + plan approved
  (`docs/superpowers/plans/2026-11-23-ui-redesign-phase1-panel.md`). Owner changed the panel layout
  (labels ran into each other, buttons too tight): Rec/File/Sync Start/Stop/Start moved to the right third,
  Quick FX down to Volume + Reverb, Auto Fill stays with the sections. Task 1 (embedded fonts, `c38fb13`)
  Task 2 (palette, metrics, scaling, `6291080`) and Task 3 (models, `017837b`) , Task 4 (rubber
  buttons + knobs, `6cb3f71`), Task 5 (screen, `3e2d85d`) and Task 6 (panel, `11d1b82`) done; Task 7 (wire the
  panel into the app, `d425e08`) live-checked 2026-11-23; Task 8 (reference PNGs rendered locally, owner's
  choice, `7a9d799`), the 1:1 check write-up (`37d0ce9`) and the Sonnet review (fixes `92a5803`) done; merged.
  Owner to try: playback with a style that has tracks, chords from a hardware keyboard (manual check steps 6, 10).
  Next phases (keys, each page, Dust, finishes, sign-in) need their own plans. Owner reports fixed: knobs turn like real knobs, knob lag (save on
  release), hand cursor on every button, LED glow cut on the left; key tests failing on the Romanian layout.
- ✅ **Phase 4a, the Style page + track picker + notes page** (2026-11-24, `feat/ui-style-page`): boards
  `StageStyles`, `PageTracks`, `PageNotes`. Owner answers: picking an instrument keeps the track name; the
  open-style delete crash fixed first; notes page without "Edit selected note" / Undo (a hint line). Agent calls:
  instrument caption Drums / As recorded / —, percussion message on the screen, "Open a style on the left", Exit on
  every page, the files-are-safe note kept. Chord indent fixed in the same batch (owner).
- ✅ **Phase 4b, the Effects page** (2026-11-24, `feat/ui-effects-page`): board `StageEffects`; replaces the Sound
  Effects window (agent calls: the window goes; Esc menu "Effects" opens the page; Effects and MIDI Settings replace
  each other as before).
- ⬜ Next pages (spec order): Settings, Voices, Chords, Recorder, Esc menu, Sections v2; then keys, Dust, finishes,
  sign-in.
- ✅ **Same look on every resolution** (owner, 2026-11-23: "the UI should look the same on any resolution
  … everything must be visible on all screens"): one uniform scale, panel controls = the board scaled and
  centred (`StageRegions::panelContent`, `33a370c`); tested 21:9, 4:3, portrait, 150 % scaling, tiny.
  Every later phase (keys, pages) must keep this rule.
- ⬜ **Idea — assignable triggers for every panel action** (owner, 2026-11-23): Fill, Var, Intro, Ending,
  Break, Start/Stop and "everything" can also be fired by a **piano key** (MIDI note from the keyboard)
  or a **PC-keyboard key**, chosen in a **keybind menu**; both inputs possible. Not designed yet: needs its
  own spec after phase 1. Open points to settle then: the PC keyboard already plays notes
  (`Source/Midi/keyListener.h`, `KeyboardListener`), so bound keys must not also sound; a piano key used as
  a trigger likewise (e.g. a learn mode, or keys outside the playing range); where the menu lives
  (Settings page); how bindings persist (settings keys).
- Design (2026-11-23): owner picked direction A "Stage" (matte charcoal
  hardware panel), revised: slim panel, bigger centred screen, beat lamps in the app's colours, every
  feature on the panel or a page, realistic keys, real knobs/faders, new smooth Dust, six instrument
  finishes (Korg-edition quality, original), Sections editor v2, tempo hold/type/wheel/reset, Remember me.
  Spec: `docs/superpowers/specs/2026-11-23-ui-redesign-design.md` (+ `-part2.md`); boards in
  `specs/assets/2026-11-23-ui-redesign/`. **Owner's conditions:** behaviour fully integrated before
  building each part; before finishing, the agent checks the running app visually **1:1** against the
  accepted boards.
- ⬜ **Remember me** (owner, 2026-11-23): stay signed in until Log out; the last email is always
  remembered ("like a browser"). Needs the login bugs below fixed first (spec part 1 §10).
- Owner decisions 2026-11-23: Home/Login use the instrument look (no forest picture); five finishes
  (not Pearl accordion); Sections editor v2 and the new Dust approved.

## 2. Open engineering threads (from the wiki)
- ✅ **Use-after-free: stop callbacks outlive their player** — fixed 2026-11-24 (owner signed off; liveness token +
  the player stops its timer in its destructor). Was: (root-caused 2026-11-22).
  `ArrangerEngine`/`TrackPlayer` post beat-bar updates with `callAsync([this]…)`; destroying a style panel
  calls `stop()` then frees the engine, so the queued callback runs on freed memory. Only in-app trigger:
  deleting the style whose tab is open. **Reproduced live 2026-11-23**: the Debug app crashed (0xC0000005,
  same `std::function<void(double)>` frame). Found because `Project Tests.exe` with no flag crashed;
  bisected to the CurrentStyleComponent start test + any message pump; crash stack confirms.
  Spec: `docs/superpowers/specs/2026-11-22-stop-callback-use-after-free-design.md`.
- ⬜ Test isolation: the single-process run (`Project Tests.exe`, no flag) now completes but 4 `NoteLayer` GL pixel
  checks fail there only (they pass in `--display-tests`); likely a window left by an earlier test (found 2026-11-24).
- ⬜ Delay read offset still jumps on CC94 changes (needs a fractional read; `DelayLine` in `va-effects`).
- ⬜ Swap `juce::dsp::Chorus` (CC93) for the BBD model from `va-effects`.
- ⬜ 🔒 Missing-SFZ popup never smoke-tested; Phase 7a manual sign-off 0/9.
- ⬜ Prune merged branches (`fix/sfz-integration`, `fix/dsp-quality`, `docs/wiki-sync-main`,
  `arhitecture/add-arhitecture-optimize-big-files`).
- ❓ Instrument presets (reverb/chorus/cutoff… per instrument) are never sent — only the program change.
  Owner unsure (2026-11-22); left as is until they decide.
- ⬜ `AudioHandler` allocates a `juce::MidiBuffer` on every audio callback (audio thread must not allocate).
- ⬜ `SupabaseClient`: request JSON is built by string concatenation, so a `"` or `\` in an email,
  password or username breaks the request; `signup()` never reads the reply body (sign-up always shows
  "Invalid server response"). Found in the Backend names cleanup, not fixed there.
- ✅ (2026-11-24, `TableDialog::resultIdFor`) Notes table (right-click a track → notes): with **no rows selected**, the reset/modify dialog shows
  "Change All" / "Cancel", but Cancel gets button id 2 = "selected", so Cancel runs the *selected* action on
  an empty selection and calls `onConfirmed()` (`TableContainer::showModifyChangeDialog`).
- ⬜ Names check gaps: `MidiNotesTableModel.cpp:209` / `:231` compare `columnId` with bare `2` / `3`, and
  `SoundEffectWindowComponent.cpp:498` has inline row names (`"TONE"`…); `check_named_values.py` misses
  both shapes (found 2026-11-23 while designing the UI).
- ✅ (2026-11-24) Renaming a track from the track list doesn't focus the name box: `TrackListComponent::renameFromTrackList`
  looks up the editor "folderName" although its editor is "trackName" (seen in the Playback names cleanup).

## 3. Owner's TODO file (last priority, owner's words summarised)
| Item | State |
|---|---|
| BPM applied when a style / track loads | ✅ |
| Change BPM while playing | ⬜ locked while the arranger plays (`CurrentStyleComponent.cpp:189`) |
| Real arranger with sections, loop based | ✅ |
| Track playtime, send it for a website TOP list | ⬜ **switched off**: the only `PlaytimeTracker` creation (`MainComponent.cpp` ~595) is commented out, so nothing is sent. The tracker also ignores its interval argument (always 300 s). Corrected 2026-11-22; was marked ✅ |
| Login / logout inside the app, persistent login (JSON in AppData) | ⬜ no logout or saved session in the code. Also: `LoginComponent::handleLogin()` returns before checking anything (Login always succeeds), and "Forgot Pass?" sends nothing |
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
