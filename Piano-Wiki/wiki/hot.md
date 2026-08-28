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
2026-08-29. **App renamed to "Arranger Workstation" (`128cf87`, on `fix/sfz-integration`).**
`AppInfo::appName` (new, header-only, dependency-free) is now the single source of truth; every
hardcoded `"Piano Synth2"` is gone from `Source/`. Also renamed the Projucer project + both target
names and the `.jucer` file itself, so the exe/PDB/solution are `Arranger Workstation.*` now.
Build output was wiped clean, so the next build recompiles from scratch. **The `%APPDATA%` folder
moved with the name and old data was deliberately NOT migrated** — the app starts with an empty SFZ
library, so `b5765e9`'s missing-SFZ popup smoke test needs SFZs re-added first. The PC-keyboard
layout fix noted below is now committed (`a73d4c8`). Still open: Phase 7a manual sign-off (0/9),
a GUI eyeball after the test-strip, and a rewritten `README.md` sitting uncommitted.

## Branch topology (CORRECTED 2026-08-15 — the old entry was wrong)
`fix/sfz-integration` branches from **`main`**, *not* from Phase 7a. Verified: `git merge-base --is-ancestor 614b4ec b5765e9` → **NO**; `Source/Arranger/NttScales.h` exists only on the 7a line.

```
6f2ba81 (main, origin/main)  — Phase 1–6b + chord-zone mute, merged via PR #8
   ├── 614b4ec  Phase 7a NTT engine + MIDI hardening   (origin/ci-test-separation)
   │     └── cf423b0  NTT bypass/bass-inversion fix    (arranger-phase7a-ntt-engine)
   │           └── 2047f7c → 438afe3 → 919ffa9 → 1a726fe → 33142a4
   │                        (ci-test-separation ← HEAD; CI Tasks 1–6, NOT pushed)
   └── 5c75d4f → cfe100b → 69a89b1 → ab0fe6c → b5765e9 → a73d4c8 → 128cf87 (fix/sfz-integration)
        (b5765e9 pushed; a73d4c8 pc-keyboard fix + 128cf87 rename are local)
```

The two 7a-line branches briefly diverged when `cf423b0` landed; `ci-test-separation` was rebased onto it, so the line is linear again. **Nothing on the 7a line is pushed** — `origin/ci-test-separation` still points at `614b4ec`.

**Two parallel unmerged branches off `main`.** Consequences:
- ⚠️ **Merge-conflict risk.** Both touch `Source/Midi/MidiHandler.cpp`, `Source/UI/MainComponent.cpp`, `MainComponent.h`. `614b4ec` fixes the chord-zone internal-synth leak via `suppressInternalFeed`; `5c75d4f` re-implements the same intent via the `channelForHand` parity rewrite. These will collide.
- The 2026-08-09 log note "no such symbol existed on this branch" was **not** a history error — `suppressInternalFeed` is simply on the other branch.
- CI Task 1's guard is **not** on `fix/sfz-integration`: `Source/UI/OverlayComponent.cpp:31` still calls `getInstance()->systemRequestedQuit()` unguarded.

