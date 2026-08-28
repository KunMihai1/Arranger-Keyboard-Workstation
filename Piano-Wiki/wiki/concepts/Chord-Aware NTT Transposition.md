---
type: concept
title: "Chord-Aware NTT Transposition"
complexity: intermediate
domain: arranger
aliases:
  - "NTT"
  - "note transposition table"
status: developing
created: 2026-06-25
updated: 2026-08-15
tags:
  - concept
  - arranger
  - chord
  - ntt
related:
  - "[[Chord Recognition & Transposition]]"
  - "[[Arranger Engine]]"
  - "[[ArrangerScheduler]]"
  - "[[Arranger Phase Roadmap]]"
---

# Chord-Aware NTT Transposition

How one accompaniment note recorded in the **original** chord is re-pitched to the **active** (held) chord. This is `ChordTransposer::transpose (noteNumber, PartKind, NttType)`.

> [!warning] Source lives on an unmerged branch
> Phase 7a is committed as **`614b4ec`** on `arranger-phase7a-ntt-engine` and is **not on `main`**. `Source/Arranger/NttScales.h` does not exist on `main` or on `fix/sfz-integration`. Read it with `git show 614b4ec:Source/Arranger/ChordTransposer.cpp`. See [[Arranger Phase Roadmap]].

> [!key-insight] Remap roles, snap the rest — chosen per track, not globally
> Phase 7a replaced a single global rule with **four per-track tables**. On the `Chord` table, a note that sits on a *chord tone of the recorded chord* is moved to **the same role** of the active chord (recorded 3rd → active 3rd, 5th → 5th, 7th → 7th); everything else is a **passing tone** and snaps to the active chord's scale. The recorded line keeps its shape instead of being slid wholesale or flattened onto chord tones.

## The four tables (`NttType`, `Chord.h`)

| Type | Rule | Typical use |
|---|---|---|
| `NoTranspose` | Identity — returns the note untouched | Drums / percussion |
| `Parallel` | Uniform shift by `active.root - original.root`, then octave-fold | Bass lines, riffs that must keep their exact intervals |
| `Chord` | Role-remap chord tones + scale-snap passing tones, then octave-fold | Default for harmony parts |
| `Fixed` | Same computation as `Chord`, but the result is placed in the octave **nearest the original note** — no register drift | Parts that must stay in their register |

> [!warning] Two unrelated things are both called "Fixed"
> `PartKind::Fixed` (Drum/Perc — *never* transposed, checked first) is **not** `NttType::Fixed` (transposes exactly like `Chord`, then pins the octave). `Chord.h` calls this out explicitly. Likewise `NttType::Chord` is unrelated to `ChordMode` (which is *recognition*, see [[Chord Recognition & Transposition]]).

## Decision order — the part that actually matters

`transpose()` short-circuits in this exact order. Reading it out of order is how you get the behaviour wrong:

```
1. PartKind::Fixed, or invalid active/original chord   -> note unchanged
2. if (!nttEnabled) return note                          // master bypass, short-circuits everything
3. Bass part + bassInversion + active.bassNote >= 0     -> note + (bassNote - original.root)   [RETURNS]
4. ntt == NoTranspose                                   -> note unchanged
5. ntt == Parallel                                      -> wrapToOctave(note + interval)
6. interval == 0 && same quality                        -> note unchanged   (home-chord verbatim)
7. Chord / Fixed                                        -> role remap or scale snap
8. Fixed -> nearest octave to original;  Chord -> wrapToOctave
```

