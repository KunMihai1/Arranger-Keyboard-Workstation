# CI/CD + Test-Target Separation — Design

**Date:** 2026-06-26
**Author of record:** KunMihai
**Status:** design / spec (revised after build-reproducibility review)

---

## 1. Goal

Two outcomes, one effort:

1. **A clean production binary.** The standalone app must contain **no test code**. Today `Project Synth2.jucer` lists 34 `tests/*.cpp` files and the test runner lives (commented) in `Source/Main.cpp`, so the shipped app *is* the test exe. Split the tests into their own headless build target so the app compiles only `Source/`.
2. **A CI merge gate.** A GitHub Actions pipeline builds the test target and runs the suite on every feature-branch push and on PRs into `main`, so bugs are caught before they reach `main`. `main` stays a clean production line and never runs tests.

## 2. Non-goals (explicitly out of scope)

- **CMake migration.** Stay in Projucer (user decision). The dual-file-list cost is accepted.
- **Vendoring JUCE into the repo.** JUCE is stock 8.0.12 (see §3); CI **fetches** it pinned to that version rather than committing it. Keeps the repo lean.
- **Integration / hardware / network tests in CI.** `MidiDevice_HW`, `MidiHandler_HW` (real MIDI devices) and the Supabase client API test (network + credentials) cannot pass on a clean runner. They stay **local-only**.
- **Building the full app in CI.** The test target already compiles ~all of `Source/`, so a compile break is caught anyway. Building the GUIApp too only adds `Main.cpp` (≈100 lines) for double the build time. Skipped.
- **Build caching, release packaging, code signing, multi-OS.** Windows-only Debug build.
- **Automating branch protection.** Requiring the check to block merges is a one-time GitHub repo setting the user flips by hand (documented here).

## 3. Background — current state (corrected)

- Projucer project, `projectType="guiapp"`, single VS2022 exporter, Windows-only. A `.jucer` is locked to one project type, so a GUIApp and a headless ConsoleApp **cannot** coexist in one `.jucer`.
- **JUCE is NOT in the repo.** Only the third-party `SFZero` module is vendored (`modules/SFZero`). The JUCE framework is **stock JUCE 8.0.12**, referenced from **machine-specific paths outside the repo**, and inconsistently: most modules point at `../../JUCE/JUCE/modules`, while `juce_audio_processors_headless` points at a *second* folder `../../NEW JUCE/JUCE/modules`. On the author's machine JUCE 8.0.12 (with `Projucer.exe`) lives at `Desktop/JUCE`, and that one folder contains **all** the referenced modules, including `juce_audio_processors_headless` — so the "NEW JUCE" pointer is a stale leftover.
- `juce_audio_processors_headless` is an **official JUCE 8.0.12 module** (`vendor: juce`, "audio processor classes without UI"), not a custom fork — so a stock JUCE 8.0.12 download contains it.
- The generated `Builds/VisualStudio2022/` and `JuceLibraryCode/` are **gitignored** (`.gitignore` lines 33–34) — *not* committed. Every machine regenerates them with Projucer. → **CI must run Projucer**, then MSBuild.
- The current test runner (commented in `Source/Main.cpp`) registers `juce::UnitTest`s by category, runs them via `--run-tests` + `--unit-tests` / `--arranger-tests` / `--integration-tests`, writes `test-results.txt`, and **sets the process return value to the failure count** (0 = green) — already CI-friendly.
- Categories: `Unit` (~661 passes), `Arranger` (~701), `Integration` (~129, includes the HW + Supabase tests).

## 4. Architecture

Two Projucer projects share the same `Source/` tree; CI fetches JUCE 8.0.12 to build them.

