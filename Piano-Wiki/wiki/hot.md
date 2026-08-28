---
type: meta
title: "Hot Cache"
updated: 2026-08-29T00:00:00
tags:
  - meta
  - hot
status: evergreen
---

# Recent Context

## Last Updated
2026-08-29. **`ci-test-separation` merged into `fix/sfz-integration` (`50e2d02`) and pushed.** That branch now carries *everything*: the "Arranger Workstation" rename, the SFZ popup work, the PC-keyboard layout fix, **Phase 7a NTT**, the wiki, and **CI**. Pushing it runs the workflow for the first time on this line. Verified locally before commit: app builds Debug x64 **0 errors / zero `test_*.obj`**, unit **665/0**, arranger **705/0**. Stale branches pruned — 12 refs, 6 local + 6 remote. Full detail in [[log]].

## Branch topology (rewritten 2026-08-29 — the old two-parallel-branch diagram is obsolete)
```
6f2ba81 (main, origin/main)  — Phase 1–6b, merged via PR #8. NO workflow file, NO CI.
   └── … → b5765e9 → a73d4c8 → 128cf87 → 71063de → 94aea18 → 539dc37
             └── 50e2d02  (fix/sfz-integration, pushed)   ← merge of e394005
```
Only **`main`** and **`fix/sfz-integration`** exist now, local and remote. The 7a line, the CI branch and the phase branches were deleted after merging (SHAs in [[log]] if one is ever needed). **The old "merge-conflict risk between two parallel branches" warning is resolved** — that merge is done; `suppressInternalFeed` was dropped in favour of the `channelForHand` parity rewrite, which already covers it.

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
- 🔴 **UNFIXED — the delay bypass freezes the delay line.** `AudioHandler.cpp:181-193` keeps the buffer *write* inside `if (delayMix > 0.005f)`, so re-enabling delay replays stale audio at full mix. Fix options on [[Audio & SFZ Playback]].
- ⚠️ **Other tests may be silently stale.** `test_midi_handler.cpp` asserted behaviour `5c75d4f` had deliberately changed and never failed, because the runner in `Main.cpp` was commented out — the app compiled tests without running them. Anything touching code changed between `d05e07b` and now deserves suspicion. CI catches this from now on.
- 🟡 **`b5765e9` missing-SFZ popup still not smoke-tested** — and now harder, since the rename left the SFZ library empty. Re-add SFZs first.
- 🟡 **Phase 7a manual sign-off 0/9** — `docs/superpowers/test-plans/2026-06-26-arranger-phase7a-ntt-testing.md`. Automated suites green.
- 🟡 **Next step: PR `fix/sfz-integration` → `main`.** User reports `main`'s required-approvals set to 0 (unverified from here — GitHub admin auth needed). `main` has no workflow of its own; it inherits CI once this lands.
- 🟢 **OpenGL crash parked** (unreproducible); tripwire logs to `Desktop/piano_gl_errors.log`, which has never appeared.
- 📝 **`wiki/log.md` has a permanent gap** — entries between 2026-06-25 and 2026-08-15 were lost to a truncating write on 2026-08-29 and were unrecoverable (the vault was gitignored at the time; it is tracked now). Marked inline in the file.
- 📝 Possible polish (not done): make the chord-zone mute a no-op when `rightHandBound == -1`.
