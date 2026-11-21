# Arranger Phase 7a — Real NTT Engine (Note Transposition Tables)

> **Status:** design / spec. Supersedes the single root-shift + third-flip algorithm in
> `ChordTransposer` with a proper per-track, type-dispatched transposition engine that remaps
> *every* note role and snaps passing tones to a chord-quality scale.

**Date:** 2026-06-26
**Branch (current):** `arranger-engine-phase4-chords` (Phase 4/5/6/6b still uncommitted at time of writing — see Build Order note)
**Author of record:** KunMihai

---

## 1. Goal

Replace the current one-size transposition (`ChordTransposer::transpose`) with a real **NTT engine**:
a small menu of **per-track transposition types**, where the "smart" type remaps each accompaniment
note to the **musically correct** tone of the held chord (not a fixed parallel shift), and snaps
non-chord (passing) tones to the held chord's **scale**. This is the foundation a later Yamaha/Korg
**style importer** consumes (the imported style data specifies an NTT type per channel).

This phase is **Layer 2** of Korg's two-layer system. **Layer 1** (Chord Variation tables — recording
multiple voicings per section) is a separate later spec, **Phase 7b**, built on top of this engine.

## 2. Non-goals (explicitly deferred)

- **Chord Variation (CV) tables** — recording up to 6 voicings per section + the chord→CV map. → Phase 7b.
- **Full user-editable scale/NTT tables** for every chord quality. → later "user-editable NTT tables" phase.
  (This phase ships only a *curated* minor-scale selector — see §5.)
- **The Yamaha/Korg binary importer** itself. → Phase 8 (Yamaha) / Phase 9 (Korg).
- Microtuning / non-12-TET, slash-chord voicings beyond the existing Bass Inversion.

## 3. Background — what exists today

`ChordTransposer::transpose(noteNumber, PartKind)` (`Source/Arranger/ChordTransposer.cpp`) does three
things: (a) **Parallel root shift** of every note by `active.root - original.root`; (b) a **third-flip**
that nudges *only* notes landing on the original 3rd when major↔minor; (c) **Bass Inversion** re-basing
for slash chords. It deliberately does **not** remap the 5th/7th, and deliberately does **not** snap
passing tones (a code comment guards against "destroying non-chord-tone content").

The third-flip is, in effect, role-remapping applied to a single role. This phase **generalizes it to
all roles** and adds the scale-aware passing-tone handling that makes snapping safe rather than
destructive (the very concern the old comment raised).

**Drum/percussion identity already exists** and is reused, not reinvented: `TrackIOHelper::foundPercussion`
(`Source/Common/IOHelper.cpp:276`) classifies a track as percussion iff it has any note on **channel 10**;
`ArrangerChannels::kDrumChannel = 10` and the melodic-channel allocator skips 10. NTT `NoTranspose` keys
off this existing classification — no new detection.

## 4. The four NTT types

Each track stores one `NttType`. The engine dispatches on it per note.

| Type | Behavior |
|---|---|
| **NoTranspose** | Note returned unchanged. Default for Drum/Perc tracks (channel-10 convention). |
| **Parallel** | Every note shifted by the same `active.root - original.root` interval, then wrap-around (§6). Default for Bass. This is today's base behavior (without the third-flip). |
| **Chord** | The smart remap. Chord-tone notes → same **role** of the active chord; passing tones → parallel-shift then **snap to the active chord's scale** (§5). Then wrap-around. Default for Acc. |
| **Fixed** | Same role + scale logic as **Chord**, but the result is placed in the **octave nearest the original note** (minimal movement / no register drift). For guitar/organ voicings. |

**Bass Inversion** (existing) layers on top, unchanged: when on and a bass note is present, the Bass
part re-bases as it does today, regardless of NTT type.

### 4.1 Role remap (Chord / Fixed, Case A — chord tones)

A note's **role** = its semitone degree above `original.root` (`(note - original.root) mod 12`). If that
degree is a chord tone of the **original** chord (per `chordIntervals(original.quality)` in `Chord.h`),
map it to the **same-index role** of the **active** chord (`chordIntervals(active.quality)`):

- original root (degree 0) → active root
- original 3rd → active 3rd (maj/min per held chord)
- original 5th → active 5th (incl. ♭5 of dim, ♯5 of aug)
- original 7th → active 7th (only if the active chord has a 7th; otherwise treat as passing tone)

The octave of the original note is preserved (the role substitution happens within the pitch class, then
the original octave offset is reapplied), before wrap-around.

