# Arranger Engine — Phase 4 Design Spec (Chord Recognition & Transposition)

**Date:** 2026-06-24
**Status:** Designed (awaiting plan)
**Author:** Mihai
**Depends on:** Phases 1–3 (merged to `main`, HEAD `63b6aa7`) — `ArrangerModel`,
`ArrangerScheduler`, `ArrangerSectionSequencer`, `ArrangerEngine`, `ArrangerPatternBuilder`,
the style-authoring editor (`ArrangerStyleEditor`), `.style` persistence (`ArrangerStyleFile`
schema v3 + `ArrangerStyleIOHelper`), and the existing keyboard split (`MidiHandler`,
`rightHandBoundSetting`).
**Parent roadmap:** `Arranger Phase Roadmap` (wiki) §Phase 4/5.

---

## 1. Context & Goal

The arranger plays genuinely distinct, authored sections (Phase 3) and switches between them with
real transport rules (Phase 2). But the accompaniment is **frozen in the key it was recorded in** —
play any chord on the keyboard and the backing does not follow. The data model has reserved the
hooks for this since Phase 1 (`ArrangerTrack::partType`, `ArrangerTrack::originalChord`) but nothing
reads them.

**Goal:** make the accompaniment **follow the chord the player fingers** — the defining feature of an
arranger keyboard. When the player holds a chord in the left-hand zone, every pitched accompaniment
part (Bass, Acc) transposes in real time to that chord; drums and percussion never move. The
transposition is **chord-aware** (Korg/Yamaha-grade): it detects the real chord including quality and
extensions and maps each note onto the correct tone of the played chord via a Note Transposition
Table (NTT), not a naive block shift.

### Why full chord-aware (Option 3), and why staged

The user chose full **chord-aware NTT** over a generic block-shift, because a block shift only sounds
right for plain major/minor and breaks on 7ths/sus/dim — exactly the chords this feature exists to
support. NTT is larger, so the build is **staged**: a Stage-1 walking skeleton wires the entire
signal path end-to-end with a stubbed (root-only block-shift) transposer, proving input → detection →
engine → transposition → sound is real before the hard music theory lands. Each later stage swaps in
more intelligence without re-plumbing.

---

## 2. Scope

### In
- **`Chord`** value type + **`ChordDetector`** (pure): held note-set → recognized `Chord`
  (root + quality + extensions + bass note), Fingered recognition by interval-set matching.
- **`ChordTransposer`** (pure): note + `originalChord` + `activeChord` + transposable flag →
  transposed note. Stage 1 = block shift; Stage 3 = real NTT (note-role → chord-tone).
- **Event transposability tagging** in `ArrangerEngine::rebuildFromStyle`
  (`transposable = partType ∈ {Bass, Acc}`), carried through the scheduler to emit time.
- **Engine chord mailbox**: lock-guarded `setActiveChord(Chord)` (input thread) drained by the timer
  thread; emit loop runs each transposable event through `ChordTransposer`.
- **Chord scan-area switch** in `MidiHandler`: `Split` (default, left-zone via `rightHandBoundSetting`)
  / `Full` (all notes) — feed only; routing seam present from Stage 1.
- **Original-chord capture**: auto-detect at config build / Update Tracks, **C-major fallback**,
  editable root+quality combos in the `ArrangerStyleEditor` top bar, persisted **style-level** in the
  `.style` file (schema **v3 → v4**, back-compat default C major).
- **Chord Memory** (hold last chord when the left hand lifts) and an optional **Bass Inversion** toggle.

### Out / deferred
- **Full-Keyboard chord/melody inference** — the routing mode is wired now (Stage 1) but the smart
  inference that separates chord from melody is **Stage 6**, tuned by ear on the finished pipeline.
- **Single-Finger / One-Finger recognition mode** — Fingered only this phase; the detector interface
  leaves room for it later.
- **Microtuning / scale (non-12-TET) remapping, user-editable NTT tables** — later.
- **Yamaha/Korg binary style importer** — still deferred (its NTT/CV payload now has a consumer, but
  the parser is a separate project).

