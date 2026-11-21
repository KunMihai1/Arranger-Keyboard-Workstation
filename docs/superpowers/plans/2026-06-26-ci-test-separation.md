# CI/CD + Test-Target Separation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. **Note:** several steps are manual **Projucer GUI** actions (marked 🖱️) that an agent cannot perform — a human runs those.

**Goal:** Split the test code into a headless `ci/Tests.jucer` ConsoleApp so the production app carries no test code, make the JUCE dependency reproducible, and add a GitHub Actions gate that runs `unit` + `arranger` on branch pushes and PRs into `main`.

**Architecture:** Two Projucer projects share `Source/`: the existing `Project Synth2.jucer` (guiapp, loses the 34 test files + its `Main.cpp` runner) and a new `ci/Tests.jucer` (consoleapp, compiles `Source/` minus `Main.cpp` + `tests/` + a new `ci/test_main.cpp`). JUCE modules are referenced via Projucer's **global module path** so every machine and CI can point at their own JUCE 8.0.13. CI downloads the JUCE 8.0.13 Windows release (modules + `Projucer.exe`), sets the global path, resaves `ci/Tests.jucer`, MSBuilds it, and runs the suite.

**Tech Stack:** JUCE 8.0.13, Projucer, Visual Studio 2022 / MSBuild, GitHub Actions (`windows-latest`), PowerShell.

---

## Status — 2026-08-15 (27 / 33 steps done)

Branch `ci-test-separation`, rebased onto `cf423b0` so it now sits on top of the Phase 7a NTT fix.

| Task | State |
|---|---|
| 1 — OverlayComponent null-guard | ✅ `2047f7c` |
| 2 — JUCE modules via global path | ✅ **was already done before this plan ran.** Every `juce_*` module declares `useGlobalPath="1"` in both `.jucer` files; `SFZero` stays local. Verified: Projucer's `defaultJuceModulePath` is set, and both generated `.vcxproj` resolve to `Latest Juce\JUCE\modules`. No commit was needed. |
| 3 — `ci/Tests.jucer` + `test_main.cpp` | ✅ `438afe3` — unit 665/0, arranger 705/0, both exit 0 |
| 4 — strip tests from the app | ✅ `919ffa9` — app builds clean, **zero** `test_*.obj` |
| 5 — `.github/workflows/ci.yml` | ✅ `1a726fe` created; **not yet pushed, so never executed** |
| 6 — README | ✅ `33142a4`; branch protection still outstanding |

**Remaining (all need a push or a human):** Task 4 Step 4 (launch the app and eyeball the GUI),
Task 5 Steps 2–3 (push the branch, watch the first run), Task 6 Steps 1–3 (prove the gate goes red,
enable branch protection on `main`, confirm it blocks a merge).

**Corrections to this plan, learned by executing it:**
- Task 2's Step 5 grep (`0 occurrences of "NEW JUCE"`) **does not pass and is not meant to** — the
  stale `<MODULEPATHS>` entries are inert leftovers that Projucer ignores when a module sets
  `useGlobalPath="1"`. The plan already says the Step 4 build is the real proof. Reading
  `<MODULEPATHS>` instead of `<MODULES>` gives a false "not done" reading.
- The 🖱️ steps in Tasks 2–4 are **not** GUI-only. `Projucer.exe --resave` and
  `--set-global-search-path` cover them, so a `.jucer` can be edited as XML and regenerated headlessly.
- ⚠️ **Projucer returns immediately.** It is a GUI-subsystem exe, so `& Projucer.exe --resave` does
  not block. Locally this let MSBuild start against a half-written project and silently compile the
  old file list — the app "built clean" while still containing all 34 test files. Wait on it
  explicitly (`Start-Process -Wait`), and verify by counting `test_*.obj`, not by exit code.
- Both confirm-on-first-run values are now verified: the asset URL
  `https://github.com/juce-framework/JUCE/releases/download/8.0.13/juce-8.0.13-windows.zip` and the
  Projucer CLI syntax `--set-global-search-path windows defaultJuceModulePath <path>`.

---

## Global Constraints

