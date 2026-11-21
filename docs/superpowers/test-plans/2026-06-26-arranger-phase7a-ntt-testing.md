# Phase 7a — NTT Engine: Test Guide

**Branch:** `arranger-phase7a-ntt-engine`
**What changed:** per-track Note Transposition Tables (NTT) replace the old single root-shift + third-flip. Four per-track types, a curated minor-scale selector, a master on/off bypass, schema v5 + migration, and two new UI surfaces (Settings + style editor).

This guide has two halves: **automated tests** (already green) and **manual smoke tests** (need your eyes/ears).

---

## 0. Build & run the automated tests

> **UPDATED 2026-08-15 — the instructions below replace the original ones.** The app no longer
> contains any test code: the `--run-tests` runner was deleted from `Source/Main.cpp` and the 34
> test files were removed from `Project Synth2.jucer`. Tests now live in the headless
> **`ci/Tests.jucer`** console target. There is nothing left to re-comment before committing.

```powershell
# from the repo root — build the TEST target (not the app), then run each suite.
# Results land in test-results.txt (CWD) as "<name> | passes: N | failures: M".
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "ci\Builds\VisualStudio2022\Project Tests.sln" /t:Build /p:Configuration=Debug /p:Platform=x64 /m

$exe = (Get-ChildItem -Recurse -Filter "Project Tests.exe" ci\Builds | Select-Object -First 1).FullName
& $exe --arranger-tests      # ChordTransposer, NttScales, ArrangerStyleIO, ArrangerEngine, scheduler
& $exe --unit-tests
& $exe --integration-tests   # local-only: needs real devices/network. NOT run in CI.
```

**Expected (as of 2026-08-15):**

| Suite | Result | Note |
|---|---|---|
| `--arranger-tests` | TOTAL passes: **705**, failures: **0** | was 701; +4 from the bypass/bass-inversion regression test |
| `--unit-tests` | TOTAL passes: **665**, failures: **0** | was recorded as 661 under the old app-hosted runner |
| `--integration-tests` | local-only | needs real MIDI devices / network |

The process **exit code = number of failures** (0 = all green).

You do not have to run these by hand — `unit` + `arranger` run automatically in CI on every push
(`.github/workflows/ci.yml`), and a PR into `main` cannot merge unless they are green.

### What the automated tests already prove
- **NttScales**: each chord quality returns the right scale; the minor row swaps with the selector (Dorian↔Aeolian↔Harmonic).
- **ChordTransposer**: NoTranspose identity; Parallel = uniform interval; Chord role-remap (3rd/5th/7th land correctly across Maj/Min/Dom7/etc.); passing-tone scale snap with downward tie-break; Fixed = nearest octave; wrap-around fold-back; master bypass; **home-chord verbatim**; bass-inversion regression intact.
- **ArrangerStyleIO**: `nttType` round-trips through save/load; a v4 file with **no** `nttType` migrates to part-derived defaults (Drum→NoTranspose, Bass→Parallel, Acc→Chord).
- **ArrangerEngine**: a track set to **Parallel** emits the parallel result and **not** the Chord result — proving the per-track type reaches emit time.

> These cover the engine + persistence. The three things below (UI + actual audio) are **not** automated — that's what you test by hand.

---

## 1. Settings UI

Open **Settings** (the MIDI/settings window) with **Arranger mode** on.

| # | Step | Expected |
|---|---|---|
| 1.1 | Scroll to the arranger toggles | A new **"Chord transposition (NTT)"** toggle and a **"Minor scale"** row with **Dorian / Aeolian / Harmonic** buttons appear below "Mute chord keys…". |
| 1.2 | Look at the Minor-scale buttons | Selected button = warm-orange fill + dark text; unselected = dark card + muted text (same style as the Chord-Mode Fingered/1-Finger/Full-Kbd row). Exactly one is selected. |
| 1.3 | Check panel sizing | Nothing is clipped at the bottom; the panel grew to fit the two new rows (480×630). |
| 1.4 | Toggle "Chord transposition" off, close & reopen Settings | The toggle stays off (persisted as `NttEnabled`). |
| 1.5 | Click Aeolian, close & reopen Settings | Aeolian stays selected (persisted as `NttMinorScale`). |
| 1.6 | Fully restart the app | Both choices restore on launch (default = NTT **on**, scale **Dorian**). |

**Watch for:** misaligned/overlapping buttons, wrong colors vs. the rest of the panel, the row falling off the bottom edge.

---

## 2. Style editor — per-track NTT strip

Open a style in the **config/style editor**.

| # | Step | Expected |
|---|---|---|
| 2.1 | Look at the bottom of the editor | A **"NTT per track:"** header with a horizontal strip of cells below it — one per track: the **track name** on top, a **4-way combo** (No Transpose / Parallel / Chord / Fixed) under it. |
| 2.2 | Check the timeline | It kept its full width; it only lost a little height to the strip. |
| 2.3 | Check defaults | Drum/perc tracks default to **No Transpose**, bass-named tracks to **Parallel**, everything else to **Chord**. |
| 2.4 | Many tracks | If there are more cells than fit, the strip **scrolls horizontally** (scrollbar at the bottom of the strip). |
| 2.5 | Change a track's combo while previewing | The live preview rebuilds and you hear that track behave differently immediately. |
| 2.6 | Change a few types → **Save** → reopen the style | Your per-track picks are restored exactly. |
| 2.7 | **Update Tracks** (re-import the recording) | Cells rebuild; types reset to the part-derived defaults for the new tracks (expected — new recording = fresh tracks). |

