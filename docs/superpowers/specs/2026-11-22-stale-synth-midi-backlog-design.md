# Stale MIDI backlog for the internal SFZ synth — design

**Status:** implemented on `fix/stale-synth-midi-backlog` (2026-11-22).

## Problem
`MidiHandler` queues every note it sees for the internal SFZ synth in `incomingMidiMessages`: live
notes from a MIDI keyboard or the PC keyboard, and (once an `AudioHandler` exists) everything the
arranger and the track player inject. Only one thing empties that queue: the SFZ audio callback
(`AudioHandler::audioDeviceIOCallbackWithContext` → `getNextMidiBlock`).

With the **External MIDI** engine no audio callback runs, so nothing drains the queue:
1. **It grows for the whole session** (every note-on/off, every CC, every arranger event).
2. **Switching to Internal SFZ replays all of it at once.** The first audio block takes the entire
   backlog at sample 0, so every note played since launch starts in the same instant: a burst.

The TODO note "arranger doubled to the external MIDI synth and SFZ" was the reason to look here. The
audible doubling does not happen (the two outputs are never open together: `disconnectCurrentOutput`
closes one before the other opens), but this backlog does.

## Evidence
- Code path above (MidiHandler.cpp `addEvent` sites; AudioHandler.cpp is the only `getNextMidiBlock` caller;
  `MainComponentActions.cpp` wires the inject callback whenever an `AudioHandler` exists).
- `tests/unit/test_internal_synth_midi_queue.cpp`: notes played before the synth starts reach its
  first block (fails before the fix), and notes played after it stops are still kept (fails before the fix).

## Options
1. Discard the backlog when the synth starts. Fixes the burst only; the queue still grows unattended.
2. **Queue only while the internal synth is running.** `AudioHandler` switches queueing on in
   `audioDeviceAboutToStart` and off in `audioDeviceStopped` (JUCE calls these when the callback is
   added/removed and when the device opens/closes), dropping whatever is pending each time.

## Decision
Option 2: it removes both the growth and the burst, and the switch lives where the consumer's life
cycle is already known. `MidiHandler::setInternalSynthListening (bool)`; the flag is atomic and changed
under `midiMutex` together with clearing the queue.

**Consequence handled (found in review):** a note-off or pedal-up sent while the synth is stopped is now
dropped instead of replayed, so a key held across an engine switch would hang when the synth restarts.
`AudioHandler::audioDeviceAboutToStart` therefore calls `allNotesOff` (no tail; also lifts the sustain
pedal) on every channel: the synth always starts from silence.

## Acceptance
- Notes played before the synth starts are not delivered to it.
- Notes played while it runs are delivered as before.
- Notes played after it stops are not kept.
- A note held when the synth stops is silent after it restarts (real SFZ + rendered audio in the test).
- Unit + arranger suites green; app builds.
