---
type: meta
title: "Operations Log"
created: 2026-06-06
updated: 2026-11-28
tags:
  - meta
  - log
status: evergreen
---

# Operations Log

Append-only. Newest entries at the TOP. Never edit past entries.

## 2026-11-28 — Pressed keys are lit amber (`feat/amber-pressed-keys`)
- Owner: "you barely see what notes you pressed". Measured on the rendered keyboard: a pressed key differed from a
  resting one by about 32 RGB steps (white) and 11 (black). The owner picked the amber-lit key from three options.
- Only the `StagePalette` pressed colours changed (white `#e89a4a` → `#ffbf73` → `#ffc985` → `#f5b566`, lip
  `#d8903f`; black `#7a4510` → `#b56a1f`). The drawing and the pushed-down shade are unchanged.
- Test: the pressed white and the new pressed black must each be amber (red above blue by more than 60) and more than 100 RGB steps from the resting key.
- unit 5968 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — Background work no longer outlives its owner (`fix/detached-load-threads`)
- An audit of the 9 detached-thread sites found three real hazards. The others capture SafePointers, weak
  tokens or values.
- `AudioHandler::loadSfz`: detached loads wrote into the handler's synths. A new test runs 16 loads and then destroys
  the handler, and the old code crashed the runner with an access violation. Now an owned `ThreadPool` that the
  destructor drains, plus a weak alive token for the completion message.
- The `MainComponentActions` device upload holds its own `shared_ptr` to the client, not `this`.
- `KeyboardUI` note on/off (MIDI thread → `callAsync`) holds a `SafePointer` (found by the Sonnet review).
- unit 5963 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — CC93 chorus is a bucket-brigade model (`feat/bbd-chorus`)
- `BbdChorus` (port of `va-effects`' BbdChorus, Juno settings) replaces `juce::dsp::Chorus`: the reconstruction
  corner follows the delay (slower clock, darker), a 2:1 / 1:2 compander and a -78 dB noise floor.
- CPU (Release): 116 ns per stereo sample against 200 for the original. Two changes: filter coefficients every
  16 samples, and sqrt in place of pow. 16 parts with the chorus on = 8.9 % of one core.
- Opening on a sounding note measured a 0.057 step (threshold 0.05): fixed with a 12 ms dry warm-up and detectors
  that start neutral.
- Sonnet review: faithful to the original. Confirmed: no denormal flushing anywhere (exact silence: 111 → 152 ns), so the callback
  now holds `juce::ScopedNoDenormals`; plus a stale comment, a re-open test.
- unit 5962 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — CC94 delay time glides (`fix/delay-fractional-read`)
- The delay read whole samples and moved its read head at once when CC94 changed the time: the echo stepped (a click;
  a new test measured a 0.231 step on a 0.5 sine, threshold 0.05).
- `FractionalDelayLine` (header-only port of `va-effects`' `DelayLine`, Lagrange-3: measured there below -200 dB
  under modulation, where linear pumps the treble and allpass mistunes). `ChannelDSP` writes then reads at
  `delayTimeSamples`, which glides to the CC target at most 0.5 sample per sample (double; lands exactly).
- Sonnet review: no correctness bugs; fixed the float smoother stalling short of the target and the first glide reading
  backwards (exponential 150 ms started at 1.1-2.3 samples per sample), and made the test's guard run the real line.
- unit 5948 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — Every AlertWindow → StageAlert (`feat/stage-alerts`)
- `StageAlert` (new): `showMessage` (OK only; Enter / Esc / click outside = OK), `showQuestion`, `showChoice` (a second
  action, for the notes table's Change All / Change selected), `showInput`. One at a time app-wide (a replaced one is
  freed later, never under its own callback); hops to the message thread; hosts on the main scene (an anchor in
  another window, e.g. Add Device, stays there). `StageDialog` gained `hasCancel`, the alternative button and `onClosed`.
- Converted: 48 `showMessageBoxAsync` (script, icons dropped), 3 `showOkCancelBox`, the key-count and table-value
  inputs, the table's 2-3 button choices (ids via `TableDialog::resultIdFor`, as before).
- Live: a notice opened while Play builds the scene was first hidden behind the GL note layer (the "Preparing session"
  overlay restores it); every place that shows the note layer again now calls `keepNoteLayerUnderAlert`, and the focus
  returns after the closing key (deferred: JUCE moves focus off the hidden dialog after the hook). Esc works after.
- Sonnet review, verified and fixed: Add Device's notices went into the small dialog window; page-hosted alerts could
  strand when the page hid (now always the main scene; `current()` ignores hidden ancestors); the uncover could show the
  note layer over a page / menu / loading overlay; the table's second question lost its "Modify …" title; chained
  alerts kept focus inside the dead dialog; SafePointers in the style editor's questions.
- unit 5943 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — Audio thread: no allocation, no waiting (`fix/audio-thread-realtime`)
- The audio callback allocated every block with MIDI: a local `MidiBuffer`, then a fresh one per active channel (and
  scanned the block 16 times); the no-SFZ notice posted `callAsync` (allocates a message) from the audio thread.
  `getNextMidiBlock` took `midiMutex` with a blocking lock while the MIDI thread holds it across MIDI-out sends and
  listener calls.
- Now: `MidiChannelSplitter` (pure) — 16 buffers reserved off the audio thread, one pass, SysEx skipped unread (a
  long message copy would allocate); `incomingMidi` reserved too; the notice is an atomic mask read by a 250 ms
  message-thread timer (`reportChannelsWithoutSfz`); `ScopedTryLock` — if the MIDI thread holds the queue, its events
  go out one block later instead of the audio thread waiting.
- Tests (12 new): routing, storage never reallocated over 50 full blocks, the audio side returns while a parked MIDI
  thread holds the lock (failed before the fix), the queued note arrives next block, the no-SFZ report fires once.
- Sonnet review, verified: SysEx copies could still allocate (fixed), a failing test run could leave a thread parked
  on stack references (fixed). Accepted: a busy MIDI thread delays notes by one buffer.
- unit 5915 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped).

