---
type: meta
title: "Operations Log"
created: 2026-06-06
updated: 2026-09-04
tags:
  - meta
  - log
status: evergreen
---

# Operations Log

Append-only. Newest entries at the TOP. Never edit past entries.

## 2026-09-04 — ARCHITECTURE.md added; ADR-002 flags MainComponent/Display for future refactor
Added `ARCHITECTURE.md` at the repo root: subsystem breakdown, MainComponent's
composition-root role (with a real callback-wiring snippet), the two-parallel-engines
mode split (`TrackPlayer` vs `ArrangerEngine`, kept separate deliberately — they're two
app modes, not a legacy duplicate), the arranger's pure-core/impure-shell split, the
threading model, and three Mermaid diagrams (system context, whole-app components,
arranger internals).

Recorded **ADR-002**: `MainComponent.cpp` (~3,083 lines) and `Display.cpp` (~1,174
lines) are flagged as large integration files, but refactoring either is deliberately
deferred — no concrete pain justifies the churn yet. `Styles/CurrentStyleComponent.cpp`
(~1,375 lines) has the same shape and is noted for the same future scrutiny, though not
otherwise addressed. If a refactor does happen: lean on small DI-style coordinator
classes (constructed, handed references) over a wholesale rewrite; explicitly avoid an
event bus, since most of `MainComponent`'s coupling is direct 1:1 calls, not genuine
many-to-many fan-out — a bus would trade a bluntly-readable file for indirection without
solving a real problem. Trigger to revisit: wanting to unit-test the audio+arranger
stack without the GUI, a second host window, or real concurrent work on disjoint areas —
not general file-size discomfort. See [[ADR-002 Defer MainComponent Display Refactor]].

## 2026-08-29 (later) — `ci-test-separation` merged in (`50e2d02`); CI live on the branch; branches pruned
`fix/sfz-integration` now carries everything: the rename, the SFZ popup work, the PC-keyboard fix, **Phase 7a NTT**, the wiki, and **CI itself**. Pushed; the workflow runs on this branch from now on.

