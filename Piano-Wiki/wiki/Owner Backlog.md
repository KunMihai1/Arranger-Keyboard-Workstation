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
- ✅ **Phase 4c, the Settings page** (2026-11-24, `feat/ui-settings-page`): board `StageSettings`; the MIDI
  Settings window becomes the page (MIDI / Arranger / Chords) plus Display (Particles, Note names, Particle colours…,
  particle style Sparks / Dust / Smoke = spec B9, key `ParticleStyle`). Agent calls: the field that does not apply to
  the engine is dimmed, not hidden; SFZ library… stays in the MIDI column until the Voices page; Keyboard field = the
  old "Update key count"; Esc menu "Settings" opens the page; the Effects ↔ MIDI window exclusivity is gone.
- ✅ **UI polish fixes, one PR** (owner, 2026-11-25, `fix/ui-polish`): Settings LEDs cut on the left (`RowToggle`
  gets room for its glow); "INSTRUMEN" / "SOR" (`SilkLabel` draws every glyph, no curtailing); the small freeze on
  add / remove / rename folder (measured 0.5–1 s, `AlertWindow`'s balanced text layout → in-app `StageDialog` on the
  track picker, the styles list and the strips; messages on the stage screen); the strip's instrument list → a
  "Choose instrument · slot N" sub-page (owner: like Voices); tooltips in the Stage look (owner: dark panel + silk
  text); the chosen instrument now plays. Not done: the section-config errors and the other ~100 `AlertWindow`s
  (later PR); `SFZLibraryUI` keeps the old `InstrumentChooserComponent`.
- ✅ **Phase 4d, the Voices page** (2026-11-26, `feat/ui-voices-page`): board `StageVoices`; panel Voices opens it
  (the call-out instrument tree goes). Instrument column (8 groups / 33 GM programs, search across groups, title-row
  Left hand / Right hand = the Part LEDs, none lit → "Choose a part" on the screen), range / split / transpose (moved
  from the "Style settings…" sub-view, which now opens this page) and the SFZ library (moved from the Settings page;
  Remove = right-click). Not done: the old "Assigned" `AlertWindow` and the Assign / Import choosers' look (later
  PR); the strips' 128-program picker (PR #67) and this page's 33-program list stay separate.
