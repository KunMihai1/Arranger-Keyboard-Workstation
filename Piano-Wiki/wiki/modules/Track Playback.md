---
type: module
title: "Track Playback"
path: "Source/Playback/ (TrackPlayer, Track, TrackListComponent, TrackEntry, MidiNotesTableModel)"
status: active
language: cpp
purpose: "Classic multi-track MIDI sequencing, playback and per-track UI"
depends_on:
  - "[[MIDI Handling]]"
used_by:
  - "[[Keyboard & Main UI]]"
  - "[[Arranger Engine]]"
created: 2026-06-06
updated: 2026-11-24
tags:
  - module
  - playback
related:
  - "[[Arranger Engine]]"
sources: []
---

# Track Playback

## Purpose
The original ("classic") playback engine: plays multiple MIDI tracks together with per-track volume, instrument and channel, BPM/time-signature changes during playback, and the UI to manage tracks.

## Key files
- `TrackPlayer.h/.cpp` — `MultipleTrackPlayer`: `juce::HighResolutionTimer` + `TrackListener` + `Subject<TrackPlayerListener>`. Plays a vector of `TrackEntry`, each as a filtered `juce::MidiMessageSequence`. Supports live BPM change (`applyBPMchangeDuringPlayback`), time signature, per-channel volume/instrument, `onElapsedUpdate` (beats) and `onMidiMessage` (SFZ inject). Non-percussion tracks start at `baseChannelTrack = 2`.
- `Track.h/.cpp` — `Track`: a UI `juce::Component` for one track (volume slider, instrument chooser, name, channel, type), serializable to/from JSON. Acts as `Subject<TrackListener>`; supports copy/paste/rename/delete and note-info display.
- `TrackListComponent.h/.cpp` (+ `TrackListComponentFolders.cpp`, `TrackListComponentPage.cpp`) — the track library
  (folders → tracks, MIDI import, rename / remove dialogs), shown as the Style page's **track picker** (board
  `PageTracks`): LCD rows with glyphs, details column, Open folder / Use in slot N (= double-click), SORT stepping the
  hidden sort combo. Picking returns to the style through `callAsync` (never deletes the picker inside its own click).
- `Track.h/.cpp` — one style slot drawn as a track strip (name on an LCD strip, instrument button, `StageFader`,
  Notes). `MidiNotesTableModel` + `TableContainer` (`CustomTableContainer*.cpp`) — the **notes page** (board
  `PageNotes`): # column, velocity bars, Whole track / Back to recorded buttons that set the old choice and run the
  old action; Cancel in their confirm dialog is a dismissal (`TableDialog::resultIdFor`).
- `MidiNotesTableModel.h/.cpp` — table model showing the notes of a track.
- Supporting headers: `TrackEntry.h` (track data + `MidiMessageSequence`), `TrackListener.h`, `TrackPlayerListener.h`, `StyleSection.h`.

## How it works
`MultipleTrackPlayer` holds filtered MIDI sequences and steps each track's `nextEventIndex` on a high-res timer, emitting to MIDI-out and `onMidiMessage`. BPM changes rescale event timings live. The `Track` components provide the editing UI and notify the player of volume/instrument changes via the listener/subject pattern.

## Connects to
- Depends on: [[MIDI Handling]] (shared MIDI-out, channels).
- Used by: [[Keyboard & Main UI]]; [[Arranger Engine]] consumes the same `TrackEntry` data (`ArrangerPatternBuilder` builds sections from tracks). The arranger is a **parallel** engine — the classic player is left intact.

## Notes / gotchas
- **Posted callbacks carry a liveness token** (2026-11-24, fixes the open-style delete crash): `MultipleTrackPlayer` owns a
  `std::shared_ptr<bool> liveness`; every `callAsync` lambda holds a weak copy and returns if it expired, so a stop
  posted just before destruction no longer runs in freed memory (test `CallbackLifetime`).
> [!key-insight] Two engines coexist
> `MultipleTrackPlayer` (classic) and `ArrangerEngine` (new) are independent. Output is re-routed when switching engines — see commit history on `arranger-engine-phase1`.