```
Piano-app/
  Project Synth2.jucer            guiapp     -> app binary, Source/ only, NO tests
  Source/                         shared
  Source/Main.cpp                 app entry — app project ONLY
  modules/SFZero                  vendored third-party module (unchanged)
  ci/
    Tests.jucer                   consoleapp -> headless test binary  (NEW, own subfolder)
    test_main.cpp                 NEW console entry that runs the suite
    Builds/  JuceLibraryCode/     generated, gitignored
  tests/                          34 test files (compiled by the test project only)
  .github/workflows/ci.yml        NEW
  .gitignore                      add ci/Builds/ (ci/JuceLibraryCode/ already matched)
```

`Tests.jucer` lives in its **own subfolder (`ci/`)** so it generates its own `Builds/` + `JuceLibraryCode/` without clashing with the app's (two `.jucer` in one folder would both regenerate `JuceLibraryCode/`).

### 4.1 App project change — `Project Synth2.jucer`

- **Remove all 34 `tests/*.cpp` entries** from the file list.
- **Remove the `--run-tests` runner block from `Source/Main.cpp`** (moves to `ci/test_main.cpp`, §4.3); `Main.cpp` returns to being only the JUCEApplication launcher.
- Re-save in Projucer, commit. **Result:** production app compiles only `Source/`, carries zero test code.

### 4.2 Test project — `ci/Tests.jucer` (new)

- `projectType="consoleapp"`, headless.
- **File list:** every `Source/*` file **except `Source/Main.cpp`**, plus all 34 `tests/*.cpp`, plus `ci/test_main.cpp`.
- **Modules:** the same JUCE 8.0.12 modules + `SFZero` the app uses, pointing at the **normalized** JUCE location (§4.4.1).
- **Header search paths:** mirror the app's (`Source;Source/Arranger;...`, adjusted for the `ci/` location).
- VS2022 exporter, Debug, `targetName="Project Tests"`; generated `ci/Builds/` + `ci/JuceLibraryCode/` are gitignored.

### 4.3 `ci/test_main.cpp` (new) — the console entry

Lifts the existing runner logic out of `Main.cpp` into a standalone `int main(int, char**)`:

- Create a `juce::ScopedJuceInitialiser_GUI` (component/message-thread tests need it).
- Parse the category flag (`--unit-tests` / `--arranger-tests` / `--integration-tests`; default = all).
- Run `juce::UnitTestRunner` over the chosen category, `setAssertOnFailure(false)`.
- Aggregate passes/failures; write the `"<name> | passes: N | failures: M"` lines + `TOTAL` to **`test-results.txt`** (CWD) **and** print them to **stdout** (CI logs show them without an artifact download).
- `return` the total failure count (0 = success).

This permanently removes the commented-block coupling from `Main.cpp`.

### 4.4 JUCE acquisition + module-path normalization (the reproducibility fix)

#### 4.4.1 Normalize module paths (one-time repo change)
Today the `.jucer` JUCE module paths are machine-specific and split across two folders. Normalize **both** `.jucer` files so every `juce_*` module resolves from **one** consistent location that both the author's machine and CI can fill — a sibling `JUCE/` checkout (matching the author's existing `Desktop/JUCE` layout). The stale `../../NEW JUCE/...` pointer for `juce_audio_processors_headless` is repointed to the same JUCE location (the module exists there). `SFZero` stays at `modules/`. The exact path strings are pinned in the plan after a verifying Projucer resave.

#### 4.4.2 CI fetches JUCE
CI downloads the **JUCE 8.0.12 Windows release zip**, which contains both the stock module tree (every referenced module, including `juce_audio_processors_headless`) **and a prebuilt `Projucer.exe`** — so JUCE and Projucer arrive in one step, no building Projucer from source. CI places the modules at the normalized location, then runs `Projucer.exe --resave ci/Tests.jucer` → generates `ci/JuceLibraryCode/` + `ci/Builds/`. Then MSBuild.

### 4.5 CI workflow — `.github/workflows/ci.yml`

