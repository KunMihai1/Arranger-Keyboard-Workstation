# Arranger — hard testing plan

**Status:** planned (owner, 2026-11-22). The arranger is the newest feature and gets serious testing:
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
