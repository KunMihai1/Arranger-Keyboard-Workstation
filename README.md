## Built With

- [JUCE](https://juce.com) — an open-source C++ framework for developing cross-platform audio applications.  
- Licensed under the [JUCE License (GPLv3 / Commercial)](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)

This project uses JUCE under the terms of the [GNU General Public License v3 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.html).


## Building & Tests

**Prerequisite:** JUCE **8.0.13**. In Projucer, set **Global Paths → Path to JUCE modules** to your
local `JUCE/modules`. Both projects resolve JUCE through that global path, so nothing is vendored
and no machine-specific path is committed. Only `SFZero` is referenced from the in-repo `modules/`.

The repo has **two** Projucer projects sharing `Source/`:

| Project | Target | Purpose |
|---|---|---|
| `Project Synth2.jucer` | GUI app | the shipped application — compiles **no** test code |
| `ci/Tests.jucer` | Console app | headless test runner (`Project Tests.exe`) |

- **App:** open `Project Synth2.jucer`, save, then build `Builds/VisualStudio2022` (Debug x64).
- **Tests:** open `ci/Tests.jucer`, save, build `ci/Builds/VisualStudio2022`, then run
  `Project Tests.exe --unit-tests` and `--arranger-tests`. The **exit code is the number of failing
  assertions** (0 = green), and results are written to `test-results.txt` in the working directory.
  Integration / `*_HW` / Supabase tests are local-only — they need real devices and network.

Either project can also be regenerated from the command line, which is what CI does:

```
Projucer.exe --set-global-search-path windows defaultJuceModulePath "<path>\JUCE\modules"
Projucer.exe --resave "ci\Tests.jucer"
```

> **Note:** `Projucer.exe` is a GUI-subsystem executable and returns immediately when launched
> directly, so a following build can race its file writes and silently compile a stale file list.
> Wait on it explicitly — e.g. PowerShell `Start-Process -Wait -NoNewWindow`.

**CI:** `.github/workflows/ci.yml` runs `unit` + `arranger` on every feature-branch push and on PRs
into `main`. `main` itself runs no workflow. Once branch protection is enabled with the **CI / tests**
check required, a PR cannot merge until it is green.

Each CI test step checks **two** things: a zero exit code, and that the suite ran at least
`MIN_UNIT_ASSERTIONS` / `MIN_ARRANGER_ASSERTIONS` assertions (set in the workflow). The exit code
alone is not sufficient — the runner also exits 0 when it runs *no* tests, so a mistyped category or
a dropped translation unit would otherwise pass silently. If you legitimately remove tests, lower the
floor in the same commit; if you add a lot, raise it.

**Adding a `Source/` file:** add it to **both** `Project Synth2.jucer` and `ci/Tests.jucer` — Projucer
keeps explicit file lists. Forgetting the second one shows up as a link error in CI.


# Piano App — Arranger Keyboard

A software arranger keyboard built with JUCE — the desktop counterpart to a Yamaha PSR or Korg Pa.
You play, record and mix MIDI in real time, and an auto-accompaniment engine backs you: hold a chord
in the left hand and a recorded style follows you through intros, variations, fills and endings.

Sound comes from **sampled instruments (SFZ)**, not synthesis — there are no oscillators in here.

## Features

### Playing
- Real-time play from the on-screen keyboard, a connected MIDI controller, or the PC keyboard —
  mapped by physical scan code, so it works under any OS keyboard layout.
- Split keyboard — independent ranges and instruments for the left and right hand, for layered
  performance.
- Visual keyboard feedback showing which keys are active as you play.

### Arranger
- **Auto-accompaniment engine.** A 10 ms high-resolution timer drives a monotonic beat playhead.
  Section changes (Intro / Variation / Fill / Break / Ending) are queued and take effect exactly on
  the next bar line, and BPM changes alter the rate going forward without jumping musical position.
- **Chord following.** The chord held in the split zone is recognised — quality templates, sus2/sus4
  disambiguation, and hysteresis so passing melody notes don't churn the chord — and every
  accompaniment note is re-pitched at emit time. Per-track transposition modes role-remap chord tones
  (3rd / 5th / 7th), snap passing tones to scale, and handle bass inversion; drums stay fixed.
- **Style authoring.** Record a performance once, carve it into named sections on a draggable
  timeline, and save a self-contained versioned `.style` file. Older files migrate forward.

### Tracks
- Load MIDI files and pick which tracks to play along with.
- Non-destructive editing — adjust timestamps, velocities and notes without altering the original
  recorded data.

### Sound
- 16 SFZ samplers, one per MIDI channel, each with its own effects chain driven by MIDI CCs:
  low-pass filter (CC74 cutoff / CC71 resonance), distortion (CC80), chorus (CC93), reverb (CC91),
  tremolo (CC92), delay (CC94), random mod (CC95) and expression (CC11).
- Every stage is bypassed at its neutral value, so unused effects cost nothing per block, and
  derived values (LFO phase increment, distortion normalisation) are cached rather than recomputed
  per sample.
- Instruments load asynchronously, so the audio thread never blocks on file I/O.

### Backend
- Supabase-backed accounts, MIDI device registration (VID/PID) and playtime tracking, over a
  managed PostgreSQL database.

## Architecture notes

- The arranger runs **parallel** to the classic multi-track player rather than replacing it.
- Scheduling, section sequencing and time math are **pure functions**; the only threaded, impure
  piece is the engine's timer loop. That separation is what makes the engine testable.
- Audio, timer and GUI threads exchange state only through lock-guarded mailboxes — nothing mutates
  engine state directly from the message thread.

## Tech

- **Language / framework:** C++17, JUCE 8.0.13
- **Audio:** real-time SFZ sample playback (SFZero), per-channel MIDI-CC effects chain
- **Backend / API:** Supabase Edge Functions (HTTPS)
- **Database:** PostgreSQL (managed via Supabase)
- **Tests:** 1,300+ assertions across 30+ suites, run headlessly from a separate console target

## What I Learned

- Keeping audio, timer and GUI threads apart without locks in the hot path — and how much easier
  the engine is to test once the scheduling logic is pure.
- Writing per-sample effects by hand (waveshaping, LFOs, circular-buffer delay) and what it costs
  to run sixteen of them per block.
- Real chord/accompaniment transposition, the way hardware arrangers actually do it.
- Structuring a JUCE project so a test target and the shipped app can share one `Source/` tree.

## Roadmap

- Chord variation handling
- Yamaha style import
- Korg style import
- Richer visual feedback: velocity, timing, key animation
