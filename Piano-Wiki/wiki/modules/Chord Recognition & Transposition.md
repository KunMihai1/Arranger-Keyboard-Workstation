---
type: module
title: "Chord Recognition & Transposition"
path: "Source/Arranger/ (Chord.h · ChordDetector.* · ChordTransposer.*)"
status: active
language: cpp
purpose: "Phase 4: recognize the chord the player holds in the split zone and transpose the accompaniment to it (Korg/Yamaha-grade NTT)"
depends_on:
  - "[[Arranger Engine]]"
  - "[[MIDI Handling]]"
used_by:
  - "[[Arranger Engine]]"
  - "[[Keyboard & Main UI]]"
created: 2026-06-25
updated: 2026-06-26
tags:
  - module
  - arranger
  - chord
related:
  - "[[Chord-Aware NTT Transposition]]"
  - "[[Arranger Style Authoring]]"
  - "[[Arranger Phase Roadmap]]"
sources: []
---

# Chord Recognition & Transposition

> [!key-insight] Phase 4 — the accompaniment follows the chord you hold
> The player holds a chord in the **left/split zone**; [[MIDI Handling]] recognizes it and hands it to the [[Arranger Engine]], which re-pitches **every** accompaniment track at emit time so the backing plays in the chord/key you're holding — drums excluded. The transposition math is [[Chord-Aware NTT Transposition]].