## 2026-11-28 — Branches pruned; merging deletes the branch
- Deleted 41 merged branches on GitHub, the merged local ones, `refactor/names-playback-2` (its patches are in
  `main`), the stale `origin-pr/*` refs and a clean agent worktree. Left: `main`, and `feat/particles-sparks-smoke`
  (checked out with uncommitted changes in the owner's main folder).
- Owner rule, now in AGENTS.md › Git: merging a PR deletes its branch (`gh pr merge --delete-branch` + local).

## 2026-11-28 — Sign-in part 2 (`feat/ui-sign-in`)
- Log in remembers the last email (`RememberedEmail`, settings key `LastEmail`, never the password): "Remembered from
  last time. *Not you?*" (`StageTextLink`) while the field holds it. Home: "Signed in as …" + Log out (back to Log in,
  tokens forgotten, email kept). No "logged in" box.
- Pure `BackendJson`: request bodies via `juce::JSON` (a `"` in a password / device name broke them; ordinary values
  byte-identical) and the login / sign-up reply readers (old inline decisions, unchanged). `signup()` now reads the
  reply — before, every sign-up showed "Invalid server response".
- **Login still lets anyone in.** Live, a login attempt gave "connection failed": `ecmlftmkoqszdwjugqtn.supabase.co`
  does not resolve (DNS: non-existent domain) while supabase.co does. Owner chose to keep the bypass
  (`checkAccountWithServer = false`) until the backend is back. Remember me hidden, Forgot password? unchanged.
- Sonnet review, verified: Log in with nothing typed (the bypass) left Home without Log out and wiped the remembered email — fixed. Rejected: non-ASCII escaping (juce::JSON writes UTF-8 by default). Live: Log in → Signed in row → Log out → remembered row → Not you?.
- unit 5901 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped); display not run (owner using the screen).

## 2026-11-27 — Log in and Home on the instrument (`feat/ui-front-screens`, redesign sign-in part 1)
- Before: a forest picture with a centred login card, then a "Play" text on the picture. Now (boards StageLogin /
  StageHome): the panel is always shown — on Log in / Home its screen is asleep ("Welcome" / "Ready", lamps dark) and
  its controls inactive; the page area holds the restyled `LoginComponent` (LCD fields, Show, lit Log in, Sign up,
  Forgot password?; Sign up mode: Username, Create account, Back to log in; Enter submits) or `StageHomePage`
  (Settings · Play · Exit big keys); `frontKeys` (a drawn-only `KeyboardUI`) sits under them. `MainComponentFront.cpp`.
- What the account buttons do is unchanged (Login still always succeeds — next PR). `inPlayView` replaces two checks
  of `!playButton.isVisible()` (they were also true before login).
- Layouts in pure `FrontScreensLayout`, checked against the boards' renders (Play key top 124.4 vs 125, email field
  97.4 vs 97.6, Log in row 258.6 vs 258.9 board px). Live: login → Home → Play → HOME → Home; Settings from Home and
  Esc back; Esc menu on Home.
- Sonnet review, verified: Esc was swallowed by a focused login field (now passed on), focus lost after login (Home grabs it), two async callbacks without a null check, Show state kept between Log in / Sign up, a misplaced colour comment — all fixed. Not changed: the unreachable login body (next PR), the hidden playButton kept as the Play relay.
- unit 5861 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped); display 13 / 0 before the review fixes (one GL pixel read failed on the rerun while the desktop was in use).

## 2026-11-27 — Panel finishes (`feat/ui-finishes`, redesign phase 5, board `SkinsFinish`)
- Settings › Display › Finish: Stage (default) and Rosso burst, Champagne, Tobacco burst, Midnight flake, Concert
  black; saved as `PanelFinish`. Panel surface + key slip + side strips; controls, screen and keys unchanged.
