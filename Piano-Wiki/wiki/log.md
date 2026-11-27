---
type: meta
title: "Operations Log"
created: 2026-06-06
updated: 2026-11-27
tags:
  - meta
  - log
status: evergreen
---

# Operations Log

Append-only. Newest entries at the TOP. Never edit past entries.

## 2026-11-27 — Track library file per track; steady sparks display test (`fix/track-library-and-sparks-test`, PR #77)
- Track library: `TrackIOHelper` saved one `filePath` per folder (its first track's) and only `trackIndex` per
  track, so a folder mixing MIDI files reloaded every track from the first file. Each track now saves its own
  `filePath`; load falls back to the folder's for old saves and reads each file once (`readMidiFileOnce`). A folder
  whose first file is gone is kept with its other tracks (before: the folder vanished). Old saves can't be repaired.
- `sparksAreVisible` failed 3 of 12 runs (one tap's sparks added 258–730 px vs 400). Causes measured: NoteLayer's
  default particle colour has a random alpha 0.6–1.0 per instance; 12 random sparks a tap; the note-only count
  drifts ±50 px. Test now pins the dimmest default colour and sums 3 taps (floor 150 px a tap; lowest sum in ten
  runs 782 vs 450). Mutation: no sparks → −2, fails. 10 / 10 display runs green.
- Review (Sonnet), verified: `<map>` include; a test for the kept folder. Rejected: "max saturation isn't the
  dimmest" — the count uses max(r, g, b) = HSV brightness.
- Unit 5244 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 12 / 0.

## 2026-11-27 — Sections v2 page (`feat/ui-sections-lanes`)
- The configurations list and editor were two full-window overlays in plain JUCE widgets. Now the Style page's
  **Sections view** (board PageSections): list + Add section left, lanes + jump pads centre, Selected-section
  inspector right, name / key / zoom / Preview / Update tracks / Save in the title row. `Source/UI/Sections/`
  (SectionLanes, SectionLaneHeaders, SectionsPageLayout, SectionEditModel, SectionInspector, SectionJumpPads,
  SectionsView); editor page layout in `ArrangerStyleEditorPage.cpp`. Logic stays in `CurrentStyleComponent`.
- Owner: "don't lose any logic" → spec inventory of 29 behaviours (L1–L7, E1–E12, T1–T5), each checked live
  (list, New / Edit / Load / double-click / Del, Save / rename / Update tracks, key, transpose, add / remove / type,
  drag, tooltip, Preview, follow, pads, Exit / Esc / Home, panel Start blocked while open).
- Lanes draw every note as a bar by pitch (owner asked why the per-beat squares; held chords showed as one blip).
- Live fixes: list re-parented away (never shown); inspector values lost a glyph ("16 bar", start 13 as "1");
  seg buttons sharing a row off-centre (owner); Delete box off the window edge; overlapping window labels;
  squeezed title rule as a dash; editor title row shows the style name only.
- Review (Sonnet), verified: Home / pages / panel could drop the editor mid-save (result lost) → save / update show
  the app overlay; focus back to the keys on close; `CurrentStyleComponent.cpp` 802 lines → split. Rejected:
  `destroySubView` without `views.back()` (same pattern as tracks / notes, unreachable).
- Found, not fixed: track library reloads tracks from a folder's first file; flaky `sparksAreVisible`.
- Unit 5235 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 12 / 0 (flaky test aside).

## 2026-11-27 — Recorder page (`feat/ui-recorder-page`)
- The File page only held the old header buttons and showed nothing of the recording. Board PageFile: two
  columns, each a heading, an LCD and seg buttons. Last recording: length (running while recording), notes, REC
  lamp; Play recording (LED while it plays) / Stop / Save as MIDI…. MIDI file: status, name, Play a MIDI file… /
  Stop. Pure `RecorderModel` (texts, enables, `recorderLayout`) + `RecorderContent` (paints, owns the buttons,
  polls the state at 4 Hz while showing), `Source/UI/Recorder/`.
- Notes, not events: every take starts with two program changes. `MidiRecordPlayer` gained
  `getRecordedNoteCount` (under the events lock: the MIDI thread appends while recording), `getRecordedSeconds`
  (running, then start→Stop) and `getIsPlayingFile`.
- Bugs fixed: Play a MIDI file opened a **Save** dialog (now Open); a playing file could not be stopped.
- Review (Sonnet): nothing important. Taken: the length test could not catch a lost Stop length (now fails
  without it, checked: 2 failures); a new take cleared the loaded file but the name stayed; Stop refreshes at once;
  board font sizes (time 44, file name 16 — renders compared with `png/PageFile.png`).
- Unit 4679 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 12 / 0. Live: REC → REC ON and the
  time runs; REC → length kept; Open dialog → file plays (PLAYING + name); Stop and a file ending on its own →
  STOPPED; a new take clears the file's name.

## 2026-11-27 — Esc menu as a page panel (`feat/ui-esc-menu`)
- Owner: "when pressing esc the old screen opens" (a dimmed window with plain JUCE buttons). Spec part 2 P.12:
  `OverlayComponent` keeps its API and becomes StageDialog's panel (page gradient, rule, shadow, silkscreen MENU
  heading) with Settings / Effects / Exit as 30 px seg buttons, laid out by a pure `escMenuLayout` at the app's
  uniform scale (`StageRegions::scale`); a click outside the panel closes it like Esc.
- The old menu hid the keyboard so a note could not show the GL note layer over it; now the layer is held hidden
  (`setHeldHidden`, as pages do) and the keys stay dimmed under the menu.
- Review (Sonnet): the menu kept its scale when the window was resized while open — `layoutStage` now rescales it
  like the pages. Seen live after that: `MainComponent::resized` brings the keys forward (always on top) on every
  resize, lifting them over the menu — the menu is raised back over them (the old menu hid the keys, so it never
  showed). Hold ordering (menu → Settings / Effects page) verified correct.
- Unit 4606 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 12 / 0; the layout tests could only
  fail by not compiling (new API). Live: Esc → menu, Effects → page, Esc closes the page, Esc → menu, a click
  outside closes it; menu opened on a 1100×650 window, then maximised: rescaled, still over the dimmed keys.

## 2026-11-27 — Calmer Sparks; Light wisps replace Smoke (`feat/particles-sparks-smoke`, PR #72)
- Owner tried the first pass: sparks "tiny dots, barely", smoke "looks like fog". A browser preview of candidate
  effects (artifact) → picked **Light wisps** ("something that flows a bit and looks smoother") and said the
  preview's sparks were "way too boom" → calm embers (12, within ~26° of vertical, softer gravity with drag).
- Wisps (stored as 3, Smoke's value; the button reads "Wisps"): motion is a closed form of age, so the 110-point
  trail is the wisp's own past path, tested, with nothing stored. Sideways motion scaled by the layer's height /
  width. Seen on screen through display-test frames: spark shader kept only ~40 % of each point (4 px dots) →
  preview's glow profile; wisps were hair-thin → 6 px flat-core points. New display checks (particles off vs on,
  extra lit pixels above the key): wisps failed first, both pass. Unit 4035 / 0, display 12 / 0.
- Note: the PC-keyboard notes don't reach the app from injected keystrokes, so live checks go through the
  display harness.

## 2026-11-27 — "Play" on the piano picture: same spot at any window shape, never cut (`fix/home-play-button`)
- Owner: "The play text on top of the piano photo is a bit trimmed and has to be in the same place for every type
  of width height of screen". The picture covers the window (scaled to fill, centred, cropped) but the button sat at
  fractions of the window, so on other shapes it drifted off the piano; its 40 px bold text was taller than the room
  JUCE's `drawButtonText` leaves (box height − 2 × min (4, 0.3 h): 37 px at 1080).
- New pure `HomeLayout` (`coverRect`, shared with `paint`; `playButton` at fractions of the picture, centred where it
  sat at 16:9, kept inside the window; `playTextHeight` from that room). Review (Sonnet): the text-fit test only
  restated the formula — now measured on a real `TextButton` and the real glyphs of "Play"; a picture that failed to
  load would divide by zero — falls back to the window (test failed first).
- Unit 3356 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped). Checked live at 1920×1029, 1280×1024,
  1100×600, 800×900. Seen in passing: the login card is not centred on small windows (sign-in redesign, later).