### 4.2 Passing-tone snap (Chord / Fixed, Case B — non-chord tones)

If the note's degree is **not** a chord tone of the original chord: parallel-shift it by
`active.root - original.root`, then **snap to the nearest pitch class in the active chord's scale**
(§5 table). Ties resolve downward (toward the lower scale tone). Octave preserved, then wrap-around.

## 5. The scale table (data-driven)

The scale used for passing-tone snapping is chosen by the **held (active) chord's quality**. Stored as
**data** (a lookup the engine reads), not a hardcoded `switch`, so user choice and future expansion are
cheap and the audio path stays branch-light.

| Active quality | Scale | Degrees (from active root) |
|---|---|---|
| Maj, Maj7 | Ionian (major) | 0 2 4 5 7 9 11 |
| Dom7 | Mixolydian | 0 2 4 5 7 9 10 |
| Min, Min7 | **(selectable — see below)** | default Dorian: 0 2 3 5 7 9 10 |
| Dim | Whole-half diminished | 0 2 3 5 6 8 9 11 |
| HalfDim (min7♭5) | Locrian | 0 1 3 5 6 8 10 |
| Aug | Whole-tone | 0 2 4 6 8 10 |
| Sus2, Sus4 | Mixolydian | 0 2 4 5 7 9 10 |
| None | (engine returns note unchanged — no active chord) | — |

### 5.1 Curated minor-scale selector (the one user choice this phase)

A global `MinorScaleChoice` setting picks the **Min/Min7** row from a curated list:

- **Dorian** — `0 2 3 5 7 9 10` (default; bright natural-6, common arranger choice, avoids ♭6 clash)
- **Aeolian** (natural minor) — `0 2 3 5 7 8 10` (darker, textbook minor)
- **HarmonicMinor** — `0 2 3 5 7 8 11` (raised 7th)

Changing the setting swaps only the minor row in the data table. Persisted via the existing
`PropertiesFile` (key `NttMinorScale`, values `0/1/2`). Lives in Settings (`MIDIWindow`), next to Chord
Memory / Bass Inversion / Synchro Start / Count-In.

## 6. Wrap-around (register guard)

Parallel and Chord apply a musical register guard replacing today's crude `while (>127)` MIDI clamp: if a
remapped note ends up more than **one octave above** where it started (the pre-transpose note), fold it
**down** an octave; symmetrically fold up if more than an octave below. Repeat until within range. This
keeps a transposed pattern from drifting away into an extreme register. **Fixed** satisfies this by
construction (it already picks the nearest octave). MIDI-range safety (0–127) is still enforced as a final
clamp.

## 7. Master bypass toggle — "Chord Transposition: On/Off"

A global Settings toggle. When **Off**, the engine forces **every** track to `NoTranspose` — the style
plays in its **recorded home key** no matter what chords are played — while **transport** (sections, fills,
intro/ending, Synchro Start, Count-In) and **chord recognition** keep running untouched. When **On**
(default), per-track NTT types apply as normal.

Layering (top is always independent of the layers below it):

```
Transport (sections / fills / synchro / count-in)   ← always on, independent of NTT
Chord recognition (detect what is played)            ← unaffected by the master toggle
NTT transposition (apply to notes)                   ← THIS is what the master toggle bypasses
```

Implemented as a one-line engine path (force `NoTranspose`) + one Settings checkbox (key `NttEnabled`,
default true).

## 8. Data model & persistence

**New enums** (in `Source/Arranger/Chord.h`, beside the existing ones):

```cpp
enum class NttType         { NoTranspose, Parallel, Chord, Fixed };
enum class MinorScaleChoice { Dorian, Aeolian, HarmonicMinor };
```

**Per-track field** (in `Source/Arranger/ArrangerModel.h`, `struct ArrangerTrack`):

```cpp
NttType nttType = NttType::Chord;   // overridden by part-derived default at construction/migration
```

**Part-derived default** (computed from signals already present):
- `ArrangerPartType::Drum` / `Perc` → `NoTranspose`
- name contains `"bass"` (as `ArrangerPatternBuilder.cpp:53` already detects) / `ArrangerPartType::Bass` → `Parallel`
- otherwise (`Acc`) → `Chord`

**Schema & migration:** bump the on-disk `.style` schema version (confirm the *actual* serialized version
in the style I/O code during planning — the in-header `ARRANGER_SCHEMA_VERSION` constant reads `2`, which
looks stale vs. the v3/v4 history; the bump must be applied wherever the `.style` is actually versioned).
**Backward compatible:** any track loaded without an `nttType` field receives the part-derived default, so
**no existing style breaks**.

