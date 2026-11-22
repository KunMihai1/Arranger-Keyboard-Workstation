# Build time — design and measurements

**Status:** implemented on `perf/build-time` (2026-11-22).

## Problem
The owner asked for faster builds. A precompiled header, a trimmed JUCE module list and a CI cache of
compiled JUCE objects were already in place, so this note looks only for time that is still lost.

## Conditions
Ryzen 7 7435HS (8 cores / 16 threads), 24 GB, VS 2022 17.14 (MSVC 14.44), JUCE 8.0.13, Debug x64,
`msbuild <sln> /m /nodeReuse:false`. Medians of 3 runs. A harness polled the process table during
every run and threw away any sample where a `cl`/`link`/`MSBuild`/test process outside its own build tree
was busy (another agent was building in the main checkout).
Scenarios: clean build (output folder deleted); touch `Source/Arranger/ArrangerEngine.cpp`; touch
`Source/Arranger/ArrangerModel.h` (35 of 69 app files depend on it); **CI-warm** (test project, JUCE
objects present, everything else deleted, `/p:CL_MPCount=4` like the 4-vCPU runner); **CI-cold**
(test project clean, 4 slots).

## Where the time goes (clean app build, 42 s)
- **Compile, 34 s.** `cl /Bt+` per file: 185 s of CPU, 137 s of it in JUCE module files (longest:
  `include_juce_graphics_Harfbuzz.cpp` 13 s, `include_juce_graphics.cpp` 11 s), 48 s in our 69 files,
  4.5 s for the PCH. 185 s over 16 threads would be ~12 s; it takes 34 s because MSBuild calls the CL
  task once per distinct option set and runs those `/MP` batches **one after another**: PCH (1 file),
  19 module files, our 69 files, 5 module files that Projucer gives `/bigobj`, 1 C file. Each batch waits
  for its slowest file, so the cores idle at each tail.
- **Link, 7.3 s** (11 s in the evening session). MSBuild picks the **32-bit-hosted** `HostX86\x64`
  compiler and linker unless `PreferredToolArchitecture` says otherwise.

## Options measured
| Option | Result | Decision |
|---|---|---|
| 64-bit-hosted tools (`PreferredToolArchitecture=x64`) | link 7.3 → 5.0 s (app), 7.0 → 1.9 s (tests); header touch 6.3 → 5.4 s; nothing slower | **keep** |
| One `/bigobj` option set for all module files (24 module files in one batch) | clean CL 34 → 30 s on top of x64; target itself 0 ms; own-file batches untouched (they use the PCH, so the target skips them) | **keep** |
| MultiToolTask (`UseMultiToolTask`) | clean 42 → 30 s, but header touch +0.7…1.8 s (one `cl.exe` and one PCH load per file) and CI-warm 18.3 → 22.6 s vs x64 alone; also without `EnforceProcessCountAcrossBuilds` (app header 6.3 s vs 5.4 s) | reject |
| Remove more JUCE modules | every remaining module is used or required by SFZero (`juce_audio_processors`) | nothing to remove |
| Unity/jumbo build of our files | our code is 48 s CPU, ~3 s wall at 16 threads; would slow one-file rebuilds | reject |
| sccache/buildcache in CI | MSVC PCH (`/Yc`/`/Yu`) not cacheable; JUCE objects are already cached | reject (not measured) |
| `/Z7` for the app, `/DEBUG:FASTLINK` | link is already 1–5 s with x64 tools; FASTLINK changes the debugging setup | not measured |

## Decision
Two MSBuild settings Projucer cannot express, in repository-root files that apply to both projects
and survive a re-save:
- `Directory.Build.props`: `PreferredToolArchitecture=x64`.
- `Directory.Build.targets`: a target before `ClCompile` gives every non-PCH `.cpp` the same
  `/bigobj` option, so the JUCE module files form one `/MP` batch (`/bigobj` only raises the COFF section
  limit; it is already on the five largest module files). `ci/Directory.Build.targets` imports it.
- CI: the compiled-JUCE-objects cache key also hashes both files.

## Before / after (same evening session, quiet machine)
Wall seconds, median of 3 (link step in brackets). The 32-bit linker was slower in this session than in
the morning (clean link 11 s vs 7 s); only same-session pairs are compared.

| Scenario | App before | App after | Tests before | Tests after |
|---|---|---|---|---|
| Clean build | 44.1 (11.2) | **29.8** (4.9) | 43.6 (10.9) | **27.7** (1.8) |
| Touch one `.cpp` | 2.57 (0.83) | 2.64 (0.71) | 2.58 (0.79) | 2.59 (0.70) |
| Touch `ArrangerModel.h` | 7.06 (3.37) | **5.45** (1.05) | 8.09 (3.62) | **5.64** (1.01) |
| CI-warm, 4 slots | | | 25.6 (10.9) | **16.5** (1.7) |
| CI-cold, 4 slots | | | 58.2 (10.8) | **45.5** (1.8) |

The one-file rebuild is unchanged: MSBuild's own `Build` target got 0.1 s faster, and the app's +0.07 s
wall-clock difference is outside the build targets, within noise.

## Acceptance
- App and test runner build; `--unit-tests`, `--arranger-tests`, `--integration-tests` totals unchanged.
- Projucer re-save keeps the settings (they are not in generated files).
- No scenario measurably slower.

## Independent re-check (orchestrator, on current `main` 46cb68d)
- App `/t:Rebuild`, alternating with and without the two root files: 43.8 / 43.3 s before, 29.1 / 28.9 s
  after. Before used only `HostX86` `cl.exe`, after only `HostX64`; 0 errors and 236 warnings in both.
- Test runner (after a Projucer 8.0.13 re-save, which changed no tracked file): unit 870 / 0, arranger
  770 / 0, integration 130 / 0 with 7 skipped.
