---
type: module
title: "Arranger Engine"
path: "Source/Arranger/"
status: active
language: cpp
purpose: "Real-time multi-section arranger playback with bar-synced section switching"
depends_on:
  - "[[Track Playback]]"
  - "[[Audio & SFZ Playback]]"
  - "[[Styles System]]"
used_by:
  - "[[Keyboard & Main UI]]"
created: 2026-06-06
updated: 2026-06-26
tags:
  - module
  - arranger
related:
  - "[[ArrangerScheduler]]"
  - "[[ArrangerSectionSequencer]]"
  - "[[Section Switch Flow]]"
  - "[[ADR-001 Pure Schedulers and Parallel Engine]]"
  - "[[Chord Recognition & Transposition]]"
  - "[[Chord-Aware NTT Transposition]]"
sources: []
---

# Arranger Engine

> [!key-insight] Active development — branch `arranger-engine-phase1`
> The arranger is the core of current work. It runs **parallel** to the classic [[Track Playback]] (`MultipleTrackPlayer`), which is left untouched. See [[ADR-001 Pure Schedulers and Parallel Engine]].

## The sub-system (one page each)
| Piece | Role |
|-------|------|
| **`ArrangerEngine`** (this page) | Real-time timer loop; owns schedulers + sequencer; dispatches MIDI |
| [[ArrangerSectionSequencer]] | Pure state machine — *which* section is active, switches at bar lines |
| [[ArrangerScheduler]] | Pure per-section loop — *what* MIDI to emit for a window |
| [[ArrangerPatternBuilder]] | Builds `ArrangerStyle` from [[Track Playback]] tracks |
| [[ArrangerModel]] | Plain data model (style/section/track/event) |
| [[ArrangerTime]] | Pure seconds↔beats / bar math |

Concepts: [[Monotonic Beat Playhead]] · [[Bar-Synced Section Switching]] · [[Loop-Seam Note-Off]] · [[Arranger Phase Roadmap]]
Flow: [[Section Switch Flow]]

## What `ArrangerEngine` itself does
A `juce::HighResolutionTimer` (10 ms) that is the only impure, threaded piece. It owns one [[ArrangerScheduler]] per section plus the single [[ArrangerSectionSequencer]], converts wall-clock time into a [[Monotonic Beat Playhead]], and `dispatch`es resulting MIDI to **both** the MIDI-out device (`sendMessageNow`) and `onMidiMessage` (the SFZ inject into [[Audio & SFZ Playback]]).

### Timer loop — `hiResTimerCallback`
1. Read the high-res clock, compute `deltaSeconds` since the last tick.
2. `deltaBeats = secondsToBeats(deltaSeconds, currentBpm)` → window `[playhead, playhead + deltaBeats)`.
3. `renderRange(from, to)`, then `playheadBeats = to`.
4. `notifyActiveSection(false)` — if the active section index changed, marshal `onActiveSectionChanged` to the UI (drives the live button highlight).
5. If `timerShouldStop` (set by an Ending), `callAsync(stop)` — `stopTimer()` must run off the timer thread.
6. Otherwise push `onElapsedBeats(playhead)` to the message thread for the beat bar.

> Accumulating from per-tick deltas means a **BPM change mid-playback** alters the rate going forward without jumping the musical position.

### `renderRange` (the mapping step)
1. **Drain a UI-queued switch first** (under `requestLock`): if `hasQueuedRequest`, call `sequencer.queue(requestedType, requestedName)`. Doing this *inside* `renderRange` (not only in the timer callback) means a direct `renderRange` call — e.g. **unit tests** — also honours queued switches. *(This was the fix for the engine tests that call `queueSection` then `renderRange` directly.)*
2. `sequencer.advance(from, to)` → `SectionSegment`s. For each segment:
   - If its `sectionIndex` differs from `currentSchedulerIndex`, **flush** the outgoing scheduler's hung notes (`flushActiveNotes`) and `reset()` both outgoing and incoming so the new section enters clean at its bar 0 — see [[Loop-Seam Note-Off]].
   - Advance that section's scheduler over the segment's local beats and dispatch each event.
3. If `step.stopRequested`: `haltAudio()` and set `timerShouldStop`.

### Callbacks back to the UI (all marshalled via `callAsync`)
- `onElapsedBeats(beats)` — beat bar position.
- `onActiveSectionChanged(idx, type, name)` — fired when the active section index changes (or `idx<0`); the host highlights the matching live section button. The editor instead reads `getActiveSectionIndex()` + `getActiveSectionLocalBeats()` to draw a **section-relative** timeline arrow.
- `onStoppedItself()` — fired only when the engine stops *because it was playing* (an Ending completed, or a manual stop): the host drops its "playing" flag so the beat bar leaves its red play colour and returns to the yellow downbeat.