**Watch for:** combos not pre-selecting the saved value, a stale strip after load/update, the popup menu opening in the wrong place, the strip stealing PC-keyboard focus.

---

## 3. Audio behavior — the heart of the phase

Set up: a style with a clear melodic/accompaniment track and a bass track, **Arranger mode on**, pick a chord mode (Fingered is easiest), engage the arranger and start playback. Hold chords in the left/split zone.

### 3.1 Home chord = verbatim (Korg-faithful)
- Hold the **same chord the style was recorded in** (shown as the editor's "Recorded key", e.g. C major).
- **Expected:** the backing sounds exactly like the raw recording — including any chromatic/passing notes. No "correction."

### 3.2 Smart remap (Chord type)
- On a track set to **Chord**, hold a **different** chord — e.g. recorded in C major, now hold **A minor**.
- **Expected:** the line follows the new chord's *shape* — the recorded 3rd becomes A-minor's minor 3rd, the 5th becomes A-minor's 5th, etc. It sounds like the phrase was re-voiced for A minor, not just slid up/down.
- Try a **major→dominant** (C → G7) and a **diminished** to hear the 5th/7th move to the right tones.

### 3.3 Passing-tone scale snap + minor selector
- Keep holding a **minor** chord on a Chord-type track that has audible passing/run notes.
- Switch **Minor scale**: **Dorian** (bright natural-6) → **Aeolian** (darker ♭6) → **Harmonic** (raised 7).
- **Expected:** the *passing* notes change color between the three; chord tones don't move. The difference is subtle but audible on runs.

### 3.4 Parallel vs Chord
- Set the same track to **Parallel**, hold A minor.
- **Expected:** the whole line slides by a fixed interval (no role remap) — including the 3rd, so a major recording stays "major-shaped" over the minor chord. Compare to Chord to hear the difference.

### 3.5 Fixed (no register drift)
- Set a track to **Fixed**, hold a chord whose root is far from the recorded root (e.g. recorded C, hold A).
- **Expected:** same tones as Chord, but the notes stay near their **original octave** instead of drifting up/down a register.

### 3.6 Drums never move
- The drum/perc track (channel 10, No Transpose) stays identical no matter what chord you hold.

### 3.7 Bass inversion still works
- With a **bass** track (Parallel) and **Bass Inversion** on, hold a slash chord (e.g. C/E).
- **Expected:** the whole bass line re-bases onto the played bass note — unchanged from before Phase 7a.

### 3.8 Master bypass ⚠️ **behaviour changed 2026-08-15 — test this one carefully**
- Turn **"Chord transposition (NTT)" OFF** in Settings, keep playing and holding chords.
- **Expected:** the backing plays in the **recorded key** regardless of what you hold — BUT sections/fills/intro/ending, Synchro Start, Count-In, and chord **recognition** all keep working. Turn it back on → transposition resumes.

> **Run this test with Bass Inversion ON — that is the case that was broken.** Until `cf423b0` the
> bypass only rewrote the per-track NTT type, and the bass-inversion branch returns *before* that is
> read, so the bass kept transposing while everything else fell back to the recorded key. And it was
> not a slash-chord edge case: `ChordDetector` sets `bassNote` on **every** recognized chord, so a
> plain root-position Am over a C-recorded style moved the bass **+9 semitones**. What you are
> listening for: with the bypass OFF and Bass Inversion ON, hold an Am — the **bass must stay in C**,
> in unison with the rest of the backing. If the bass alone follows your chord while the keys and
> drums do not, the fix regressed.
>
> Then re-check **3.7** (bypass back ON, Bass Inversion still ON, hold C/E) to confirm the fix did
> not disable bass inversion itself — the bass should re-base onto E as before. The automated suite
> covers both directions, but only your ears confirm it in real audio.

---

## 4. Migration / backward compatibility

| # | Step | Expected |
|---|---|---|
| 4.1 | Open a style saved **before** this branch (no `nttType` in its JSON) | Loads fine; no error. |
| 4.2 | Check its editor NTT strip | Drums = No Transpose, bass = Parallel, others = Chord (derived from each track's part). |
| 4.3 | Play it | Transposes sensibly out of the box with zero manual setup. |
| 4.4 | Re-save it, inspect the `.style` JSON | Now has `"schemaVersion": 5` and a `"nttType"` per track. |

---

## 5. Regression — nothing else broke

- **Recognition modes** (Fingered / 1-Finger / Full-Keyboard) behave exactly as before — they decide *what chord* is recognized; NTT only changes what happens *after*.
- **Transport**: sections, fills, intro-once, ending-then-stop, Synchro Start, Count-In all unchanged.
- **Editor preview**: Preview still starts immediately; the idle playhead still parks at the first section; moving a section mid-play still rebuilds cleanly.
- **Chord-zone mute** and **Bass Inversion** toggles still work.

---

## 6. Sign-off checklist

- [ ] Automated: all three suites 0 failures.
- [ ] Settings: both new controls render correctly, persist, and restore on restart.
- [ ] Editor: per-track strip shows correct defaults, saves/reloads, drives live preview.
- [ ] Audio: home-chord verbatim; Chord remap; Parallel ≠ Chord; Fixed no drift; drums fixed; bass inversion intact.
- [ ] Minor selector audibly changes passing tones (Dorian/Aeolian/Harmonic).
- [ ] Master bypass plays in recorded key while transport + recognition keep running.
- [ ] Old styles migrate and play correctly.
- [ ] No regression in transport / recognition / preview.
- [ ] App launches and the GUI is intact after the test-strip (`919ffa9` removed 34 test files + the `--run-tests` runner; the app has not been started since).
