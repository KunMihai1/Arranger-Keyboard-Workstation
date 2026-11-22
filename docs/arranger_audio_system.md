# Arranger And Audio System

This note explains the arranger files in `Source/Arranger`, the audio/SFZ files in `Source/Audio`, and the way they connect to the rest of the app. It also explains the musical-time model: beats, bars/measures, BPM, loop windows, and section switching.

## Short Version

The arranger turns recorded MIDI tracks into reusable musical sections:

1. Recorded track events start as MIDI messages with timestamps in seconds.
2. `ArrangerPatternBuilder` converts note timestamps from seconds to beats.
3. `ArrangerSourceBuilder` turns app tracks into saved source tracks with fixed MIDI channels.
4. `ArrangerStyleEditor` lets the user mark bar windows as sections like Intro, Variation, Fill, Break, and Ending.
5. `ArrangerStyleIOHelper` saves those source tracks and windows into a `.style` JSON file.
6. At playback time, `ArrangerPatternBuilder` slices the saved source tracks into section-local loops.
7. `ArrangerEngine` advances a beat playhead, asks `ArrangerSectionSequencer` which section is active, asks `ArrangerScheduler` which MIDI notes should fire, then dispatches those messages.
8. Messages either go to an external MIDI output, or into the internal SFZ audio path through `MidiHandler` and `AudioHandler`.

The core idea is:

```text
source tracks + bar windows -> runtime sections -> section sequencer -> loop scheduler -> MIDI -> sound
```

## Musical Time Model

### Beat

In this codebase, a "beat" means one quarter note.

That is stated directly in `Source/Arranger/ArrangerTime.h`:

```cpp
/** Pure musical-time conversions. A "beat" is one quarter note. */
```

So the beat unit is not "whatever the time signature denominator says". It is always quarter-note time.

### BPM

BPM means quarter-note beats per minute.

The conversion is:

```text
beats = seconds * (bpm / 60)
seconds = beats * (60 / bpm)
```

Examples:

```text
120 BPM: 1 second = 2 beats
60 BPM:  1 second = 1 beat
90 BPM:  1 second = 1.5 beats
```

This is used when converting recorded MIDI from seconds into arranger beat positions.

### Bar / Measure

Bar and measure mean the same thing here.

The code computes bar length in quarter-note beats:

```text
beatsPerBar = timeSigNum * (4 / timeSigDenom)
```

Examples:

```text
4/4 = 4 * (4 / 4) = 4 quarter-note beats per bar
3/4 = 3 * (4 / 4) = 3 quarter-note beats per bar
6/8 = 6 * (4 / 8) = 3 quarter-note beats per bar
```

Important detail: for `6/8`, the code is counting total quarter-note duration in the bar. It does not model the common "two dotted-quarter pulses" feel separately. It simply says the bar is 3 quarter-note beats long.

### Absolute Beats Vs Local Beats

The arranger uses two beat coordinate systems.

Absolute beats:

```text
0.0, 1.0, 2.0, 3.0, 4.0, ...
```

These are the monotonic playback position since the engine started.

Local section beats:

```text
0.0 ... sectionLengthBeats
```

These are positions inside the currently active section's loop.

For example, in 4/4:

```text
bar 1 starts at beat 0
bar 2 starts at beat 4
bar 3 starts at beat 8
bar 4 starts at beat 12
```

A saved section window uses 1-based bar numbers:

```text
startBar = 3
lengthBars = 2
```

That means:

```text
startBeats = (3 - 1) * 4 = 8
lengthBeats = 2 * 4 = 8
window covers source beats [8, 16)
```

If a source note is at beat `10.5`, the runtime section stores it at:

```text
10.5 - 8 = 2.5 local beats
```

That rebasing is what lets a section loop from local beat `0`.

### Half-Open Time Windows

Schedulers generally treat ranges as:

```text
[fromBeats, toBeats)
```

That means the start is included, the end is excluded. This avoids double-triggering an event exactly on a boundary when one timer tick ends and the next begins.

## Saved Style Vs Runtime Style

There are two related representations.

### Saved `.style` File

Defined by `Source/Arranger/ArrangerStyleFile.h`.

Saved styles contain:

- `SourceTrackFile`: each source MIDI track stored once, with note events in beat time.
- `SectionWindow`: a bar range pointing into the source tracks.
- `ArrangerStyleFile`: metadata plus source tracks plus windows.

This is compact because the notes are not duplicated for each section. The file says: "Here are the recorded tracks; Intro is bars 1-2, Variation is bars 3-6, Fill is bar 6", and so on.

### Runtime `ArrangerStyle`

Defined by `Source/Arranger/ArrangerModel.h`.

Runtime styles contain:

- `ArrangerStyle`: whole style.
- `ArrangerSection`: one playable section with a fixed loop length.
- `ArrangerTrack`: one track inside a section.
- `TimedBeatEvent`: one MIDI event at a beat position inside the section loop.

At runtime, each `SectionWindow` is sliced out of the source tracks and rebased to beat `0`. This creates independent section loops that the engine can schedule quickly.

## Main Data Types

### `TimedBeatEvent`

One MIDI message positioned in beat time:

```cpp
struct TimedBeatEvent
{
    double beats = 0.0;
    juce::MidiMessage message;
};
```

For note duration, there is no separate duration field. A note duration is represented by a note-on event and a later note-off event.

### `ArrangerTrack`

One playable track inside a runtime section.

Important fields:

- `id`: track identity, usually matching the original `TrackEntry`.
- `name`: display name.
- `partType`: drum, percussion, bass, or accompaniment.
- `instrument`: General MIDI program number.
- `channel`: MIDI channel used for playback.
- `volume`: sent as CC7 before playback.
- `pattern`: beat-timestamped note events for this section.

`originalChord` is reserved for later harmonic/transposition phases. In the current arranger, notes are not reharmonized to live chords.

### `ArrangerSection`

One section such as `Variation 1`, `Intro 1`, or `Ending 1`.

Important fields:

- `type`: Intro, Variation, Fill, Break, Ending, CountIn.
- `lengthBars`: how many bars the section loop lasts.
- `afterComplete`: what happens after the section reaches its own end.
- `tracks`: all tracks that play inside this section.

### `ArrangerAfterComplete`

This controls section behavior:

```text
Loop        -> keep looping this section
FallThrough -> play once, then return/fall into a variation
Stop        -> play once, then stop the engine
```

Typical use:

```text
Variation -> Loop
Intro     -> FallThrough
Fill      -> FallThrough
Break     -> FallThrough
Ending    -> Stop
```

## Arranger File Guide

### `ArrangerModel.h`

Defines the runtime arranger schema.

This file is the foundation for the engine. It defines:

- section types
- part types
- completion behavior
- beat-timestamped MIDI events
- tracks
- sections
- full runtime styles

The runtime engine reads `ArrangerStyle`, not the saved `.style` file directly.

### `ArrangerStyleFile.h`

Defines the saved file schema.

This file is for `.style` files on disk. It separates:

- source tracks, which contain the full captured MIDI material
- section windows, which say which bars become Intro/Variation/Fill/etc.

This design avoids storing duplicate MIDI notes when several sections point to different bar ranges of the same recording.

### `ArrangerEnums.h`

Converts enum values to/from strings for JSON and UI.

Examples:

```text
ArrangerSectionType::Variation <-> "Variation"
ArrangerAfterComplete::Stop    <-> "Stop"
ArrangerPartType::Drum         <-> "Drum"
```

When loading unknown strings, it falls back to safe defaults:

```text
unknown section type -> Variation
unknown after rule   -> Loop
unknown part type    -> Acc
```

### `ArrangerMidiHex.h`

Converts raw MIDI messages to hex strings and back.

This is used by `ArrangerStyleIOHelper` so `.style` files can store MIDI messages in JSON:

```json
{ "b": 2.0, "m": "903C64" }
```

Here:

- `b` is the beat position.
- `m` is the raw MIDI bytes as uppercase hex.

### `ArrangerTime.h/.cpp`

Pure musical-time math.

Functions:

- `secondsToBeats(seconds, bpm)`
- `beatsToSeconds(beats, bpm)`
- `beatsPerBar(timeSigNum, timeSigDenom)`
- `barsForBeats(beats, beatsPerBar)`