- **Sequencing:** Do this on a **new branch off `main`, AFTER Phase 7a is merged.** This plan deletes the `--run-tests` runner from `Source/Main.cpp`, which collides with the Phase 7a branch (where that runner is temporarily uncommented). Phase 7a's `Main.cpp` must be back to baseline (runner commented) before starting.
- **JUCE version:** exactly **8.0.13** (the version every referenced module, incl. `juce_audio_processors_headless`, declares). CI pins this.
- **Commits:** never auto-commit — ask the user before each `git commit`; never add Claude attribution to a commit message.
- **CI gate scope:** `unit` + `arranger` only. Never run `integration` / `*_HW` / Supabase in CI.
- **Production binary:** the app target (`Project Synth2.jucer`) must end up compiling **zero** `tests/` files.
- **Local JUCE:** the author's JUCE 8.0.13 + `Projucer.exe` live at `C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE` (sibling of the repo); its `modules/` contains all referenced modules including `juce_audio_processors_headless`.

---

## File structure (what changes)

| Path | Action | Responsibility |
|---|---|---|
| `Source/UI/OverlayComponent.cpp` | modify | null-guard `JUCEApplication::getInstance()` so console tests don't crash |
| `Project Synth2.jucer` | modify | JUCE modules → global path; remove 34 `tests/*.cpp`; (resave) |
| `Source/Main.cpp` | modify | delete the `--run-tests` runner block |
| `ci/Tests.jucer` | create | consoleapp test project (Source−Main.cpp + tests + test_main) |
| `ci/test_main.cpp` | create | console entry: runs UnitTestRunner, exit = failure count |
| `.gitignore` | modify | ignore `ci/Builds/` (and `ci/JuceLibraryCode/` if not already matched) |
| `.github/workflows/ci.yml` | create | fetch JUCE 8.0.13, resave, build, run unit+arranger |
| `README.md` | modify | document: build prereq (JUCE global path), how to run tests, branch-protection step |

---

## Task 1: Null-guard `JUCEApplication::getInstance()` in OverlayComponent

A console test binary has no `JUCEApplication` instance, so `getInstance()` returns `nullptr`; the existing unconditional `->systemRequestedQuit()` would crash if an Overlay test reaches it.

**Files:**
- Modify: `Source/UI/OverlayComponent.cpp:31`

**Interfaces:**
- Produces: nothing consumed by later tasks; a safety precondition for Task 3 running the Overlay tests headlessly.

- [x] **Step 1: Confirm the current call site and scan for siblings**

Run:
```bash
grep -rn "JUCEApplication::getInstance" Source/
```
Expected: exactly two hits — `Source/Main.cpp` (fine; excluded from the test target) and `Source/UI/OverlayComponent.cpp:31`. If more appear in `Source/` (excluding `Main.cpp`), apply the same guard pattern to each.

- [x] **Step 2: Apply the guard**

In `Source/UI/OverlayComponent.cpp`, replace:
```cpp
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
```
with:
```cpp
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
```

- [x] **Step 3: Verify it compiles (existing app build)**

Close the app, then build Debug x64 (same command used throughout the project):
```powershell
Get-Process -Name "Project Synth2" -ErrorAction SilentlyContinue | Stop-Process -Force
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "Builds\VisualStudio2022\Project Synth2.sln" /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
Expected: `EXIT 0`, builds clean. (Full behavioral verification of Overlay tests happens in Task 3, once they run in the console target.)

- [x] **Step 4: Commit** (ask the user first)

```bash
git add Source/UI/OverlayComponent.cpp
git commit -m "fix(ui): null-guard JUCEApplication::getInstance for headless test runs"
```

---

## Task 2: Normalize JUCE module paths to Projucer's global path

Today the `.jucer` references JUCE modules from machine-specific, split locations (`../../JUCE/JUCE/modules` + a stale `../../NEW JUCE/...` for `juce_audio_processors_headless`). Switch every `juce_*` module to Projucer's **global module path** so each machine/CI supplies its own JUCE 8.0.13. `SFZero` stays a local (in-repo) path.

**Files:**
- Modify: `Project Synth2.jucer` (via Projucer GUI + a resave)

**Interfaces:**
- Produces: an app `.jucer` whose JUCE modules use the global path. Task 3's `ci/Tests.jucer` will use the same global path; Task 5's CI sets that global path.

- [x] **Step 1: 🖱️ Set the global JUCE modules path in Projucer (one-time, this machine)**

Open Projucer (`C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE\Projucer.exe`) → **File → Global Paths…** → set **"Path to JUCE modules"** to `C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE\modules`. Save.

- [x] **Step 2: 🖱️ Switch each JUCE module to "global path"**

Open `Project Synth2.jucer` in Projucer → **Modules** panel. For **every `juce_*` module** (not `SFZero`), select it and set its path source to **"Use global path"** (toggle off the per-exporter explicit path). Leave `SFZero` pointing at the in-repo `modules` folder.

- [x] **Step 3: 🖱️ Save/resave the project**

In Projucer: **File → Save Project** (regenerates `Builds/` + `JuceLibraryCode/` from the global path).

- [x] **Step 4: Verify the app still builds from the global path**

```powershell
Get-Process -Name "Project Synth2" -ErrorAction SilentlyContinue | Stop-Process -Force
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "Builds\VisualStudio2022\Project Synth2.sln" /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
Expected: `EXIT 0`. This proves the global-path normalization resolves all modules (including `juce_audio_processors_headless`) from the single JUCE folder.

