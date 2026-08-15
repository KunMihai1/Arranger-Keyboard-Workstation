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


# Piano App — Synth & Arranger

This is a synth/arranger app I built using JUCE. The idea was to make something that’s fun to play with but also useful for experimenting with tracks and arrangements. It’s still a work in progress, but it already lets you play, record, and mix MIDI tracks in real time.

## Features

- Play notes in real time — the piano keys respond instantly.
- Record your performance — capture what you play without noticeable lag.
- Load MIDI files — pick which tracks you want to play along with.
- Track Modifications — modify time stamps, velocities, notes without affecting the original state of the track.
- Visual keyboard — see which keys are active as you play.
- Keyboard range assignment — select distinct ranges for left and right hands and assign different instruments to each, enabling layered performances.
- Built with future arranger features in mind (more coming soon).

## Tech

- **Framework:** JUCE
- **Language:** C++
- **Audio:** Low-latency, real-time processing
- **UI:** JUCE components for cross-platform desktop apps
- **Backend / API:** Supabase Edge Functions (HTTPS API)
- **Database:** PostgreSQL (managed via Supabase)

## What I Learned

Working on this project taught me a lot about low-latency audio programming and the challenges of real-time performance. Some highlights:

- Handling audio and GUI threads without lag
- Recording and playing MIDI in sync
- Structuring a JUCE project for expandability
- Optimizing C++ code for real-time responsiveness

It was a great experience for getting hands-on with how music software actually works under the hood.

## Future Plans

- Expand arranger features: patterns, automation, loops, and divide tracks into sections (intro, main, ending)
- Enhance visual feedback: velocity, timing, key animations
- Integrate and improve MIDI controller support
- Apply BPM changes dynamically during playback, efficiently
- Unit testing for everything

## Skills I Gained

- Real-time audio programming
- MIDI and event handling
- C++ and JUCE framework
- UI/UX for music applications
- Optimizing for performance