`barsForBeats` rounds up to whole bars. For example, in 4/4:

```text
8.0 beats    -> 2 bars
8.0001 beats -> 3 bars
0 beats      -> 0 bars
```

This is how the arranger decides the minimum loop length needed to contain all recorded notes.

### `ArrangerChannels.h/.cpp`

Defines arranger MIDI channel allocation.

Rules:

- drums use channel `10`
- melodic accompaniment uses `2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13`
- channels `1` and `16` are reserved for live left/right hand instruments
- channel `10` is skipped because it is drums
- channels `14` and `15` are avoided because the app uses them for recording/remapping paths

If more than 11 melodic arranger tracks are requested, extra melodic tracks are dropped by the source builder.

### `ArrangerDefaults.h/.cpp`

Creates the default section setup for a new captured recording.

The default is one `Variation 1` spanning all available bars:

```text
Variation 1: start bar 1, length = total bars, afterComplete = Loop
```

The user can then add Intro, Fill, Break, and Ending windows in the editor.

### `ArrangerSourceBuilder.h/.cpp`

Converts live app `TrackEntry` objects into saved `SourceTrackFile` objects.

It does three key things:

1. Assigns stable arranger playback channels.
2. Copies track metadata like name, instrument, and volume.
3. Calls `ArrangerPatternBuilder::buildBeatEvents` to convert note timestamps from seconds to beats.

Percussion tracks become drum source tracks on channel `10`. Melodic tracks become accompaniment tracks on the melodic channel sequence.

### `ArrangerPatternBuilder.h/.cpp`

Builds beat-based arranger content.

Main responsibilities:

- convert `MidiMessageSequence` note events from seconds to beats
- ignore non-note events when building patterns
- force events onto the arranger-assigned MIDI channel
- build a one-section demo style from selected tracks
- build demo Intro/Variation/Fill/Ending sections from a single loop
- slice saved source tracks into section windows
- build the runtime `ArrangerStyle` from a saved `ArrangerStyleFile`

The most important conversion is:

```cpp
ArrangerTime::secondsToBeats(msg.getTimeStamp(), referenceBpm)
```

`referenceBpm` must match the tempo/timebase of the `TrackEntry::sequence` being converted. If the sequence has already been scaled to the current playback tempo, the current playback tempo must be passed in.

#### Slicing Sections

`sliceSection` takes a source section and a beat range:

```text
startBeats = (startBar - 1) * beatsPerBar
endBeats = startBeats + lengthBars * beatsPerBar
```

It keeps events inside `[startBeats, endBeats)` and stores them as:

```text
localBeat = sourceBeat - startBeats
```

That is the heart of turning a recording into reusable arranger sections.

### `ArrangerStyleIOHelper.h/.cpp`

Reads and writes `.style` files as JSON.

On save:

- writes style metadata
- writes source tracks
- writes each event as `{ "b": beat, "m": midiHex }`
- writes section windows with type, start bar, length, and after-complete rule

On load:

- parses JSON
- rebuilds `SourceTrackFile` objects
- decodes MIDI hex back to `juce::MidiMessage`
- rebuilds `SectionWindow` objects

The actual folder comes from `IOHelper::getArrangerStylesFolder()`, which points to the app data folder's `ArrangerStyles` subfolder.

### `ArrangerScheduler.h/.cpp`

Schedules MIDI events for one section loop.

It is deliberately pure and testable:

- no UI
- no audio
- no MIDI output device
- no threading

Given a loop of events and a playhead range, it returns the events that should happen inside that range.

Example:

```text
loop length = 4 beats
event at beat 0.0
event at beat 2.0

advance(0.0, 1.0) -> emits beat 0.0 event
advance(1.0, 3.0) -> emits beat 2.0 event
advance(3.5, 4.5) -> crosses loop seam, then emits next loop's beat 0.0 event
```

It also tracks active notes. If a note-on happens but no note-off appears before the loop seam, it emits a synthetic note-off at the seam. This prevents hanging notes when the loop restarts.

When switching sections, `flushActiveNotes` closes notes that are still sounding in the outgoing section.

### `ArrangerSectionSequencer.h/.cpp`

Decides which section is active over time.

