---
type: meta
title: "Operations Log"
created: 2026-06-06
updated: 2026-11-25
tags:
  - meta
  - log
status: evergreen
---

# Operations Log

Append-only. Newest entries at the TOP. Never edit past entries.

## 2026-11-25 — UI polish fixes, one PR (`fix/ui-polish`)
- Owner's six reports. LEDs cut on the Settings page: `RowToggle::boundsForBoardBox` leaves room for the glow.
  "INSTRUMEN" / "SOR": `SilkLabel` draws every glyph (JUCE curtailed the last one when the width rounded down).
- Freeze on add / remove / rename folder, measured 541–1019 ms per dialog; a stack sampler put 33 of 34 samples in
  `AlertWindow::updateLayout` → `createLayoutWithBalancedLineLengths`. New `StageDialog` (overlay on the page frame)
  for the track picker, the styles list and the strips; messages go to the stage screen (`onInfo`). After: the
  message thread stalls at most 41 ms (WM_NULL round-trips). The section-config errors and ~100 other
  `AlertWindow`s stay for a later PR.
- Strip instrument list → `InstrumentPicker` sub-page ("Choose instrument · slot N", owner: like Voices).
  Tooltips → `StageTooltip` (dark panel, rule border, silk text, sized to the text, scaled with the stage).
- The chosen instrument now plays: the program baked in at style load and the MIDI file's own program changes won,
  the arranger config copied only volumes, and the SFZ wasn't reloaded on a pick (see [[Styles System]]).
- Sonnet review, each claim checked: fixed: a click on a dialog let `MainComponent` take the focus back
  (reproduced: the typed name was lost), and the dialog didn't follow a window resize. Kept on purpose: the strip's
  instrument beats a saved arranger config's (the owner's report). Not done: `setTooltip` tooltips are not scaled
  (no visible user today).
- Tests: unit 1788, arranger 770, integration 130 (+7 skipped).