- ✅ **Phase 4e, the Chord helper page** (2026-11-26, `feat/ui-chords-page`): board `PageChords`; search, LCD list
  "name · notes", the chosen chord with three voicings drawn on mini keyboards; a click shows and plays it. Also
  fixed (on `main` too): any note played while a page was open brought the GL note layer back over the page. Not
  done: the PNG chord diagrams are no longer used but stay in the project (owner's call to delete); the loading
  label hides the note layer without the page hold (same as before).
- ✅ **Sparks and Smoke** (owner, 2026-11-27: "the sparks can't even be seen … smoke is like in the background
  transparent"), PR #72: pure `ParticleRecipe`, unit-tested; Dust unchanged. Owner tried a first pass: sparks
  "tiny dots, barely", smoke "looks like fog"; on a browser preview picked **Light wisps** to replace Smoke and
  said sparks were "way too boom" → calm embers. Seen on screen through display-test frames; display checks that
  both light up the area above the key.
- ✅ **Esc opens the old screen** (owner, 2026-11-27) — the Esc menu is a page panel now (spec P.12,
  `feat/ui-esc-menu`): MENU heading, Settings / Effects / Exit seg buttons, app scale, click outside closes;
  the keys stay dimmed under it (note layer held hidden as on a page).
- ✅ **"Play" text on the piano photo is trimmed** (owner, 2026-11-27) and must sit in the same place at every
  window width / height — PR #73 (`HomeLayout`).
- ✅ **Recorder page** (board PageFile, 2026-11-27): length / notes / REC lamp, Play / Stop / Save as MIDI…, MIDI
  file status with Play / Stop. Fixed on the way: Play a MIDI file opened a Save dialog; a playing file had no Stop.
- ✅ **Sections v2** (board PageSections, 2026-11-27; owner: "make sure you don't lose any logic"): list + editor
  are the Style page's Sections view instead of two overlays; lanes with every note by pitch, transpose button per
  lane, zoom, jump pads, Selected-section inspector. All 29 inventory items of the spec checked live.
- ✅ **Realistic keys** (board StageRevised, 2026-11-27): ivory / ebony keys with lip, rounded fronts, gaps on a dark
  bed, black-key shadow; a pressed key looks pushed down (no more green). Checked 1:1 against the board live.
- ✅ **New smooth Dust** (board Dust, 2026-11-27): a burst on press, then a soft stream while the key is held; fine
  specks and soft motes drift on a smooth flow (no more jitter), fade in and out, grow as they rise; additive glow.
- ✅ **Panel finishes** (board SkinsFinish, 2026-11-27): Settings › Display › Finish = Stage (default) · Rosso burst ·
  Champagne · Tobacco burst · Midnight flake · Concert black (Pearl accordion not built, owner). Panel surface, key
  slip and side strips; textures from the board's own feTurbulence numbers. Agent calls to check: side strips 16 px
  (26 would cover the first LEDs); labels print dark where the finish is light (Champagne, Tobacco's bright middle).
- ✅ **Log in and Home on the instrument** (boards StageLogin / StageHome, owner decision 2026-11-23): the panel shows
  (screen asleep: Welcome / Ready, controls inactive), the page area holds the LCD-field login form or Home's
  Settings · Play · Exit keys, the keys are drawn underneath. The forest picture and its Play text are gone (asked the
  owner 2026-11-27 whether to keep the photo instead — no answer; spec decision followed, easy to revert).
- ✅ **Sign-in part 2** (`feat/ui-sign-in`): remembered email (`LastEmail`) with *Not you?*, Home "Signed in as … ·
  Log out", no "logged in" box, escaped backend JSON (`BackendJson`), sign-up reads its reply (it never did).
- ⬜ **Owner: the Supabase backend is unreachable** — `ecmlftmkoqszdwjugqtn.supabase.co` does not resolve (DNS
  non-existent domain, 2026-11-28; paused or deleted?). Owner decision 2026-11-28: Login keeps letting the typed email
  in until it is back; then set `checkAccountWithServer = true` (`LoginComponent.cpp`) — the server path is built and
  its replies unit-tested, but a real login was never seen working.
- ⬜ Remember me (needs the auth proxy to accept `grant_type=refresh_token`, outside the repo) · Forgot password?
  (needs a reset endpoint) — both wait on the backend.
- ✅ **Bug found 2026-11-27 — track library reloads tracks from the wrong file** (PR #77). A folder stored one
  `filePath` (the first MIDI file added) and each track only `trackIndex`, so tracks from other files came back as
  that first file's. Each track now saves its own `filePath` (old saves fall back to the folder's; they can't be
  repaired, the other files were never recorded). A folder whose first file is gone stays, with its other tracks.
- ✅ **Flaky display test `sparksAreVisible`** (PR #77): failed 3 of 12 runs. Causes: the default particle colour
  picks its alpha at random (0.6–1.0) per launch, 12 random sparks a tap, note-only count ±50 px. Now the dimmest
  default colour and three taps summed (floor 450; lowest of ten runs 782); 10 of 10 green; fails with no sparks.
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
- 🟡 **Remember me** (owner, 2026-11-23): stay signed in until Log out; the last email is always
  remembered ("like a browser"). Remembered email + Log out done (#82); staying signed in waits on the backend.
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
- ✅ Prune merged branches (2026-11-28: 41 merged branches deleted on GitHub, the local ones and a stale agent
  worktree; only `main` left, plus `feat/particles-sparks-smoke` checked out in the owner's main folder). Rule since:
  merging a PR deletes its branch (AGENTS.md › Git).
- ❓ Instrument presets (reverb/chorus/cutoff… per instrument) are never sent — only the program change.
  Owner unsure (2026-11-22); left as is until they decide.
- ✅ (`fix/audio-thread-realtime`) `AudioHandler` allocated on every audio callback (a fresh `MidiBuffer`, one per
  active channel, `callAsync` for the no-SFZ notice) and could wait on `MidiHandler`'s lock: now reserved buffers
  (`MidiChannelSplitter`), a message-thread timer for the notice, and a try-lock (busy → next block).
- ✅ (#82, `BackendJson`) `SupabaseClient`: request JSON is built by string concatenation, so a `"` or `\` in an email,
  password or username breaks the request; `signup()` never reads the reply body (sign-up always shows
  "Invalid server response"). Found in the Backend names cleanup, not fixed there.
- ✅ (2026-11-24, `TableDialog::resultIdFor`) Notes table (right-click a track → notes): with **no rows selected**, the reset/modify dialog shows
  "Change All" / "Cancel", but Cancel gets button id 2 = "selected", so Cancel runs the *selected* action on
  an empty selection and calls `onConfirmed()` (`TableContainer::showModifyChangeDialog`).
- ✅ Names check gaps (re-checked 2026-11-28, nothing to fix): the bare `2` / `3` in `MidiNotesTableModel.cpp` sit in
  commented-out code (the checker rightly skips comments), and the Effects page rewrite named the effect rows.
- ✅ **Every `juce::AlertWindow` replaced** (`feat/stage-alerts`, 2026-11-28): 48 notices, 3 OK/Cancel questions and 3
  hand-built boxes now go through `StageAlert` (page-look `StageDialog`s: OK-only notices, an alternative button
  for Change All / Change selected). They cover the whole scene like the old modal boxes, hide the GL note layer
  while open and give the keys back after.
- ✅ (2026-11-24) Renaming a track from the track list doesn't focus the name box: `TrackListComponent::renameFromTrackList`
  looks up the editor "folderName" although its editor is "trackName" (seen in the Playback names cleanup).

## 3. Owner's TODO file (last priority, owner's words summarised)
| Item | State |
|---|---|
| BPM applied when a style / track loads | ✅ |
| Change BPM while playing | ⬜ locked while the arranger plays (`CurrentStyleComponent.cpp:189`) |
| Real arranger with sections, loop based | ✅ |
| Track playtime, send it for a website TOP list | ⬜ **switched off**: the only `PlaytimeTracker` creation (`MainComponent.cpp` ~595) is commented out, so nothing is sent. The tracker also ignores its interval argument (always 300 s). Corrected 2026-11-22; was marked ✅ |
| Login / logout inside the app, persistent login (JSON in AppData) | 🟡 Log out + remembered email done (#82); the account check and a saved session wait on the backend (unreachable 2026-11-28). Before: Also: `LoginComponent::handleLogin()` returns before checking anything (Login always succeeds), and "Forgot Pass?" sends nothing |
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
