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
updated: 2026-06-06
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
- `TrackListComponent.h/.cpp` — the list/container of `Track`s.
- `MidiNotesTableModel.h/.cpp` — table model showing the notes of a track.
- Supporting headers: `TrackEntry.h` (track data + `MidiMessageSequence`), `TrackListener.h`, `TrackPlayerListener.h`, `StyleSection.h`.

## How it works
`MultipleTrackPlayer` holds filtered MIDI sequences and steps each track's `nextEventIndex` on a high-res timer, emitting to MIDI-out and `onMidiMessage`. BPM changes rescale event timings live. The `Track` components provide the editing UI and notify the player of volume/instrument changes via the listener/subject pattern.

## Connects to
- Depends on: [[MIDI Handling]] (shared MIDI-out, channels).
- Used by: [[Keyboard & Main UI]]; [[Arranger Engine]] consumes the same `TrackEntry` data (`ArrangerPatternBuilder` builds sections from tracks). The arranger is a **parallel** engine — the classic player is left intact.

## Notes / gotchas
> [!key-insight] Two engines coexist
> `MultipleTrackPlayer` (classic) and `ArrangerEngine` (new) are independent. Output is re-routed when switching engines — see commit history on `arranger-engine-phase1`.