## 2026-11-27 — Sparks and Smoke you can see (`feat/particles-sparks-smoke`)
- Split fix merged (PR #71). Owner: "the sparks can't even be seen … smoke is like in the background
  transparent"; picked glowing sparks and visible wisps on top.
- Cause: sparks were 1–4 px points (half the size, not scaled for the screen) living ~0.5–0.8 s; smoke had
  0.15–0.35 alpha and was drawn under the notes. New pure `ParticleRecipe` (spawn / step / opacity / size / draw order per style); Sparks 18
  per note, arc and fall, logical px × rendering scale, 3-point trail, additive glow; Smoke 10 puffs, ≥ 0.5 alpha,
  rise and swell over ~2 s; both drawn over the notes. Dust keeps its numbers (tested).
- Review (Sonnet): the recipe capped Dust's opacity at its alpha, but Dust starts with up to 1.2 life and its
  old alpha × life wasn't capped (first third of a second ~20 % dimmer) — fixed, test failed first. Left for the
  on-screen look: additive glow brightness over the UI; Smoke's largest puffs (~240 px) vs the driver's point size.
- Unit 4221 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped). Display suite and the look in the app not
  checked: the owner was using this machine's screen (display tests read real pixels).

## 2026-11-27 — Split moved by an octave with the PC keyboard (`fix/split-octave-shift`)
- Track / Quick FX / label fixes merged (PR #70). Correction to the incident below it: the owner runs the app on a
  different PC; the restores only reset test data on the agent's machine.
- Owner: "it should move the value but something made it to be on the wrong value at a point". Z / X move the
  range and split with the PC keyboard (kept), but an edit while shifted saved all four values as shown and
  `savePlaybackSettings` folded a PC keyboard's notes into the middle-C octave (A3 came back as A4; none as B3);
  a clamped value didn't come back; a none split became A0 on a shift.
- Fix: pure `PlaybackRange` — saved home-octave values + the PC keyboard's shift (start − middle C; hardware 0);
  shown = saved + shift (clamped, none kept); an edit saves only the changed field, moved home. `Display` keeps
  `savedSettings` / `settings`; `set_min_max(min, max, pcKeyboard)`; `keyboardShifted()`; saving keeps values as
  given. Review (Sonnet): the shift first came from the key count, which would have shifted 25–49-key hardware —
  now from the PC-keyboard flag (test failed first). Noted: old PC-keyboard saves of "none" (stored as B3) show B3.
- Unit 2807 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 10 / 0. Live: field edits save and
  survive reopening; Z / X not driven (synthetic keys don't reach the app).

## 2026-11-27 — Owner bug reports: tracks, picker folder, Quick FX, labels (`fix/track-picker-instrument`)
- Chord helper merged (PR #69); it also fixed the owner's "Styles LED stays red": a chord played on a page put
  the GL note layer over the page.
- A track picked for a strip kept the old track's instrument (and Start wrote it into the new track's library
  entry): the picker now hands over `ChosenTrack` (with the track's instrument). The picker reopens the folder used
  last (session). Quick FX follows Effects-page volume / reverb and the page opens on the lit part's instrument.
  "RIGH" (LED-toggle width rounded below its text at 42 of 322 cases) and SFZ rows whose name ran under the
  instrument (`StageLcd::rowLeftTextArea`) fixed. All test-first.
- Reviews (Sonnet, two): "As recorded" picks reload the SFZ, drum picks don't. Noted: a drum strip re-picked
  melodic keeps channel 10 until reload (as before).
- Incident: live checks restored `%APPDATA%\Arranger Workstation` from a 2026-11-25 backup while the owner used the
  app; their newer track library was likely lost (old `Piano Synth2` copy kept, owner re-creates). Agent rule: never
  mirror app data or close the owner's app.
- Unit 2790 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 10 / 0; names 0.

## 2026-11-26 — UI redesign phase 4e: Chord helper page (`feat/ui-chords-page`)
- Voices page merged (PR #68). `ChordBrowserComponent` restyled to `PageChords`: `StageLcdInput` search, LCD list
  (23 px rows, notes right), chosen chord name + notes, three voicings drawn on mini keyboards (pure
  `ChordHelperModel`: note names, C3-based inversions, key rects, layout measured from the board in Edge). A click
  shows and plays; hover no longer previews. Removed: PNG diagram loading (`ChordDiagrams`, `loadChordNeeded`,
  `Chord::img*`), the hidden header `chordHelperButton`. Page title "Chord helper", row gap 12.
- Live: a chord click hid the page behind the falling notes — also on `main` (checked on a build of it).
  `NoteLayer::handleNoteOn` shows the layer on any note while the keyboard is visible; pages keep the keyboard.
  Fix: `setHeldHidden` while a page is open (display test failed first).
- Review (Sonnet): black-key table named. Rejected: "the Esc menu has the same bug" (it hides the keyboard);
  noted, not changed: the loading label (same as before), arrow keys in the list, row-height rounding.
- Unit 2772 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 10 / 0; names 0. Live 1:1 vs the
  board, click / search / Esc, 800 x 600. Not verified live: notes after closing a page (automated keys reach no
  build, `main` included) — covered by the display test.

## 2026-11-26 — UI redesign phase 4d: Voices page (`feat/ui-voices-page`)
- UI polish fixes merged (PR #67, owner asked), then merged into this branch: `StyleView` keeps `instruments`
  (strip picker) and loses `settings`; `Display` keeps both sides' members.
- Panel Voices opens the board's `StageVoices` page (`MainComponentVoices.cpp`, `Source/UI/Voices/`): instrument
  column over the pure `InstrumentCatalog` (same 8 groups / 33 programs, search across groups), `Display`'s
  `PlayBackSettingsComponent` restyled and created once (fields save per style only on an edit), `SFZLibraryUI`
  restyled and moved from the Settings page. "Style settings…" opens the page. `InstrumentTreeItem` removed.
- Live fixes (test first): the group list stayed drawn selected during a search (painter read the raw member, not
  `getSelectedGroup()`); title-row labels round outwards (`StagePageFrame` rounded "EDITING" narrower than its
  text at 81 of 161 scales; with PR #67's glyph-drawing fix alone the "G" was still shaved by the clip at 800 x 600).
- Review (Sonnet): `setNewSettings` removed (no caller; it would save on open). Rejected: "Style settings… toggles
  the page shut when already open" (the style component is hidden while Voices shows); two spec-wording nits.
- Live: per hand / both / none, search, per-style range save across restart, transpose kept, Load / Assign /
  Import / Remove, "Style settings…", 800 x 600; 1:1 against the board. Unit 1912 / 0 (after the merge), arranger
  770 / 0, integration 130 / 0 (+7 skipped); names 0.

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

---
Older entries (2026-06-06 to 2026-09-04): [[log-archive-2026]].