It does not emit MIDI. It only says:

- which section is active
- when a queued section switch should happen
- when a one-shot section has completed
- whether playback should stop

Important behavior:

- starts on the first Variation by default
- section switches happen at the next bar boundary
- pressing multiple section buttons before the boundary keeps only the latest request
- Intro and Fill can play once then fall through
- Ending can play once then request stop
- Auto Fill can insert a Fill before switching from one Variation to another

The sequencer splits a time advance into `SectionSegment`s. For example, if the engine advances from beat `3.8` to `4.2` in 4/4, that crosses a bar boundary at beat `4.0`, so the sequencer may return two segments:

```text
3.8 -> 4.0: old section
4.0 -> 4.2: new section
```

Each segment gives local beat positions for the section scheduler.

### `ArrangerEngine.h/.cpp`

The real-time arranger playback engine.

It owns:

- the current runtime `ArrangerStyle`
- one `ArrangerScheduler` per section
- one `ArrangerSectionSequencer`
- a high-resolution timer
- optional external MIDI output
- optional `onMidiMessage` callback for SFZ injection

Playback flow:

1. `start()` resets schedulers and the sequencer.
2. It sends program changes and CC7 volume for all section tracks.
3. It starts a high-resolution timer.
4. Every timer tick, it measures wall-clock delta time.
5. It converts that delta from seconds to beats using the current BPM.
6. It calls `renderRange(fromBeats, toBeats)`.
7. `renderRange` asks the sequencer for active section segments.
8. Each segment is passed to the correct section scheduler.
9. Scheduler output MIDI is dispatched.
10. If an Ending requests stop, the engine silences all notes and stops itself safely.

Dispatch sends MIDI to both possible destinations:

```cpp
if (auto out = outputDevice.lock())
    out->sendMessageNow(m);

if (onMidiMessage)
    onMidiMessage(m);
```

So the same engine can drive an external MIDI synth, the internal SFZ synth, or both if both are connected.

### `ArrangerTimelineGeometry.h/.cpp`

Pure geometry for the section editor timeline.

It maps:

```text
bar number <-> x pixel
section window <-> rectangle
mouse x <-> snapped bar number
```

The timeline uses 1-based bar numbers. It can snap drags to whole bars and detect whether the user grabbed a region body, left edge, or right edge.

### `ArrangerTimelineComponent.h/.cpp`

The visual timeline editor.

It draws:

- bar grid
- bar numbers
- colored section regions
- selected section outline
- playhead line

It supports:

- selecting a section
- dragging a section to another bar
- resizing a section from the left or right edge
- tooltips like `Intro 1 (bars 1-2)`
- auto-scrolling while dragging near the viewport edge

It knows only about `SectionWindow`s. It does not save files and does not talk directly to audio. It sends callbacks when windows change or a section is selected.

### `ArrangerStyleEditor.h/.cpp`

The full arranger authoring UI.

It connects:

- source tracks
- section windows
- timeline editing
- style saving
- live preview through `ArrangerEngine`

New style flow:

```text
selected TrackEntry objects
-> ArrangerSourceBuilder::fromTrackEntries
-> default Variation window
-> timeline editing
-> buildStyle()
-> engine.setStyle()
```

Save flow:

```text
editor state
-> ArrangerStyleFile
-> ArrangerStyleIOHelper::saveToFile
-> .style JSON file
```

Preview flow:

```text
windows/sourceTracks
-> ArrangerPatternBuilder::buildStyleFromWindows
-> ArrangerEngine::setStyle
-> ArrangerEngine::start
```

The editor also temporarily takes over `engine.onElapsedBeats` so it can show the playhead inside the active section's original timeline window. When the editor closes, `CurrentStyleComponent` restores the normal beat-bar callback.

Long operations like saving, loading, and updating tracks are run on background threads to avoid freezing the UI.

### `ArrangerStyleListComponent.h/.cpp`

The browser for saved `.style` files.

It lists files from the arranger styles folder and exposes callbacks for:

- create new
- edit selected
- delete selected
- load selected
- close browser

It also marks the active configuration with a visual indicator.


> Continued in [part 2 of 2](arranger_audio_system-part2.md).
