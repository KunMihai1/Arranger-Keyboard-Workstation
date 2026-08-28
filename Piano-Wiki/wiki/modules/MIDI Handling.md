---
type: module
title: "MIDI Handling"
path: "Source/Midi/ (MidiHandler, MidiRecordPlayer, MidiDevicesDB, keyListener, InstrumentHandler, MidiHandlerAbstractSubject)"
status: active
language: cpp
purpose: "MIDI input/output, device management, note routing, recording and playback"
depends_on:
  - "[[Supabase Backend]]"
used_by:
  - "[[Keyboard & Main UI]]"
  - "[[Audio & SFZ Playback]]"
created: 2026-06-06
updated: 2026-08-29
tags:
  - module
  - midi
related:
  - "[[Audio & SFZ Playback]]"
  - "[[Chord Recognition & Transposition]]"
sources: []
---

# MIDI Handling

## Purpose
The input/output spine of the app: enumerates and opens MIDI/audio devices, receives incoming MIDI, routes notes to the correct channel/hand, applies instrument presets and CC effects, and records/plays back performances.

## Key files
- `MidiHandler.h/.cpp` — two classes:
  - **`MidiDevice`** — wraps device enumeration and I/O (MIDI in, MIDI out, audio out via `juce::AudioDeviceManager`). Holds per-channel instrument parameters (volume, reverb, brightness, chorus, expression, attack/decay/release, pan, etc.) and the playable note range. Extracts VID/PID from device identifiers.
  - **`MidiHandler`** — `juce::MidiInputCallback` + `DisplayListener`. Handles incoming messages, splits left/right hand onto channels, applies instrument presets (program change + CC), injects keyboard/SFZ messages, and broadcasts to `MidiHandlerListener`s. Tracks last-sent CC values (reverb 91, brightness 74, expression 11, chorus 93, resonance 71, sustain 64).
- `MidiRecordPlayer.h/.cpp` — records incoming MIDI with timestamps (`RecordedEvent`), plays it back on a `juce::Timer`, and saves/loads `.mid` files. Has an `onSfzMessage` callback so the SFZ synth renders during playback; `remapChannel()` handles channel remaps from program changes.
- `MidiDevicesDB.h/.cpp` — `MidiDevicesDataBase`: a JSON store (in the app-data folder) of known devices (VID, PID, name, key count). Seeds initial devices, queries key counts. Uses `IFileSystem` for testability.
- `keyListener.h/.cpp` — computer-keyboard → MIDI input mapping.

## How it works
`MidiDevice` opens the hardware; `MidiHandler` is registered as its input callback. Incoming notes are range-checked, hand-split onto channels, and fanned out to listeners ([[Keyboard & Main UI]], [[Audio & SFZ Playback]], record player). The device DB resolves how many keys a connected controller has (by VID/PID) to set the playable range.

## Phase 4: chord input (the split zone feeds the arranger)
`MidiHandler` now also recognizes the chord the player holds and forwards it to the [[Arranger Engine]] — full path in [[Chord Recognition & Transposition]].
- **Zone split.** Notes **below `rightHandBoundSetting`** (the existing per-style left/right split) are the **chord zone**; `inChordZone()` decides. `ChordScanArea` (an atomic) switches between **split** and **full-keyboard** scan (full-keyboard wired as a mode now, tuned later).
- **Detection.** Held zone notes go to a `ChordDetector` member via `feedChordNote()` (which **always forwards** the detected chord — no de-dupe — because the engine resets to its home key on every Start, so a re-pressed chord must re-send). On a change, `onChordChanged(ArrangerChord)` fires → routed to the engine by [[Keyboard & Main UI]].
- **Chord Memory** (`setChordMemory`) latches the last chord so it persists when you lift your hand; **Full-keyboard** scan via `setChordScanArea`. Both are Settings toggles.
- **Thread-state cleanup.** `channel` is no longer a shared `MidiHandler` member — it's a per-note local via `channelForHand()`. The PC keyboard (`keyListener`) and physical MIDI in are mutually exclusive on one input, so this removed a real input/UI-thread shared-state hazard (commit `11687db`).

## Connects to
- Used by: [[Keyboard & Main UI]] (`MainComponent` owns the handler), [[Audio & SFZ Playback]] (`AudioHandler` takes a `MidiHandler&`), [[Track Playback]] / [[Arranger Engine]] (share MIDI-out; arranger also consumes `onChordChanged`).
- Depends on: [[Supabase Backend]] for device registration / playtime.

## Notes / gotchas
> [!key-insight] Hand-split channels
> Notes are routed to different MIDI channels by hand (`setCorrectChannelBasedOnHand`), which downstream synths and the recorder must respect. `channelForHand(note)` returns **16** for notes at/above `rightHandBoundSetting`, else **1**.

> [!warning] Two input paths must stay in parity
> Physical MIDI in → `handleIncomingMidiMessage`; PC keyboard → `noteOnKeyboard`/`noteOffKeyboard`. They must feed the same three sinks identically: (1) the `noteOnReceived` visual feed ([[Keyboard & Main UI]] `NoteLayer`), (2) the `handleIncomingMessage` listener feed, (3) the internal SFZ via `incomingMidiMessages` on the **hand-split channel + transpose**, mute-gated. Fixed 2026-08-09 (`5c75d4f`): the physical path had its note-on `ok` flag set *inside* the external-MIDI-OUT device lock, so on an internal-SFZ-only setup (no MIDI-OUT open) `ok` stayed 0 → no on-screen notes + skipped synth feed, while the PC keyboard worked. External MIDI-OUT is **optional**; only the `midiOut->sendMessageNow(...)` calls belong behind `getDeviceOUT().lock()`. The internal synth feed is fed on `channelForHand()` (not the hardware's raw channel).

## `handleIncomingMidiMessage` — the note-on contract (settled 2026-08-29)

> [!key-insight] Playability is decided from the note RANGE, never from the MIDI-OUT device
> `5c75d4f` moved the `ok` decision ahead of the `midiDevice.getDeviceOUT().lock()` block. Gating it
> on that lock was the bug: with no external MIDI-OUT open, `ok` stayed 0 and a physical keyboard
> produced nothing on the internal SFZ. Now `ok = 1` whenever the note is neither
> `startNoteSetting` nor `endNoteSetting` (both default **-1**), and external MIDI-OUT is treated as
> optional — forwarded to only when a device is actually open. This mirrors `noteOnKeyboard`, which
> is the parity the file's own warning demands.

> [!key-insight] Three explicit sink paths, not one trailing write
> The old code ended with a single `incomingMidiMessages.addEvent(processedMessage, 0)`. It is now
> split: **note-on** added inside the `!muteChordZone` guard (so a muted chord-zone onset never
> reaches the internal SFZ), **note-off** always added (nothing can stick), and **non-note**
> messages passed through raw so CC/pitch-bend still reach the synth. This is a superset of the
> `suppressInternalFeed` flag that briefly existed on the Phase 7a line, which is why that flag was
> dropped when the branches merged in `50e2d02`.

> [!warning] A test asserted the old behaviour for months without failing
> `test_midi_handler.cpp` carried *"noteOn does NOT notify listener without output device"* — a
> direct assertion of the defect `5c75d4f` fixed. It never went red because the `--run-tests` runner
> in `Main.cpp` was commented out on that branch: the app **compiled** the tests without ever
> **running** them. Corrected in `50e2d02`. Treat any other test touching code changed between
> `d05e07b` and 2026-08-29 with the same suspicion; CI now runs on every push, so the window is closed.