- `FractalNoise` = SVG 1.1 feTurbulence reference code. Oracle: Edge rendering the same filters into a canvas — raw
  noise within 7/255 (fractal) / 19 (turbulence) premultiplied, another seed 40+ off; texture coverage matches (flake
  lit 16.5 % vs Edge 17.1 %, flame / brush / rosewood means within a few /255). The board PNGs never showed the noise.
- Render (Debug, 1920 × 198): 118–250 ms per finish → cached per size, re-rendered 150 ms after a resize settles.
- Labels: light silkscreen measured unreadable on Champagne and on Tobacco's bright middle (1.8–2.7 : 1). One ink
  per printed column from its average background, a label switching only below 3:1 → every label ≥ 3.05 : 1 (test
  over 11 label spots per finish). Agent call: side strips 16 px (26 would cover the first LEDs at x = 20).
- Review (Sonnet), verified and fixed: a scale change at the same pixel size kept the old texture (cache keyed by
  scale too); Concert's pinstripes would print on the 12 px slip (slip = bare surface); the test copied the
  column choice (now `PanelFinish::inkColumnAt`, used by both). Not changed: straight vs premultiplied gradient
  blending (slightly lighter mixes), ink averaging per paint (tenths of a ms).
- Live at 1920 × 1080 (125 %): each finish switched, restart keeps it, set back to Stage afterwards.
- Unit 5825 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 13 / 0.

## 2026-11-27 — New smooth Dust (`feat/ui-dust`, redesign phase 4, board `Dust`)
- Old Dust: 40 particles once per note-on, a new random sideways kick every frame (jitter), linear fade, fixed
  size. New pure `DustField` = the board's "new" mode: burst of 26 (rise × 1.8), then 60 / s per held key; 86 %
  specks / 14 % motes with the board's life / size / alpha ranges; two-wave sine flow field, damping e^(−1.4 dt), lift
  16 (1 − p); fade smoothstep in (0–.12) and out (.3–1); ease-out growth; ≤ 3200, oldest first. Board units (1200
  wide) → NDC; additive point sprites with the board's radial gradient; still under the notes.
- `NoteLayer` render thread keeps which keys are held (noteOn / noteOff / releaseAll / clear) and streams Dust for
  them each frame. `ParticleRecipe` lost its Dust branch (Sparks / Wisps unchanged).
- Tests: `test_dust_field` (burst, 60 / s, kinds and ranges, fade / growth curve, one frame = the board's formulas,
  budget, scale); display `dustIsVisible` (3 taps lit 610–786 px in 6 runs, floor 300; emission off → 40, fails).
- Review (Sonnet), verified and fixed: switching Sparks → Dust left live sparks never ageing (layer animating
  forever) → recipe particles dropped while Dust is the style; a note outside the piano burst Dust at x = 0 → guarded;
  tests pinned to the board's numbers. Not changed: a held key's stream keeps its old x after a resize (low).
- Live: chord C held 1.2 s next to the board's own script rendered the same way; this screen runs at **125 %**
  (positions × 1.25), which also showed the cached keys stay crisp when scaled.
- Unit 5788 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 13 / 0.

## 2026-11-27 — Realistic keys (`feat/ui-keys`, redesign phase 3)
- `KeyboardUI` drew flat white rectangles with black outlines and a **green** pressed key. Now the board
  `StageRevised` keys: pure `PianoKeyShapes` geometry, board materials (`StagePalette` "Keys"), pressed white
  darker with a shaded top and a short lip, pressed black darker with a shorter slope; only white keys outside the
  device range dimmed and note names as before. Draw order fixed: bed, whites, black shadows, blacks.
- Paint at 1920 × 224 (Debug): 1.9 ms before; 8.75 ms drawing the materials every paint → each look cached once
  per size as an image: 1.3 ms. Edges snapped to whole pixels so the gaps stay the bed colour.
- Review (Sonnet), verified: a white key's repaint area (its step, `bounds`) could end 1 px before its drawn edge —
  reproduced, 10 keys at 1075 px left a stale column; `areaOf` = bounds ∪ shape, test compares every press with
  the released render at 1075 / 1366 / 1920. Added: all-88 shape order test. Rejected: "cached bitmaps blur under
  display scaling" — a 2× render with logical-size images stays sharp in JUCE's renderer (test passed with the
  change forced off), so no change; shadow width ±1 px on some black keys (inside a 4 px blur); raw `this` in
  `callAsync` and `keys[]` insert outside 21..108 are pre-existing.
- Live 1:1 at 1920 × 1080 vs the board (side-by-side crop); pressed looks checked on a rendered image (computer
  keyboard input is off in this machine's settings, left unchanged).
- Unit 5510 / 0, arranger 770 / 0, integration 130 / 0 (+7 skipped), display 12 / 0.

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