> [!done] FIXED in `cf423b0` (2026-08-15) — step 2 below is now `return noteNumber;`
> The decision order above documents the **fixed** code. Historical bug, kept because it explains why the ordering is load-bearing: step 2 used to be `ntt = NttType::NoTranspose;`, which ran **before** step 3's early return and so could never reach the bass. With **"Chord transposition (NTT)" OFF** and Bass Inversion ON, the bass was still shifted by `active.bassNote - original.root` while every other part played in the recorded key — a bitonal result nobody would ask for.
>
> **Verified by reverting:** with the old code the regression test failed 3/4 at exactly the predicted magnitudes — root-position Am `Expected 43, Actual 52` (+9), C/E `Expected 43, Actual 47` (+4). With the fix: arranger **705/0**, unit **665/0**.
>
> **This is not a slash-chord edge case.** `ChordDetector::recognizeSet` sets `bassNote = lowestPc` for *every* recognized chord in every mode — it is never left at `-1` for a valid chord. Holding a plain root-position A minor against a style recorded in C shifts the bass **+9 semitones**. The bypass is silently a no-op for the bass on every chord you hold.
>
> Why it reads as a bug rather than a design choice:
> 1. **It contradicts the stated contract.** Phase 7a test guide §3.8: with the bypass off "the backing plays in the **recorded key** regardless of what you hold."
> 2. **The formula is only coherent while transposing.** `active.bassNote - original.root` mixes the *active* chord's bass with the *recorded* root. A genuine "recorded key but still honour inversions" rule would be relative to the active chord's own root, so a root-position chord contributes 0. This one contributes the full root interval.
> 3. **Its shape is the classic regression.** The bass-inversion early-return is pre-existing Phase 4 code — the first branch after the validity guard in `6f2ba81`. Phase 7a inserted the new bypass *above* it, and a type-rewrite cannot reach past an early return.
> 4. **The test that claims to cover it doesn't.** `test_chord_transposer.cpp` → `"master bypass: disabled NTT returns input notes for all parts/types"` does assert on `PartKind::Bass`, but never calls `setBassInversion(true)`, so `bassInversion` stays `false` and the buggy branch is never entered. It passes while the invariant it names is false.
>
> **Recommended fix** — make the bypass a true short-circuit, replacing the type-rewrite:
> ```cpp
> if (! nttEnabled)
>     return noteNumber;   // master bypass: play the recorded home key, no exceptions
> ```
> Equivalent to gating step 3 on `nttEnabled`, with fewer moving parts. Add a regression test: bypass off, `setBassInversion (true)`, active chord with `bassNote != original.root`, expect the input note back.
>
> The code comment ("independent of NTT **type**") justifies independence from the per-track `NttType` selector, which is legitimate — a bass track should invert whether it is `Parallel` or `Chord`. The master *enable* switch is a different control that merely happens to be implemented by rewriting the type. That is where it leaks.

## Step 7 in detail — role remap vs. scale snap

```cpp
deg  = mod12 (note - original.root)          // the note's degree in the RECORDED chord
orig = chordIntervals (original.quality)     // e.g. Maj7 -> {0,4,7,11}
act  = chordIntervals (active.quality)       // e.g. Min  -> {0,3,7}
idx  = indexOfDegree (orig, deg)

if (idx >= 0 && idx < act.size())
    result = note + interval + (act[idx] - orig[idx]);   // same ROLE of the active chord
else
    result = snapToScale (note + interval);              // passing tone -> active scale
```

- `chordIntervals()` lives in `Chord.h` and is **shared with `ChordDetector`**, deliberately, so recognition templates and transposition tones can never drift apart.
- **A recorded extension can demote to a passing tone.** If the recorded chord is richer than the active one — recorded `Maj7` (`idx` 3 for the 11th semitone) over an active plain `Maj` (`act.size()` 3) — the guard `idx < act.size()` fails and the recorded 7th is scale-snapped rather than role-mapped. Same for `Min7`/`HalfDim` over triads.

### The snap scales (`NttScales.h`)

Chosen by the **active** chord's quality, root-relative:

| Active quality | Scale | Degrees |
|---|---|---|
| `Maj`, `Maj7` | Ionian | 0 2 4 5 7 9 11 |
| `Dom7`, `Sus2`, `Sus4` | Mixolydian | 0 2 4 5 7 9 10 |
| `Min`, `Min7` | **user-selectable** (below) | — |
| `Dim` | whole-half diminished (8 tones) | 0 2 3 5 6 8 9 11 |
| `HalfDim` | Locrian | 0 1 3 5 6 8 10 |
| `Aug` | whole-tone (6 tones) | 0 2 4 6 8 10 |
| `None` / default | Ionian | 0 2 4 5 7 9 11 |

The **minor row is the one curated, user-facing choice** (`MinorScaleChoice`, Settings): **Dorian** `0 2 3 5 7 9 10` (default), **Aeolian** `0 2 3 5 7 8 10`, **Harmonic** `0 2 3 5 7 8 11`. Only Min/Min7 consult it. User-editable tables for *every* quality remain deferred.

`snapToScale` picks the nearest scale pitch by absolute distance, considering both the candidate above and an octave below it; **ties break downward** (`d == bestDist && cand < best`).

### Octave handling — two different rules

