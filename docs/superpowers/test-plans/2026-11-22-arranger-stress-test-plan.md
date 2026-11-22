# Arranger — hard testing plan

**Status:** A done (PRs #28, #29); B run live on 2026-11-22, results below. The arranger is the newest feature and gets serious testing:
normal use *and* extreme, adversarial use, under pressure — not a quick "it plays, fine".

Two parts:
- **A. Headless engine stress tests** — automated, run in CI, no screen needed. Can start any time.
- **B. Live in-app scenarios** — drive the running app; **needs the owner's screen unlocked** (app focus,
  audio, screenshots). Back up `%APPDATA%\Arranger Workstation` first, restore after.

Every scenario records: what was done, the expected result, what happened, and evidence (test output,
screenshot, log). A failure becomes a bug with a reproducing test before any fix.

## A. Headless engine stress (ArrangerEngine / ArrangerSectionSequencer / ChordDetector / ChordTransposer)
1. **Transport spam:** 10,000 random start/stop/restart calls; after each, `isPlaying()` matches the last
   call and no note is left sounding (every note-on has its note-off on the output).
2. **Section-switch storms:** queue a different section every tick for thousands of bars (Var, Fill,
   Break, Intro, Ending, unknown names); the active section is always a real one, fills return to the
   right variation, auto-fill picks the target's fill, an Ending stops exactly once.
3. **Chord storms:** a new chord every 1/64 note, including invalid/empty chords, all 12 roots × every
   quality, slash chords, and chords held across section seams; no stuck notes, no out-of-range MIDI
   notes (0..127), bass inversion and NTT on/off and all minor-scale modes.
4. **Tempo extremes:** BPM at the slider limits and changed mid-bar (when allowed); bar boundaries stay
   exact over long runs (no drift: compare playhead beats to elapsed time × BPM).
5. **Style edge cases:** empty style, sections with no tracks, zero-length or 64-bar sections, odd time
   signatures, many tracks on one channel, drum channel only, a style reloaded while playing.
6. **Synchro Start / Count-In:** armed then stopped, armed then restarted, chord arriving exactly at
   Start, count-in interrupted by a section change.
7. **Long run:** an hour of simulated playback at high tempo with random input; memory and note counts
   stay flat.

## B. Live scenarios in the app (needs an unlocked screen)
1. Every section button in every order while playing, including rapid double-presses and presses during
   a fill; the highlighted button always matches what sounds.
2. Start / Stop / Start-while-playing / Stop-while-stopped from the mouse, the PC keyboard and MIDI.
3. Chord modes (Fingered, 1-Finger, Full keyboard) with fast chord changes, held chords at Start, chord
   memory on/off, mute chord keys, Synchro Start, Count-In.
4. Switching style, SFZ instrument, MIDI output and audio output **while playing**; switching arranger
   mode on/off while playing.
5. The style editor and Section Configurations: create, edit, save, load, delete while a style plays.
6. Window resize, minimise/restore, focus loss during playback; the beat bar stays on the beat.
7. Sound: arranger output must not be doubled to the external MIDI synth when SFZ is the engine
   (suspected from the TODO notes — both engines send to the MIDI output *and* the SFZ synth).

## B. Results — live run, 2026-11-22

Setup: Debug build, External MIDI engine (Microsoft GS Wavetable Synth), arranger mode on, Style 1 with six
tracks of a real 15-track song (demo arrangement: Intro 1 / Var 1 / Fill 1 / Ending 1). Sound was **measured**,
not assumed: the speakers were recorded through WASAPI loopback and reported in dBFS per window
(silence = -104 dBFS, playing = about -20 to -30 dBFS). App data backed up before and restored after (0 differences).

| # | Scenario | Result |
|---|---|---|
| B1 | 200 random section clicks in 30–150 ms gaps while playing | No hang, responsive throughout. Buttons for sections the style lacks (Var 2–4, Fill 2–4, Break, Intro 2–3) do nothing; the highlight stays on the playing section. |
| B1 | Fill 1 while playing | Var 1 lit until the bar line, Fill 1 for exactly one bar (2 s at 120 BPM), back to Var 1. |
| B1 | Ending 1 while playing | Lit at the next bar, 4 bars, engine stops itself, beat bar idle, BPM unlocked, highlight back to the start section. |
| B2 | 150 random Start/Stop (+ Ending) clicks in 20–300 ms gaps | 0 not-responding samples; final Stop idle, final Start plays. |
| B2/B3 | Section + transport storm, then Stop | Sound gone ~350 ms after Stop (-104 dBFS). |
| B3 | 102 random 3-key PC-keyboard chords in 15 s while playing, then Stop | Sound gone ~850 ms after Stop (live-note release tails). |
| B3 | Chord held through 60 Start/Stop clicks, Stop, release | Silent 2 s later. No stuck notes. |
| B4 | Switch style while playing | Keeps playing the previous style's arrangement under the new style's name until Stop (one CurrentStyleComponent is reused; the engine keeps what Start built). Stop works. **Intended** (owner, 2026-11-22): a new style takes effect on the next Start. |
| B6 | Minimise 6 s while playing, maximise | Beat bar exactly on the beat afterwards (6 frames checked against elapsed time). |
| B7 | Doubled output to external synth + SFZ | Does not happen (only one output is ever open). Found instead: notes queued for the SFZ synth were never drained with External MIDI and burst into the synth on switching — fixed, see `specs/2026-11-22-stale-synth-midi-backlog-design.md`. |

Also observed: a section clicked while **stopped** becomes the start section, including an Ending (Start then
plays the ending once and stops). **Intended** (owner, 2026-11-22): any section may be the start section.

Not run: B4 switching SFZ instrument / audio output (no SFZ library on this machine), B5 style editor and
Section Configurations while playing, Synchro Start / Count-In from the UI (covered headless in A).
