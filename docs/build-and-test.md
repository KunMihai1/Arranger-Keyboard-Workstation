# Build and test details

`AGENTS.md` carries the essentials; read this before changing the build, the test runner or CI.

## Test runner
- `ci/Tests.jucer` builds `Project Tests.exe` (`ci\Builds\VisualStudio2022\Project Tests.sln`); the app
  compiles **no** test code. Results go to `test-results.txt` in the *working directory*.
- **Exit code = number of failing assertions.** Read the `TOTAL | passes: N | failures: M | skipped: K`
  line too: a run that executes zero tests also exits 0.
- **Categories** (second argument of `juce::UnitTest`); a test in any other category never runs:

  | Category | Flag | What goes there | CI |
  |---|---|---|---|
  | `Unit` | `--unit-tests` | logic and single components; files only in the temp folder | yes |
  | `Arranger` | `--arranger-tests` | the arranger engine and model | yes |
  | `Integration` | `--integration-tests` | several components together; MIDI-hardware tests skip without a device | yes |
  | `Integration-display` | `--display-tests` | real windows and pixel reads: needs an unlocked desktop with a GPU | no |
  | `Integration-api` | `--api-tests` | real backend calls that **create accounts**: never by default | no |

  No flag runs every category except `Integration-api`.
- **Skipping:** a test that can't run here logs `SKIPPED - <reason>` (instead of passing vacuously). The
  runner counts them per case and lists them; CI shows them in the log and the run summary.
- Tests never write to `%APPDATA%` (`IOHelper::getFile()` has no test override): keep disk I/O out of
  the code under test, or pass an explicit temp file.
- The shared runner never dispatches its message queue: deliver an async notification by calling the
  listener directly, once per notification the code posts.
- CI enforces assertion floors per suite (`.github/workflows/ci.yml`): lower one in the same change
  that removes tests, raise it when adding many.

## Build
- **Precompiled header:** `Source/PrecompiledHeader.h` (`<JuceHeader.h>`) is force-included into every
  non-module file of both projects. It hides a missing `#include`, so still include what you use. Put
  only stable headers in it. Projucer *copies* it on save, so an edit only takes effect after re-saving
  both `.jucer` files.
- **Projucer version:** re-save with the one matching the modules (8.0.13,
  `Desktop\Latest Juce\JUCE\Projucer.exe`); an older Projucer makes `JuceHeader.h` stop the build with
  "last saved using an outdated version of the Projucer".
- **JUCE modules:** only the ones the code uses are enabled. Using a class from another module (e.g.
  `juce_osc`) means adding that module to **both** `.jucer` files.
- **MSBuild settings Projucer can't express** live in the repository-root `Directory.Build.props` /
  `.targets` (both projects; `ci/Directory.Build.targets` imports the root one) and survive a re-save:
  the 64-bit-hosted compiler and linker, and one `/bigobj` option set for all JUCE module files so they
  compile as a single `/MP` batch. Measurements: `docs/superpowers/specs/2026-11-22-build-time-design.md`.
- **A header change that alters a type's layout -> `/t:Rebuild`.** Incremental builds miss
  `Source/Main.cpp`'s dependencies and the result is silent heap corruption, not a build error.
- **CI caches the compiled JUCE module objects** (runs on `main` fill it; PRs read it). The test project
  builds with `/Z7` (`ci/Directory.Build.targets`) so each `.obj` is self-contained. The key hashes the
  generated `ci/JuceLibraryCode/` files and the `Directory.Build.*` files, so changing modules, JUCE
  options or those overrides just rebuilds cold.
