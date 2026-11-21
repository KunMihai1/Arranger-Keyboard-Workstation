---
type: meta
title: "Hot Cache"
updated: 2026-09-04T00:00:00
tags:
  - meta
  - hot
status: evergreen
---

# Recent Context

## Last Updated
2026-08-30. **Both outstanding branches are merged into `main`.** PR #10 landed
`fix/sfz-integration` (the rename, the SFZ popup work, the PC-keyboard layout fix,
Phase 7a NTT, the wiki and **CI**); PR #11 landed `fix/dsp-quality` (the delay
bypass freeze, per-sample CC smoothing, ADAA on the distortion). `main` is now the
only line that matters and it carries the workflow, so CI runs on it. Full detail
in [[log]].

## Branch topology (rewritten 2026-08-30 — everything is on main now)
```
ab5d356 (main, origin/main)  ← PR #11, merge of fix/dsp-quality
   └── bec111a   fix(audio): delay freeze, CC smoothing, ADAA
f7b4df3                      ← PR #10, merge of fix/sfz-integration
   └── 494693f → 2690b5f → 50e2d02 → …
6f2ba81                      — Phase 1–6b, merged via PR #8. No CI on this commit.
```
**`fix/dsp-quality` and `fix/sfz-integration` are merged and can be pruned** — they
still exist locally and on the remote, along with a stale local `sfz-integration`.
Nothing depends on them; the repo's practice after a merge is to delete them, and
that has not been done for these two yet.

## Key Recent Facts
- Piano-App is a JUCE C++ **sample-based** arranger keyboard (SFZ via sfzero — **no oscillators anywhere**). [[Keyboard & Main UI]] (`MainComponent`) wires everything. Two parallel engines: classic [[Track Playback]] and the [[Arranger Engine]]; sound via [[Audio & SFZ Playback]].
- **App name lives in `Source/Common/AppInfo.h`** (`AppInfo::appName`). Data is in `%APPDATA%\Arranger Workstation\`; the old `Piano Synth2` folder was **deliberately not migrated**, so the SFZ library starts empty.
- **Tests live in `ci/Tests.jucer`**, not the app. The app compiles zero test code — verify by counting `test_*.obj`, never by exit code.

## Build / Test
- App: `Builds\VisualStudio2022\Arranger Workstation.sln`, MSBuild Debug x64 — **close the app first** (`Arranger Workstation.pdb` lock). JUCE **8.0.13** at `Desktop\Latest Juce\JUCE`.
- Tests: build `ci/Builds/VisualStudio2022/Project Tests.sln`, run `Project Tests.exe --unit-tests` / `--arranger-tests`. **Exit code = failing assertions.** Results → `test-results.txt` in the CWD.
- **Projucer detaches** — always `Start-Process -Wait`, then verify by counting objects.
- A header change that alters a type's layout needs **`/t:Rebuild`**, not an incremental build.
- **CI skips docs-only pushes.** `paths-ignore` on the *push* trigger covers `Piano-Wiki/**` and
  `**/*.md`, so wiki syncs no longer burn a full Windows build. It is deliberately **not** on the
  `pull_request` trigger — a docs-only PR must still emit the required `CI / tests` check, or
  branch protection would block it waiting for a run that never comes.

## Active Threads / Next
- ✅ **FIXED and merged to `main` — the delay bypass freeze** (PR #11, `ab5d356`). The write is now unconditional and only the read/mix is gated. Same change also smooths every CC-driven parameter per sample and antialiases the distortion with first-order ADAA (alias-to-signal −13.07 → −19.25 dB, measured in `test_channel_dsp.cpp`). Unit **707/0**, arranger **705/0**, app builds clean. Detail on [[Audio & SFZ Playback]].
- 🟡 **The delay *read offset* still jumps.** `updateCC` case 94 moves `delayReadOffset` instantly, which is a pitch discontinuity and clicks. Needs fractional (Lagrange) interpolation on the read position — a separate change from the mix smoothing already done. **The replacement now exists**: `DelayLine` in the sibling `va-effects` repo, where the measurements are also written up. Integer-position reading ignores a modulation smaller than half a sample *entirely* — the output is bit-identical to no modulation — so this bug is worse than "it jumps a bit".
- ⚠️ **Other tests may be silently stale.** `test_midi_handler.cpp` asserted behaviour `5c75d4f` had deliberately changed and never failed, because the runner in `Main.cpp` was commented out — the app compiled tests without running them. Anything touching code changed between `d05e07b` and now deserves suspicion. CI catches this from now on.
- 🟡 **`b5765e9` missing-SFZ popup still not smoke-tested** — and now harder, since the rename left the SFZ library empty. Re-add SFZs first.
- 🟡 **Phase 7a manual sign-off 0/9** — `docs/superpowers/test-plans/2026-06-26-arranger-phase7a-ntt-testing.md`. Automated suites green.
- 📝 **`ARCHITECTURE.md` added at the repo root** — subsystem breakdown, MainComponent's
  composition-root role, the two-parallel-engines mode split, the arranger's pure-core
  design, plus system/component Mermaid diagrams. Static snapshot; this wiki stays the
  living version.
- 📝 **ADR-002 recorded** — `MainComponent` (3,083 lines) and `Display` (1,174 lines)
  flagged as large integration files; refactor deliberately deferred until a concrete
  trigger (isolated testing, second window, concurrent work on disjoint areas). If it
  happens: prefer small DI-style coordinators, not an event bus — most of the coupling
  is 1:1 calls, not real fan-out. See [[ADR-002 Defer MainComponent Display Refactor]].
- 🟡 **Prune the two merged branches.** `fix/dsp-quality` and `fix/sfz-integration` remain local and remote, plus a stale local `sfz-integration`. All three are fully contained in `main`.
- 🟡 **Next DSP change: swap `juce::dsp::Chorus` (CC93) for the BBD model** from `va-effects`. Keeps the CC mapping; gives a like-for-like A/B against a stock implementation in the same chain. The constraint is budget — sixteen `ChannelDSP` instances have to fit in what they cost before.
- 🟢 **OpenGL crash parked** (unreproducible); tripwire logs to `Desktop/piano_gl_errors.log`, which has never appeared.
- 📝 **`wiki/log.md` has a permanent gap** — entries between 2026-06-25 and 2026-08-15 were lost to a truncating write on 2026-08-29 and were unrecoverable (the vault was gitignored at the time; it is tracked now). Marked inline in the file.
- 📝 Possible polish (not done): make the chord-zone mute a no-op when `rightHandBound == -1`.