## 9. UI

Two controls, both via existing patterns:

1. **Per-track NTT-type selector** — in `Source/Arranger/ArrangerStyleEditor` track rows. A 4-way selector
   (NoTranspose / Parallel / Chord / Fixed) per track, persisted into the `.style`.
2. **Minor-scale selector** (curated 3-way) **and** the **master Chord-Transposition On/Off** toggle — in
   `Source/UI/SettingsWindow` (`MIDIWindow`), grouped with the existing arranger toggles, persisted via
   `PropertiesFile`.

## 10. Engine wiring

`ArrangerEngine` already calls `transposer.transpose(...)` at emit time and already knows each track's
part. Changes:
- Pass each track's `NttType` into `transpose(noteNumber, part, nttType)` (new signature).
- `transposer.setMinorScale(MinorScaleChoice)` applied from the setting (set off the audio path, e.g. on
  load / setting change — never allocate or branch heavily inside the per-note call).
- `transposer.setNttEnabled(bool)` for the master toggle (when false, `transpose` returns the input note
  for all parts except it still honors `NoTranspose` trivially).

**Purity / latency rule preserved:** `ChordTransposer` stays free of JUCE GUI/threads; `transpose` is a
pure function reading small fixed tables — no allocation, no locking on the per-note path. All table
selection (minor scale, enabled flag, original/active chord) is set ahead of time, read atomically at emit.

## 11. Testing (pure unit tests — `tests/unit/test_chord_transposer.cpp`)

- **NoTranspose**: returns input unchanged for every note (and is forced for Drum/Perc parts).
- **Parallel**: every note shifts by exactly `active.root - original.root`.
- **Chord role correctness**, per active quality: assert the 3rd/5th/7th land on the correct pitch classes
  for Maj, Min, Dom7, Maj7, Min7, Dim, HalfDim, Aug, Sus2, Sus4.
- **Passing-tone snap**: a note that is non-chord in the original lands on the nearest **active-scale** tone.
- **Minor-scale switch**: the same passing tone over a minor chord resolves to a different note under
  Dorian vs Aeolian vs HarmonicMinor.
- **Fixed**: result is in the octave nearest the original note (no register drift), roles still correct.
- **Wrap-around**: a note pushed >1 octave from origin folds back within an octave; final 0–127 clamp holds.
- **Master bypass**: with NTT disabled, all parts return their input notes.
- **Migration**: a style loaded without `nttType` fields gets correct part-derived defaults
  (Drum→NoTranspose, bass-named→Parallel, Acc→Chord).
- **Bass Inversion** regression: existing inversion behavior unchanged when layered on the new engine.

## 12. Build order (each step its own TDD cycle)

1. `NttType` + `MinorScaleChoice` enums; `ArrangerTrack.nttType` field + part-derived default + schema
   bump/migration. (Test: migration defaults.)
2. Data-driven scale table + `MinorScaleChoice` selection. (Test: table returns expected degrees; minor row swaps.)
3. `transpose(note, part, NttType)` rewrite — the four types incl. role remap + passing-tone snap. (Tests: §11 type/role/snap.)
4. Wrap-around register guard. (Test: fold-back.)
5. Master `setNttEnabled` bypass + Settings toggle (`NttEnabled`). (Test: bypass returns inputs.)
6. Minor-scale Settings selector (`NttMinorScale`) wiring. (Test via step 2 + a settings round-trip if applicable.)
7. Per-track NTT-type selector in `ArrangerStyleEditor`; persist into `.style`.
8. `ArrangerEngine` wiring: pass per-track type; apply minor-scale + enabled from settings.

**Sequencing note:** Phase 4/5/6/6b are uncommitted on `arranger-engine-phase4-chords`. Per the user's
workflow, those should be **smoke-tested and committed before** 7a implementation begins, so this engine is
built on a verified base rather than stacked on unverified work.

## 13. Phase relationship

```
Phase 7a (this spec) — NTT engine (Layer 2)
        │  provides per-track NttType + transposition
        ▼
Phase 7b — Chord Variation tables (Layer 1): record up to 6 voicings/section + chord→CV map (separate spec)
        │
        ▼
Phase 8 — Yamaha importer (.sty/.prs): SMF + CASM → .style, sets NttType per channel
        │
        ▼
Phase 9 — Korg importer (.sty/.set): proprietary binary container
```