- `wrapToOctave (origin, n)` (used by `Parallel` and `Chord`) only folds when the result drifts **more than** an octave: `while (n - origin > 12) n -= 12`. So up to exactly ±12 semitones of drift is allowed to pass.
- `NttType::Fixed` instead re-seats the resulting **pitch class** in the octave nearest the original note: `(up - noteNumber <= 6) ? up : up - 12`. A tritone (exactly 6) resolves **upward**.
- `clampMidi` folds by octaves rather than clipping, so an out-of-range result stays musical instead of piling up on note 0/127.

## Home chord = verbatim (step 6)

When the played chord matches the recorded chord's **root and quality**, the source is already correct, so it is returned **untouched** — chromatic passing tones and all. This is deliberate Korg-faithful behaviour and is the acceptance test the pre-7a snap-to-tone attempt failed. Bass inversion can differ while root/quality match, which is exactly why step 3 sits above it.

## Bass inversion (Bass parts)

A Settings toggle, layered on top of NTT and independent of the track's type:
- **Off:** the bass follows the chord **root** like any other part — C/E sounds the bass on **C**.
- **On:** the whole bass line is **re-based onto the played bass note** — `note + (active.bassNote - original.root)` — so C/E sounds the bass on **E**. This re-bases the *entire line*, not just its root note, which is the Yamaha/Korg behaviour.

> [!key-insight] It's genuinely subtle to hear
> Re-basing on inversions (E-G-C vs C-E-G) is a small audible difference, easy to miss without good monitoring — confirmed during testing via a debug log of `G1 -> E1`. Not a bug.

## Defaults, persistence, migration

- Per-track default is derived from the part role (`ArrangerEnums::nttDefaultForPart`): **Drum/Perc → `NoTranspose`, Bass → `Parallel`, Acc → `Chord`**. Applied both to freshly built tracks (`ArrangerSourceBuilder`) and to migration.
- `ArrangerTrack::nttType` / `SourceTrackFile::nttType` carry it; **schema v5** (`ArrangerStyleFile.h`) adds `"nttType"` as a string per track.
- **Migration is by absence, not by version number:** `ArrangerStyleIOHelper::loadFromFile` reads the property and, if the string is empty, falls back to `nttDefaultForPart(t.partType)`. So a pre-v5 style transposes sensibly with zero manual setup, and re-saving stamps `schemaVersion: 5` plus an explicit per-track value.

## How the type reaches emit time

The transposer is **stateless and runs per dispatched event** in the [[Arranger Engine]] — the stored pattern is never rewritten, so the saved `.style` stays in its home key and a held chord can change mid-loop.

`ArrangerEngine::rebuildFromStyle` flattens every track's events into one merged vector and builds **two parallel tag vectors** — `PartKind` (Phase 4) and `NttType` (Phase 7a) — handed to `ArrangerScheduler::setLoop`. The scheduler keeps them aligned through its stable sort (`sortedParts`, `sortedNtt`), stamps both onto each `EmittedEvent`, and — critically — remembers them per sounding note (`activeNoteParts`, `activeNoteNtt`) so **loop-seam and section-switch note-offs carry the same tags as their note-on**. See [[ArrangerScheduler]], [[Loop-Seam Note-Off]].

The engine's `activePlayedNote` map closes the loop on the other side: a note-off is re-pitched to the value its note-on actually played, so a chord change mid-note can't hang it. Shorter tag vectors pad with `PartKind::Fixed` / `NttType::Chord`, which keeps the two legacy `setLoop` overloads working.

Settings → `MainComponent` → `Display` → `CurrentStyleComponent` → `ArrangerEngine::setNttEnabled` / `setMinorScale` wires the two global controls; the per-track type is edited in the style editor's NTT strip.

## What this replaced (pre-7a history)

Phase 4 shipped a single global rule: shift every note by the root interval, then flip **only** the third when the quality changed major↔minor. That itself replaced a first attempt that *snapped every note to the nearest active-chord tone* — which destroyed melody and voicing (passing tones, 7ths and tensions all collapsed onto root/3rd/5th) and which the user heard as "really weird, C doesn't even restore the original". Phase 7a keeps that hard-won constraint — **identity on the home chord** — while adding real per-role remapping and scale-aware snapping.

## Related
[[Chord Recognition & Transposition]] (what decides the active chord) · [[Arranger Engine]] (emit-time dispatch) · [[ArrangerScheduler]] (tag alignment) · [[Arranger Phase Roadmap]] (7b Chord Variation tables build on this).