## 2026-11-24 — UI redesign phase 4c: Settings page (`feat/ui-settings-page`)
- Effects page merged (PR #65).
- The Settings button and the Esc menu's "Settings" open the board's `StageSettings` page instead of the MIDI
  Settings window. `MIDIWindow` (`SettingsWindow.h`) is a component now: MIDI (four LCD fields with step arrows,
  Keyboard = the old "Update key count", Add device…, SFZ library… until the Voices page) / Arranger (seven LED rows) /
  Chords (Chord mode, Minor scale); same settings keys, defaults and callbacks. MainComponent adds Display: Particles,
  Note names on keys, Particle colours… and the particle style (spec B9: `NoteLayer::setParticleStyle` finally
  called, key `ParticleStyle`, default Dust). New widgets `StageField`, `RowToggle` (`f4f465f`).
- Gone: the window, its focus-loss hide / restore, the Effects ↔ MIDI window exclusivity.
- Review: the stored particle style reached no layer before Play created it (now applied there, and validated);
  the SFZ library panel drew under the Display column; switching pages left it open; a device added with an odd key
  count showed a blank Keyboard field. Rejected: "Esc menu Settings closes an open page" (Esc closes the page first).
- Live: toggles, choices, engine switch, Esc menu, 800 x 600; 1:1 against the board. Unit 1694 / 0, arranger
  770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0; names 0.

## 2026-11-24 — UI redesign phase 4b: Effects page (`feat/ui-effects-page`)
- Style page work merged (PR #64, `e970b53`).
- The Effects button and the Esc menu's "Effects" open the board's `StageEffects` page instead of the always-on-top
  Sound Effects window: Instrument 1st / 2nd | Sustain (green LED), Tone / Envelope / Space / Modulation, 16
  effects-size knobs with values. `SoundEffectContentComponent` restyled in place (API and `MainComponentEffects`
  wiring unchanged); the hidden 1st / 2nd combo stays the state; the window class is gone.
- Review (`14efc0b`): Effects and the MIDI Settings window replace each other again; Quick FX changes refresh the
  open page; a dead loader removed.
- Live: turn, per-instrument values, Sustain, Esc menu, 800 x 600; 1:1 against the board. Unit 1660 / 0, arranger
  770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0; names 0.

## 2026-11-24 — UI redesign phase 4a: Style page, track picker, notes page (`feat/ui-style-page`)
- **Specs / plans are local only now** (owner): `docs/superpowers/` is git-ignored (PR #63); AGENTS.md says no
  secrets, local files stay ignored, agents don't look in the owner's environment.
- **Delete-the-open-style crash fixed** (`34be599` + review `77cae73`): `ArrangerEngine` / `MultipleTrackPlayer` own a
  liveness token, every posted lambda holds a weak copy; the player now stops its timer in its own destructor.
  Test `CallbackLifetime` crashed before (exit 3). The no-flag run completes now (4 GL pixel checks fail only there).
- Styles page = board `StageStyles`: LCD styles list (Add / Rename / Delete), 8 track strips (`StageFader` follows
  the hand), title row PLAY · All tracks · Solo | Sections... · Style settings... · Exit; tabs removed
  (`StyleViewRouter`, `DisplayViews.cpp`). Picking an instrument keeps the track name (owner). Track picker =
  `PageTracks` (rename box focus fixed); notes page = `PageNotes` (owner: hint line, no Undo; Cancel = dismissal).
  Stage screen chord indent fixed. Every page frame got Exit and is centred like the panel (was 38 px off at
  1920 x 991).
- 1:1 check at 1920 x 991 (+ 1280 x 720, 800 x 600): `docs/superpowers/test-plans/2026-11-23-ui-redesign-style-page-visual.md`
  (local). Sonnet review: 4 fixed, 3 rejected with reasons (`77cae73`).
- Tests: unit 1629 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0; names 0.

## 2026-11-23 — UI redesign phase 1 merged (PR #61)
- `feat/ui-stage-panel` merged into `main` (`908891f`, merge commit as usual). CI: unit 1501 / 0, arranger 770 / 0,
  integration 95 / 0 (+13 skipped: no MIDI hardware or display on the runner, same as `main`), names 0, 800-line OK.
- `gh` is now installed system-wide (`C:\Program Files\GitHub CLI\gh.exe`) and git's credential helper uses it.
- Still for the owner to try: real playback with a style that has tracks, and chords from a hardware keyboard
  (steps 6 and 10 of `docs/superpowers/test-plans/2026-11-23-ui-redesign-phase1-visual.md`).

## 2026-11-23 — Stage panel branch review (plan Task 9)
- Sonnet review of `main..feat/ui-stage-panel`, every claim checked against the code. Fixed (`92a5803`): chords from a
  hardware keyboard updated the Stage screen on the MIDI input thread (now `callAsync` + `SafePointer`); every tempo
  step committed to the style and re-scaled all tracks (up to 33×/s while holding Tempo +) — holds and drags now
  commit once on release, like the old BPM slider, and the refresh no longer pulls the screen back mid-gesture; a
  quick knob turned with no part chosen stayed moved (springs back now). All three checked live.
- Rejected with reasons: section LED sync on other paths (no failing path), note-layer restore order with the
  arranger overlay (the overlay closes first), "/4" label (nothing sets a denominator today), per-tick screen refresh
  (same rate as the old beat bar; repaints coalesce).
- Repeatable manual check for the wiring added to the visual test plan (`d4fb477`).
- Tests: unit 1501 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0. Next: PR (owner's go).

## 2026-11-23 — Reference PNGs (plan Task 8); 1:1 visual check write-up; message fix
- Boards rendered without the design canvas (`7a9d799`): `specs/assets/2026-11-23-ui-redesign/render/render.sh`
  (headless Edge + stand-in runtime `support.js`, `<dc-import>` supported) → 16 PNGs at 2× in `png/`; a second run
  is pixel-identical; it also renders a board at the app's own scale for comparisons. `.gitattributes` keeps
  `*.sh` LF (`c6b040b`).
- 1:1 check (`37d0ce9`, `docs/superpowers/test-plans/2026-11-23-ui-redesign-phase1-visual.md` + images): live app
  at 1920 × 991 (scale 1.536) vs the board at the same scale. New difference found and fixed (`7dce83b`): a message
  took the whole top row; on the board it replaces STYLE only, PLAY and 4/4 · TEMPO stay. Now it takes the row
  only when it would not fit beside them (14 px gap). Text looks heavier on the board only because of ClearType.
- Tests: unit 1490 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0. Sonnet review next.

## 2026-11-23 — Stage panel wired into the app (plan Task 7); live check; layout-proof key tests
- `MainComponentStage.cpp` + `DisplayStage.cpp` (`d425e08`): the panel replaces the header. Sections call the old buttons'
  handlers (LEDs mirror them), Start/Stop/tempo go to the open style (`CurrentStyleComponent::commitTempo`),
  `CustomBeatBar::onBeatsElapsed` feeds lamps and bar, every `temporaryPopup` is a screen message. Styles,
  Recorder, Settings and Chords are `StagePageFrame` pages hosting today's controls; Effects / Voices / Home open
  what they opened before. Esc closes a page first.
- Live check by the agent (screenshots + clicks on the running app, board rendered headless at the same scale):
  sections, Auto Fill, Sync Start, Rec (+ messages), tempo +/hold/wheel/right-click/type/−, every page, Esc,
  knob turning, Part toggles; window sizes 1920 × 991, 1280 × 720, 1024 × 768, 800 × 600, 1900 × 500.
- Fixed from it, each with a test: knob turned against the hand (relative circular turning); LED glow cut on the
  left (glow room) and ~3× the board's CSS blur (Gaussian fall-off, ring energy within 3 %); labels one
  letter-spacing too wide (JUCE's width already includes it, up to 5 px off the board); tempo editor showed an
  inverted block; chord helper call-out covered the panel and swallowed Esc (now the Chords page;
  `ChordBrowserComponent::onEscape`). Also save-on-release quick knobs, opaque panel, hand cursor.
- `test_key_listener.cpp` (`733408c`) presses keys by position (scan code → active layout), so it passes on the owner's
  Romanian layout (2 failures before; two "too high" tests had been passing vacuously).
- Not tested live: real playback (the owner's styles have no tracks). Lesson: the Sound Effects window is a
  separate desktop window titled like the app — drive the largest window of the process, not the title.
- Tests: unit 1488 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 8 / 0.

## 2026-11-23 — Stage panel assembled (plan Task 6); label fix
- `Source/UI/Stage/StagePanel` (`11d1b82`): all controls inside the scaled, centred board; section buttons report
  `SectionGroupNames` names ("Ending 1" printed "End 1"); the panel runs the Tempo −/+ hold itself through
  `StageScreen::stepTempo`; quick knobs report `QuickKnob::volume / reverb`. Tests check ≥ 20 board px between caps.
- A throwaway render (1920 × 198, 2560 × 208) showed "LEF"/"RIGH": label width lacked CSS trailing letter spacing and
  rounding cut the last glyph. Fixed with a test that lays labels out like `drawText` (`7b0a7ac`).
- Lesson: `Copy-Item` keeps the old timestamp, so MSBuild skipped the restored file once (touch after restoring).
- Tests: unit 1419 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-23 — Stage screen (plan Task 5)
- `Source/UI/Stage/StageScreen` (`3e2d85d`): bezel, LCD rows (space-between like the board), beat lamps; top
  line = message for 2.5 s / STYLE / app name asleep; tempo wheel (Shift 5), drag (6 px/step), type (Enter /
  Esc / leave), right-click = style tempo; locked → "TEMPO LOCKED WHILE PLAYING". Editor is a hidden child,
  never deleted inside its own callback. UTF-8 text follows the `const char*` + `fromUTF8` convention.
- Tests: unit 1227 / 0 (StageScreen 23 / 0), arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-23 — Stage rubber buttons and knobs (plan Task 4); keybind idea noted
- `Source/UI/Stage/StageWidgets` (`6cb3f71`): `RubberButton` (key / screen / ledToggle; each draws its LED,
  cap and label; only the cap takes clicks) and `StageKnob` (0–127, ±135°, vertical drag). `StageFonts` now
  takes CSS values: size = JUCE point height; JUCE kerning is relative to ascent + descent, so CSS em is
  converted with the font's height-to-points factor. No `StageLookAndFeel` (not needed in phase 1).
- Owner idea recorded in [[Owner Backlog]]: every panel action assignable to a piano key or PC key via a
  keybind menu (needs its own spec; the PC keyboard already plays notes).
- Tests: unit 1204 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped). App builds, no new warnings.

## 2026-11-23 — Same look on every window shape
- Owner: the UI must look the same on any resolution, everything visible on all screens. `StageRegions`
  gains `panelContent` (the 1200 × 124 board × `s`, centred; wider windows only add plain panel at the
  sides) — `33a370c`. Checks at 21:9, 4:3, portrait, 150 % scaling, 320 × 200, 1920 × 1032. Spec §4.1.
- Tests: unit 1171 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-23 — Stage panel models (plan Task 3)
- `Source/UI/Stage/StageModels` (`017837b`): `TempoModel` (60–200, typed/wheel/step/reset, refuses while
  locked, hold delays 400 then max (30, 130 − 10 × step)), `BeatLampModel`, `ScreenMessages` (upper case,
  2.5 s), `PageRouter`. Plan's hold-delay example "(1) = 130" contradicted "(5) = 80"; board wins, plan fixed.
- Tests: unit 1034 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped). App builds.

## 2026-11-23 — Stage panel re-laid out; Stage fonts embedded
- Owner: panel labels must not run into each other and the buttons were too tight. Measured with the
  real font: section buttons 32 px apart, "INTRO 2/3" 32.8 px, "AUTO FILL" 42.9 px, "SYNC ST." 38.1 px;
  21 buttons cannot fit the 280 × 110 px left third with real gaps. Owner chose: transport (Rec, File /
  Sync Start, Stop, Start) moves to the right third, Quick FX keeps Volume + Reverb (Chorus and
  Brightness stay on the Effects page), Auto Fill stays on the left after Fill 4. LEDs now sit beside
  the buttons; labels ≥ 12.2 px apart, buttons ≥ 20 px. Board, spec §2/§3.3/§4.2, part 2 map and plan
  updated (`f946646`).
- Plan Task 1: Barlow Semi Condensed (Medium, SemiBold) + Share Tech Mono (SIL OFL 1.1) in
  `Resources/Fonts/`, in BinaryData of both `.jucer`s; `Source/UI/Stage/StageFonts` (`c38fb13`).
  Found: DirectWrite names Barlow as family "Barlow" + style "Semi Condensed SemiBold".
- Plan Task 2 (`6291080`): `StagePalette` (spec §3.1) in `AppColours.h`, `StageMetrics.h` (board sizes,
  new layout), `StageScale::regionsFor` (scale = min (w / 1200, h / 645); 1920 × 1032 → panel 198,
  felt 6, notes 585, keys 224, slip 19; note area never negative).
- Tests: unit 957 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped). App builds.
- Owner: plan tasks (and their wiki sync) are committed without asking once green; push/PR still asked.

## 2026-11-23 — UI redesign designed with the owner; spec written
- Mock-ups on a private canvas, iterated with the owner: three real-gear directions → A "Stage" chosen and
  revised (slim panel, centred bigger screen); beat lamps in the app's red/yellow; every feature mapped to
  the panel or a page (Effects, Settings, Style, track picker, Notes, Sections v2, Voices, Chords,
  Recorder, Home, Login); tempo hold/type/wheel/right-click reset; dust prototype; skins v3 = instrument
  finishes after Korg's Kronos editions (owner: "decent"); Remember me = stay signed in, email always kept.
- Found while mapping: the Sections editor edits windows over the recording (not a sequence — the first
  mock-up was wrong and was redone); `setParticleStyle` is never called (always Dust); the playback
  settings hold one range + split points + one transpose (Voices board corrected).
- Spec `docs/superpowers/specs/2026-11-23-ui-redesign-design.md` + part 2, with the owner's two conditions
  (behaviour integrated first; visual 1:1 check by the agent before finishing). Awaiting approval.

## 2026-11-23 — Use-after-free reproduced live
- Debug app: Play → open "Style 1" → Delete it from the Styles list → Remove. The process died within
  3 s; Windows log `0xc0000005` at offset `0x2bb6fa` = `std::_Func_class<void,double>::operator()` (PDB),
  the same frame as the no-flag test-run crash. App data backed up and restored (0 differences).
- Spec corrected: the style panel is created once and reused, so deleting the open style is the only
  in-app trigger (closing the tab, Home, switching styles do not destroy it). Fix still awaits sign-off.

## 2026-11-22 — Names cleanup: all test files done, allowlist empty
- PRs #52–#56 converted the 41 remaining test files (41 → 23 → 14 → 6 → 3 → 0). Each file has a
  `<Name>TestValues` namespace (test name, category, descriptions, messages, values); tests check exactly
  what they did before — every suite kept its assertion count (unit 870, arranger 770, integration 130 +7
  skipped, display 8) and the ADAA aliasing figure matches main's CI log (naive −13.07 dB, ADAA −19.25 dB).
- Each part had a Sonnet review (no defects in any part; nits verified and fixed). `SFZLibraryUI` now uses
  `StyleNames::defaultStyle` instead of its own copy.
- Found, not fixed: running all test categories in one process (no flag) is unreliable — see Owner Backlog.

## 2026-11-22 — Names cleanup: UI part 4 — all of Source/ done
- KeyboardUI, NoteLayer, SettingsWindow, SoundEffectWindowComponent converted (46 → 41; only tests left).
  Dead code removed: `KeyboardUI::paintKeyboard` (never called) and SettingsWindow's unused slider look.
  NoteLayer split per ADR-003: particle system → `NoteLayerParticles.cpp`, shared names `NoteLayerNames.h`.
- Live smoke test of all UI parts on the new build (app data backed up and restored, 0 differences): login,
  play screen, keyboard + falling notes on a held chord, style panel, play-mode dropdown, Esc menu, Effects
  and MIDI Settings windows all render and read their saved values.

## 2026-11-22 — Names cleanup: UI part 3 (MainComponent)
- `MainComponent*.cpp` converted (52 → 46). Its widget classes moved to `MainComponentWidgets.cpp` (ADR-003)
  to stay under 800 lines. New shared names: `SettingsFileKeys` (settings-file keys), `EffectControllers`
  (effect CC numbers), `HandChoiceNames`, `DeviceNames::noDevicesFound`; section buttons and the instrument
  tree are named tables.

## 2026-11-22 — Names cleanup: UI part 2
- Display, DisplayStyleStore, playback settings panel and the notes tables converted (57 → 52). New persisted
  names for allStyles.json / mySections.json (`AppDataNames::stylesFile`, `AllStylesJsonKeys::styles`,
  `StyleNames::defaultStyleId`/`styleIdPrefix`/`newStylePrefix`), exact text kept. Found and logged: the notes
  table's Cancel runs the "selected" action when nothing is selected.

## 2026-11-22 — Names cleanup: UI part 1
- 23 smaller UI files converted (78 → 57). Shared names: `OutputEngineIds`, `MinorScaleButtons`,
  `SettingsWindowText`, `InstrumentGroupNames`, section-button prefixes in `SectionGroupNames`.
  `NoteField::Note` fields `x`/`y` are now `left`/`top`. Review: no defects. Suites 870 / 770 / 130.

## 2026-11-22 — Names cleanup: Backend (Backend done) + findings
- All 9 `Source/Backend` files converted (87 → 78); request bodies byte-identical (raw strings became
  escaped strings). Behaviour unchanged.
- Found and logged in [[Owner Backlog]], not fixed: playtime tracking is switched off (its creation is
  commented out; the backlog had it as done), `handleLogin()` skips the credential check, "Forgot Pass?"
  sends nothing, `signup()` never reads the reply, request JSON is not escaped.

## 2026-11-22 — Names cleanup: Styles (Styles done)
- All 11 `Source/Styles` files converted (98 → 87). New shared names: `StyleDialogText`, `SectionGroupNames`,
  `PlayModeIds`, `SectionHighlight`; persisted `StyleNames::defaultStyle` ("DEFAULT Style") and
  `AllStylesJsonKeys` (allStyles.json keys, exact text kept); `SoundSettings::setValue` uses
  `EffectSettingsKeys`. Two reviews: no defects; nits applied. Suites 870 / 770 / 130 (+7 skipped).

## 2026-11-22 — CI once per PR; build-time result on GitHub
- `ci.yml`: `push` runs only on `main` (owner approved); PRs keep their `pull_request` run, so each PR
  commit builds once instead of twice. The post-merge `main` run still fills the caches.
- Measured on GitHub after PR #43: warm build step 41–47 s before → 40–41 s after (within noise). The
  local gains stand (clean 44 → 30 s, header edit 7.1 → 5.5 s); the CI prediction did not hold.
- PR #44 first failed the 800-line cap (`TrackListComponent.cpp` grew to 860 lines while naming values);
  split per ADR-003 into `TrackListComponentFolders.cpp` + `TrackListComponentNames.h`.

## 2026-11-22 — Names cleanup: Playback part 2 (Playback done)
- `Track`, `TrackListComponent` converted (102 → 98); shared persisted names `AppDataNames::tracksFile`
  ("myTracks.json") and `StyleTrackJsonKeys` (exact text kept). Review: no defects; follow-up gives each
  value one name (`Track::defaultChannel`, reuse `TrackEntryDefaults::keepCurrent`).
- Found, kept as is (behaviour-identical refactor), logged in [[Owner Backlog]]: renaming a track from
  the track list focuses the wrong editor name. Suites 870 / 770 / 130 (+7 skipped).

## 2026-11-22 — Faster builds (MSBuild overrides)
- Research agent measured where build time goes: JUCE module files compiled in two serial `/MP` batches
  (five have `/bigobj`), and MSBuild used the 32-bit-hosted compiler/linker.
- New root `Directory.Build.props` (64-bit tools) and `Directory.Build.targets` (one `/bigobj` option for
  all module files); CI cache key hashes both. Clean build 44 → 30 s (app), 44 → 28 s (tests); header
  edit 7.1 → 5.5 s; CI-warm test build 25.6 → 16.5 s. Rejected: MultiToolTask, unity build, sccache.
- Re-checked on current `main`: app rebuild 43.5 → 29.0 s; suites 870 / 770 / 130 (+7 skipped).
  [[Dependencies Index]]

## 2026-11-22 — Live checks + style tab lost after Home (fixed)
- Live on an unlocked screen (app data backed up and restored, 0 differences): Synchro Start from the UI
  (beat bar held, groove on the first chord), Count-In (4 clicks 0.5 s apart, accented first), style
  editor while playing (live stops, editor + preview + Add Fill work, live plays again after Close).
- Bug found: Settings tab → Home → Play → Styles removed the style tab, then the style list renamed the
  Styles tab. Cause: `Display` closed temporary tabs by position with flags Home never cleared. Fix:
  `TemporaryTabs` (next to `MyTabbedComponent`) closes them by name; 5 tests (2 failed before).
- Build pitfalls noted: Projucer must be 8.0.13 (`Desktop\Latest Juce\JUCE`); a stopped build can
  leave orphaned `cl.exe` holding the PCH.
- Owner request recorded: UI refinement + piano skins ([[Owner Backlog]] 1b).

## 2026-11-22 — Names cleanup: Playback part 1
- `TrackEntry.h`, `TrackPlayer`, `MidiNotesTableModel` converted (107 → 102). Shared names: `TrackTypeNames`
  (persisted "melodic"/"percussion"), `TrackEntryDefaults`, `NoteTableColumns` (Notes-information column
  ids), `NoteNames`. Behaviour unchanged; review found no defects.

## 2026-11-22 — Names cleanup: arranger editor UI and style IO
- Style editor, timeline (component + geometry), configuration list and the `.style` reader/writer
  converted; `Source/Arranger` is now fully off the allowlist (116 → 107). Behaviour unchanged.
- UI colours of these screens moved to `AppColours.h` (house rule). `.style` JSON keys are one named
  table (`StyleJsonKeys`, exact text). The editor's duplicated key-detection loop is one helper.

## 2026-11-22 — 800-line cap covers docs/
- Owner: the cap applies to `docs/` too. Seven documents over 800 lines (six old arranger plans and
  `docs/arranger_audio_system.md`, 971–1425 lines) split at their `##` headings, never inside a code
  block: part 1 keeps the file name, part 2 is `<name>-part2.md`, each links to the other. Checked that
  the parts rejoin to the original text exactly. CI's length check now scans `docs/**/*.md` (80 files, 0 over).

## 2026-11-22 — Auto Fill plays the fill of the variation you leave
- Owner's rule: Var 2 → Var 1 plays Fill 2 (it used to pick the fill numbered like the target).
  `ArrangerSectionSequencer::queue` now looks the fill up by the active variation's name; fallback to
  any Fill and the landing on the target are unchanged. Three new tests (none covered Auto Fill before):
  the outgoing fill both ways (failed before the fix: Fill 2 / Fill 1 swapped), any-fill fallback, no fill.
  Arranger suite 763 → 770. Spec `2026-11-22-autofill-outgoing-variation-design.md`.

## 2026-11-22 — Names cleanup: arranger engine and model
- 21 arranger files converted (chord, model, enums, NTT scales, transposer, detector, scheduler,
  sequencer, builders, time, channels, count-in, MIDI hex, engine); behaviour unchanged, persisted
  enum / chord / default-section names identical. Shared names live in `Chord.h` (MusicFacts,
  PitchClasses, ChordShapes, ChordQualityNames), `ArrangerModel.h` (ArrangerStyleDefaults,
  DefaultSectionNames, BeatTolerance, TrackRoles) and `ArrangerChannels.h`. Allowlist 137 → 116.
- Checker fix: a UTF-8 byte-order mark hid a file's first `#include` (read as `utf-8-sig` now).
- Owner answers recorded in [[Owner Backlog]]: `docs/` gets the 800-line cap; auto-fill must play the
  fill of the variation you leave (Var 2 → Var 1 = Fill 2); presets left as is (owner unsure).

## 2026-11-22 — Owner Backlog page
- New [[Owner Backlog]]: standing work, open engineering threads and the owner's TODO file, each with
  its state checked in the code (implemented items marked done). Linked from index, hot and AGENTS.md.

## 2026-11-22 — Names cleanup: Source/Midi done; owner decisions
- `MidiHandler`, `MidiDevice`, `MidiDevicesDB`, `MidiRecordPlayer`: named values, behaviour unchanged.
  Shared names in `MidiHandler.h`: `HandChannels` (1 / 16), `DeviceListChoice` (MIDI in / MIDI out /
  audio out), `DeviceNames::pcKeyboard`, `PlayableRangeResult`. Keyboard sizes and the 30 seed devices
  are tables; `myDevices.json` and its keys unchanged. Allowlist 142 → 137. Suites: same totals as main.
- Owner decisions: a style switch while playing takes effect on the next Start (intended); any section,
  an Ending included, may be the start section (intended); restore-down stays work-area sized, no
  proportional scaled window ("over engineering").
- MIDI protocol facts (note/controller/channel ranges) shared once as `MidiFacts` in `MidiHandler.h`
  (review found no defects; this removes the duplicated copies it pointed out).
- Rule: the 800-line cap now covers wiki pages too (`.raw/` exempt); CI checks `Piano-Wiki/**/*.md`.
  Largest page today: log.md, 404 lines.

## 2026-11-22 — "Restore down" fits the work area
- Measured: restored window was 1938x1077 at (0,0) on a 1920x1080 screen (work area 1920x1020): off the
  right edge and under the taskbar. Cause: JUCE restores to the bounds before `setFullScreen`, which were
  the content size (`MainComponent` = screen width x height-40) plus the frame.
- Fix (`Source/Main.cpp`): create the native window hidden, size it to the work area minus the frame, then
  go full screen. Restored now 1920x1020 at (0,0); maximise back -9,-9,1929,1029 as before.
- Not smaller on purpose: at 1440 px wide the style panel overlaps the record/stop/Chord-helper buttons
  (header needs near-full width). A smaller restorable window needs a header layout change (owner's call).

## 2026-11-22 — Live arranger run + SFZ queue backlog fix
- GL display tests re-run on an unlocked desktop: 8/8.
- Arranger plan part B run live with sound measured through WASAPI loopback (results table in
  `docs/superpowers/test-plans/2026-11-22-arranger-stress-test-plan.md`): section, transport and chord
  storms, held chord through Start/Stop spam, minimise during playback — no hang, no stuck note, beat bar on time.
- B7 ("arranger doubled to MIDI out and SFZ"): not real. Found instead: `MidiHandler` queued every note
  for the SFZ synth even with External MIDI, nothing drained it, and switching to SFZ fired the whole
  backlog in one block. Now queued only while the synth's audio device runs (`setInternalSynthListening`,
  switched by `AudioHandler` start/stop). Spec `2026-11-22-stale-synth-midi-backlog-design.md`; 4 new tests.
- Owner questions: style switch while playing keeps the previous arrangement; an Ending can be a start section.

## 2026-11-22 — Names cleanup: Source/Midi, part 1
- `InstrumentHandler.cpp`: the 33 per-instrument presets are one named table (values unchanged).
  Finding: `MidiHandler::applyInstrumentPreset` never sends the preset CCs (only the program change),
  so the presets currently have no effect — left as is for the owner to decide.
- Key listener: range guards, velocity, default 60..77 range, no-note value named; reverse loops use
  `index-- > {}`; `std::size` instead of sizeof division. `MidiRecordPlayer.h` defaults named.
- Checker accepts a `[[attribute]]` before a constant (self-test 18/18). Allowlist 142.

## 2026-11-22 — Names cleanup: Source/Audio done
- `AudioHandler.cpp`: MIDI CC numbers (`ControllerNumber`), CC→parameter mappings, smoothing times,
  audibility thresholds, channel layout and the 1-based MIDI channel → array index all named;
  loop indices and `sr`/`ch`/`cc`/`val` renamed. `SFZLibraryUI.cpp`: layout, colours, dialog texts
  named (`style_default` id text unchanged). Allowlist 146.

## 2026-11-22 — Names cleanup: Source/Audio, part 1
- `AudioHandler.h`: DSP facts named in `DspConstants` (16 MIDI channels, stereo, fallback sample rate,
  the smoothing formula 1 - e^(-1/samples), ln 2 in the overflow-safe log-cosh, the ADAA near-equal
  threshold); zeros use `{}`; `x`/`a`/`y`/`sr` renamed. `SFZlibrary.cpp`: GM range + fallback label.
  `SFZLibraryUI.h`: button texts named. Allowlist 148. Left in Audio: AudioHandler.cpp, SFZLibraryUI.cpp.

## 2026-11-22 — Arranger stress: style edge cases, count-in, Synchro
- Storms over edge-case styles (no sections, no tracks, 0- and 64-bar sections, 7/8, 5/4, 3/4 and an
  invalid 0/0) leave every message valid and nothing sounding — the engine's guards hold
  (`beatsPerBar` normalises bad signatures, the sequencer clamps 0 bars to 1).
- Count-in clicks exactly once per beat at any window size (accent first) and a stop cancels it;
  Synchro Start opens only on a real chord and a stop disarms it. No bug found. Arranger 763/0.
- Note: count-in clicks are note-ons without note-offs (GM side-stick, one-shot) — harmless.

## 2026-11-22 — Fix: arranger left a note stuck when a pattern restarted the same pitch
- Found by the stress tests (review pointed at it): `ArrangerEngine::activePlayedNote` kept ONE sounding
  pitch per (channel, original note). A pattern that starts the same pitch again before releasing it,
  with a chord change in between, overwrote the first transposed pitch — it never got a note-off.
- Now a FIFO queue per key: each note-off releases the oldest start. Test (first start under D, second
  under F#): 1 stuck note before, 0 after. Storm also checks every window for leaks (≤ 4 sounding).

## 2026-11-22 — Arranger hard testing, part A: headless stress tests
- `test_arranger_stress.cpp` (category Arranger, fixed seed, `renderRange` instead of the timer):
  20,000 render windows of chord/section/option storms (every quality incl. none, slash chords, unknown
  section labels, NTT/bass-inversion/minor-scale toggles, notes at the MIDI range edges); 600 stops at
  random moments; an Ending completing; the same pitch started twice across a chord change.
- Invariants on every emitted message: note/channel/velocity in range, no note left sounding after a
  stop or an Ending, the active section always real. All hold — no arranger bug found at this level.
- Mutation check: removing `silenceArrangerNotes()` from `haltAudio` makes them fail. Arranger suite
  721/0 in 0.8 s. Part B (live in-app scenarios) still needs an unlocked screen.

## 2026-11-22 — Names cleanup: Source/Common done; checker closes a loophole
- `HelperFunctions.cpp`: note helpers use named MIDI facts; the 48-branch chord-diagram if/else chain
  became one named table (generated from the old chain) + a lookup loop.
- `IOHelper.cpp`: every on-disk key named; the keys of all file/settings formats now live in
  `Source/Common/PersistedKeys.h` (each key text unchanged and present exactly once). Single-letter
  loop variables and parameters renamed. Kept under the 800-line cap by that split.
- Checker: a constant may hold literals only if its initialiser is plain values, so
  `const juce::String ch = channel == 1 ? "First" : "Second";` no longer hides a 1 and two strings.
- Arranger hard-testing plan written: `docs/superpowers/test-plans/2026-11-22-arranger-stress-test-plan.md`
  (A: headless stress, any time; B: live, needs an unlocked screen). Allowlist: 151 files.

## 2026-11-22 — AGENTS.md slimmed; no single-letter names; checker fixes
- AGENTS.md 139 → 100 lines: build/test detail moved to `docs/build-and-test.md`, the full naming
  rule to `docs/names-not-values.md`; AGENTS.md keeps short summaries + one-line pointers.
- Owner tightened the rule: no single-letter names, loop indices included; only JUCE's `g` in paint().
  The checker now flags single-letter declarations; 6 clean headers had parameter names fixed.
- Checker: a pure-virtual `= 0` is syntax; a constant's initialiser may span lines until `;`
  (5 interface headers became clean). Allowlist: 153 files.

## 2026-11-22 — Skipped tests are counted and listed
- A test that can't run here logs `SKIPPED - <reason>`; the runner counts these per case and prints
  `| skipped: N` on every line and the TOTAL, plus a list (test / case: reason). CI prints
  `N passed, K skipped` per suite and writes both, with the reasons, to the GitHub run summary.
- The list exposed a mis-gated HW test: the *output*-device name case skipped on *no input device*
  and used the input index; fixed (it now runs: integration 130 passed / 7 skipped locally).

## 2026-11-22 — New house rule: names, not values (enforced by CI)
- Owner's rule: no bare numbers (not even 0/1), no inline strings (incl. UI text, test names and
  messages), meaningful identifiers (only `g` in paint() and short loop indices). In AGENTS.md.
- `ci/check_named_values.py` flags literals outside named-constant definitions; CI runs it first
  (~1 s). Ratchet: `ci/named-values-allowlist.txt` holds the 158 of 192 files not converted yet
  (6,117 lines); it may only shrink, clean files must leave it, new files never join it.
- Plan: one behaviour-identical cleanup PR per subsystem (Common, Audio, Midi, Arranger, Playback,
  Styles, Backend, UI, tests); persisted keys keep their exact text.

## 2026-11-22 — Tests for the SFZ library (had none)
- `test_sfz_library.cpp` (9 cases, 34 assertions): add/duplicate/missing file, assignment guards,
  remove/clear (empty style dropped), import overwrite + no-op cases, save→load round trip (twice,
  no duplicates), missing/corrupt file → empty library, GM names 0..127. No bug found; a mutation
  (no clear on load) is caught. Unit 846/0.
- Coverage map: note editing is already covered via `applyChangesToASequence`; the track player's
  timing reads the real clock, so a deterministic test would need a time seam (not done).

## 2026-11-22 — Fix: `MidiDevice::sendMidiCC` crashed with no open output (found by CI)
- CI runners list the Windows GS synth but cannot open it; `MidiDevice_HW` treated it as hardware and
  `sendMidiCC` dereferenced the null output (0xC0000005). The test now probes that the output opens
  (else skips); `sendMidiCC` returns without an output. App callers already checked `isOpenOUT()`.
- The test runner now prints each test as it starts (`ConsoleTestRunner`), so a crash shows where.
- CI integration: 95 assertions on the runner (floor 85).

## 2026-11-22 — Tests: every test runs in a known category; integration suite in CI
- `AddDeviceWindow` test was category `Component`, which no flag ran — it never ran anywhere. Now
  `Unit` (5 assertions, all pass). Two empty test files (`test_settings_window*.cpp`) removed.
- GL test moved to `Integration-display` (`--display-tests`; real window + pixel reads, needs an
  unlocked desktop). `--api-tests` added for `Integration-api` (real backend, creates accounts).
  No flag now runs everything **except** the API tests (it used to run them too).
- CI runs `--integration-tests` (≈2 s; 93 assertions without MIDI hardware, floor 85). Unit floor
  730 → 790 (811 now). Category table in AGENTS.md.

## 2026-11-22 — Fix: Start pressed while already playing broke the BPM lock
- Track player: `MultipleTrackPlayer::start()`/`stop(true)` post a play-state notification and the
  listeners *flipped* state (BPM slider; notes table Change/Apply). Start-while-playing = uncounted
  stop + counted start → one flip too many. Listeners now read `MultipleTrackPlayer::isPlaying()`
  (new; `isTimerRunning()`, mutex-guarded in JUCE). `updateObjects` ignores track-player
  notifications in arranger mode (the arranger owns the BPM lock there).
- Arranger: `haltAudio` posts the 'stopped' callback; after an immediate restart it arrived late and
  cleared the playing state. The posted target is now `ArrangerEngine::notifyStoppedUnlessRestarted()`,
  which returns while `playing` (also true when Synchro-armed).
- Tests `test_start_while_playing.cpp` (4 cases, all values/strings named per the new rule): 6
  failures before, unit 806/0 + arranger 705/0 after. Live: track mode Start/Start/Stop/Stop and
  arranger Start/Start all keep the right lock.

## 2026-11-22 — Fix: style slots switched to another track's data
- The shared `UUID → TrackEntry*` map is now only ever pointed into the library: the picker no
  longer stores `&availableTracksFromFolder[i]` (a copy overwritten on every folder open), and
  Display rebuilds the map (`buildTrackUuidMap`) after any track/folder removal.
- `TrackListComponent`: post-confirm work extracted to `removeTracks()`/`removeFolders()` (no disk
  I/O; dialog callbacks save). Folder removal notifies after the erase; track removal uses the open
  folder (`currentFolderName`) and `erase(remove_if(...), end())` — `entry.folderName` goes stale
  after a folder rename and used to create a phantom folder + `erase(end())` (UB).
- Tests `test_track_list_removal.cpp` (4 cases): folder case failed and rename case aborted the
  runner before; unit 786/0, arranger 705/0 after. Verified live with the reproduction steps.
- Noted, not changed: Notes-information table keeps references into the entry while open; a
  folder rename leaves `TrackEntry::folderName` stale (nothing reads it after load now).
- Spec: `docs/superpowers/specs/2026-11-22-track-map-stale-pointers-design.md`.

## 2026-11-22 — Fix: Solo Start on a "None" track crashed
- `startPlaying()` Solo path dereferenced the null map entry that `applyBPMchangeBeforePlayback`'s
  `operator[]` inserts for every "None" track; now null-checked like the all-tracks path.
- Test case in `test_current_style_start.cpp` (style load → Solo → click Track → Start): unit run
  crashed 0xC0000005 before, 758/0 after; arranger 705/0. Verified live: "There are no tracks to play".
- Null entries themselves left alone (every other reader already checks).

## 2026-11-22 — Live reproduction of the code-reading bug list

Driven in the running Debug app on main (a182b35), app data backed up and restored afterwards.

- **Confirmed – crash:** Solo mode, click a "None" track, Start → access violation (c0000005).
  No BPM drag needed: loading the style already runs `applyBPMchangeBeforePlayback`, whose
  `mapUuidToTrackEntry[...]` inserts a null entry for each None track; the Solo path in
  `startPlaying()` derefs it without the null check the All-tracks path has.
- **Confirmed – picker folder switch:** assign Bass from folder A, browse into folder B without
  choosing → the slot still says Bass but Notes information shows folder B's track (C8 test notes).
  The map holds `&(*availableTracksFromFolder)[i]`, a copy overwritten on folder open.
- **Confirmed – removing a folder track:** removing a track stored *before* a used one shifts it
  (`remove_if` + erase) → the slot shows another track's notes. Removing one stored after it does not.
- **Confirmed – Start while playing:** track player: BPM unlocked while playing and locked after
  Stop (toggle in `updateObjects`). Arranger: BPM unlocked while still playing (stale async
  `onStoppedItself` from `haltAudio`).
- **Corrected – taskbar:** a normal launch is maximised correctly; only Windows "restore" gives a
  window the size of the whole screen (0,0-1938,1077). The earlier sighting and the "black keys too
  long" look came from the test helper's SW_RESTORE. Black keys are 0.6 of the key height as drawn.
- Not reproducible live (races): SFZ load order, MIDI-thread recording without a lock.

## 2026-11-21 — Start-with-nothing-to-play fix, CI object cache, bug hunt by code reading
- **PR #16 merged** (precompiled header + 10 modules dropped): CI 4–5 min → ~2 min.
- **PR #17 `fix/start-without-tracks`**: Start on a style with all tracks "None" left the beat light
  red and (arranger mode) the BPM slider locked — the button set `isPlaying` before
  `startPlaying()`, which returns early. `startPlaying()` now returns bool; test
  `test_current_style_start.cpp` (red 3 → green). Unit 756, arranger 705.
- **PR #18 `ci/cache-juce-objects`**: JUCE module code is 76 % of compile CPU (`/Bt+`). Test project
  builds with `/Z7` (`ci/Directory.Build.targets`); CI restores the module `.obj`s + tlogs keyed on
  JUCE version + runner image + hash of generated `ci/JuceLibraryCode`; saved only from `main`
  (CI now runs on push to main). Simulated: 53.6 → 23.8 s.
- **In-app run of the GPU note test plan: 12/12 pass** (login button skips credentials).
- **Found by reading code — to reproduce live before fixing:**
  1. Start while the arranger plays: stale async `onStoppedItself` (ArrangerEngine::haltAudio)
     clears `isPlaying` / unlocks BPM after the restart.
  2. Start while the track player plays: BPM slider toggled by `updateObjects()` ends up inverted
     (`stop(false)` doesn't toggle, `start()` does).
  3. Crash: Solo mode + "None" track selected + BPM drag (`mapUuidToTrackEntry[...]` inserts nullptr)
     + Start → `*it->second` on nullptr (CurrentStyleComponent.cpp ~64).
  4. Track picker stores `&(*availableTracksFromFolder)[i]` (a copy, rewritten on folder switch) in
     the shared map (Display.cpp ~356) → wrong track plays / use-after-free.
  5. Removing a track erases mid-`deque` in `groupedTracks` → other map pointers shift; map not rebuilt.
  6. `AudioHandler::loadSfz`: detached threads per load → out-of-order completion leaves the older
     instrument; also captures `this` (quit during loading).
  7. Window `setFullScreen(true)` covers the taskbar → style list's button row hidden (fix: maximised).

## 2026-11-21 — Faster builds: precompiled header + 10 unused JUCE modules dropped
PRs #13–#15 (docs, perf, refactor) merged. Then `build/faster-builds`: measured first — every
source file re-parsed all 24 modules' headers (6.8 s to compile one small `.cpp`), and CI spent
194 s of 225 s compiling. A/B on four scratch copies (same machine, tests built on 4 cores like
the CI runner): precompiled header alone 202 → 65 s, module drop alone 202 → 169 s, both 202 → 53 s.
Adopted both. In-repo: clean app build 92 → 39 s, one `.cpp` 8.7 → 2.1 s, `MainComponent.h`
(7 files) 10.9 → 3.5 s; suites unchanged (unit 748, arranger 705); app starts in 0.7–0.8 s
(unchanged — instruments already load off-thread after login). Dropped: analytics, animation,
audio_utils, box2d, cryptography, javascript, midi_ci, osc, product_unlocking, video — none used,
none a dependency of a kept module. See [[Dependencies Index]].

## 2026-11-21 — Agent rules, docs/ tracked, GPU note rendering, 800-line file cap
Three branches, merged in order (PRs: docs → perf → refactor):
- **`docs/agent-rules-and-docs`** — root `AGENTS.md` (spec → TDD, root cause + measurement first,
  real problems only, minimal diff, no AI attribution, Sonnet review whose claims the orchestrator
  verifies) and `CLAUDE.md` importing it. `/docs` un-ignored (24 specs/plans/test plans now
  versioned). Merged `arhitecture/add-arhitecture-optimize-big-files`; `ARCHITECTURE.md` moved to
  `docs/`. README's stale `Project Synth2.jucer` → `Arranger Workstation.jucer`.
- **`perf/note-layer-gpu`** — benchmark harness (exact copy of the old `NoteLayer` vs 5 variants,
  scripted loads, unlocked re-run) showed the old `juce::Graphics` path held the MessageManager
  lock every frame: Debug ~90 notes 18 fps / UI frozen ≤90 ms; Release ~30 % of a core (NVIDIA
  spin in `SwapBuffers`). New: SDF-shader notes in `renderOpenGL`, component painting off,
  `NoteField` pure model (+41 unit assertions), command queue, `DwmFlush` pacing → Release 1.9 %
  CPU, msg latency p99 0.23 ms, Debug 60 fps. Review fixes: particles under notes again,
  dt-scaled drag, SafePointer made on the message thread. Local integration test reads real
  pixels (8/8). CI unit floor 650 → 730. See [[GL Overlay Rendering]].
- **`refactor/split-large-files`** — 7 files over 800 lines split verbatim by responsibility
  (line-multiset verified, suite totals unchanged: unit 748, arranger 705, integration 137).
  CI now fails on any file > 800 lines. [[ADR-003 800-Line File Cap Split By Responsibility]]
  partly supersedes [[ADR-002 Defer MainComponent Display Refactor]].

## 2026-09-04 — ARCHITECTURE.md added; ADR-002 flags MainComponent/Display for future refactor
Added `ARCHITECTURE.md` at the repo root: subsystem breakdown, MainComponent's
composition-root role (with a real callback-wiring snippet), the two-parallel-engines
mode split (`TrackPlayer` vs `ArrangerEngine`, kept separate deliberately — they're two
app modes, not a legacy duplicate), the arranger's pure-core/impure-shell split, the
threading model, and three Mermaid diagrams (system context, whole-app components,
arranger internals).

Recorded **ADR-002**: `MainComponent.cpp` (~3,083 lines) and `Display.cpp` (~1,174
lines) are flagged as large integration files, but refactoring either is deliberately
deferred — no concrete pain justifies the churn yet. `Styles/CurrentStyleComponent.cpp`
(~1,375 lines) has the same shape and is noted for the same future scrutiny, though not
otherwise addressed. If a refactor does happen: lean on small DI-style coordinator
classes (constructed, handed references) over a wholesale rewrite; explicitly avoid an
event bus, since most of `MainComponent`'s coupling is direct 1:1 calls, not genuine
many-to-many fan-out — a bus would trade a bluntly-readable file for indirection without
solving a real problem. Trigger to revisit: wanting to unit-test the audio+arranger
stack without the GUI, a second host window, or real concurrent work on disjoint areas —
not general file-size discomfort. See [[ADR-002 Defer MainComponent Display Refactor]].

## 2026-08-29 (later) — `ci-test-separation` merged in (`50e2d02`); CI live on the branch; branches pruned
`fix/sfz-integration` now carries everything: the rename, the SFZ popup work, the PC-keyboard fix, **Phase 7a NTT**, the wiki, and **CI itself**. Pushed; the workflow runs on this branch from now on.

- **The merge (`50e2d02`, parents `539dc37` + `e394005`).** Four conflicts:
  - **`Source/Midi/MidiHandler.cpp`** → took this branch's parity rewrite whole and **dropped `suppressInternalFeed`**. The 7a flag kept a muted chord-zone onset off the internal SFZ feed; `5c75d4f` already does that by moving the `incomingMidiMessages` note-on write inside the `!muteChordZone` guard, and goes further by splitting the single trailing `addEvent` into explicit note-on / note-off / passthrough paths. Note-offs still always pass, so the no-stuck-note invariant holds. Merged file is byte-identical to the pre-merge branch version.
  - **`Source/Main.cpp`** → combined: no `--run-tests` path in the production app (CI side) **+** `AppInfo::appName` window title (rename side).
  - **`.gitignore`** → kept `ci/Builds/` + `ci/JuceLibraryCode/`, but **`Piano-Wiki/` stays un-ignored**; re-adding it would have silently stopped tracking new wiki pages.
  - **`Arranger Workstation.jucer`** → rename kept, 74-line Tests group removed (matches the CI branch's own 74-line deletion exactly).
- **`Source/Common/AppInfo.h` added to BOTH `.jucer` file lists** (id `aPp1nF`). It had been in neither since the rename created it — a violation of the maintenance contract the CI README documents.
- 🔑 **A stale test surfaced, and the reason matters more than the fix.** `test_midi_handler.cpp` asserted *"noteOn does NOT notify listener without output device"* — the **exact behaviour `5c75d4f` deliberately changed**, since gating on the `midiOut` lock is what stopped a physical keyboard sounding on the internal SFZ. `startNoteSetting`/`endNoteSetting` default to `-1`, so note 60 sets `ok = 1` and the listener now fires with no device open. The test had been wrong since `5c75d4f` landed but **never ran**, because the runner in `Main.cpp` was commented out on this branch. Updated to assert the intended contract. **Any test touching code changed between `d05e07b` and now may have drifted the same way — the app compiled tests without running them.**
- **Verified locally before committing:** app builds Debug x64 **0 errors**, **zero `test_*.obj`**, `Arranger Workstation.exe` produced. Unit **665/0**, arranger **705/0**, both exit 0 — matching baseline and clearing the CI floors (650/690).
- **Branch pruning — 12 refs deleted, 6 local + 6 remote.** `arranger-engine-phase1-2-3`, `arranger-engine-phase4-5-6`, `arranger-engine-phase4-chords`, `arranger-phase7a-ntt-engine`, `ci-test-separation` (all merged, 0 unique commits), `ci-gate-redcheck` (throwaway red-check, purpose served — its commit was self-labelled for deletion), and `sfz-integration` (2 merge commits, **0** files changed vs merge base). Only `main` and `fix/sfz-integration` remain. Deleting `ci-gate-redcheck` should have auto-closed its PR.
- **Earlier the same day:** `71063de` brought the Piano-Wiki vault into git (it had been gitignored, which is why the `log.md` truncation was unrecoverable); `539dc37` rewrote the README off the "synth" framing — the app is a **sampler**, no oscillators exist in the codebase.

## 2026-08-29 — Archived from the hot cache (previously undocumented in this log)
These were resolved on 2026-08-19 but lived only in `hot.md`, which has been trimmed back toward its ≤500-word budget. Preserved here verbatim in substance.

- ✅ **NOT A BUG — "app is silent on both machines" was chord-zone mute left on.** `ChordZoneMute=1`, `ArrangerModeEnabled=1`, `rightHandBound=-1`. `muteChordZoneNote()` = `chordZoneMute && arrangerEngaged && inChordZone(note)`, and **with no split set `inChordZone()` returns true for the whole keyboard** (`MidiHandler.cpp:1029`) — every key muted, presenting as total silence. Device path was clean. **Possible polish (not done): make the mute a no-op when `rightHandBound == -1`.**
- ✅ **FIXED (`a73d4c8`) — `;` and `'` dead on non-US layouts.** `mapKeyMidi` switched on `key.getKeyCode()`, which JUCE resolves through the *active layout*; under Romanian those keys report 537/539 and fell through to `default: return -1`. Now keyed off **physical scan code** (`pianoKeyScanCodes[]` + `refreshKeyMapIfLayoutChanged()`), plus `releaseAllHeldNotes()` on layout switch. ⚠️ **The real lesson: use `MapVirtualKeyExW`, never unsuffixed `MapVirtualKey`** — the ANSI variant returns `'?'` (63) for out-of-codepage chars, so two offsets collided and `operator[]` silently dropped one. Fixed with the `Ex`+`W` form and `emplace`.
- ✅ **RESOLVED — intermittent `0xC0000374` STATUS_HEAP_CORRUPTION at startup was a stale-object build artifact.** Adding members to `KeyboardListener` changed `sizeof(MainComponent)`, but MSBuild did not recompile `Main.cpp`, whose `.obj` stayed 41 min stale; `new MainComponent()` allocated the OLD size while the ctor wrote at NEW offsets. **Rule: a header change that alters a type's layout needs `/t:Rebuild`.**
- 📝 **Desktop is OneDrive-redirected** to `C:\Users\Oricum\OneDrive\Desktop`, so `File::userDesktopDirectory` writes land there, not `%USERPROFILE%\Desktop`.
- 📝 **Seam note-off tags are vestigial** — the scheduler stamps `PartKind`/`NttType` on synthetic note-offs but nothing reads them; the engine resolves offs via its own `activePlayedNote` map. Do not build on those tags.

> [!warning] Partial reconstruction — 2026-08-29
> This file was truncated to 0 bytes on 2026-08-29 (a write that opened the file in truncating
> mode and then failed mid-encode). It was rebuilt from Obsidian File Recovery snapshots plus
> in-session context. **Entries between 2026-06-25 and 2026-08-15 were NOT recoverable** — the
> File Recovery database only holds snapshots up to 2026-06-25, because later entries were written
> to disk outside Obsidian and were never snapshotted. OneDrive version history had nothing.
> The work from that gap is still summarised in [[hot]] and `.vault-meta/last-sync.json`;
> only the dated log entries are lost.

## 2026-08-29 — App renamed to "Arranger Workstation" (`128cf87`); `ChannelDSP` chain mapped
Session was mostly a LinkedIn/README accuracy pass that turned into a rename and an audio-path audit.

- **`128cf87` — rename.** New `Source/Common/AppInfo.h` (`AppInfo::appName`, header-only and
  dependency-free) is the single source of truth. Removed every hardcoded `"Piano Synth2"` from
  `Source/`: `IOHelper::getAppDataFolder()` now backs the call sites in `Display`,
  `TrackListComponent` and `MidiDevicesDataBase` (several were open-coded
  `userApplicationDataDirectory` lookups, not even going through `IOHelper`). `Main.cpp` passes
  `AppInfo::appName` to `MainWindow` instead of `ProjectInfo::projectName`, so the title bar no
  longer depends on the build system. Projucer project + both target names renamed, and the
  `.jucer` file itself `git mv`'d (git recorded it as a 98% rename). `.gitignore` had the old
  filename listed **twice** — deduped to one entry.
- **Migration deliberately skipped (user's call).** `%APPDATA%` moved with the name and the old
  `Piano Synth2` folder was left behind: settings, `SFZLibrary.json`, `allStyles.json`,
  `mySections.json`, `myTracks.json` and 2 `.style` files (~1.2 MB) are all still there but
  invisible to the app. Consequence to remember: `b5765e9`'s missing-SFZ popup smoke test now
  needs SFZs re-added first. Note the settings *filename* also derives from the app name
  (`options.applicationName`), so any future migration must rename the file, not just the folder.
- **Build hygiene.** Projucer left the old `.sln`/`.vcxproj`/`.filters`/`.user` behind and the user
  rebuilt against them by mistake (exe timestamp 6 min after the resave, still old-named) — the
  *source* changes were in that binary, only artifact naming wasn't. Old project files deleted,
  `.vcxproj.user` (F5 `--run-tests`) ported, and `x64/{Debug,Release}/App` wiped so the next build
  is clean by construction — which also removes the stale-`.obj` path behind the `0xC0000374`
  incident. `.vs/Project Synth2` cache is still locked by a running VS.
- **`ChannelDSP` fully mapped** — see [[Audio & SFZ Playback]]. Serial in-place chain, fixed order,
  neutral-value bypass per stage (and *why* the filter's check differs). Recorded which effects are
  hand-written (tanh distortion + gain compensation, tremolo LFO, circular-buffer delay, smoothed
  random mod) vs JUCE's (`StateVariableTPTFilter`, `Reverb`, `Chorus`), since that distinction
  matters for how the work gets described.
- 🔴 **Bug found, not fixed: the delay bypass freezes the delay line.** `AudioHandler.cpp:181-193`
  keeps the buffer *write* inside `if (delayMix > 0.005f)`, so switching delay off freezes the
  buffer and switching it back on replays stale audio at full mix. Fix options on the module page.
- **Effects reach both engines.** One knob fires `sendMidiCC` (external, its own DSP) *and*
  `injectCC` (internal, `ChannelDSP`). CC7/10/91/92/93 match GM2; **CC80/94/95 diverge** (GM2 reads
  94 as celeste, 95 as phaser), so those three behave differently on external gear.
- **`a73d4c8`** — the PC-keyboard layout fix is now committed (hot.md had it as uncommitted).
- **`README.md` rewritten but left uncommitted** — dropped the "synth" framing (it's a sampler; no
  oscillators exist in the codebase), restructured Features around the arranger, added architecture
  notes and a real roadmap. The `## Building` section was deliberately left untouched so it doesn't
  collide with `ci-test-separation`'s better "Building & Tests" rewrite.

## 2026-08-15 — CI test-separation executed: Tasks 1–6 committed (`2047f7c`…`33142a4`, not pushed)
Ran the CI plan end to end on `ci-test-separation`, rebased first onto `cf423b0` so it sits on the NTT fix. 27/33 plan steps now done; the 6 that remain all need a push or a human. Plan file (`docs/…` — **gitignored**, stays local) updated with a status header and real checkboxes, fixing the stale-tracker problem this session opened with.

- **`2047f7c`** CI Task 1 (rebased). **`438afe3`** Task 3 — `ci/Tests.jucer` + `ci/test_main.cpp` + `.gitignore`. **`919ffa9`** Task 4 — dropped the 34-file Tests group from `Project Synth2.jucer` and deleted the `--run-tests` runner from `Main.cpp`. **`1a726fe`** Task 5 — `.github/workflows/ci.yml`. **`33142a4`** Task 6 — README "Building & Tests" (also replaced boilerplate that referenced a `MyPlugin.jucer` which never existed here).
- **Final verification:** app builds Debug x64 EXIT 0 with **zero** `test_*.obj`; test target builds EXIT 0; unit **665 / 0**, arranger **705 / 0**, both exit 0.
- **CORRECTION — Task 2 was never pending.** The earlier audit called it "the blocker" after reading the `<MODULEPATHS>` block. Those entries are **inert leftovers**; `<MODULE … useGlobalPath="1">` is what governs, and every `juce_*` module in *both* `.jucer` files already had it (`SFZero` correctly local). Verified three ways: Projucer's `defaultJuceModulePath` is set to `Latest Juce\JUCE\modules`; both generated `.vcxproj` resolve there; **zero** files under either `Builds/` or `JuceLibraryCode/` mention the stale `NEW JUCE` path. Task 2's own Step 5 grep is cosmetic — the plan says the build is the real proof.
- ⚠️ **Projucer is a GUI-subsystem exe and does not block.** `& Projucer.exe --resave` returned instantly (`$LASTEXITCODE` came back *empty*), so MSBuild started against a half-written project and compiled the **old** file list: the app reported `EXIT 0` while still containing all 34 test files. Caught only by counting `test_*.obj` (34, timestamped *after* the regenerated `.vcxproj`). Fix = `Start-Process -Wait -NoNewWindow`, now encoded in the workflow, the README and the plan. **Never trust a Projucer-then-build sequence on exit code alone.**
- **The 🖱️ steps were not GUI-only.** `--resave` and `--set-global-search-path` exist in 8.0.13, so Tasks 2–4's manual steps were done headlessly: edit the `.jucer` XML, resave, rebuild. The Tests group was lines 6–79; after deletion the XML validated and GROUP open/close stayed balanced 10/10.
- **Both confirm-on-first-run values verified** rather than trusted: the release asset really is `juce-8.0.13-windows.zip` (checked against the GitHub releases API), and the Projucer flag syntax matches `--help`. Workflow additions beyond the plan: JUCE zip cached by key, Projucer located by search instead of assuming the archive layout, and both suites' results uploaded under distinct names (the plan's single `test-results.txt` would have been overwritten by the second run).

---

> [!missing] Gap: 2026-06-25 → 2026-08-15
> Entries from this period were lost and could not be recovered. Known from other pages, the
> period covered: the Phase 7a NTT engine (`614b4ec`) and its bypass/bass-inversion fix
> (`cf423b0`), the GL overlay crash fix (`cfe100b`), the `channelForHand` parity rewrite
> (`5c75d4f`), and the missing-SFZ popup (`b5765e9`). See [[hot]], [[Chord-Aware NTT Transposition]]
> and [[GL Overlay Rendering]] for the substance.

## 2026-06-25 — Sync: Phase 4 chord recognition + transposition (branch `arranger-engine-phase4-chords`)
Synced the whole Phase 4 feature (25 commits `11687db..0d1d364`, branched off `main`'s `63b6aa7`) plus the still-**uncommitted** keyboard-focus fixes from this session. Phase 4 = the accompaniment now transposes to the chord the player holds, Korg/Yamaha-grade. New pages [[Chord Recognition & Transposition]] (module) + [[Chord-Aware NTT Transposition]] (concept); updated [[Arranger Engine]], [[MIDI Handling]], [[Arranger Style Authoring]], [[Keyboard & Main UI]], [[Arranger Phase Roadmap]] (Phase 4 → ✅), index.
- **Chord value type** (`c0b8276`): `ArrangerChord` (root/quality/bassNote) — renamed from `Chord` (`48e2fef`) to dodge `HelperFunctions::Chord`; `PartKind {Fixed,Acc,Bass}`; `chordIntervals`.
- **ChordDetector** (`ac74b6a`): held-notes → chord via interval templates; Sus2/Sus4 by root==bass; full-keyboard hysteresis (≥3); `detectKeyFromEvents` key-finder.
- **ChordTransposer / NTT** (`2e01684`, corrected in `ba5d420`): **root-shift + third-flip**, NOT snap-to-chord-tone (the first version destroyed the melody — "sounds weird, C doesn't restore original"). Identity preserved. Bass inversion (`6c4ab90`) **re-bases the whole bass line** onto the played slash-bass.
- **Emit-time wiring** (`09c96e1`): scheduler `PartKind` tags + engine `chordLock` mailbox; `dispatchEmitted` transposes per event; `activePlayedNote` map keeps note-off matched to note-on. `setStyle` resets active chord → home key every Start; detector **always re-forwards** (`924393f`).
- **Original chord persistence** (`5be0afa`): style-level `originalRoot/Quality`, schema **v3→v4** (v3 ⇒ C major). Auto-detected at authoring (`34ec155`) + editable root/quality combos (`bf66c76`). Bass track = name contains *bass* (`cebb477`).
- **Input** (`53ab878`, `11687db`): split-zone (`< rightHandBoundSetting`) chord input; `channel` made a per-note local (removed input/UI shared state). Settings toggles — bass inversion / full-keyboard / chord memory (`56b9717`, panel grown `b214b9c`).
- **This session (uncommitted): keyboard-focus fixes.** Root cause: once focus left `MainComponent`, nothing reliably reclaimed it (no `mouseDown` handler; scattered grabs). Added a **global mouse listener** (`addMouseListener(this,true)` + `mouseDown` → grab focus on any non-text click, skipping modals/overlay/TextEditor), plus targeted re-grabs after Play-init and after arranger **Start** (new `onRequestPlayFocus` chain CurrentStyleComponent→Display→MainComponent). Also BPM slider locked during arranger play. Builds clean (MSBuild Debug x64). **Not committed yet** — awaiting user's play-test.
- **Known issues recorded for later** (in [[hot]]): (1) note visuals stay drawn after clicking a style until next play; (2) held notes hit a max length and "freeze" — want a visual sustain cue; (3) holding a chord while pressing Start silences those held notes (likely start/`haltAudio` all-notes-off). Logged in [[Arranger Engine]] too.

## 2026-06-24 — Sync: arranger UX batch committed + merged to `main`; arranger play-start fix (`63b6aa7`)
Caught the wiki up after a long gap. The 2026-06-08 "uncommitted working-tree batch" is now **committed and on `main`** (PR #7 `arranger-engine-phase1` merged via `6c8c6f8`). `main` is the latest branch and HEAD is `63b6aa7`. Synced `1957c0e..63b6aa7`:
- `daf101d` **fix update juce** — JUCE update touch-ups (`Main.cpp`, `MainComponent`, tooltip/tree/style-view).
- `e4ac01c` **Bug fixes + UI for sections edit** — editor **Update Tracks** button (disabled until the config is saved; replaces a saved config's notes with the current recording, keeps sections, overwrites the same `.style` with confirmation via `updateTracksFromRecording` + `onRequestCurrentTracks` wired to `ArrangerSourceBuilder::fromTrackEntries`); new section **lands at the viewport bar** not bar 1 (`ArrangerTimelineComponent::barForX` → `xToSnappedBar`, clamped); `applyLiveTrackVolumes` overlays slider volumes by UUID before Start.
- `8979b6d` **Non-blocking UI edit operations** — the async save/load/update-tracks work (heavy serialize/parse/disk off the UI thread; editor + app loading overlays via the `onBusy` chain).
- `b0dd620` **Update Project Synth2.jucer** — Projucer re-save (file list / header paths).
- `63b6aa7` **starting arranger play fix** (HEAD) — the actual start-on-chosen-section fix. `startPlaying` now calls `engine.selectStartSection(pendingStartType, pendingStartName)` **after `setStyle`, before `start()`** in both the active-config and demo branches (undoes the `stopPlaying(false)` wipe). The host now owns the durable choice (`CurrentStyleComponent::pendingStartType`/`pendingStartName`, default `Variation`/`Variation 1`); the old free function `routeSectionToArranger` became the member `selectOrQueueSection` (playing→`queueSection`, stopped→arm+highlight), plus `highlightPendingStartSection` called on config load / mode-enable / after Stop. `applyActiveConfig` re-arms Var 1 on every load. `ArrangerEngine::indexOfSection` switched from exact-string to **type + trailing-number** match (so "Var 2"→"Variation 2" instead of silently falling back to Var 1).
- Module pages [[Arranger Engine]] and [[Arranger Style Authoring]] already described this batch accurately (written while uncommitted) — no content change needed; only hot.md / log / sync pointer advanced.

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