- [x] **Step 5: Confirm the .jucer no longer hardcodes the split JUCE paths**

Run:
```bash
grep -c "NEW JUCE" "Project Synth2.jucer"; grep -oE 'MODULEPATH id="juce_[a-z_]+"' "Project Synth2.jucer" | head
```
Expected: `0` occurrences of `NEW JUCE`. (JUCE `MODULEPATH` entries may be gone or marked global, depending on Projucer's serialization — the build passing in Step 4 is the real proof.)

- [x] **Step 6: Commit** (ask first)

```bash
git add "Project Synth2.jucer" Builds/ JuceLibraryCode/ 2>/dev/null; git add "Project Synth2.jucer"
git commit -m "build: reference JUCE modules via Projucer global path (reproducible builds)"
```
(Note: `Builds/` and `JuceLibraryCode/` are gitignored, so only the `.jucer` is actually staged. The `git add` of them is a harmless no-op kept for clarity.)

---

## Task 3: Create the headless `ci/Tests.jucer` ConsoleApp + `ci/test_main.cpp`

**Files:**
- Create: `ci/test_main.cpp`
- Create: `ci/Tests.jucer` (via Projucer GUI)
- Modify: `.gitignore`

**Interfaces:**
- Consumes: the global JUCE module path (Task 2); the OverlayComponent guard (Task 1).
- Produces: `ci/Builds/VisualStudio2022/...` containing `Project Tests.exe`, which accepts `--unit-tests` / `--arranger-tests` / `--integration-tests` (default: all) and **returns the failure count as its exit code**. Writes `test-results.txt` to CWD and prints the same to stdout.

- [x] **Step 1: Write `ci/test_main.cpp`**

```cpp
#include <JuceHeader.h>
#include <iostream>

// Headless console entry for the test target. Mirrors the category flags the app's old
// --run-tests path used, but as a standalone main() so the production app carries no test code.
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // MessageManager for component/message-thread tests

    juce::String commandLine;
    for (int i = 1; i < argc; ++i)
        commandLine += juce::String (argv[i]) + " ";

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    if (commandLine.contains ("--arranger-tests"))
        runner.runTestsInCategory ("Arranger");
    else if (commandLine.contains ("--unit-tests"))
        runner.runTestsInCategory ("Unit");
    else if (commandLine.contains ("--integration-tests"))
        runner.runTestsInCategory ("Integration");
    else
        runner.runAllTests();

    int pass = 0, fail = 0;
    juce::StringArray lines;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* r = runner.getResult (i);
        pass += r->passes;
        fail += r->failures;
        lines.add (r->unitTestName + " | passes: " + juce::String (r->passes)
                   + " | failures: " + juce::String (r->failures));
        for (auto& m : r->messages)
            if (m.isNotEmpty())
                lines.add ("    " + m);
    }
    lines.add ("TOTAL | passes: " + juce::String (pass) + " | failures: " + juce::String (fail));

    const juce::String report = lines.joinIntoString ("\n");
    std::cout << report << std::endl;                                   // CI logs
    juce::File::getCurrentWorkingDirectory()
        .getChildFile ("test-results.txt")
        .replaceWithText (report);                                      // artifact

    return fail;   // exit code = number of failing assertions (0 = green)
}
```

- [x] **Step 2: 🖱️ Create `ci/Tests.jucer` in Projucer as a Console Application**

In Projucer: **File → New Project → Console Application**, name `Project Tests`, location the repo's `ci/` folder (so the project file is `ci/Tests.jucer`). Add a **Visual Studio 2022** exporter, Debug config, `targetName = Project Tests`.

- [x] **Step 3: 🖱️ Add modules (global path) — match the app**

In the new project's **Modules** panel, add **the same JUCE modules the app uses** plus `SFZero`. Set every `juce_*` module to **"Use global path"** (same global path as Task 2). Add `SFZero` from the repo's `../modules` (relative to `ci/`). Tip: open `Project Synth2.jucer` side-by-side to copy the exact module list.

- [x] **Step 4: 🖱️ Add source files**

In the **Files** panel, add to the project:
- The whole `../Source` tree **except `../Source/Main.cpp`** (right-click the added `Main.cpp` → remove it from this project; it stays on disk).
- The whole `../tests` tree (all 34 `tests/**/*.cpp`).
- `./test_main.cpp`.

Set header search paths to mirror the app's, adjusted for the `ci/` location (e.g. `../Source;../Source/Arranger;../Source/Midi;../Source/Audio;../Source/Playback;../Source/Styles;../Source/Backend;../Source/UI;../Source/Common`).

- [x] **Step 5: 🖱️ Save the project**

**File → Save Project** → generates `ci/Builds/VisualStudio2022/` + `ci/JuceLibraryCode/`.

- [x] **Step 6: Add the test build output to `.gitignore`**

Append to `.gitignore`:
```
# Headless test build target (generated)
ci/Builds/
ci/JuceLibraryCode/
```
Verify they're ignored:
```bash
git check-ignore ci/Builds/x ci/JuceLibraryCode/x
```
Expected: both paths printed (= ignored).

- [x] **Step 7: Build the test target**

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "ci\Builds\VisualStudio2022\Project Tests.sln" /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
Expected: `EXIT 0`, produces `Project Tests.exe` somewhere under `ci\Builds\VisualStudio2022\x64\Debug\`.

- [x] **Step 8: Run the suite and verify exit codes**

```powershell
$exe = (Get-ChildItem -Recurse -Filter "Project Tests.exe" ci\Builds | Select-Object -First 1).FullName
& $exe --unit-tests;     "unit exit=$LASTEXITCODE"
& $exe --arranger-tests; "arranger exit=$LASTEXITCODE"
```
Expected: both print their `TOTAL | passes: N | failures: 0` and `exit=0`. Counts should match the current suite (~661 unit, ~701 arranger). The Overlay tests run here without crashing (Task 1 guard).

- [x] **Step 9: Commit** (ask first)

```bash
git add ci/Tests.jucer ci/test_main.cpp .gitignore
git commit -m "test: add headless ci/Tests.jucer console target running the unit/arranger suites"
```

---

## Task 4: Strip tests + runner from the production app

Now that the tests build & run in their own target, remove them from the app so the production binary carries no test code.

**Files:**
- Modify: `Project Synth2.jucer` (via Projucer GUI — remove the 34 `tests/*.cpp`)
- Modify: `Source/Main.cpp` (delete the `--run-tests` runner block)

**Interfaces:**
- Consumes: the working test target from Task 3 (the tests now live there).

- [x] **Step 1: Delete the `--run-tests` runner from `Source/Main.cpp`**

In `initialise(const juce::String& commandLine)`, remove the entire `if (commandLine.contains("--run-tests")) { ... quit(); return; }` block (the one currently re-commented at baseline / temporarily uncommented during Phase 7a). The method should end up as just:
```cpp
    void initialise(const juce::String& commandLine) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }
```
(If `commandLine` becomes unused and the compiler warns, that warning is pre-existing and harmless.)

- [x] **Step 2: 🖱️ Remove the 34 `tests/*.cpp` from `Project Synth2.jucer`**

In Projucer, open `Project Synth2.jucer` → **Files** panel → remove the entire `tests` group (all 34 files) from the project. They stay on disk (the test target uses them). **File → Save Project**.

- [x] **Step 3: Verify the app no longer compiles tests**

```powershell
Get-Process -Name "Project Synth2" -ErrorAction SilentlyContinue | Stop-Process -Force
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "Builds\VisualStudio2022\Project Synth2.sln" /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```
Expected: `EXIT 0`. Then confirm no test objects were produced for the app:
```powershell
Get-ChildItem -Recurse Builds\VisualStudio2022\x64\Debug -Filter "test_*.obj" | Measure-Object | % Count
```
Expected: `0`.

- [ ] **Step 4: Verify the app still launches normally**

Launch the built app (no args) and confirm the GUI opens as usual (no test behavior). Close it.

- [x] **Step 5: Confirm the tests are gone from the app .jucer**

```bash
grep -c "tests/" "Project Synth2.jucer"
```
Expected: `0`.

- [x] **Step 6: Commit** (ask first)

```bash
git add "Project Synth2.jucer" Source/Main.cpp
git commit -m "build: remove test code from the production app (tests live in ci/Tests.jucer)"
```

---

## Task 5: GitHub Actions workflow

**Files:**
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Consumes: `ci/Tests.jucer` (Task 3), JUCE 8.0.13 global-path setup (Task 2).

⚠️ **Confirm-on-first-run values** (do these lookups before the first push):
1. **JUCE 8.0.13 release asset URL.** Open `https://github.com/juce-framework/JUCE/releases/tag/8.0.13` and copy the exact **Windows** zip asset name (historically `juce-8.0.13-windows.zip`). Put the real URL in the workflow.
2. **Projucer global-path CLI.** Verify the syntax with the downloaded Projucer: `Projucer.exe --help`. The documented form is `--set-global-search-path <os> <key> <path>` with `os=windows` and `key=defaultJuceModulePath`. Adjust if the help text differs for 8.0.13.

- [x] **Step 1: Create `.github/workflows/ci.yml`**

```yaml
name: CI

on:
  push:
    branches-ignore: [ main ]
  pull_request:
    branches: [ main ]

concurrency:
  group: ci-${{ github.ref }}
  cancel-in-progress: true

jobs:
  tests:
    runs-on: windows-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Download JUCE 8.0.13 (modules + Projucer)
        shell: pwsh
        run: |
          # CONFIRM the asset name on the 8.0.13 release page before first run.
          $url = "https://github.com/juce-framework/JUCE/releases/download/8.0.13/juce-8.0.13-windows.zip"
          Invoke-WebRequest -Uri $url -OutFile "$env:RUNNER_TEMP\juce.zip"
          Expand-Archive "$env:RUNNER_TEMP\juce.zip" -DestinationPath "$env:RUNNER_TEMP\juce"
          # The zip extracts a JUCE/ folder containing modules/ and Projucer.exe
          "JUCE_DIR=$env:RUNNER_TEMP\juce\JUCE" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8

      - name: Point Projucer at the JUCE modules (global path)
        shell: pwsh
        run: |
          & "$env:JUCE_DIR\Projucer.exe" --set-global-search-path windows defaultJuceModulePath "$env:JUCE_DIR\modules"

      - name: Resave the test project
        shell: pwsh
        run: |
          & "$env:JUCE_DIR\Projucer.exe" --resave "ci\Tests.jucer"

      - name: Add MSBuild to PATH
        uses: microsoft/setup-msbuild@v2

      - name: Build the test target
        shell: pwsh
        run: msbuild "ci\Builds\VisualStudio2022\Project Tests.sln" /p:Configuration=Debug /p:Platform=x64 /m

      - name: Run unit tests
        shell: pwsh
        run: |
          $exe = (Get-ChildItem -Recurse -Filter "Project Tests.exe" ci\Builds | Select-Object -First 1).FullName
          & $exe --unit-tests
          if ($LASTEXITCODE -ne 0) { Write-Error "unit tests failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

      - name: Run arranger tests
        shell: pwsh
        run: |
          $exe = (Get-ChildItem -Recurse -Filter "Project Tests.exe" ci\Builds | Select-Object -First 1).FullName
          & $exe --arranger-tests
          if ($LASTEXITCODE -ne 0) { Write-Error "arranger tests failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: test-results
          path: test-results.txt
```

- [ ] **Step 2: Commit + push on a throwaway branch to exercise the workflow** (ask first)

```bash
git add .github/workflows/ci.yml
git commit -m "ci: add GitHub Actions test gate (unit + arranger) on branch pushes and PRs"
git push -u origin <this-branch>
```

- [ ] **Step 3: Verify the run (GitHub UI / gh)**

```bash
gh run list --branch <this-branch> --limit 1
gh run watch
```
Expected: the workflow runs on the push, builds the test target, both test steps exit 0, the job is **green**, and `test-results.txt` is attached as an artifact. If the JUCE URL or Projucer flag was wrong, fix per the confirm-on-first-run notes and re-push.

---

## Task 6: Prove the gate, enable branch protection, document

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Prove the gate fails red (temporary failing test)**

On a branch, add a deliberately failing assertion to any existing test (e.g. in `tests/unit/test_chord.cpp`, inside an existing `beginTest` block: `expect (false, "intentional CI red check");`). Commit, push, open a PR into `main`.
Expected: the workflow runs and the check is **red** (job exits non-zero). Then **revert** the failing assertion and confirm the check goes **green**.

- [ ] **Step 2: 🖱️ Enable branch protection on `main`**

GitHub → repo **Settings → Branches → Add branch protection rule** → Branch name pattern `main` → check **"Require status checks to pass before merging"** → select the **CI / tests** check (it appears in the list after the first run from Step 1) → Save. This is what actually blocks a red PR from merging.

- [ ] **Step 3: Verify the gate blocks a merge**

With the temporary failing test still on a PR (before reverting), confirm the **Merge** button is disabled / blocked by the failing required check. Then revert the failing test so the PR is mergeable.

- [x] **Step 4: Document it in `README.md`**

Add a short "Building & Tests" section:
```markdown
## Building & Tests

**Prerequisite:** JUCE 8.0.13. In Projucer, set **Global Paths → Path to JUCE modules** to your
local `JUCE/modules`. Both `Project Synth2.jucer` (the app) and `ci/Tests.jucer` (the headless
test runner) resolve JUCE through that global path.

- **App:** open `Project Synth2.jucer`, save, build `Builds/VisualStudio2022` (Debug x64).
- **Tests:** open `ci/Tests.jucer`, save, build `ci/Builds/VisualStudio2022`, then run
  `Project Tests.exe --unit-tests` and `--arranger-tests` (exit code = failure count).
  Integration / `*_HW` / Supabase tests are local-only (they need real devices/network).

**CI:** `.github/workflows/ci.yml` runs `unit` + `arranger` on every feature-branch push and on
PRs into `main`. `main` is protected: a PR can't merge until the check is green. `main` itself
runs no workflow.

**Adding a Source file:** add it to **both** `Project Synth2.jucer` and `ci/Tests.jucer` in
Projucer (Projucer keeps explicit file lists). A miss shows up as a CI link error.
```

- [x] **Step 5: Commit** (ask first)

```bash
git add README.md
git commit -m "docs: document the test target, JUCE global path, and CI gate"
```

---

## Self-Review (plan vs spec)

**Spec coverage:**
- §1 clean production binary → Task 4. ✅
- §1 CI gate on branches/PRs not main → Task 5. ✅
- §3 JUCE not vendored / 8.0.13 / gitignored Builds → Tasks 2, 3 (gitignore), 5 (fetch). ✅
- §4.1 app loses 34 tests + runner → Task 4. ✅
- §4.2 ci/Tests.jucer consoleapp in own subfolder → Task 3. ✅
- §4.3 test_main.cpp (ScopedJuceInitialiser_GUI, categories, stdout+file, exit=failures) → Task 3 Step 1. ✅
- §4.4.1 module-path normalization (global path) → Task 2. ✅
- §4.4.2 CI fetches JUCE 8.0.13 release (modules + Projucer) → Task 5. ✅
- §4.5 workflow (triggers, concurrency, steps, artifact, gate scope) → Task 5. ✅
- §4.6 OverlayComponent guard → Task 1. ✅
- §4.7 branch protection (manual) → Task 6. ✅
- §5 maintenance contract (dual file lists) → documented in Task 6 README. ✅
- §6 sequencing (after Phase 7a) → Global Constraints. ✅
- §7/§8 success + verification → Task 3 Step 8, Task 4 Step 3, Task 5 Step 3, Task 6 Steps 1/3. ✅

**Placeholder scan:** the two genuinely external facts (JUCE release asset URL; exact Projucer `--set-global-search-path` syntax) are flagged as **confirm-on-first-run** with the exact lookup method — not silent placeholders. Everything else is concrete.

**Type/name consistency:** `Project Tests.exe`, `ci/Tests.jucer`, `ci/Builds/VisualStudio2022`, `--unit-tests` / `--arranger-tests`, and `test-results.txt` are used identically across Tasks 3, 5, 6. The exe is always located by recursive `Get-ChildItem` rather than a hardcoded subfolder, so the unknown ConsoleApp output folder name never needs pinning.

**Known manual steps:** all Projucer GUI actions are marked 🖱️ (Tasks 2, 3, 4) — a human performs those; the rest (code, CLI, YAML, git) are automatable.
