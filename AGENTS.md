# AGENTS.md

Rules for any AI agent (Claude Code, Codex, Cursor, …) working in this repository.
Tool-specific additions live in `CLAUDE.md`; this file is the source of truth for *how work is done*.

## The project in one paragraph

**Arranger Workstation** — a JUCE 8.0.13 / C++17 desktop arranger keyboard (Windows, VS2022).
Sound is **sample-based (SFZ via SFZero)** — there are no oscillators. `MainComponent` is the
composition root; the big subsystems are the Arranger Engine, MIDI handling, Audio/SFZ + per-channel
DSP, Track Playback, Styles, Supabase backend and the UI (keyboard + OpenGL note layer).
Most of the code was written by hand and the base is solid: **treat existing behaviour as intentional
until proven otherwise.**

## Where knowledge lives — read before crawling `Source/`

1. `docs/ARCHITECTURE.md` — one-page system overview with diagrams.
2. `Piano-Wiki/wiki/hot.md` — recent context, open threads (≤500 words).
3. `Piano-Wiki/wiki/index.md` → the relevant `_index.md` → individual pages.
4. `Piano-Wiki/wiki/decisions/` — ADRs. Don't relitigate a recorded decision without new evidence.
5. `docs/superpowers/specs/`, `plans/`, `test-plans/` — design specs, implementation plans and
   manual test plans per feature.

The wiki is the architecture documentation. Keep it in sync (see *Finishing work*).

## Core principles

1. **Real problems only.** Before fixing or "improving" anything, show it is a real problem:
   a failing test, a measurement, a reproducible bug, a crash log. No speculative refactors,
   no renames or reformatting "while I'm here", no churn for taste.
2. **Root cause first.** Reproduce → measure → find the cause → then fix. Never patch a symptom.
   If you can't explain *why* the bug happens, you're not ready to fix it.
3. **Measure, then decide.** Performance work starts with a baseline number and ends with a
   comparison under the same conditions. Report numbers, not impressions.
4. **Simplest solution that fully solves the case.** Prefer the smallest correct change at the
   quality bar of the app (polished, robust, app-themed UI). Simple ≠ hacky.
5. **Minimal diff.** Touch only what the change requires. Unrelated cleanups go in their own
   change, and only if they meet rule 1.

## Workflow: spec → test → code

1. **Spec** — non-trivial work (new feature, behaviour change, multi-file fix) gets a short spec in
   `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`: problem, evidence, options considered,
   decision, acceptance criteria. Get the user's sign-off before implementing.
2. **Plan** — for multi-step work, a plan in `docs/superpowers/plans/`.
3. **Test first (TDD)** — write the failing test, watch it fail for the right reason, write the
   minimum code to pass, refactor while green. Bug fixes start with a test that reproduces the bug.
   Code that genuinely can't be unit-tested (GL rendering, device I/O) gets a measurement harness
   or a written manual test plan in `docs/superpowers/test-plans/` instead — say which, and why.
4. **Verify** — build the app *and* run both suites; read the numbers, don't trust exit codes alone.

## Build & test (Windows, VS2022)

- **App:** `Builds\VisualStudio2022\Arranger Workstation.sln`, Debug x64. Close the running app
  first (it locks `Arranger Workstation.pdb` → `C1041`), or redirect `/p:IntDir=… /p:OutDir=…`.
- **Tests:** `ci/Tests.jucer` → `ci\Builds\VisualStudio2022\Project Tests.sln`, then
  `Project Tests.exe --unit-tests` and `--arranger-tests`. **Exit code = number of failing
  assertions.** Results go to `test-results.txt` in the *working directory*; check the
  `TOTAL | passes: N` line. The app itself compiles **no** test code.
- **Projucer returns immediately** — always `Start-Process -Wait -NoNewWindow`.
- **New `Source/` file** → add it to **both** `Arranger Workstation.jucer` and `ci/Tests.jucer`.
- **Precompiled header:** `Source/PrecompiledHeader.h` (`<JuceHeader.h>`) is force-included into
  every non-module file of both projects. It hides a missing `#include`, so still include what you
  use. Put only stable headers in it. Projucer *copies* it into the build folder on save, so an
  edit to it only takes effect after re-saving both `.jucer` files.
- **JUCE modules:** only the ones the code uses are enabled. Using a class from another module
  (e.g. `juce_osc`) means adding that module to **both** `.jucer` files.
- **A header change that alters a type's layout → `/t:Rebuild`.** Incremental builds miss
  `Source/Main.cpp`'s dependencies and the result is silent heap corruption, not a build error.
- CI (`.github/workflows/ci.yml`) enforces assertion floors. Removing tests → lower the floor in
  the same change; adding many → raise it.
- **CI caches the compiled JUCE module objects** (the runs on `main` fill it; PRs read it). The test
  project builds with `/Z7` (`ci/Directory.Build.targets`) so each `.obj` is self-contained. The key
  hashes the generated `ci/JuceLibraryCode/` files, so changing modules or JUCE options just
  rebuilds cold — no manual invalidation needed.

## Codebase rules

- JUCE threading: audio thread never allocates/locks; GL calls only from `renderOpenGL` /
  `newOpenGLContextCreated` / `openGLContextClosing` (see wiki *GL Overlay Rendering*).
- Anything over the live scene must hide the GL `NoteLayer` or be drawn inside it.
- Schedulers/sequencers stay pure (no I/O) — ADR-001. No event bus; prefer small DI-style
  extractions only when a change forces it.
- Colours come from `Source/Common/AppColours.h`. App name from `Source/Common/AppInfo.h`.
- **No source or test file over 800 lines** (CI enforces it). When a class outgrows that, split its
  definitions by responsibility into extra translation units of the same class
  (`MainComponentInit.cpp`, `MainComponentOverlays.cpp`, …) — see ADR-003 in the wiki.

## Git

- Branch from `main` (`fix/…`, `feat/…`, `perf/…`, `docs/…`); merge through a PR with CI green.
- **Never commit, push, or open a PR without the user's explicit approval.**
- **No AI attribution anywhere** — no `Co-Authored-By`, no "Generated with …" line, no tool name in
  commit messages, PR titles or PR bodies. Commits are authored by the user alone.
- Conventional-commit style subjects (`fix(audio): …`, `perf(ui): …`, `docs(wiki): …`); the body
  explains *why* and quotes the evidence (test counts, measurements).

## Finishing work

1. Build + both suites green; numbers quoted.
2. **Independent review:** dispatch a reviewer agent (Claude: a Sonnet subagent) over the diff.
   The orchestrator then **verifies every claim** against the code before acting on it — a review
   finding is a hypothesis, not a fact. Fix the confirmed ones; note the rejected ones and why.
3. Sync the wiki (procedure in `Piano-Wiki/CLAUDE.md`): affected pages, `log.md`, `hot.md`.
4. Report to the user: what changed, the evidence, what was *not* done.