---

## 3. Decisions captured during brainstorming

| Question | Decision |
|---|---|
| Transposition intelligence | **Full chord-aware NTT** (Option 3), built in stages |
| Where transposition applies | **At emit time, per note, events tagged transposable** (Fork A1) — not pattern-rebuild, not channel-filter |
| What transposes | **Bass + Acc** by `partType`; **Drum + Perc never** transpose |
| Where chord detection lives | **Standalone pure `ChordDetector`**, fed by `MidiHandler` (Fork B1) — not inline in the handler |
| Chord input zone | **Split** (reuse existing left-hand zone, `rightHandBoundSetting`) as default; **Full Keyboard** as a wired-now / tuned-Stage-6 mode |
| Recognition mode | **Fingered** (play the real chord) |
| Original chord source | **Auto-detect at config build → save into config → editable in editor → C-major fallback** |
| Original-chord storage | **Style-level** (one source chord per config; matches `ArrangerStyleFile` source-once model) |
| Chord Memory | **On** (hold last chord on release) |
| Bass Inversion | **Optional toggle, off by default** |
| Threading | **Lock-guarded mailbox**, mirroring the proven `requestLock`/`queueSection` pattern |

---

## 4. Architecture

Pure music-theory units (testable with no audio/threads) + thin wiring into the existing engine,
handler, editor, and serializer. The **sequencer is untouched**; the **scheduler** changes only to
carry the transposable tag.

### 4.1 `Chord` value type (new, in e.g. `Source/Arranger/Chord.h`)
```cpp
enum class ChordQuality { None, Maj, Min, Dom7, Maj7, Min7, Dim, HalfDim, Aug, Sus2, Sus4 /*…extensible*/ };

struct Chord
{
    int          root      = -1;   // pitch class 0..11 (C..B); -1 = none
    ChordQuality quality   = ChordQuality::None;
    int          bassNote  = -1;   // pitch class of lowest fingered note (for Bass Inversion); -1 = use root
    bool isValid() const { return root >= 0 && quality != ChordQuality::None; }
};
```
A `Chord` is pitch-class based (octave-independent) plus a concrete bass pitch class for inversions.

### 4.2 `ChordDetector` (pure, new)
```cpp
class ChordDetector
{
public:
    void noteOn (int noteNumber);
    void noteOff (int noteNumber);
    void reset();
    /** Best chord for the currently-held notes, or Chord::none (root=-1) if unrecognized. */
    Chord current() const;
private:
    std::set<int> heldNotes;   // absolute note numbers in the chord zone
};
```
- Held-note tracking is trivial; recognition normalizes held notes to a pitch-class set and matches
  against known interval templates (root-relative), choosing the best fit (and lowest note → `bassNote`).
- **Stage 1** returns root only (quality = `Maj` placeholder) so the skeleton runs.
- **Stage 2** returns full quality + extensions, handling inversions (the same pitch-class set matches
  regardless of voicing; the root is the template root, not the lowest note).

### 4.3 `ChordTransposer` (pure, new) — the NTT
```cpp
class ChordTransposer
{
public:
    void setOriginalChord (Chord recorded);     // the "from" key (config-level)
    void setActiveChord   (Chord played);       // the "to" chord (live)
    /** Map one note. transposable=false (Drum/Perc) returns note unchanged. */
    int  transpose (int noteNumber, bool transposable) const;
};
```
- **Stage 1:** block shift — `note + (active.root − original.root)` for transposable notes, clamped
  to 0..127.
- **Stage 3:** real NTT — classify the note's scale-degree role relative to `original`, map to the
  corresponding tone of `active` (handles 7th/sus/dim, nearest-chord-tone snapping, voicing-aware).
- **Bass Inversion:** when enabled and the note belongs to a Bass part, target the `active.bassNote`
  instead of the root. (Bass parts identified via the transposable path; see §4.4 note.)