### Lifecycle
- `setStyle` → `rebuildFromStyle`: builds one scheduler per section (merging all the section's tracks' patterns into a single event list; `loopLen = lengthBars * beatsPerBar`) and `sequencer.setStyle`.
- `start`: resets schedulers + sequencer, optionally `startAt(pendingStartIndex)`, calls `sendInstrumentSetup` (program-change + CC7 volume per track across **all** sections so any section we switch to already sounds right; channel 10 drums keep their kit and get no program-change), zeroes the playhead, `startTimer(10)`.
- `queueSection`: **does not touch the sequencer directly.** It only writes a request into a lock-guarded mailbox (`requestedType` / `requestedName` / `hasQueuedRequest`) that the timer thread drains in `renderRange`. `selectStartSection` (used while stopped) sets `pendingStartIndex` for the next `start`.
- `indexOfSection(type, name)` (used by `selectStartSection`) matches by **type + trailing number** (like the sequencer's `findSection`), so an abbreviated live label `Var 2` resolves to section `Variation 2` even though the strings differ; falls back to the first section of that type. *(Was an exact-string match before — "Var 2" silently fell back to Variation 1.)*

> [!key-insight] Choosing the start section (the durable choice lives in the UI, not the engine)
> The engine's `pendingStartIndex` is throwaway: `start()` consumes it (`startAt`, then `pendingStartIndex = -1`) and `stop()` clears it. Because `CurrentStyleComponent::startPlaying()` calls `stopPlaying(false)` first (→ `stop()`), any engine-side selection is gone by the time `start()` runs. So the *remembered* start lives in the host as `pendingStartType`/`pendingStartName` (default `Variation`/`Variation 1`), and `startPlaying` re-applies `selectStartSection(pendingStart…)` right before `start()`. See [[Arranger Style Authoring]] → "Start-section selection".
- `stop` / `haltAudio`: all-notes-off on channels 1–16, reset state, reset the beat bar via `callAsync`. `haltAudio` uses `playing.exchange(false)` and, **only if it was actually playing**, re-arms the first Variation (`notifyActiveSection(true)`, since `sequencer.reset()` lands there) and fires `onStoppedItself` — so after an Ending the buttons clear and Var 1 lights, ready to restart.

### Thread model & the `requestLock`
Two threads touch the engine: the **timer thread** (advances music, dispatches MIDI) and the **message thread** (UI clicks). The single invariant: **only the timer thread mutates the sequencer/schedulers.** The UI never does — it leaves a request in the `requestLock`-guarded mailbox, drained timer-side in `renderRange`. The reverse direction (engine → UI: `onElapsedBeats`, `onActiveSectionChanged`, `onStoppedItself`) is marshalled back with `callAsync`. `playing` is a `std::atomic<bool>` (lock-free), not a mutex. So it's one tiny `CriticalSection` for the UI→timer hand-off, held for microseconds — not heavy locking.

## Connects to
- Depends on: [[Track Playback]] (`TrackEntry` source via [[ArrangerPatternBuilder]]), [[Audio & SFZ Playback]] (`onMidiMessage`), [[Styles System]] (section buttons).
- Used by: [[Keyboard & Main UI]] — section buttons call `queueSection` / `selectStartSection`.

## Gotchas
> [!key-insight] Thread seam
> The schedulers/sequencer are pure (no I/O), so all real-time risk lives in this one class. `timerShouldStop` is set on the timer thread and honoured on the message thread; the Ending stop is deferred via `callAsync` because `stopTimer()` self-joins and must not run on the timer thread.

## Phase 4: chord transposition (emit-time)
The engine now re-pitches the accompaniment to the chord the player holds — see [[Chord Recognition & Transposition]] and [[Chord-Aware NTT Transposition]].
- **Chord mailbox.** `setActiveChord` / `setOriginalChord` write `pendingChord` under a `chordLock` and set `hasChordUpdate`; the **timer thread** drains it (same UI→timer hand-off discipline as `requestLock`). `setBassInversion` toggles the bass behaviour. `setStyle` **resets the active chord to the style's original on every Start**, so each play begins in the home key.
- **Part tagging.** `rebuildFromStyle` tags every scheduled event with a `PartKind` (`Bass` for a track named *bass*, `Fixed` for drums/perc, `Acc` otherwise); `EmittedEvent` carries `part`, and `ArrangerScheduler::setLoop` gained a `std::vector<PartKind>` overload to thread it through.
- **Dispatch.** `dispatchEmitted` runs each note through the `ChordTransposer` member before sending. An `activePlayedNote` map (keyed by channel + original note) stores the transposed value so the matching **note-off shifts identically** — else notes hang at the loop seam (`activeNoteParts`).

## Phase 5/6/6b + editor hardening (2026-06-25/26, uncommitted)
- **Transport feel.** `start(bool useTransportFeel=true)` arms **Synchro Start** (`synchroArmed`, released on the first valid chord) and/or a **Count-In** bar (`ArrangerCountIn.h`); `hiResTimerCallback` holds at the downbeat while armed. The **editor preview passes `useTransportFeel=false`** so it always plays — it feeds the engine no chords, so an armed Synchro gate would otherwise wait forever (the "preview won't start" bug).
- **`silenceArrangerNotes()`** replaces the old blanket all-notes-off (CC123 ch 1–16) in start/halt/restyle: targeted note-offs for the engine's **own** notes only (flush each scheduler + drain `activePlayedNote`), so the player's manually-held chord **survives Start** and a mid-play rebuild doesn't strand notes.
- **Thread-safe `setStyle` (mid-play restyle).** `setStyle` runs on the **message thread** but is now safe while playing — the editor rebuilds the preview on every region move / key change (each is a full `buildStyle()` → `rebuildFromStyle`). If playing, it `stopTimer()`s first (this **blocks until the in-flight timer callback returns**, so the timer thread is idle), silences, rebuilds, then resumes (`startTimer`), realigning **`playheadBeats=0`** so the groove restarts on a clean downbeat instead of an arbitrary `playhead % loopLen` phase (the "retake glitch"). No render-path lock; never the audio thread.

> [!resolved] Fixed (2026-06-25): held notes survive Start
> The old blanket all-notes-off killed the player's manually-held notes too. `silenceArrangerNotes()` (above) only closes the engine's own notes, so a chord held while pressing Start now keeps sounding.
