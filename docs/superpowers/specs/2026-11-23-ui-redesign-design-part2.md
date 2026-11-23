# Arranger UI redesign — part 2: pages and integration map

Part 1: `2026-11-23-ui-redesign-design.md` (rules §0, design system §3, layout §4, screen §5).
Every page below has its board in `docs/superpowers/specs/assets/2026-11-23-ui-redesign/`; the board wins
over this text. Example values on the boards (track names, folders, notes, SFZ file names, emails) are
placeholders: the app shows real data.

All pages share the page frame of part 1 §6: page surface, title row (silkscreen title · rule · actions ·
**Exit**), size = the note area, GL `NoteLayer` hidden while open.

---

## P.1 Effects (board `StageEffects`)

- Title row: **Effects** · rule · *Instrument* **1st / 2nd** (= today's channel selector "First / Second
  instrument", left-hand / right-hand channel) · **Sustain** (LED toggle = today's sustain pedal toggle,
  CC64) · **Exit**.
- Four groups in a 2 × 2 grid, four effects-size knobs each, value under the knob (0–127):
  **Tone:** Brightness, Expression, Chorus, Resonance · **Envelope:** Attack, Decay, Release, Vibrato ·
  **Space:** Volume, Reverb, Delay, Pan · **Modulation:** Distortion, Filter, Tremolo, Random mod.
- Each knob sends the same controller as today (`EffectControllers::*` in `SoundEffectWindowComponent`),
  to the selected channel, and shows the stored value when the channel changes (as today).
- Replaces the Sound Effects window (`SoundEffectWindowComponent`); the Esc menu's "Effects" opens this page.

## P.2 Settings (board `StageSettings`)

Four columns:

- **MIDI:** Output engine (External MIDI / Internal SFZ synth) · MIDI in · MIDI out · Audio out (only
  meaningful for the internal synth, as today) · **Keyboard** (key count 49 / 61 / 76 / 88 of the selected
  device = today's "Update number of keys" + device list) · **Add device…** (today's Add MIDI Device window).
- **Arranger** (LED toggles, same settings keys as today): Arranger mode · Bass inversion · Chord memory ·
  Synchro start · Count-in · Mute chord keys · Chord transposition (NTT).