- **The merge (`50e2d02`, parents `539dc37` + `e394005`).** Four conflicts:
  - **`Source/Midi/MidiHandler.cpp`** → took this branch's parity rewrite whole and **dropped `suppressInternalFeed`**. The 7a flag kept a muted chord-zone onset off the internal SFZ feed; `5c75d4f` already does that by moving the `incomingMidiMessages` note-on write inside the `!muteChordZone` guard, and goes further by splitting the single trailing `addEvent` into explicit note-on / note-off / passthrough paths. Note-offs still always pass, so the no-stuck-note invariant holds. Merged file is byte-identical to the pre-merge branch version.
  - **`Source/Main.cpp`** → combined: no `--run-tests` path in the production app (CI side) **+** `AppInfo::appName` window title (rename side).
  - **`.gitignore`** → kept `ci/Builds/` + `ci/JuceLibraryCode/`, but **`Piano-Wiki/` stays un-ignored**; re-adding it would have silently stopped tracking new wiki pages.
  - **`Arranger Workstation.jucer`** → rename kept, 74-line Tests group removed (matches the CI branch's own 74-line deletion exactly).
- **`Source/Common/AppInfo.h` added to BOTH `.jucer` file lists** (id `aPp1nF`). It had been in neither since the rename created it — a violation of the maintenance contract the CI README documents.
- 🔑 **A stale test surfaced, and the reason matters more than the fix.** `test_midi_handler.cpp` asserted *"noteOn does NOT notify listener without output device"* — the **exact behaviour `5c75d4f` deliberately changed**, since gating on the `midiOut` lock is what stopped a physical keyboard sounding on the internal SFZ. `startNoteSetting`/`endNoteSetting` default to `-1`, so note 60 sets `ok = 1` and the listener now fires with no device open. The test had been wrong since `5c75d4f` landed but **never ran**, because the runner in `Main.cpp` was commented out on this branch. Updated to assert the intended contract. **Any test touching code changed between `d05e07b` and now may have drifted the same way — the app compiled tests without running them.**
- **Verified locally before committing:** app builds Debug x64 **0 errors**, **zero `test_*.obj`**, `Arranger Workstation.exe` produced. Unit **665/0**, arranger **705/0**, both exit 0 — matching baseline and clearing the CI floors (650/690).
- **Branch pruning — 12 refs deleted, 6 local + 6 remote.** `arranger-engine-phase1-2-3`, `arranger-engine-phase4-5-6`, `arranger-engine-phase4-chords`, `arranger-phase7a-ntt-engine`, `ci-test-separation` (all merged, 0 unique commits), `ci-gate-redcheck` (throwaway red-check, purpose served — its commit was self-labelled for deletion), and `sfz-integration` (2 merge commits, **0** files changed vs merge base). Only `main` and `fix/sfz-integration` remain. Deleting `ci-gate-redcheck` should have auto-closed its PR.
- **Earlier the same day:** `71063de` brought the Piano-Wiki vault into git (it had been gitignored, which is why the `log.md` truncation was unrecoverable); `539dc37` rewrote the README off the "synth" framing — the app is a **sampler**, no oscillators exist in the codebase.

## 2026-08-29 — Archived from the hot cache (previously undocumented in this log)
These were resolved on 2026-08-19 but lived only in `hot.md`, which has been trimmed back toward its ≤500-word budget. Preserved here verbatim in substance.

- ✅ **NOT A BUG — "app is silent on both machines" was chord-zone mute left on.** `ChordZoneMute=1`, `ArrangerModeEnabled=1`, `rightHandBound=-1`. `muteChordZoneNote()` = `chordZoneMute && arrangerEngaged && inChordZone(note)`, and **with no split set `inChordZone()` returns true for the whole keyboard** (`MidiHandler.cpp:1029`) — every key muted, presenting as total silence. Device path was clean. **Possible polish (not done): make the mute a no-op when `rightHandBound == -1`.**
- ✅ **FIXED (`a73d4c8`) — `;` and `'` dead on non-US layouts.** `mapKeyMidi` switched on `key.getKeyCode()`, which JUCE resolves through the *active layout*; under Romanian those keys report 537/539 and fell through to `default: return -1`. Now keyed off **physical scan code** (`pianoKeyScanCodes[]` + `refreshKeyMapIfLayoutChanged()`), plus `releaseAllHeldNotes()` on layout switch. ⚠️ **The real lesson: use `MapVirtualKeyExW`, never unsuffixed `MapVirtualKey`** — the ANSI variant returns `'?'` (63) for out-of-codepage chars, so two offsets collided and `operator[]` silently dropped one. Fixed with the `Ex`+`W` form and `emplace`.
- ✅ **RESOLVED — intermittent `0xC0000374` STATUS_HEAP_CORRUPTION at startup was a stale-object build artifact.** Adding members to `KeyboardListener` changed `sizeof(MainComponent)`, but MSBuild did not recompile `Main.cpp`, whose `.obj` stayed 41 min stale; `new MainComponent()` allocated the OLD size while the ctor wrote at NEW offsets. **Rule: a header change that alters a type's layout needs `/t:Rebuild`.**
- 📝 **Desktop is OneDrive-redirected** to `C:\Users\Oricum\OneDrive\Desktop`, so `File::userDesktopDirectory` writes land there, not `%USERPROFILE%\Desktop`.
- 📝 **Seam note-off tags are vestigial** — the scheduler stamps `PartKind`/`NttType` on synthetic note-offs but nothing reads them; the engine resolves offs via its own `activePlayedNote` map. Do not build on those tags.

> [!warning] Partial reconstruction — 2026-08-29
> This file was truncated to 0 bytes on 2026-08-29 (a write that opened the file in truncating
> mode and then failed mid-encode). It was rebuilt from Obsidian File Recovery snapshots plus
> in-session context. **Entries between 2026-06-25 and 2026-08-15 were NOT recoverable** — the
> File Recovery database only holds snapshots up to 2026-06-25, because later entries were written
> to disk outside Obsidian and were never snapshotted. OneDrive version history had nothing.
> The work from that gap is still summarised in [[hot]] and `.vault-meta/last-sync.json`;
> only the dated log entries are lost.

## 2026-08-29 — App renamed to "Arranger Workstation" (`128cf87`); `ChannelDSP` chain mapped
Session was mostly a LinkedIn/README accuracy pass that turned into a rename and an audio-path audit.

- **`128cf87` — rename.** New `Source/Common/AppInfo.h` (`AppInfo::appName`, header-only and
  dependency-free) is the single source of truth. Removed every hardcoded `"Piano Synth2"` from
  `Source/`: `IOHelper::getAppDataFolder()` now backs the call sites in `Display`,
  `TrackListComponent` and `MidiDevicesDataBase` (several were open-coded
  `userApplicationDataDirectory` lookups, not even going through `IOHelper`). `Main.cpp` passes
  `AppInfo::appName` to `MainWindow` instead of `ProjectInfo::projectName`, so the title bar no
  longer depends on the build system. Projucer project + both target names renamed, and the
  `.jucer` file itself `git mv`'d (git recorded it as a 98% rename). `.gitignore` had the old
  filename listed **twice** — deduped to one entry.
- **Migration deliberately skipped (user's call).** `%APPDATA%` moved with the name and the old
  `Piano Synth2` folder was left behind: settings, `SFZLibrary.json`, `allStyles.json`,
  `mySections.json`, `myTracks.json` and 2 `.style` files (~1.2 MB) are all still there but
  invisible to the app. Consequence to remember: `b5765e9`'s missing-SFZ popup smoke test now
  needs SFZs re-added first. Note the settings *filename* also derives from the app name
  (`options.applicationName`), so any future migration must rename the file, not just the folder.
- **Build hygiene.** Projucer left the old `.sln`/`.vcxproj`/`.filters`/`.user` behind and the user
  rebuilt against them by mistake (exe timestamp 6 min after the resave, still old-named) — the
  *source* changes were in that binary, only artifact naming wasn't. Old project files deleted,
  `.vcxproj.user` (F5 `--run-tests`) ported, and `x64/{Debug,Release}/App` wiped so the next build
  is clean by construction — which also removes the stale-`.obj` path behind the `0xC0000374`
  incident. `.vs/Project Synth2` cache is still locked by a running VS.
- **`ChannelDSP` fully mapped** — see [[Audio & SFZ Playback]]. Serial in-place chain, fixed order,
  neutral-value bypass per stage (and *why* the filter's check differs). Recorded which effects are
  hand-written (tanh distortion + gain compensation, tremolo LFO, circular-buffer delay, smoothed
  random mod) vs JUCE's (`StateVariableTPTFilter`, `Reverb`, `Chorus`), since that distinction
  matters for how the work gets described.
- 🔴 **Bug found, not fixed: the delay bypass freezes the delay line.** `AudioHandler.cpp:181-193`
  keeps the buffer *write* inside `if (delayMix > 0.005f)`, so switching delay off freezes the
  buffer and switching it back on replays stale audio at full mix. Fix options on the module page.
- **Effects reach both engines.** One knob fires `sendMidiCC` (external, its own DSP) *and*
  `injectCC` (internal, `ChannelDSP`). CC7/10/91/92/93 match GM2; **CC80/94/95 diverge** (GM2 reads
  94 as celeste, 95 as phaser), so those three behave differently on external gear.
- **`a73d4c8`** — the PC-keyboard layout fix is now committed (hot.md had it as uncommitted).
- **`README.md` rewritten but left uncommitted** — dropped the "synth" framing (it's a sampler; no
  oscillators exist in the codebase), restructured Features around the arranger, added architecture
  notes and a real roadmap. The `## Building` section was deliberately left untouched so it doesn't
  collide with `ci-test-separation`'s better "Building & Tests" rewrite.

## 2026-08-15 — CI test-separation executed: Tasks 1–6 committed (`2047f7c`…`33142a4`, not pushed)
Ran the CI plan end to end on `ci-test-separation`, rebased first onto `cf423b0` so it sits on the NTT fix. 27/33 plan steps now done; the 6 that remain all need a push or a human. Plan file (`docs/…` — **gitignored**, stays local) updated with a status header and real checkboxes, fixing the stale-tracker problem this session opened with.

- **`2047f7c`** CI Task 1 (rebased). **`438afe3`** Task 3 — `ci/Tests.jucer` + `ci/test_main.cpp` + `.gitignore`. **`919ffa9`** Task 4 — dropped the 34-file Tests group from `Project Synth2.jucer` and deleted the `--run-tests` runner from `Main.cpp`. **`1a726fe`** Task 5 — `.github/workflows/ci.yml`. **`33142a4`** Task 6 — README "Building & Tests" (also replaced boilerplate that referenced a `MyPlugin.jucer` which never existed here).
- **Final verification:** app builds Debug x64 EXIT 0 with **zero** `test_*.obj`; test target builds EXIT 0; unit **665 / 0**, arranger **705 / 0**, both exit 0.
- **CORRECTION — Task 2 was never pending.** The earlier audit called it "the blocker" after reading the `<MODULEPATHS>` block. Those entries are **inert leftovers**; `<MODULE … useGlobalPath="1">` is what governs, and every `juce_*` module in *both* `.jucer` files already had it (`SFZero` correctly local). Verified three ways: Projucer's `defaultJuceModulePath` is set to `Latest Juce\JUCE\modules`; both generated `.vcxproj` resolve there; **zero** files under either `Builds/` or `JuceLibraryCode/` mention the stale `NEW JUCE` path. Task 2's own Step 5 grep is cosmetic — the plan says the build is the real proof.
- ⚠️ **Projucer is a GUI-subsystem exe and does not block.** `& Projucer.exe --resave` returned instantly (`$LASTEXITCODE` came back *empty*), so MSBuild started against a half-written project and compiled the **old** file list: the app reported `EXIT 0` while still containing all 34 test files. Caught only by counting `test_*.obj` (34, timestamped *after* the regenerated `.vcxproj`). Fix = `Start-Process -Wait -NoNewWindow`, now encoded in the workflow, the README and the plan. **Never trust a Projucer-then-build sequence on exit code alone.**
- **The 🖱️ steps were not GUI-only.** `--resave` and `--set-global-search-path` exist in 8.0.13, so Tasks 2–4's manual steps were done headlessly: edit the `.jucer` XML, resave, rebuild. The Tests group was lines 6–79; after deletion the XML validated and GROUP open/close stayed balanced 10/10.
- **Both confirm-on-first-run values verified** rather than trusted: the release asset really is `juce-8.0.13-windows.zip` (checked against the GitHub releases API), and the Projucer flag syntax matches `--help`. Workflow additions beyond the plan: JUCE zip cached by key, Projucer located by search instead of assuming the archive layout, and both suites' results uploaded under distinct names (the plan's single `test-results.txt` would have been overwritten by the second run).

---

> [!missing] Gap: 2026-06-25 → 2026-08-15
> Entries from this period were lost and could not be recovered. Known from other pages, the
> period covered: the Phase 7a NTT engine (`614b4ec`) and its bypass/bass-inversion fix
> (`cf423b0`), the GL overlay crash fix (`cfe100b`), the `channelForHand` parity rewrite
> (`5c75d4f`), and the missing-SFZ popup (`b5765e9`). See [[hot]], [[Chord-Aware NTT Transposition]]
> and [[GL Overlay Rendering]] for the substance.

## 2026-06-25 — Sync: Phase 4 chord recognition + transposition (branch `arranger-engine-phase4-chords`)
Synced the whole Phase 4 feature (25 commits `11687db..0d1d364`, branched off `main`'s `63b6aa7`) plus the still-**uncommitted** keyboard-focus fixes from this session. Phase 4 = the accompaniment now transposes to the chord the player holds, Korg/Yamaha-grade. New pages [[Chord Recognition & Transposition]] (module) + [[Chord-Aware NTT Transposition]] (concept); updated [[Arranger Engine]], [[MIDI Handling]], [[Arranger Style Authoring]], [[Keyboard & Main UI]], [[Arranger Phase Roadmap]] (Phase 4 → ✅), index.
- **Chord value type** (`c0b8276`): `ArrangerChord` (root/quality/bassNote) — renamed from `Chord` (`48e2fef`) to dodge `HelperFunctions::Chord`; `PartKind {Fixed,Acc,Bass}`; `chordIntervals`.
- **ChordDetector** (`ac74b6a`): held-notes → chord via interval templates; Sus2/Sus4 by root==bass; full-keyboard hysteresis (≥3); `detectKeyFromEvents` key-finder.
- **ChordTransposer / NTT** (`2e01684`, corrected in `ba5d420`): **root-shift + third-flip**, NOT snap-to-chord-tone (the first version destroyed the melody — "sounds weird, C doesn't restore original"). Identity preserved. Bass inversion (`6c4ab90`) **re-bases the whole bass line** onto the played slash-bass.
- **Emit-time wiring** (`09c96e1`): scheduler `PartKind` tags + engine `chordLock` mailbox; `dispatchEmitted` transposes per event; `activePlayedNote` map keeps note-off matched to note-on. `setStyle` resets active chord → home key every Start; detector **always re-forwards** (`924393f`).
- **Original chord persistence** (`5be0afa`): style-level `originalRoot/Quality`, schema **v3→v4** (v3 ⇒ C major). Auto-detected at authoring (`34ec155`) + editable root/quality combos (`bf66c76`). Bass track = name contains *bass* (`cebb477`).
- **Input** (`53ab878`, `11687db`): split-zone (`< rightHandBoundSetting`) chord input; `channel` made a per-note local (removed input/UI shared state). Settings toggles — bass inversion / full-keyboard / chord memory (`56b9717`, panel grown `b214b9c`).
- **This session (uncommitted): keyboard-focus fixes.** Root cause: once focus left `MainComponent`, nothing reliably reclaimed it (no `mouseDown` handler; scattered grabs). Added a **global mouse listener** (`addMouseListener(this,true)` + `mouseDown` → grab focus on any non-text click, skipping modals/overlay/TextEditor), plus targeted re-grabs after Play-init and after arranger **Start** (new `onRequestPlayFocus` chain CurrentStyleComponent→Display→MainComponent). Also BPM slider locked during arranger play. Builds clean (MSBuild Debug x64). **Not committed yet** — awaiting user's play-test.
- **Known issues recorded for later** (in [[hot]]): (1) note visuals stay drawn after clicking a style until next play; (2) held notes hit a max length and "freeze" — want a visual sustain cue; (3) holding a chord while pressing Start silences those held notes (likely start/`haltAudio` all-notes-off). Logged in [[Arranger Engine]] too.

## 2026-06-24 — Sync: arranger UX batch committed + merged to `main`; arranger play-start fix (`63b6aa7`)
Caught the wiki up after a long gap. The 2026-06-08 "uncommitted working-tree batch" is now **committed and on `main`** (PR #7 `arranger-engine-phase1` merged via `6c8c6f8`). `main` is the latest branch and HEAD is `63b6aa7`. Synced `1957c0e..63b6aa7`:
- `daf101d` **fix update juce** — JUCE update touch-ups (`Main.cpp`, `MainComponent`, tooltip/tree/style-view).
- `e4ac01c` **Bug fixes + UI for sections edit** — editor **Update Tracks** button (disabled until the config is saved; replaces a saved config's notes with the current recording, keeps sections, overwrites the same `.style` with confirmation via `updateTracksFromRecording` + `onRequestCurrentTracks` wired to `ArrangerSourceBuilder::fromTrackEntries`); new section **lands at the viewport bar** not bar 1 (`ArrangerTimelineComponent::barForX` → `xToSnappedBar`, clamped); `applyLiveTrackVolumes` overlays slider volumes by UUID before Start.
- `8979b6d` **Non-blocking UI edit operations** — the async save/load/update-tracks work (heavy serialize/parse/disk off the UI thread; editor + app loading overlays via the `onBusy` chain).
- `b0dd620` **Update Project Synth2.jucer** — Projucer re-save (file list / header paths).
- `63b6aa7` **starting arranger play fix** (HEAD) — the actual start-on-chosen-section fix. `startPlaying` now calls `engine.selectStartSection(pendingStartType, pendingStartName)` **after `setStyle`, before `start()`** in both the active-config and demo branches (undoes the `stopPlaying(false)` wipe). The host now owns the durable choice (`CurrentStyleComponent::pendingStartType`/`pendingStartName`, default `Variation`/`Variation 1`); the old free function `routeSectionToArranger` became the member `selectOrQueueSection` (playing→`queueSection`, stopped→arm+highlight), plus `highlightPendingStartSection` called on config load / mode-enable / after Stop. `applyActiveConfig` re-arms Var 1 on every load. `ArrangerEngine::indexOfSection` switched from exact-string to **type + trailing-number** match (so "Var 2"→"Variation 2" instead of silently falling back to Var 1).
- Module pages [[Arranger Engine]] and [[Arranger Style Authoring]] already described this batch accurately (written while uncommitted) — no content change needed; only hot.md / log / sync pointer advanced.

## 2026-06-08 — Arranger UX batch: start-section pick, async I/O, Update Tracks, live volume (working tree)
Still `arranger-engine-phase1`, **uncommitted** (HEAD `1957c0e`). Six behavioural changes this session; updated [[Arranger Style Authoring]] and [[Arranger Engine]].  *(snapshot truncated here — the rest of this entry was not recoverable)*

## 2026-06-07 — Backlog noted: split tests into a separate project (deferred)
User wants to move the test suite out of the app build **at some point, not now**. Decision captured in [[hot]] under "Deferred / Backlog": Core static lib + App + Tests console project, so the app build stops compiling the 49 test `.cpp`s. Also corrected the stale note that the test runner is "commented out" — it's actually flag-gated via `--run-tests`. No code changed.

## 2026-06-07 (later) — Live section sync, section-relative playhead, engine tests, editor/live separation (working tree)
Still on `arranger-engine-phase1`, **uncommitted**. HEAD unchanged (`3cc1610`). This pass documents the
session's behavioural refinements; updated [[Arranger Engine]], [[Arranger Style Authoring]], [[ArrangerSectionSequencer]].
- **Click-to-jump fixed.** Bare timeline click failed to jump because sub-bar jitter → `mouseDrag` → `onWindowsChanged` → `rebuildPreview`/`setStyle` → `sequencer.reset()` wiped the queued `pendingIndex`. Now a gesture only counts as an edit when a bar value actually changes (`windowsChangedByDrag`). Root-caused via DBG instrumentation + a `getStackBacktrace()` probe; all `[sec]` tracing since removed.
- **Section-relative playhead arrow** in the editor (reads `getActiveSectionIndex()` + new `getActiveSectionLocalBeats()`/`getActiveStartAbs()`), wraps within a looping section's window; auto-follow tracks it.
- **Engine-driven live button highlight** (arranger mode): clicks only route to the engine; `onActiveSectionChanged` lights the actual sounding section (auto-fill: Var2→Fill lights→Var2). New `StyleSectionComponent::setEngineDrivenHighlight` + `setHighlightedButton`; chain `ArrangerEngine→CurrentStyleComponent→Display→MainComponent::highlightArrangerSection`.
- **Beat bar red→yellow on stop**: new `onStoppedItself` (fired from `haltAudio` only when it was playing) drops the UI playing flag; after an Ending, Var 1 re-arms.
- **Auto Fill toggle persists app-wide** (`ArrangerAutoFillEnabled` in app properties).
- **Editor/live are separate modes**: `presentOverlay` stops live playback before showing browser/editor (no inherited stale section / no Ending firing on close).
- **Engine unit tests fixed**: the queued-switch drain moved from `hiResTimerCallback` into `renderRange`, so direct `renderRange` calls (tests) apply queued switches. Documented the `requestLock` thread model (UI→timer mailbox; only the timer thread mutates the sequencer; `callAsync` back to UI).

## 2026-06-07 — Arranger style authoring + live sections + overlay fixes (working tree)
Synced commits since `c1ac203` up to HEAD `d56b503` (source folder reorg, gitignore/test toggle,
Phase 3 Part A codecs/model/IO/channels/builder, and the `d56b503` editor/timeline baseline),
plus this session's **uncommitted working-tree** refinements:
- New page [[Arranger Style Authoring]] — editor, timeline, browser, per-style config, storage,
  section semantics, Auto Fill wiring. New concept [[GL Overlay Rendering]].
- Updated [[ArrangerSectionSequencer]] — `findSection` matches by type + trailing number
  (`Var 2`→`Variation 2`); Fill/Break save `returnIndex`; **Auto Fill** (`fillReturnOverride`).
- Session changes (uncommitted, branch `arranger-engine-phase1`): active config shown only in the
  browser (✓) not the dropdown + per-style stale-state fix + persists to `allStyles.json`;
  `.style` files moved to `Piano Synth2/ArrangerStyles/`; rename-on-save + Delete button + hover
  tooltips + unique per-type section names + Add Var/Break buttons + click-to-jump preview;
  Load no longer auto-plays; loading label hides the GL note layer; ESC menu is now an in-app
  child (hides note layer + keyboard, clears notes on close); Korg semantics confirmed
  (Intro one-shot → Variation loops; Break returns to current variation).

## 2026-06-06 — Source/ reorganized into subsystem folders
- Moved 96 files from flat `Source/` into `Midi/ Audio/ Playback/ Styles/ Backend/ UI/ Common/`
  (mirrors the wiki modules); `Arranger/` already existed; `Main.cpp` stays at root.
- `.jucer` `file=` paths rewritten + `headerPath` extended per folder (bare includes still resolve).
- On branch `refactor/source-folder-structure`; pending Projucer re-save + build verify before commit.
- Updated module pages' `path:`, [[modules/_index]], and [[overview]] folder map.

## 2026-06-06 — Arranger Engine deep dive
- Read all 5 arranger `.cpp` implementations + headers.
- Split the arranger into focused pages: [[Arranger Engine]] (timer loop + hub),
  [[ArrangerScheduler]], [[ArrangerSectionSequencer]], [[ArrangerPatternBuilder]],
  [[ArrangerModel]], [[ArrangerTime]].
- Added flow [[Section Switch Flow]], decision [[ADR-001 Pure Schedulers and Parallel Engine]],
  concepts [[Monotonic Beat Playhead]], [[Bar-Synced Section Switching]],
  [[Loop-Seam Note-Off]], [[Arranger Phase Roadmap]].
- Updated all folder indexes + [[index]].

## 2026-06-06 — Codebase ingested into module pages
- Read the key headers across `Source/` and wrote 7 module pages:
  [[Arranger Engine]], [[MIDI Handling]], [[Audio & SFZ Playback]], [[Track Playback]],
  [[Styles System]], [[Keyboard & Main UI]], [[Supabase Backend]].
- Updated [[modules/_index]] and [[index]] with the real subsystem map + dependency sketch.
- Source of truth: header-level read (interfaces + doc comments), not full `.cpp` bodies.

## 2026-06-06 — Vault scaffolded
- Created Mode B (Repository) wiki structure for the Piano-app JUCE synth.
- Folders: modules, components, decisions, dependencies, flows, concepts, meta.
- Seeded index, overview, hot cache, templates, CSS theming.
- Vault is local-only (gitignored from the Piano-app repo, no remote).
- Branch at scaffold time: `arranger-engine-phase1`.
