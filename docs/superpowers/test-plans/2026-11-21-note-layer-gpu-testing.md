# NoteLayer GPU rendering — manual test plan

Covers what `tests/unit/test_note_field.cpp` can't: the GL drawing, threading and the overlay
interplay. Spec: `docs/superpowers/specs/2026-11-21-note-rendering-perf-design.md`.

Run the Debug app (Debug is where the old renderer fell to ~18 fps).

| # | Step | Expected |
|---|---|---|
| 1 | Play single notes and chords | Notes look as before: frosted white gradient, whitesmoke outline, rounded corners; held notes show the pulsing amber top glow in unison |
| 2 | Hold a chord ~3 s | Held notes grow to the top and stop; the glow keeps pulsing |
| 3 | Release | Notes rise at the same speed, shrink at the top edge and fade out |
| 4 | Fast runs / glissando, both hands | Motion stays smooth; no hitching; UI (knobs, display) stays responsive |
| 5 | Settings → particles on; change particle colour | Particles spawn on note-on in the chosen colour |
| 6 | ESC menu open → close while notes are falling | Menu draws on top; on close the screen is clean (no frozen notes) |
| 7 | Arranger style browser/editor open → close | Same as 6 |
| 8 | Loading label (Play / style load) | Label visible; notes reappear after |
| 9 | Resize / maximise / restore the window | Notes line up with the keys after the resize |
| 10 | Minimise while notes are falling, restore | No hang; screen continues or is clean |
| 11 | Idle for 10 s with nothing playing | Task Manager CPU for the app returns to idle |
| 12 | `Desktop/piano_gl_errors.log` after all of the above | Does not exist (no GL errors) |