### 4.4 Event transposability tagging (in `ArrangerScheduler` + `ArrangerEngine::rebuildFromStyle`)
The merge in `rebuildFromStyle` flattens tracks and loses `partType`. Carry a per-event flag instead:
- Extend the scheduler's stored event and `EmittedEvent` with `bool transposable` (and, for Bass
  Inversion, enough to know a note is a Bass part — a small `partType`/`isBass` tag).
- In `rebuildFromStyle`, set the flag from each track's `partType` **before** merging.
- The scheduler carries the flag verbatim through `advance()` (pure; no behaviour change otherwise).

### 4.5 Engine chord mailbox + emit-time transposition (in `ArrangerEngine`)
- New lock-guarded field set mirroring `requestLock`/`requestedType`:
  `juce::CriticalSection chordLock; Chord pendingChord; bool hasChordUpdate;` and a public
  `void setActiveChord (Chord)` callable from the input thread.
- In `renderRange`, **before** advancing, drain `pendingChord` under the lock into the
  `ChordTransposer` (timer-thread-only state), exactly where the queued section switch is drained.
- In the emit loop, replace each emitted note's number with
  `transposer.transpose(note, ev.transposable)`. **Note-offs at loop seams and `flushActiveNotes`
  must use the same transposed pitch as their note-on** — track transposition per sounding note (or
  transpose deterministically from the same active chord) so nothing hangs. This is the single most
  important correctness constraint (see §7, §8).
- `setOriginalChord` is set from the active style on `setStyle` (style-level field, §4.7).

### 4.6 Live input wiring (`MidiHandler` → engine)
- `MidiHandler` gains a `ChordScanArea { Split, Full }` setting (default `Split`) and an owned/visible
  `ChordDetector`.
- In its note-on/note-off handling (`MidiHandler.cpp:761`, `:810`), after the existing split
  classification: if the note is in the chord zone (Split → below `rightHandBoundSetting`;
  Full → any note), forward it to `ChordDetector::noteOn/noteOff`, then push
  `engine.setActiveChord(detector.current())`.
- **Chord Memory:** when the zone empties, do **not** send `Chord::none` — retain the last valid chord
  (Memory on). A future Memory-off path would send the style's `originalChord` instead.
- Right-hand (melody) notes are unaffected; chord-zone notes still sound per the existing behaviour
  (a "chord zone mute" toggle is possible later, not in scope).

### 4.7 Persistence (`ArrangerModel` / `ArrangerStyleFile` / `ArrangerStyleIOHelper`)
- Add **style-level** original chord to `ArrangerStyle` and `ArrangerStyleFile`:
  `int originalRoot = 0; ChordQuality originalQuality = ChordQuality::Maj;` (C major default).
- Bump `ARRANGER_SCHEMA_VERSION` and `ArrangerStyleFile::schemaVersion` **3 → 4**.
- `ArrangerStyleIOHelper` serializes the two fields; **loading a v3 file defaults to C major** (no
  field present) and is upgraded on next save. Malformed/newer-than-known versions handled as today.
- The per-track `ArrangerTrack::originalChord` string field is now superseded by the style-level pair;
  leave it inert/removed per implementation tidiness (decided in the plan).

### 4.8 Editor (`ArrangerStyleEditor`)
- Add **root** (`C…B`) and **quality** (`maj/min/…`) combos to the top bar, grouped next to the name
  field (`resized()` lays the toolbar). Drop to a thin second row only if width is tight.
- Pre-fill from auto-detect after `loadRecording` / Update Tracks (or C-major fallback); author edits
  feed the style-level fields; saved via the existing save path.
- **Auto-detect** runs over the recording's pitched (non-drum) source tracks at build/Update-Tracks
  time using a key-finding heuristic; low confidence → C major. Detection is offline (authoring
  thread), never in the playback path.