- **Chords:** Chord mode Fingered / 1-Finger / Full · Minor scale Dorian / Aeolian / Harmonic.
- **Display:** Particles (on/off = today's "Toggle particles") · Note names on keys (= "Note annotation") ·
  Particle colours… (today's colour picker) · **Particle style** Sparks / Dust / Smoke (new control, B9
  below) · **Finish** (part 1 §9).
- Fields step with ◂ ▸ and open a list on click (devices can be many).
- Replaces the MIDI Settings window (`SettingsWindow`) and the header toggles.

**B9 (added to the list of deliberate changes):** today `NoteLayer::setParticleStyle` exists but nothing
calls it, so the app always uses Dust. The Particle style selector exposes it and saves the choice (new
key `ParticleStyle`).

## P.3 Style (board `StageStyles`)

- Title row: **Style** · rule · *Play* **All tracks / Solo** (= today's play-mode dropdown) ·
  **Sections…** (opens P.6) · **Style settings…** (opens P.7 at the "Range, split & transpose" block, which
  is today's playback-settings tab) · **Exit**.
- **Left, Styles:** LCD list of styles, the open one marked; **Add / Rename / Delete** buttons (B8). The
  right-click menu on a style stays. DEFAULT Style cannot be renamed or deleted (as today).
- **Right, 8 track strips:** number · track name on an LCD strip (**click = track picker P.4**) ·
  instrument button (**click = today's instrument chooser** for that track) · fader with value (= track
  volume) · **Notes** (P.5). The right-click menu on the name (Rename, Delete, Copy, Paste, Notes
  information) stays.
- Percussion tracks keep today's rule (instrument change has no effect; the same message shows on the
  screen).
- Replaces the style tab (`CurrentStyleComponent` UI) and the styles list tab; the vertical tab strip goes.
  Opening a style = selecting it in this list (today: clicking it in the styles tab).

## P.4 Track picker (board `PageTracks`)

Opened by clicking a track name on the Style page; title "Choose track · slot N".

- **Folder view** (first): LCD list of folders with a folder glyph and the track count; **Open folder**
  (and double-click) · **Add folder… / Rename… / Remove** (today's folder buttons and dialogs).
- **Track view:** "◂ Back" + path "Folders › name"; rows "name (instrument)" with **MEL / PERC**;
  **Use in slot N** (and double-click, as today) · **Add from MIDI file… / Rename… / Remove**.
- **Sort:** Name / Newest / Oldest (today's three sort options).
- Right side shows the selected folder (track counts) or track (name, instrument, type, folder).
- Logic stays in `TrackListComponent` (folders, grouping, file import, rename / remove dialogs).
  Fix while here: the rename dialog focuses the wrong editor ("folderName" instead of "trackName",
  backlog section 2).

## P.5 Notes information (board `PageNotes`)

Opened from a track strip's **Notes** button or the right-click "Notes information".

- Title "Notes · <track name>", sub-line "Track n · instrument · N notes", **Undo** (= "Undo settings"),
  **Exit**.
- LCD table: # · Note · Time · Velocity (number + bar) · Channel; selected row inverted.
- **Edit selected note:** Change / Apply (today's buttons). **Whole track:** Shift time stamps… / Set
  velocities… (today's modify dialogs). **Back to recorded:** Notes / Time stamps / Velocities / Everything
  (today's four "…to default" actions).
- Logic stays in `TableContainer` / `MidiNotesTableModel`. Fix while here: with no rows selected, Cancel
  in the "Change All" dialog runs the action. The wrong button id is in `TableContainer::changeAllUI()`
  (`CustomTableContainer.cpp:536`); the modify dialogs' Cancel is already correct (backlog section 2).

## P.6 Sections editor v2 (board `PageSections`)

The editor cuts sections out of the style's recording. A section = `SectionWindow` {name, type, start bar,
length, after-complete} over the recorded tracks (`ArrangerStyleFile.h:26`).

- **Title row:** Sections · "Style n · cut sections out of your recording" · Recorded key (root + quality,
  today's two boxes) · Zoom − / + · **Preview / Stop** · **Update tracks** · **Save** · Exit.
- **Left:** Configurations list (the active one marked; New / Edit / Delete = today's saved-configurations
  list) and **Add section**: + Intro, + Var, + Fill, + Break, + Ending, + Count-in (`ArrangerSectionType`
  includes CountIn).
- **Centre, lanes:** one lane per recorded track (8), lane header = track name + **transpose mode button**
  (None / Parallel / Chord / Fixed, today's "NTT per track"; click cycles). The recording's notes are drawn
  in the lanes (a density view, not editable). Bar ruler with numbers, a stronger line every 4 bars.
  Horizontal scroll when zoomed.
- **Section windows** drawn over the lanes in the type colour (part 1 §3.1): label "name · n bars";
  selected = brighter fill, light outline, grips on both edges. **Drag the body to move, drag an edge to
  resize, snapping to bars**; a click selects and, while previewing, jumps there. This is today's
  `ArrangerTimelineComponent` behaviour (including "a bare click is not an edit") in the new look.
- **Playhead** (`#ffa63a`) while previewing, from today's `setPlayheadBar`.
- **Jump pads** under the lanes, one per section (LED lit = playing).
- **Selected section** inspector: Name · Type ◂ ▸ · Start bar ◂ ▸ · Length ◂ ▸ · **When it ends: Loop /
  Go on / Stop** (= `ArrangerAfterComplete` Loop / FallThrough / Stop) · Play this · Remove (B7).
- Every edit goes through the existing `onWindowsChanged` / save paths; Update tracks and Save keep their
  confirmation dialogs.

## P.7 Voices (board `StageVoices`)

- Title row: **Voices** · *Editing* **Left hand / Right hand** · Exit.
- **Instrument · <hand>:** search field + category list + instrument list (today's `InstrumentChooser`
  tree with search, opened today by "Instruments"). Choosing applies to the selected hand as today.
- **Range, split & transpose** (today's playback-settings tab, `PlayBackSettingsComponent`): Playing
  range start / end note · Left hand up to (left-hand bound) · Right hand from (right-hand bound) ·
  Transpose (semitones, today's slider). Same callbacks (`onChangingSettings`, `onChangingTranspose`) and
  the same saved file.
- **SFZ library** (today's SFZ Library window, opened from MIDI Settings): list of SFZ files with what each
  is assigned to · Load SFZ… · Assign to style track… · Import mappings… (`SFZLibraryUI` actions).

## P.8 Chord helper (board `PageChords`)

- Title **Chord helper** · Exit. Left: search field (LCD) + chord list "name · notes". Right: chord name
  large, its notes, three mini keyboards **Root position / 1st inversion / 2nd inversion** with the chord's
  keys lit (`#e4a33a` white, `#c27d14` black).
- Today's `ChordBrowserComponent` (in `Source/UI/chordBrowser.h/.cpp`) shows the same three voicings as images; the new page draws them. The
  chord data (`allChords`) and "chosen" callback stay.

## P.9 Recorder (board `PageFile`)

Opened by **File** on the panel.

- **Your last recording:** LCD with length and event count · **Play recording** · Stop · **Save as MIDI…**
  (today's save-recording button and file chooser). Hint: "Press REC on the panel to start and stop".
- **MIDI file:** **Play a MIDI file…** · Stop (today's "Play file recording").
- **Rec** on the panel = today's record start/stop (the round record button); the screen shows the
  messages of part 1 §5.4. Today's play / stop drawable buttons for the recording move into this page.

## P.10 Home (boards `StageHome`, `PageHome`)

- The panel is visible with the screen showing "Ready" and dark lamps; the page area shows three big keys:
  **Settings · Play (lit) · Exit**, and top-right "Signed in as <email>" + **Log out** (part 1 §10).
- Play = today's Play button (enters the play view). Settings = P.2. Exit = today's Exit.
- Owner decision (part 1 §14): the instrument look, as on the boards; the forest-and-piano picture
  (`BinaryData::MainWindow_png`) is no longer shown behind Home or Login.

## P.11 Log in / Sign up (boards `StageLogin`, `PageLogin`)

- Screen shows "Welcome", lamps dark. Left: "Arranger Workstation" + one line of text. Right: the form.
- **Log in:** Email (remembered, with "Remembered from last time · Not you?") · Password + **Show/Hide** ·
  **Remember me** (LED toggle, "stay signed in until you log out") · **Log in** (lit) · Sign up · Forgot
  password?
- **Sign up:** Username · Email · Password · **Create account** (lit) · Back to log in.
- Behaviour and dependencies: part 1 §10.

## P.12 Esc menu and dialogs

- The Esc menu (`OverlayComponent`: Settings, Effects, Exit) stays, restyled as a small page-surface
  panel with rubber buttons; Settings / Effects open P.2 / P.1. Esc first closes an open page.
- Confirmation and input dialogs (rename, remove, overwrite, update tracks…) stay JUCE dialogs, drawn with
  the page materials (page surface, silkscreen title, LCD text field, rubber buttons).

---

## P.13 Integration map (old control → new place)

Re-check every row against the code before building its phase (part 1 §0.1) and add any control missing.

| Today | Code | New place |
|---|---|---|
| Home button (header) | `MainComponent::homeButton` | Panel screen button **Home** → P.10 |
| Play button (home picture) | `MainComponent::playButton` | P.10 **Play** |
| Particle colours | `colourSelectorButton` → colour picker | P.2 Display › Particle colours… |
| Toggle particles | `particleToggle` | P.2 Display › Particles |
| Note annotation | `noteNumbersAnnotation` | P.2 Display › Note names on keys |
| Instruments | `instrumentSelectorButton` → `showInstrumentSelector` | Panel **Voices** → P.7 instrument list |
| Left hand / Right hand toggles | `left/rightHandInstrumentToggle` | Panel Quick FX **Left / Right** part buttons |
| Play file recording | `playRecordingFileButton` | P.9 Play a MIDI file… |
| Save recording | `saveRecordingButton` | P.9 Save as MIDI… |
| Record ● / Stop ■ / Play ▶ (recording) | `startRecording`, `stopRecording`, `startPlayback` | Panel **Rec**; P.9 Play recording / Stop |
| Intro 1-3, Var 1-4, Break, Fill 1-4, Ending 1-3, auto-fill checkbox | header section buttons | Panel section buttons (6 × 3), **Auto Fill** |
| Synchro start (setting) | `SynchroStart` key | Panel **Sync St.** + P.2 Arranger |
| Chord helper | `chordHelperButton` | Panel **Chords** → P.8 |
| Style tab Start / Stop | `CurrentStyleComponent` | Panel **Start / Stop** |
| Beat bar | `CustomBeatBar` | Screen beat lamps (part 1 §5.3) |
| Tempo slider + BPM label | `tempoSlider` | Screen tempo + Tempo −/+ (part 1 §5.5) |
| Play-mode dropdown: All / Solo / Section Configurations / Settings | `playSettingsTracks` | P.3 All tracks / Solo · Sections… · Style settings… |
| 8 track slots: name (click = pick, right-click menu), volume, instrument | `Track` | P.3 track strips; P.4; P.5 |
| Styles tab: list, Add, right-click Rename / Delete | `StylesListComponent`, `StyleViewComponent` | P.3 Styles list + buttons |
| Vertical tabs, right-click Close Tab | `MyTabbedComponent` | Gone: pages replace tabs |
| Playback settings tab | `PlayBackSettingsComponent` | P.7 Range, split & transpose |
| Section Configurations browser + editor | `ArrangerStyleListComponent`, `ArrangerStyleEditor`, `ArrangerTimelineComponent` | P.6 |
| Sound Effects window | `SoundEffectWindowComponent` | Panel **Effects** → P.1; Quick FX knobs |
| MIDI Settings window | `SettingsWindow` | Panel **Settings** → P.2 |
| SFZ Library window | `SFZLibraryUI` (from Settings) | P.7 SFZ library |
| Update number of keys + device list | `updateNumberOfKeysDevice`, `devicesCBUpdate` | P.2 MIDI › Keyboard |
| Add MIDI Device window | `addDeviceWindow` | P.2 MIDI › Add device… |
| Track list (folders / tracks) | `TrackListComponent` | P.4 |
| Notes information table | `TableContainer`, `MidiNotesTableModel` | P.5 |
| Temporary notifications | `temporaryNotificationUI` | Screen messages (part 1 §5.4) |
| Esc menu | `OverlayComponent` | P.12 |
| Login / Sign up / Forgot pass | `LoginComponent`, `SupabaseClient` | P.11 (+ part 1 §10) |
| Falling notes + particles | `NoteLayer` | Note area (part 1 §4.3, §8) |
| On-screen keyboard | `KeyboardUI` | Keys (part 1 §7) |
| PC keyboard as piano, key bindings | `KeyboardListener` | Unchanged; focus must return to the play surface after closing a page (as `onRequestPlayFocus` does today) |