- **Runner:** `windows-latest` (VS2022 + MSBuild available; add `microsoft/setup-msbuild` if MSBuild is not on PATH).
- **Triggers + concurrency:**
  ```yaml
  on:
    push:
      branches-ignore: [ main ]
    pull_request:
      branches: [ main ]
  concurrency:
    group: ci-${{ github.ref }}
    cancel-in-progress: true
  ```
  Feature-branch pushes and PRs into `main` run; `main` pushes run nothing; the duplicate run when a PR branch is pushed is cancelled.
- **Steps:**
  1. `actions/checkout` (the repo).
  2. Download + unzip the **JUCE 8.0.12 Windows release** to the normalized sibling location (gives both the modules and `Projucer.exe`).
  3. `Projucer.exe --resave ci/Tests.jucer` (generates `ci/JuceLibraryCode/` + `ci/Builds/`).
  4. `msbuild "ci/Builds/VisualStudio2022/Project Tests.sln" /p:Configuration=Debug /p:Platform=x64 /m`.
  5. Run the generated `Project Tests.exe --run-tests --unit-tests` → fail the job on non-zero exit.
  6. Run it with `--run-tests --arranger-tests` → fail on non-zero exit.
  7. `actions/upload-artifact` for `test-results.txt`.
- **Gate scope:** `unit` + `arranger` only.

### 4.6 Defensive fix — `Source/UI/OverlayComponent.cpp`

`OverlayComponent.cpp:31` calls `juce::JUCEApplication::getInstance()->systemRequestedQuit()`. In a **console** test binary there is no `JUCEApplication` instance, so `getInstance()` is null → that path would crash if an Overlay test reaches it. Guard it:
```cpp
if (auto* app = juce::JUCEApplication::getInstance())
    app->systemRequestedQuit();
```
(`Main.cpp`'s identical call is fine — excluded from the test target. A repo-wide scan for other `JUCEApplication::getInstance()` uses in `Source/` is part of the plan; only these two exist today.)

### 4.7 The merge gate (manual, one-time)
GitHub repo **Settings → Branches → protect `main` → Require status checks to pass before merging → select the CI check.** The workflow produces the check; branch protection enforces it. Documented, not automated.

## 5. Maintenance contract

Projucer keeps explicit per-project file lists (no globbing), so **adding/removing a `Source/*` file means updating both `.jucer` files** and re-saving both. A forgotten file on the test side surfaces as a **link error in the next CI run** — loud and immediate, never silent drift. Accepted cost of staying in Projucer.

## 6. Sequencing (prerequisite)

This work **must be its own branch off `main`, started after Phase 7a lands**, because it **deletes** the `--run-tests` runner from `Main.cpp` — which would collide with the Phase 7a branch, where that runner is temporarily uncommented.

Order:
1. Commit/merge **Phase 7a** (re-commenting its `Main.cpp` runner back to baseline).
2. Branch off `main` for this CI work; here the runner is removed from `Main.cpp` entirely.

## 7. Success criteria

- Building `Project Synth2.jucer` produces an app exe that does **not** compile or contain any `tests/` code; `--run-tests` is gone from `Main.cpp`.
- Building `ci/Tests.jucer` produces `Project Tests.exe`; `--unit-tests` and `--arranger-tests` run green locally (~661 / ~701, 0 failures) and exit 0.
- Both `.jucer` files resolve all JUCE modules from one normalized location; a fresh JUCE 8.0.12 checkout there builds both projects.
- A pushed feature branch and a PR into `main` each trigger the workflow; the suite runs; a deliberately failing test turns the check red and exits non-zero.
- A push to `main` triggers **no** workflow.
- With branch protection on, a red check blocks merging the PR.

## 8. Verification plan

- Local: build both targets against `Desktop/JUCE` (8.0.12); run the test exe for each category; confirm exit codes; confirm the app build produces no `test_*.obj`.
- Local: confirm a fresh JUCE 8.0.12 at the normalized path builds both projects (proves the path normalization is portable).
- CI: open a draft PR — first all-green to confirm the gate passes, then a temporary failing test to confirm it goes red and blocks merge; revert.
