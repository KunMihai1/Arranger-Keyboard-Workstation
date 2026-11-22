---
type: meta
title: "Dependencies Index"
created: 2026-06-06
updated: 2026-11-22
tags:
  - meta
  - index
  - dependency
status: developing
---

# Dependencies Index

External libraries, frameworks, and services with versions and risk notes.

| Dependency | Version | Used by | Risk |
|------------|---------|---------|------|
| JUCE | 8.0.13 | whole app | core framework — module list below |
| Supabase | _tbd_ | backend/auth | external service |

## JUCE modules (both `.jucer` files, since 2026-11-21)
Enabled: `juce_core`, `juce_events`, `juce_data_structures`, `juce_graphics`, `juce_gui_basics`,
`juce_gui_extra`, `juce_opengl`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`,
`juce_audio_processors` (+ `_headless`, needed by SFZero's include), `juce_dsp`, and the local
`SFZero` module. Ten unused modules were removed to cut build time; using a class from any other
module means adding it to **both** projects.

A precompiled header (`Source/PrecompiledHeader.h` → `<JuceHeader.h>`) is force-included into every
non-module file. Projucer copies it into the build folder on save, so edits need a re-save.
Measured effect: one-file rebuild 8.7 → 2.1 s, clean CI test build ~194 → ~53 s.

## MSBuild overrides (since 2026-11-22)
Repository-root `Directory.Build.props` / `.targets` hold two settings Projucer can't express; they
apply to both projects (`ci/Directory.Build.targets` imports the root one) and survive a re-save:
- the 64-bit-hosted compiler and linker (`PreferredToolArchitecture=x64`);
- one `/bigobj` option for every JUCE module file, so they compile as one `/MP` batch instead of two.

Measured: clean build 44 → 30 s (app) and 44 → 28 s (tests), header edit 7.1 → 5.5 s, CI-like test
build with cached JUCE objects 25.6 → 16.5 s; one-file rebuild unchanged (~2.6 s). MultiToolTask
was rejected (slower header rebuilds). Spec: `docs/superpowers/specs/2026-11-22-build-time-design.md`.