### 4.9 Data flow
```
keyboard ─► MidiHandler ──(ChordScanArea: Split=left-zone / Full=all)──► ChordDetector ─► Chord
                                                                              │ engine.setActiveChord  (mailbox, lock-guarded)
                                                                              ▼
section events ─► Scheduler.advance (carries `transposable`) ─► [emit loop]
                       per event ─► ChordTransposer.transpose(note, transposable) ─► dispatch
                                          Drum/Perc (transposable=false): unchanged
seams/flush: note-offs reuse the same transposed pitch as their note-on (no hung notes)
```

---

## 5. Threading model

Identical to the proven section-switch path: the **input thread writes** the detected chord into a
`CriticalSection`-guarded mailbox; the **timer thread is the only reader** and the only mutator of
`ChordTransposer` / playback state. No `ChordTransposer` or sequencer state is touched off the timer
thread. The detector lives on the input thread and is never read by the timer thread (only its
`Chord` result crosses, via the mailbox).

---

## 6. Persistence file format (`.style`, `schemaVersion = 4`)

Additive to the v3 format — two new style-level fields:
```json
{
  "schemaVersion": 4,
  "id": "…", "name": "Sunset Groove",
  "originalTempo": 120.0, "timeSigNum": 4, "timeSigDenom": 4,
  "originalRoot": 0,            // 0 = C … 11 = B
  "originalQuality": "Maj",     // stable string
  "sourceTracks": [ … unchanged … ],
  "sections":     [ … unchanged … ]
}
```
- A v3 file (no `originalRoot`/`originalQuality`) loads as **C major** and upgrades to v4 on next save.
- `originalQuality` serialized as a stable string (same idiom as `partType`/`type`).

---

## 7. Testing

**Unit — `ChordDetector`** (no audio):
- Note-sets → expected chords: C-E-G → C maj; A-C-E → A min; G-B-D-F → G dom7; D-G-A → D sus4; etc.
- **Inversions:** E-G-C and G-C-E both → C maj, root C (not the lowest note).
- Bass note captured = lowest held note (for inversion).
- Sparse/unrecognized set → `Chord::none`.
- `noteOff` removes notes; emptying the set yields `none` (Memory is the handler's job, not the detector's).

**Unit — `ChordTransposer`:**
- Stage 1: block shift correct; `transposable=false` returns the note unchanged; clamp/fold at 0..127.
- Stage 3 (NTT): per-note role mapping — C-major source onto Dm7/Dm7b5/Gsus etc. yields the specified
  target tones; drums never move; a note keeps its mapping for both its on and its off.
- Original==active (same chord) → identity for all notes.

**Unit — schema round-trip (`ArrangerStyleIOHelper`):**
- v4 save→load preserves `originalRoot`/`originalQuality`.
- v3 file (no fields) loads as C major without error.

**Engine-level (extends existing arranger suite, all on `renderRange`):**
- A chord set via `setActiveChord` is applied at the next emit; emitted transposable note numbers
  shift accordingly; drum notes do not.
- **No hung notes:** every transposed note-on has a matching note-off at the same pitch across loop
  seams and section switches (reuse the existing seam/flush tests with a non-identity chord).

**Manual smoke (the real proof):** load a style, hold C major (left hand) → plays in C; hold A minor →
backing follows, minor 3rds correct; hold G7 → dominant color present (Stage 3); lift hand → keeps
last chord (Memory); play melody in the right hand → backing unaffected; drums never change pitch;
toggle Bass Inversion → slash chords in the bass. Switch scan-area to Full (Stage 6) → both hands
drive the chord, no lurching.

---

## 8. Risks

- **Note-off / pitch matching across transposition** (highest): a note-on transposed one way and its
  off another → stuck note. Mitigated by transposing per sounding note (the off reuses the on's mapped
  pitch) and the explicit hung-note tests; reuses the engine's existing active-note/seam machinery.
- **Chord change mid-loop while notes sound:** a chord change must not retro-shift already-sounding
  notes. Apply new chord to notes emitted *after* the change; let sounding notes finish/close at the
  next seam (consistent with how section switches already behave).
- **NTT musicality** (Stage 3): edge cases (dim/aug/sus voicings) — contained to the pure transposer
  with table-driven tests; Stages 1–2 de-risk the plumbing first.
