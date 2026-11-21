# Arranger Phase 5 — Chord Recognition Modes (Fingered / Single-Finger / Full-Keyboard)

**Date:** 2026-06-25
**Branch context:** continues `arranger-engine-phase4-chords` (Phase 4 chords complete)
**Status:** Design approved (delegated) → implementation plan
**Reference target:** Korg Pa-series arranger behaviour, as closely as practical.

---

## 1. Goal

Give the arranger the three chord-recognition modes a workstation player expects, selectable
from the Settings panel:

- **Fingered** (today's behaviour) — every note in the left/split zone is a chord tone; matched
  against templates. Full vocabulary (maj/min/7/maj7/m7/dim/aug/sus…).
- **Single-Finger** (new) — Korg/Yamaha "One Finger": trigger the four common qualities with 1–2
  keys in the split zone.
- **Full-Keyboard** (re-tuned) — scan the whole keyboard; read the chord from the **bass/lowest**
  notes and never lurch on a melody played on top. Already wired; Phase 5 makes it gig-reliable.

This is purely an **input/recognition** change. The recognised `ArrangerChord` still flows to the
engine through the existing mailbox (`onChordChanged` → … → `ArrangerEngine::setActiveChord`); the
transposer, scheduler, sequencer, and threading model are untouched.

### Korg Single-Finger mapping (verified against Korg Pa docs)
| Keys held (in split zone) | Chord |
|---|---|
| root alone | **Major** |
| root + a **white** key to the left | **Dominant 7** (e.g. C+B → C7) |
| root + a **black** key to the left | **Minor** (e.g. C+B♭ → Cm) |
| root + a **white** and a **black** key to the left | **Minor 7** (C+B+B♭ → Cm7) |

Korg "One Finger" and Yamaha "Single Finger" are identical; Korg is the reference.

## 2. Non-negotiable constraints

- **Latency / threading.** Recognition runs on the **MIDI-input / PC-key thread** (in `feedChordNote`),
  never the audio or timer thread. It must stay light and lock-free: no allocations in the hot path,
  no blocking, and **no time-based debounce** (a debounce would add latency, which is forbidden).
  Stability comes from a pure, deterministic rule, not from delay.
- **Purity.** `ChordDetector` and `ChordTransposer` remain pure (no JUCE GUI/threads), so all three
  recognisers are unit-testable in isolation. This is how we validate Single-Finger and Full-Keyboard
  by enumerated cases instead of by ear.
- **No regression.** Fingered behaviour and every existing `ChordDetector` test must keep passing.

## 3. Decisions

| # | Decision | Rationale |
|---|----------|-----------|
| D1 | One `ChordDetector` with a **mode enum**; branch in `recompute()`. NOT a strategy-pattern split into recogniser classes. | The three algorithms are small and the class is already pure/tested; an enum keeps it in one readable, testable unit. |
| D2 | Replace `enum class ChordScanArea { Split, Full }` with **`enum class ChordMode { Fingered, SingleFinger, FullKeyboard }`** (moved to `Chord.h`, shared). | Single-Finger is a *recognition* mode, not a scan *area*; collapsing area+recognition into one mode is simpler than two orthogonal flags. Zone is derived from the mode. |
| D3 | Single-Finger root = **highest** held note; modifiers = notes below it, classified by **colour** (`hasWhiteBelow`, `hasBlackBelow`). | Matches Korg ("root + key to the left"); colour-presence (not exact interval) is robust to extra notes and exactly mirrors the Korg table. |
| D4 | Full-Keyboard tuning = **lowest-3 first (stop if valid), then lowest-4, then full set, else hold last**. | Bass triad always wins → a melody on top never changes the chord. Instant (pure, no debounce) → stability *and* responsiveness. Trade-off: a 7th *played in the bass hand* simplifies to its triad in Full-Keyboard (acceptable; Fingered/Single-Finger cover full vocabulary precisely). |
| D5 | Mode selector replaces the **Full-Keyboard on/off toggle** in Settings; persisted as an int `"ChordMode"`; migrate legacy bool `"ChordFullKeyboard"` (true→FullKeyboard, else Fingered). | Keeps all chord options in one place; existing users keep their behaviour with no manual migration. |
| D6 | **Chord-zone mute deferred** (out of scope). | Separable, not needed for the three modes. |

## 4. Architecture & data flow (unchanged spine)

```
key/MIDI note → MidiHandler::feedChordNote (input thread)
                  └─ inChordZone(note)?  [mode-derived: Full = whole kbd; else split zone]
                  └─ ChordDetector.noteOn/Off → recompute()  [branch on ChordMode]
                  └─ current() → onChordChanged(chord)
                        → Display → CurrentStyleComponent → ArrangerEngine::setActiveChord
                        → (timer thread) ChordTransposer  [UNCHANGED]
```

Only the boxed recognition step changes. Everything downstream is identical to Phase 4.

## 5. Components & changes

### 5.1 `Chord.h`
- Add `enum class ChordMode { Fingered, SingleFinger, FullKeyboard };`.
- (Optional) `toString(ChordMode)` / `chordModeFromString` for persistence + tests, mirroring the
  existing `toString(ChordQuality)` helpers.

### 5.2 `ChordDetector` (`Source/Arranger/ChordDetector.{h,cpp}`)
- Replace `setFullKeyboardMode(bool)` / `fullKeyboard` with `setMode(ChordMode)` / `ChordMode mode`.
  Keep `isFullKeyboardMode()`? No — expose `getMode()`.
- `recompute()` switches on `mode`:
  - **Fingered** → existing `recognizeSet(heldNotes)` (unchanged).
  - **SingleFinger** → new `recognizeSingleFinger(heldNotes)` (below).
  - **FullKeyboard** → new tuned routine (D4).
- Keep `hasHeldNotes()` (added in the held-chord-seed work) and `current()`.

**`recognizeSingleFinger(const std::set<int>& notes)`** (pure, static):
```
if notes.empty() → invalid {}
root      = *notes.rbegin()            // highest held note
rootPc    = root % 12
hasWhiteBelow = any n < root where isWhite(n%12)
hasBlackBelow = any n < root where isBlack(n%12)
quality = hasWhiteBelow && hasBlackBelow ? Min7
        : hasBlackBelow                 ? Min
        : hasWhiteBelow                 ? Dom7
        :                                 Maj
return { rootPc, quality, rootPc }      // bassNote = root (no slash in single-finger)
isBlack(pc) = pc∈{1,3,6,8,10}
```

**Full-Keyboard tuned `recompute()`**:
```
if heldNotes.size() < 3 → return        // hysteresis: hold the last recognised chord
take lowest 3 → c3 = recognizeSet(lowest3); if c3.valid → recognized = c3; return
take lowest 4 → c4 = recognizeSet(lowest4); if c4.valid → recognized = c4; return
cF = recognizeSet(heldNotes);            if cF.valid → recognized = cF; return
// else: hold the last recognised chord (do nothing)
```
(`recognizeSet` already prefers the interpretation rooted on the lowest note, so root tracks the bass.)

### 5.3 `MidiHandler` (`Source/Midi/MidiHandler.{h,cpp}`)
- Replace `std::atomic<ChordScanArea> chordScanArea` with `std::atomic<ChordMode> chordMode { ChordMode::Fingered }`.
- Replace `setChordScanArea(ChordScanArea)` with `setChordMode(ChordMode)` → stores the atomic and
  calls `chordDetector.setMode(...)`.
- `inChordZone(note)`: whole keyboard when `chordMode == FullKeyboard`, else the split zone
  (`rightHandBoundSetting` logic unchanged).
- `heldChord()` / `currentChord()` and Chord-Memory logic unchanged.

### 5.4 Settings UI (`Source/UI/SettingsWindow.{cpp,h}`)
- Replace `chordFullKeyboardToggle` (a `ToggleButton`) with a **3-way selector** `chordModeSelector`,
  styled to match the panel (segmented `TextButton`s or a `ComboBox`; pick whichever matches the
  existing chord-options styling). Label: **"Chord Mode"** with Fingered / Single-Finger / Full-Keyboard.
- Persistence: read/write int property **`"ChordMode"`** (0=Fingered, 1=SingleFinger, 2=FullKeyboard).
  **Migration:** if `"ChordMode"` is absent but legacy `"ChordFullKeyboard"==true`, default to
  FullKeyboard; otherwise Fingered. Write `"ChordMode"` on first change so the legacy key fades out.
- Replace callback `std::function<void(bool)> onChordFullKeyboardChanged` with
  `std::function<void(ChordMode)> onChordModeChanged`.

### 5.5 Host wiring (`Source/UI/MainComponent.cpp`)
- Wire `settings.onChordModeChanged = [this](ChordMode m){ midiHandler.setChordMode(m); }`.
- On startup, read the stored `ChordMode` (with migration) and call `midiHandler.setChordMode(...)`,
  mirroring how the Full-Keyboard toggle was applied before.

## 6. Testing (pure units → enumerated, not by ear)

Extend `tests/unit/test_chord_detector.cpp` (update the `detect` helper to take a `ChordMode`).

**Single-Finger** (`ChordMode::SingleFinger`):
- `{60}` → C **Maj**, root 0, bass 0.
- `{60,59}` (C + B, white-left) → C **Dom7**.
- `{60,58}` (C + B♭, black-left) → C **Min**.
- `{60,59,58}` (C + B + B♭) → C **Min7**.
- Root = highest: `{55,60}` → root C (60), G(55) is white-left → C7.
- `{}` → invalid; emptying after a hold → invalid (Memory handled upstream, not here).

**Full-Keyboard** (`ChordMode::FullKeyboard`) — keep existing tests, add:
- Bass Cmaj `{48,52,55}` + melody B♭ `70` on top → **stays C major** (lowest-3 wins; the full set
  would read C7 — proves we no longer try the full set first). *This is the key anti-lurch test.*
- Move the bass hand (`noteOff` the C-triad, `noteOn` an F-triad low) → switches to **F** (responsive).
- `<3` notes → holds last (existing).
- Chord-tone melody note added → unchanged (existing).

**Fingered** — existing tests unchanged (regression guard).

**Mode switch** — `setMode` re-recognises from current held notes (e.g. hold C+B, switch
Fingered→SingleFinger → C7).

## 7. Migration / back-compat

- Old installs with `"ChordFullKeyboard"=true` come up in **Full-Keyboard**; everyone else in
  **Fingered** (the prior default). No `.style` file changes; this is app-properties only.
- `ChordScanArea` is fully removed (internal type, not serialised), replaced by `ChordMode`.

## 8. Out of scope

- Chord-zone mute toggle (deferred).
- Single-Finger slash/bass-inversion voicings; Fingered/Full keep bass-inversion as today.
- NTT depth, chord variations, user-editable tables, microtuning — later phases (Phase 6+).
- Yamaha/Korg binary style importer — deferred further (per user).

## 9. How this sets up Phase 6

`ChordMode` and the pure recognisers are the substrate for later harmony work (NTT depth, chord
variations). Nothing here touches the engine threading or the sequencer, so Phase 6 attaches to the
same pure units.