## Key Recent Facts
- **App name lives in `Source/Common/AppInfo.h`** (`AppInfo::appName`), used by `IOHelper::getAppDataFolder()`, `MidiDevicesDataBase::getAppDataFolder()`, `MainComponent`'s `PropertiesFile::Options`, and `Main.cpp`'s `MainWindow` title. `ProjectInfo::projectName` is no longer what titles the window. Data now lives in `%APPDATA%\Arranger Workstation\`.
- Piano-App is a JUCE C++ arranger keyboard. [[Keyboard & Main UI]] (`MainComponent`) owns/wires everything. Two parallel engines: classic [[Track Playback]] and the [[Arranger Engine]]; sound via [[Audio & SFZ Playback]].
- **Missing-SFZ popup (`b5765e9`).** SFZs map per **style + instrument**, never per channel (`SFZLibraryManager`: `styleId → {instrumentNumber → entryId}`). `describeSfzSlot(channel)` inverts `AudioHandler` routing so the popup says `Violin (right hand) in style "Pop Ballad"`. GM names live in `SFZlibrary::getGMInstrumentName`. **Built clean, NOT smoke-tested live.** See [[Audio & SFZ Playback]].
- **Physical-keyboard internal-SFZ fix (`5c75d4f`, hardware-confirmed).** `ok` was set inside the MIDI-OUT lock, so with no external OUT open `noteOnReceived` never fired. Now decided first, with hand-split channel + transpose. See [[MIDI Handling]].
- **Phase 7a — real NTT (`614b4ec`, unmerged).** Per-track `NttType {NoTranspose, Parallel, Chord, Fixed}` + `NttScales.h`; `ChordTransposer` role-remaps 3rd/5th/7th and scale-snaps passing tones. Schema v5 + migration.

## Build / Test
- Build: MSBuild Debug x64 — **close the app first (PDB lock)**. `Builds\VisualStudio2022\Arranger Workstation.sln`. JUCE **8.0.13** at `Desktop\Latest Juce\JUCE`.
- Tests: `--run-tests --unit-tests|--arranger-tests|--integration-tests`. Runner in `Main.cpp:27` is **commented out** (baseline).
- Headless target `ci/Builds/.../ConsoleApp/Project Tests.exe` exists and ran **arranger 701 pass / 0 fail**.

## Active Threads / Next
- ✅ **NOT A BUG 2026-08-19 — "app is silent on laptop, then on PC too" was chord-zone mute left on.** Settings: `ChordZoneMute=1`, `ArrangerModeEnabled=1`, `ChordMode=0` (Fingered), `style_1.default.rightHandBound=-1`. `muteChordZoneNote()` = `chordZoneMute && arrangerEngaged && inChordZone(note)`, and with **no split set `inChordZone()` returns true for the whole keyboard** (`MidiHandler.cpp:1029`) — every key muted, presenting as total silence. Proved by temporary boundary logging: `noteOn: ... ok=1 muteChordZone=1 midiOutOpen=1 [sent to MIDI OUT: NO]` on every note; device path was clean (`deviceOpenOUT(0) -> OK`, single MIDI out = GS synth, `EngineOptionIndex=1`), and a raw winMM arpeggio to the GS synth was audible, clearing Windows audio early. See [[MIDI Handling]]. **Possible polish (not done):** make the mute a no-op when `rightHandBound == -1`.
- ✅ **FIXED 2026-08-19 (committed `a73d4c8`) — `;` and `'` were dead on non-US keyboard layouts.** `mapKeyMidi` switched on `key.getKeyCode()`, which JUCE computes as *scan code → VK → unshifted char **in the active layout*** (`juce_Windowing_windows.cpp:3163`, `doKeyChar`). Under US those keys report 59/39 and matched `case ';'`/`case '\''`; under **Romanian (`0x0418`) they report 537 `ș` / 539 `ț`** and fell through to `default: return -1`. Letters are identical on both layouts, which is why only these two failed and why it looked intermittent (Win+Space).
  - **Fix:** `mapKeyMidi` now keys off **physical position**. `pianoKeyScanCodes[]` holds the 18 PS/2 set-1 scan codes in semitone order; `refreshKeyMapIfLayoutChanged()` derives live key codes into `keyCodeToOffset`, rebuilding only when `GetKeyboardLayout(0)` changes. The 18-case switch is gone; both range guards preserved exactly. `intToKey` still stores `key.getKeyCode()`, so the `VkKeyScan`-based release path is untouched. Plus `releaseAllHeldNotes()` on layout switch — a held key's stored code is unresolvable on the new layout, so the note would otherwise hang.
  - ⚠️ **The first attempt shipped broken, and the reason is the real lesson: use `MapVirtualKeyExW`, never unsuffixed `MapVirtualKey`.** The unsuffixed macro resolved to the **ANSI** variant, which cannot represent a character outside the code page and returns `'?'` (63). On Romanian *both* offset 16 and 17 came back as 63, so `operator[]` let the second silently **overwrite** the first — the table lost offset 16 entirely and neither key worked. Fixed by the `Ex`+`W` form with an explicit `HKL`, plus `emplace` instead of `operator[]` so a collision can never silently drop a key.
  - **Verified** by a standalone probe running the final function body under both installed layouts: 18/18 distinct entries each, offset16/17 = 59/39 on US and 537/539 on Romanian, matching the codes the app actually reported. **Confirmed live in-app on the Romanian layout 2026-08-19.**
- ✅ **RESOLVED 2026-08-19 — intermittent `0xC0000374` STATUS_HEAP_CORRUPTION at startup was a stale-object build artifact, not a code bug.** Adding two members to `KeyboardListener` changed `sizeof(MainComponent)` (`MainComponent.h` includes `keyListener.h`). MSBuild's incremental build recompiled `MainComponent.cpp` and `keyListener.cpp` but **not `Source/Main.cpp`** — `Main.obj` stayed stale by 41 min (22:30 vs header edit 23:11). `Main.cpp` does `new MainComponent()` with the OLD size while the ctor writes members at the NEW offsets → heap overrun. Intermittent (~1 in 5) because a small overrun only trips the heap manager depending on allocation layout, and it surfaced as a debugger stop in `juce::SwapChain::create` → `CreateSwapChainForComposition` because ntdll only detects it during a later large allocation — that call site was innocent (a standalone D3D11 probe created composition swap chains with JUCE's exact parameters on both adapters, all `S_OK`). **Fixed by `/t:Rebuild`; 8/8 clean launches, 0 crash events.** Windows had no prior crash record for this app, which is what proved it was new. **Rule: a header change that alters a type's layout needs a full rebuild** — see the build-workflow memory. Unrelated to the [[GL Overlay Rendering]] crash, which was already fixed in `cfe100b` and whose tripwire has **never fired** — `Desktop/piano_gl_errors.log` does not exist, including after ~20 app launches on 2026-08-19.
- 📝 **Desktop is OneDrive-redirected** to `C:\Users\Oricum\OneDrive\Desktop`, so anything written via `File::userDesktopDirectory` lands there, **not** `%USERPROFILE%\Desktop`.
- 🟡 **`b5765e9` needs a live smoke test** (popup wording + wider box layout).
- 🟡 **Phase 7a manual sign-off pending** — 0/9 in `docs/superpowers/test-plans/2026-06-26-arranger-phase7a-ntt-testing.md`. Automated suites green.
- ✅ **CI test-separation — Tasks 1–6 code-complete (27/33 steps), all committed on `ci-test-separation`, NOT pushed.** `2047f7c` guard · `438afe3` test target · `919ffa9` app stripped · `1a726fe` workflow · `33142a4` README. App builds with **zero** `test_*.obj`; unit **665/0**, arranger **705/0**.
  - **Task 2 was never actually pending** — my earlier "blocker" reading was wrong. `<MODULEPATHS>` entries are inert; `<MODULES useGlobalPath="1">` is what governs, and both `.jucer` files already had it. Verified via Projucer's `defaultJuceModulePath` + both generated `.vcxproj` resolving to `Latest Juce\JUCE\modules`.
  - ✅ **PUSHED and CI IS GREEN.** `ci-test-separation` pushed 2026-08-15; workflow run #1 (`31886118353`) **success in 4.1 min**, all 15 steps, `test-results` artifact uploaded. The two risky steps — locating Projucer inside the archive, and `Start-Process -Wait` actually blocking on the runner — both worked. Task 5 is verified, not just written.
  - ✅ **Branch protection enabled on `main`** (2026-08-15; confirmed via API: `branches/main` → `protected: true`. The detailed config needs admin auth — 401 unauthenticated — so *which* settings are on is unverified from here).
  - ✅ **GATE PROVEN END-TO-END (plan Task 6 Steps 1/3 done).** Throwaway branch `ci-gate-redcheck` (`5a4f188`, one `expect (false, …)` in `test_chord.cpp`) → CI run #3 **failed** (unit green, arranger red) → PR into `main` showed the **Merge button blocked**. So the workflow catches real failures *and* protection is wired to the right check context. Delete the branch + close the PR.
  - ⚠️ **`main` requires 1 approving review — a solo maintainer cannot satisfy this.** GitHub forbids approving your own PR, so every PR into `main` (including the real `ci-test-separation` one) is unmergeable until **Required number of approvals** is set to **0**. Keep "Require a pull request" itself — that is what forces checks to run.
  - ✅ **Node 20 deprecation cleared (`e394005`, pushed; run #4 green, 4.4 min).** checkout v4→v7, cache v4→v6, upload-artifact v4→v7, setup-msbuild v2→v3. Both risks from the major jumps came back clean: *Download JUCE* still **skipped** (cache key survives v6) and the artifact still uploads at the same 1.3 KB.
  - **Remaining:** eyeball the app GUI after the test-strip.
  - ✅ **False-green hole closed (`1159244`, pushed; CI run #2 green, 5.1 min — JUCE download *skipped*, cache working).** The gate checked exit code only, and the runner exits 0 on *zero* tests. Both steps now also parse the `TOTAL` line and fail below a floor (`MIN_UNIT_ASSERTIONS` 650, `MIN_ARRANGER_ASSERTIONS` 690; actual 665 / 705, so 15 headroom each). A missing `TOTAL` line also fails. Verified locally on both suites **plus a negative control** with the floor raised, proving the guard fires rather than passing vacuously. Lower the floor deliberately if tests are ever removed.
  - ⚠️ **Projucer detaches.** `& Projucer.exe --resave` returns immediately, so a following MSBuild raced it and silently compiled the *old* file list — the app "built clean" while still containing all 34 test files. Use `Start-Process -Wait`, and verify by counting `test_*.obj`, never by exit code. The workflow and README both encode this.
- 🟢 **OpenGL crash parked** (unreproducible); permanent tripwire logs to `Desktop/piano_gl_errors.log`. See [[GL Overlay Rendering]].
- Roadmap after 7a: **7b Chord Variation → 8 Yamaha importer → 9 Korg importer** (no specs written). See [[Arranger Phase Roadmap]].
- 📝 **Seam note-off tags are vestigial** (documented 2026-08-19 on [[ArrangerScheduler]] + [[Loop-Seam Note-Off]]): the scheduler stamps `PartKind`/`NttType` on synthetic note-offs but nothing reads them — the engine resolves offs via its own `activePlayedNote` map instead. Harmless dead weight; do not build on those tags.
- ✅ [[Chord-Aware NTT Transposition]] rewritten 2026-08-15 against `614b4ec` (four `NttType` tables, decision order, snap scales, schema v5 migration, scheduler tag plumbing).
- ✅ **FIXED 2026-08-15 (uncommitted, on `arranger-phase7a-ntt-engine`): master NTT bypass didn't reach the bass.** Fix = `if (! nttEnabled) return noteNumber;` replacing the type-rewrite, + a regression test. **Verified by reverting:** with the old code the new test failed 3/4 with exactly the predicted values (root-position Am → 52 = 43+9; C/E → 47 = 43+4); with the fix, arranger **705/0**, unit **665/0**, both exit 0. The 4th assertion (inversion still works when the bypass is off) passes in both, so the feature is intact. Original diagnosis below.
- 🔴 **The bug: master NTT bypass didn't reach the bass.** `transpose()` rewrites the *type* to `NoTranspose` **before** the bass-inversion early-return, which returns first. So with "Chord transposition" OFF + Bass Inversion ON, the bass still shifts by `active.bassNote - original.root` while everything else plays in the recorded key. **Not a slash-chord edge case** — `ChordDetector::recognizeSet` sets `bassNote = lowestPc` for *every* chord, so a root-position Am over a C-recorded style moves the bass **+9**. The bypass test asserts on `PartKind::Bass` but never sets `bassInversion`, so it passes. Fix = `if (! nttEnabled) return noteNumber;` + a regression test. Code is on `614b4ec` (not checked out). Full reasoning on [[Chord-Aware NTT Transposition]].