- **Detection ambiguity** (Stage 2): inversions/partial chords — interval-template matching + tests;
  Full-Keyboard ambiguity is explicitly deferred to Stage 6.
- **Schema bump:** v3→v4 back-compat covered by the round-trip test; default keeps old styles playing.
- **Threading:** none new — mirrors the existing mailbox; detector never shares state with the timer.

---

## 9. Files

**New:**
- `Source/Arranger/Chord.h` — `Chord`, `ChordQuality`, string<->enum helpers.
- `Source/Arranger/ChordDetector.h` / `.cpp`
- `Source/Arranger/ChordTransposer.h` / `.cpp`
- `tests/unit/test_chord_detector.cpp`
- `tests/unit/test_chord_transposer.cpp`

**Modified:**
- `Source/Arranger/ArrangerModel.h` — add style-level `originalRoot`/`originalQuality` to
  `ArrangerStyle`; retire the now-superseded per-track `ArrangerTrack::originalChord`. (The legacy
  `ARRANGER_SCHEMA_VERSION` constant currently reads 2 while the on-disk format is already v3 — the
  persisted format version is the source of truth; align/​retire the stray constant in the plan.)
- `Source/Arranger/ArrangerStyleFile.h` — add style-level `originalRoot`/`originalQuality`; bump the
  on-disk `schemaVersion` **3 → 4** (the authoritative persisted version).
- `Source/Arranger/ArrangerStyleIOHelper.cpp` — serialize/deserialize new fields + v3 default.
- `Source/Arranger/ArrangerScheduler.h` / `.cpp` — `transposable` (+ bass) tag on stored/emitted events.
- `Source/Arranger/ArrangerEngine.h` / `.cpp` — chord mailbox, `setActiveChord`, `ChordTransposer`
  member, tagging in `rebuildFromStyle`, emit-loop transposition, `setOriginalChord` from style.
- `Source/Midi/MidiHandler.h` / `.cpp` — `ChordScanArea`, `ChordDetector`, forward chord-zone notes,
  push `setActiveChord`, Chord Memory.
- `Source/Arranger/ArrangerStyleEditor.h` / `.cpp` — root+quality combos, auto-detect pre-fill, save.
- `Source/Styles/CurrentStyleComponent.*` — wire editor combos / auto-detect at build & Update Tracks;
  expose Bass Inversion + scan-area toggles as needed.
- `tests/unit/test_arranger_style_io.cpp`, `test_arranger_engine*.cpp` — schema + transposition tests.
- `Project Synth2.jucer` — register new files.

---

## 10. Stages (each builds, all tests pass, audibly verifiable)

1. **Walking skeleton (root-only, end-to-end).** Tagging + scheduler flag; engine chord mailbox +
   emit transposition; `ChordDetector` returns root only; `ChordTransposer` block-shift; `originalChord`
   hardcoded C major; `MidiHandler` forwards left-zone notes through a `ChordScanArea` seam (default
   Split). *Outcome: finger a root, the whole backing moves; drums don't.*
2. **Fingered recognition.** `ChordDetector` → full quality + extensions, inversion-correct.
3. **NTT mapping.** `ChordTransposer` → real note-role → chord-tone transposition (the actual Option 3).
4. **Original chord.** Auto-detect at build/Update-Tracks, C-major fallback, editor combos, schema
   v3→v4 persistence; engine reads the style-level chord instead of the hardcode.
5. **Extras.** Chord Memory (hold last chord) and the optional Bass Inversion toggle.
6. **Full-Keyboard mode.** Chord/melody inference in front of the detector, tuned by ear on the
   finished pipeline.

---

## 11. How this sets up later phases

The NTT and detector are the substrate Phase 5/6 build on: user-editable NTT tables, Single-Finger
mode, scale/microtuning, and the deferred Yamaha/Korg importer (whose CV/NTT/original-chord payload
now has a real consumer) all attach to these same pure units without touching the engine's threading
or the sequencer.