## Key files
- **`Chord.h`** — the value types. `ArrangerChord` (`root` 0–11, `ChordQuality quality`, `int bassNote` = −1 if none), the `ChordQuality` enum (maj, min, dom7, maj7, min7, dim, aug, sus2, sus4, …), `PartKind { Fixed, Acc, Bass }`, `chordIntervals(quality)` (semitone template), and string helpers. **Named `ArrangerChord`, not `Chord`,** to avoid collision with the pre-existing `HelperFunctions::Chord` (the UI image-chord). `isValid()` guards an unset chord.
- **`ChordDetector.h/.cpp`** — turns a *set of held note numbers* into an `ArrangerChord`. Matches pitch-class intervals against quality templates; disambiguates **Sus2 vs Sus4** by preferring `root == bass`; applies **full-keyboard hysteresis** (only re-detect once ≥3 notes are held, so passing single melody notes don't churn the chord). `detectKeyFromEvents()` is the offline key-finder used at authoring time.
- **`ChordTransposer.h/.cpp`** — stateless map of one note. Holds `original` + `active` `ArrangerChord` and a `bassInversion` flag; `transpose(noteNumber, PartKind)` returns the re-pitched note. Math in [[Chord-Aware NTT Transposition]]. `isBassInversionOn()` getter.

## How it works (the path)
1. **Input** ([[MIDI Handling]]): notes below `rightHandBoundSetting` fall in the **chord zone**. `MidiHandler` feeds held zone notes to its `ChordDetector` (`feedChordNote`), and on a change fires `onChordChanged(ArrangerChord)`. `ChordScanArea` (split vs full-keyboard) is an atomic; Chord Memory keeps the last chord latched.
2. **Routing** ([[Keyboard & Main UI]]): `MidiHandler.onChordChanged` → `Display::setArrangerLiveChord` → `CurrentStyleComponent::setLiveChord` → `ArrangerEngine::setActiveChord`.
3. **The mailbox** ([[Arranger Engine]]): `setActiveChord` doesn't mutate engine state directly — it writes `pendingChord` under `chordLock` and sets `hasChordUpdate` (same lock-guarded hand-off discipline as the section `requestLock`). The **timer thread** drains it.
4. **Emit-time transposition**: `rebuildFromStyle` tags each scheduled event with a `PartKind` (a track named *bass* → `Bass`, drums/perc → `Fixed`, everything else → `Acc`). `EmittedEvent` carries that `part`. As the engine dispatches each event (`dispatchEmitted`), it runs the note through `ChordTransposer::transpose(note, part)`. An `activePlayedNote` map (keyed by channel + original note) records the transposed note so the matching **note-off** is shifted identically — otherwise notes hang.

## The original chord (what we transpose *from*)
- Stored at **style level** (`ArrangerModel`: `originalRoot` / `originalQuality`; schema bumped **v3 → v4**, v3 files default to **C major**). It's auto-detected at authoring (`ChordDetector::detectKeyFromEvents`) and editable — see [[Arranger Style Authoring]] → "Recorded key".
- `setStyle` **resets the active chord to the original on every Start**, so each config begins in its own home key; a re-pressed chord must be re-sent (the detector always re-forwards rather than de-duping).

## Two separate layers (don't conflate them)
The word "chord" appears in both, but they are **different stages**:
- **Recognition (INPUT) — `ChordMode` / Phase 5.** *"What chord is the player holding?"* `enum class ChordMode { Fingered, SingleFinger, FullKeyboard }` (in `Chord.h`), chosen **once, globally** (Settings 3-way selector, property `"ChordMode"`). `ChordDetector::recompute()` branches on it: **Fingered** = template-match every zone note; **Single-Finger** = Korg one-finger (root=Maj; +white-left=Dom7; +black-left=Min; +both=Min7); **Full-Keyboard** = lowest-3→4→full, bass triad wins, no debounce. Output: a single `ArrangerChord`.
- **Transposition (OUTPUT) — `NttType` / Phase 7a (designed).** *"Given that one recognized chord, how do I re-pitch each accompaniment note?"* Chosen **per track** in the style. This is where the **interim** root-shift+third-flip (`ChordTransposer`) gets replaced by the real engine.

> [!key-insight] The modes do NOT change in Phase 7
> Any recognition mode produces the same thing — one `ArrangerChord`. That single chord drives **all** tracks; each track then applies its own `NttType`. So Fingered / 1-Finger / Full-Kbd are untouched by Phase 7; only what happens *after* a chord is recognized changes. ⚠️ Naming clash: the NTT type **`Chord`** (smart per-track remap) is unrelated to the recognition **`ChordMode`** — same word, different layer.

## Chord-zone mute (2026-06-26, Korg-style)
When **on AND the arranger is engaged**, the chord-recognition keys drive the accompaniment **without sounding** their raw notes (so a one-finger C+B+B♭ isn't heard as a cluster). `MidiHandler::muteChordZoneNote(note)` = mute && `arrangerEngaged` && `inChordZone`. It gates only note-**onsets** in both input paths (offs always pass → no stuck notes); `feedChordNote` (recognition) runs **before** the mute, so detection is unaffected. Settings "Mute chord keys…" (property `"ChordZoneMute"`, default off).

## Phase 7a — the real NTT (designed, spec ready)
The current `ChordTransposer` (root-shift + a single third-flip) is **interim**. Phase 7a replaces it with **per-track `NttType {NoTranspose, Parallel, Chord, Fixed}`**: the `Chord` type remaps *every* role (3rd/5th/7th) onto the active chord + snaps passing tones to the active chord's **scale** (data-driven table; curated minor-scale selector, Dorian default), with a master **"Chord Transposition On/Off"** bypass. Spec: `docs/superpowers/specs/2026-06-26-arranger-phase7a-ntt-engine-design.md`. See [[Chord-Aware NTT Transposition]] and [[Arranger Phase Roadmap]].

## Connects to
- Depends on: [[MIDI Handling]] (chord input), [[Arranger Engine]] (emit-time dispatch + mailbox).
- Used by: [[Arranger Engine]] (`ChordTransposer` member), [[Keyboard & Main UI]] (Settings toggles: bass inversion, full-keyboard scan, chord memory).

## Notes / gotchas
> [!key-insight] Transpose at emit, off the model
> Notes are re-pitched **when dispatched**, never by rewriting the stored pattern — so the saved `.style` always stays in its home key and one held chord can change while a section loops. Note-on and note-off must transpose to the **same** value (the `activePlayedNote` map), or held notes leak.

> [!key-insight] Drums never transpose
> Channel-10 / perc tracks are tagged `PartKind::Fixed`; `transpose()` returns them unchanged.
