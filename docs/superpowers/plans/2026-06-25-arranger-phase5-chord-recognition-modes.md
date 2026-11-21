# Arranger Phase 5 — Chord Recognition Modes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Fingered / Single-Finger (Korg "One Finger") / Full-Keyboard chord-recognition modes, selectable in Settings, with Full-Keyboard re-tuned for bass-driven stability.

**Architecture:** A single pure `ChordDetector` gains a `ChordMode` enum and branches recognition in `recompute()`. Recognition stays on the MIDI-input thread; the recognised `ArrangerChord` flows to the engine through the unchanged Phase-4 mailbox. Detector changes (Tasks 1–3) are pure and TDD'd; UI/host plumbing (Task 4) exposes the modes; Task 5 verifies end-to-end.

**Tech Stack:** C++17, JUCE (Projucer/VS2022), JUCE `UnitTest` (tests compile into the app, run via `--run-tests`).

## Global Constraints

- **No commits without user approval** — the user's standing rule. Every "Commit" step here means **`git add` only** (stage); do NOT run `git commit`. Leave the work staged/uncommitted for review.
- **Latency is sacred** — recognition runs only on the MIDI-input/PC-key thread (`MidiHandler::feedChordNote`), never the audio/timer thread. No allocations in the hot path, no locks beyond the existing atomics, and **no time-based debounce**.
- **Purity** — `ChordDetector` stays free of JUCE GUI/threads so it is unit-testable.
- **No regression** — every existing `ChordDetector` test and the full unit suite (currently **562 passes, 0 failures**) must stay green after each task.
- **Korg is the reference** — Single-Finger mapping: root=Major; root+white-left=Dom7; root+black-left=Min; root+white+black-left=Min7.

### How to build & run tests (used by every task)

Build (Debug x64), expect `EXIT=0`, no `: error`:
```bash
MSB="/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
"$MSB" "Builds/VisualStudio2022/Project Synth2.sln" //p:Configuration=Debug //p:Platform=x64 //m //nologo //v:minimal
```
Run unit suite (writes `test-results.txt` to the working dir); confirm `TOTAL | ... | failures: 0` and inspect `ChordDetector` lines:
```bash
"Builds/VisualStudio2022/x64/Debug/App/Project Synth2.exe" --run-tests --unit-tests
grep -E "ChordDetector|TOTAL" test-results.txt
```
> The app must be closed before building (it locks `Project Synth2.pdb`).

---

### Task 1: `ChordMode` enum + `ChordDetector` mode API (no behaviour change)

Introduce the mode enum and swap the detector's `bool fullKeyboard` for a `ChordMode`. Behaviour is identical (Fingered & SingleFinger both use the existing `recognizeSet`; FullKeyboard keeps today's logic). Keeps the whole project green; `MidiHandler` keeps its `ChordScanArea` public API as a thin adapter for now (removed in Task 4).

**Files:**
- Modify: `Source/Arranger/Chord.h` (add enum)
- Modify: `Source/Arranger/ChordDetector.h` / `.cpp` (mode API)
- Modify: `Source/Midi/MidiHandler.cpp:974-978` (`setChordScanArea` calls `setMode` instead of `setFullKeyboardMode`)
- Test: `tests/unit/test_chord_detector.cpp` (helper + full-kbd tests use `ChordMode`)

**Interfaces:**
- Produces: `enum class ChordMode { Fingered, SingleFinger, FullKeyboard };` in `Chord.h`.
- Produces: `ChordDetector::setMode(ChordMode)`, `ChordDetector::getMode() const`. Removes `setFullKeyboardMode(bool)` / `isFullKeyboardMode()`.

- [ ] **Step 1: Update the test helper + full-kbd tests to the new API (failing to compile = our red).**

In `tests/unit/test_chord_detector.cpp`, change the helper and the three full-keyboard `setFullKeyboardMode(true)` call sites:
```cpp
static ArrangerChord detect (std::initializer_list<int> notes, ChordMode mode = ChordMode::Fingered)
{
    ChordDetector d; d.setMode (mode);
    for (int n : notes) d.noteOn (n);
    return d.current();
}
```
Replace each `ChordDetector d; d.setFullKeyboardMode (true);` with `ChordDetector d; d.setMode (ChordMode::FullKeyboard);` (three occurrences: the three `full-keyboard:` beginTest blocks).

- [ ] **Step 2: Build — expect FAIL (no `ChordMode`, no `setMode`).**

Run the build command. Expected: compile errors `'ChordMode': undeclared` / `'setMode': is not a member`.

- [ ] **Step 3: Add the enum to `Chord.h`.**

Add near the `ChordQuality` enum:
```cpp
// How the keyboard is scanned/interpreted for chord recognition.
enum class ChordMode { Fingered, SingleFinger, FullKeyboard };
```

- [ ] **Step 4: Swap the detector API in `ChordDetector.h`.**

Replace:
```cpp
    void  setFullKeyboardMode (bool shouldUseFullKeyboard) { fullKeyboard = shouldUseFullKeyboard; }
    bool  isFullKeyboardMode() const { return fullKeyboard; }
```
with:
```cpp
    void  setMode (ChordMode m) { mode = m; recompute(); }
    ChordMode getMode() const { return mode; }
```
And replace the member:
```cpp
    bool          fullKeyboard = false;
```
with:
```cpp
    ChordMode     mode = ChordMode::Fingered;
```

- [ ] **Step 5: Update `recompute()` in `ChordDetector.cpp` to branch on `mode`.**

Replace the `if (! fullKeyboard) { ... }` guard at the top of `recompute()`:
```cpp
void ChordDetector::recompute()
{
    if (mode != ChordMode::FullKeyboard)
    {
        // Fingered & Single-Finger both read the split-zone notes directly. Single-Finger gets its
        // own interpretation in a later task; for now it matches Fingered.
        recognized = recognizeSet (heldNotes);
        return;
    }

    // Full-Keyboard: only update on >=3 notes forming a known chord; otherwise hold the last
    // recognized chord so melody notes don't lurch the accompaniment.
    if ((int) heldNotes.size() < 3)
        return;   // keep the previously recognized chord (hysteresis / hold)
    // ... (existing full-keyboard body unchanged below)
```
(Leave the rest of the existing full-keyboard body as-is.)

- [ ] **Step 6: Update the one internal caller in `MidiHandler.cpp` (`setChordScanArea`).**

Change the body of `setChordScanArea` (around line 974):
```cpp
void MidiHandler::setChordScanArea(ChordScanArea area)
{
	chordScanArea.store(area);
	chordDetector.setMode(area == ChordScanArea::Full ? ChordMode::FullKeyboard : ChordMode::Fingered);
}
```

- [ ] **Step 7: Build — expect PASS (EXIT=0, no errors).**

- [ ] **Step 8: Run the unit suite — expect all green, ChordDetector unchanged.**

Run the test command. Expected: `TOTAL | ... | failures: 0`; `ChordDetector` lines all `failures: 0`.

- [ ] **Step 9: Stage (commit deferred).**

```bash
git add Source/Arranger/Chord.h Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp Source/Midi/MidiHandler.cpp tests/unit/test_chord_detector.cpp
```

---

### Task 2: Single-Finger recognition (Korg "One Finger")

**Files:**
- Modify: `Source/Arranger/ChordDetector.h` / `.cpp` (add `recognizeSingleFinger`, branch on it)
- Test: `tests/unit/test_chord_detector.cpp`

**Interfaces:**
- Consumes: `ChordMode`, `recognizeSet` (Task 1).
- Produces: `static ArrangerChord ChordDetector::recognizeSingleFinger (const std::set<int>& notes);`

- [ ] **Step 1: Write the failing tests.**

Add a new `beginTest` block in `runTest()`:
```cpp
beginTest ("single-finger: Korg one-finger mapping");
{
    using M = ChordMode;
    expect (detect({60},      M::SingleFinger).quality == ChordQuality::Maj);   // C alone
    expect (detect({60,59},   M::SingleFinger).quality == ChordQuality::Dom7);  // C + B  (white left)
    expect (detect({60,58},   M::SingleFinger).quality == ChordQuality::Min);   // C + Bb (black left)
    expect (detect({60,59,58},M::SingleFinger).quality == ChordQuality::Min7);  // C + B + Bb
    expectEquals (detect({60}, M::SingleFinger).root, 0);                        // root C
    // root is the HIGHEST held note; lower notes are modifiers:
    expect (detect({55,60},   M::SingleFinger).quality == ChordQuality::Dom7);  // root C(60), G(55) white-left
    expectEquals (detect({55,60}, M::SingleFinger).root, 0);
    // empty -> invalid
    {
        ChordDetector d; d.setMode (M::SingleFinger);
        expect (! d.current().isValid());
    }
}
```

- [ ] **Step 2: Build & run — expect this block to FAIL.**

Run build + test. Expected: `ChordDetector` shows failures in the single-finger block (it currently routes to `recognizeSet`, which won't classify `{60}` as Maj).

- [ ] **Step 3: Declare `recognizeSingleFinger` in `ChordDetector.h`.**

In the `private:` section next to `recognizeSet`:
```cpp
    static ArrangerChord recognizeSingleFinger (const std::set<int>& notes);   // Korg one-finger: 1-2 keys
```

- [ ] **Step 4: Implement it in `ChordDetector.cpp`.**

Add (near `recognizeSet`):
```cpp
ArrangerChord ChordDetector::recognizeSingleFinger (const std::set<int>& notes)
{
    if (notes.empty()) return {};

    const int root   = *notes.rbegin();          // highest held note is the root
    const int rootPc = ((root % 12) + 12) % 12;

    auto isBlack = [] (int n) {
        const int pc = ((n % 12) + 12) % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    };

    bool hasWhiteBelow = false, hasBlackBelow = false;
    for (int n : notes)
        if (n < root) { (isBlack (n) ? hasBlackBelow : hasWhiteBelow) = true; }

    ChordQuality q = (hasWhiteBelow && hasBlackBelow) ? ChordQuality::Min7
                   : hasBlackBelow                    ? ChordQuality::Min
                   : hasWhiteBelow                    ? ChordQuality::Dom7
                   :                                    ChordQuality::Maj;

    return { rootPc, q, rootPc };   // bassNote = root (no slash in single-finger)
}
```

- [ ] **Step 5: Branch to it in `recompute()`.**

Change the non-full branch in `recompute()`:
```cpp
    if (mode != ChordMode::FullKeyboard)
    {
        recognized = (mode == ChordMode::SingleFinger)
                       ? recognizeSingleFinger (heldNotes)
                       : recognizeSet (heldNotes);
        return;
    }
```

- [ ] **Step 6: Build & run — expect PASS (single-finger block green, all others unchanged).**

- [ ] **Step 7: Stage (commit deferred).**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp tests/unit/test_chord_detector.cpp
```

---

### Task 3: Full-Keyboard tuning (bass-driven, anti-lurch, instant)

Replace the full-keyboard recognition order so the **lowest 3 notes win first** — a melody on top can never change the chord — with no debounce.

**Files:**
- Modify: `Source/Arranger/ChordDetector.cpp` (full-keyboard body of `recompute()`)
- Test: `tests/unit/test_chord_detector.cpp`

**Interfaces:**
- Consumes: `recognizeSet`, `heldNotes` (`std::set<int>`, ascending).

- [ ] **Step 1: Write the failing anti-lurch test + responsiveness test.**

Add a `beginTest` block:
```cpp
beginTest ("full-keyboard: bass triad wins, melody on top never lurches");
{
    ChordDetector d; d.setMode (ChordMode::FullKeyboard);
    for (int n : {48,52,55}) d.noteOn (n);            // C major in the bass hand
    expect (d.current().quality == ChordQuality::Maj);
    d.noteOn (70);                                    // Bb melody on top: full set would read C7
    expect (d.current().quality == ChordQuality::Maj, "lurched to a 7th on a melody note");
    expectEquals (d.current().root, 0);

    // Moving the bass hand DOES change the chord (responsive, no debounce):
    for (int n : {48,52,55}) d.noteOff (n);
    for (int n : {41,45,48}) d.noteOn (n);            // F major in the bass
    expectEquals (d.current().root, 5);               // F
    expect (d.current().quality == ChordQuality::Maj);
}
```

- [ ] **Step 2: Build & run — expect FAIL.**

Expected: the `Bb melody on top` assertion fails (current code tries the full set first → C7).

- [ ] **Step 3: Replace the full-keyboard body of `recompute()`.**

Replace everything after the `if ((int) heldNotes.size() < 3) return;` line (the `recognizeSet(heldNotes)` + `for (int take : {4,3})` block) with:
```cpp
    // Favour the bass hand: try the lowest 3 (the core triad), then the lowest 4 (a bass 7th), then
    // the full set as a last resort. The first valid match wins, so a melody played on top is ignored.
    // Pure and instant (no debounce); stability comes from order + hysteresis, not from delay.
    auto lowest = [this] (int take) {
        std::set<int> s;
        for (int n : heldNotes) { s.insert (n); if ((int) s.size() == take) break; }
        return s;
    };

    for (int take : { 3, 4 })
    {
        ArrangerChord c = recognizeSet (lowest (take));
        if (c.isValid()) { recognized = c; return; }
    }
    ArrangerChord cFull = recognizeSet (heldNotes);
    if (cFull.isValid())
        recognized = cFull;
    // else: hold the previously recognized chord (hysteresis)
```

- [ ] **Step 4: Build & run — expect PASS (new block green; existing full-keyboard tests still green).**

Confirm the three original full-keyboard tests (`chord-tone melody note does not change`, `requires >=3`, `holds the chord when a non-chord melody note is added`) still pass.

- [ ] **Step 5: Stage (commit deferred).**

```bash
git add Source/Arranger/ChordDetector.cpp tests/unit/test_chord_detector.cpp
```

---

### Task 4: Expose the three modes in Settings + wire end-to-end

Swap the Full-Keyboard on/off toggle for a 3-way selector, remove `ChordScanArea`, and wire the new `ChordMode` from Settings → MainComponent → MidiHandler, with legacy-property migration.

**Files:**
- Modify: `Source/Midi/MidiHandler.h:15,468,505` and `MidiHandler.cpp:974-987` (replace `ChordScanArea`/`chordScanArea`/`setChordScanArea` with `ChordMode`/`chordMode`/`setChordMode`; `inChordZone` mode-aware)
- Modify: `Source/UI/SettingsWindow.h` (selector member + callback type) and `SettingsWindow.cpp:341,493-500` (selector UI + property + migration)
- Modify: `Source/UI/MainComponent.cpp` (wire `onChordModeChanged`; startup apply + migration)

**Interfaces:**
- Consumes: `ChordMode` (Task 1), `ChordDetector::setMode` (Task 1).
- Produces: `MidiHandler::setChordMode(ChordMode)`; `std::function<void(ChordMode)> SettingsWindow::onChordModeChanged`.

- [ ] **Step 1: MidiHandler — replace the enum/field/method.**

In `MidiHandler.h`:
- Line 15: replace `enum class ChordScanArea { Split, Full };` — delete it (now in `Chord.h` as `ChordMode`; ensure `Chord.h` is included).
- Replace `void setChordScanArea(ChordScanArea area);` with `void setChordMode(ChordMode mode);`.
- Replace `std::atomic<ChordScanArea> chordScanArea { ChordScanArea::Split };` with `std::atomic<ChordMode> chordMode { ChordMode::Fingered };`.

In `MidiHandler.cpp`, replace `setChordScanArea` and `inChordZone`:
```cpp
void MidiHandler::setChordMode(ChordMode mode)
{
	chordMode.store(mode);
	chordDetector.setMode(mode);
}

bool MidiHandler::inChordZone(int note) const
{
	if (chordMode.load() == ChordMode::FullKeyboard)
		return true;
	// Split-zone modes (Fingered, Single-Finger): the left-hand zone (below the split). With no
	// split set, the whole keyboard is the left zone (matches channelForHand semantics).
	return (rightHandBoundSetting == -1) ? true : note < rightHandBoundSetting;
}
```

- [ ] **Step 2: SettingsWindow.h — selector member + callback.**

Replace the `chordFullKeyboardToggle` member declaration with a 3-way selector and update the callback type. Add three radio-style `juce::TextButton`s (or reuse the project's segmented pattern if one exists) plus a header label:
```cpp
    juce::Label        chordModeLabel;
    juce::TextButton   chordModeFingered  { "Fingered" };
    juce::TextButton   chordModeSingle    { "Single-Finger" };
    juce::TextButton   chordModeFull      { "Full-Keyboard" };
    std::function<void(ChordMode)> onChordModeChanged;
```
Remove the old `chordFullKeyboardToggle` member and `std::function<void(bool)> onChordFullKeyboardChanged;`.

- [ ] **Step 3: SettingsWindow.cpp — build the selector, persist, migrate.**

Replace the `chordFullKeyboardToggle` setup block (around lines 493–500) with a radio group. Use `setRadioGroupId`, `setClickingTogglesState(true)`, and the existing panel colours:
```cpp
    auto readStoredMode = [this]() -> ChordMode
    {
        if (propertyFile == nullptr) return ChordMode::Fingered;
        if (propertyFile->containsKey("ChordMode"))
            return (ChordMode) juce::jlimit(0, 2, propertyFile->getIntValue("ChordMode", 0));
        // migrate legacy bool: Full-Keyboard on -> FullKeyboard, else Fingered
        return propertyFile->getBoolValue("ChordFullKeyboard", false) ? ChordMode::FullKeyboard
                                                                       : ChordMode::Fingered;
    };

    chordModeLabel.setText("Chord Mode", juce::dontSendNotification);
    settingsPanel.addAndMakeVisible(chordModeLabel);

    const int kChordModeRadio = 0x5C40DE;   // any unique non-zero group id
    for (auto* b : { &chordModeFingered, &chordModeSingle, &chordModeFull })
    {
        b->setClickingTogglesState(true);
        b->setRadioGroupId(kChordModeRadio);
        b->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        settingsPanel.addAndMakeVisible(*b);
    }

    const ChordMode stored = readStoredMode();
    (stored == ChordMode::FullKeyboard   ? chordModeFull
     : stored == ChordMode::SingleFinger ? chordModeSingle
     :                                     chordModeFingered).setToggleState(true, juce::dontSendNotification);

    auto pick = [this](ChordMode m)
    {
        if (propertyFile != nullptr) { propertyFile->setValue("ChordMode", (int) m); propertyFile->saveIfNeeded(); }
        if (onChordModeChanged) onChordModeChanged(m);
    };
    chordModeFingered.onClick = [pick]{ pick(ChordMode::Fingered); };
    chordModeSingle.onClick   = [pick]{ pick(ChordMode::SingleFinger); };
    chordModeFull.onClick     = [pick]{ pick(ChordMode::FullKeyboard); };
```

- [ ] **Step 4: SettingsWindow.cpp — lay the selector out in `resized()`.**

Replace the `chordFullKeyboardToggle.setBounds(...)` line (~341) with a label + three buttons across the row (reuse the row geometry the toggle used — `pad`, `y`, `rowH`):
```cpp
    chordModeLabel.setBounds(pad, y, 110, rowH);
    {
        const int bx = pad + 115, bw = 130;
        chordModeFingered.setBounds(bx,            y, bw, rowH);
        chordModeSingle.setBounds  (bx + bw + 6,   y, bw, rowH);
        chordModeFull.setBounds    (bx + 2*(bw+6), y, bw, rowH);
    }
```
(Adjust `y` advancement for following rows exactly as the removed toggle did, so Chord Memory / Bass Inversion rows keep their spacing.)

- [ ] **Step 5: MainComponent.cpp — wire callback + apply at startup with migration.**

Find where `onChordFullKeyboardChanged` was wired (the Settings construction / `applyStoredChordSettings` area) and replace it:
```cpp
    settings.onChordModeChanged = [this](ChordMode m) { midiHandler.setChordMode(m); };
```
At startup, where stored chord settings are re-applied (same place the old bool was read), apply the migrated mode:
```cpp
    {
        ChordMode m = ChordMode::Fingered;
        if (propertiesFile != nullptr)
        {
            if (propertiesFile->containsKey("ChordMode"))
                m = (ChordMode) juce::jlimit(0, 2, propertiesFile->getIntValue("ChordMode", 0));
            else if (propertiesFile->getBoolValue("ChordFullKeyboard", false))
                m = ChordMode::FullKeyboard;
        }
        midiHandler.setChordMode(m);
    }
```
> Find the exact property-file symbol used in `MainComponent` (it created `SettingsWindow` with a `propertiesFile`); reuse that same pointer/name. Remove any remaining reference to `onChordFullKeyboardChanged` / `setChordScanArea`.

- [ ] **Step 6: Build — resolve every remaining `ChordScanArea` / `onChordFullKeyboardChanged` reference until EXIT=0.**

Run the build. If any other file still references `ChordScanArea` or `onChordFullKeyboardChanged`, update it to the new symbols. Expected end state: EXIT=0, no errors.

- [ ] **Step 7: Run the unit suite — expect green (562+ passes, 0 failures).**

- [ ] **Step 8: Manual smoke (the app, arranger mode).**

Launch the app, open Settings: confirm a 3-way **Chord Mode** selector with the stored mode pre-selected. Switch to **Single-Finger**, start the arranger, press one left-zone key → major accompaniment; add a black key to its left → minor. Switch to **Full-Keyboard**, hold a low triad + play melody on top → backing doesn't lurch. Switch back to **Fingered** → unchanged behaviour.

- [ ] **Step 9: Stage (commit deferred).**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp Source/UI/SettingsWindow.h Source/UI/SettingsWindow.cpp Source/UI/MainComponent.cpp
```

---

### Task 5: Full verification & regression sweep

**Files:** none (verification only).

- [ ] **Step 1: Clean-ish rebuild + full unit suite.**

Build, then `--run-tests --unit-tests`. Expected: EXIT=0; `TOTAL | ... | failures: 0`.

- [ ] **Step 2: Confirm ChordDetector coverage.**

`grep "ChordDetector" test-results.txt` → every line `failures: 0`, and the Single-Finger + Full-Keyboard blocks are present.

- [ ] **Step 3: Regression smoke for earlier uncommitted work.**

In the app: verify the three earlier fixes still hold — (a) play notes → switch style → no frozen notes; (b) hold a chord → Start → stays audible; (c) held-note shows the amber sustain glow; (d) hover the Auto-Fill toggle → tooltip shows the full "Enable…" text. Confirm the held-chord seed still works with the new modes (hold a chord, Start → backing follows it).

- [ ] **Step 4: Stage anything outstanding (commit deferred — user review).**

```bash
git add -A
git status
```

---

## Self-Review

**Spec coverage:**
- Single-Finger recognition + Korg mapping → Task 2. ✓
- Full-Keyboard stability tuning (lowest-first, no debounce) → Task 3. ✓
- `ChordMode` enum + detector branch (D1, D2) → Task 1. ✓
- 3-way Settings selector + persistence + legacy migration (D5) → Task 4 (Steps 2–5). ✓
- MidiHandler `inChordZone`/mode swap (5.3) → Task 4 (Step 1). ✓
- Host wiring + startup apply (5.5) → Task 4 (Step 5). ✓
- Testing plan (§6) → Tasks 1–3 + Task 5. ✓
- Latency/purity constraints (§2) → honoured (pure detector, no debounce, no audio-thread work). ✓
- Out-of-scope items (§8) → not implemented. ✓

**Placeholder scan:** No TBD/TODO; each code step shows complete code. Task 5 Step 3 lists concrete checks. The only "find the exact symbol" notes (Task 4 Steps 5–6) are unavoidable host-integration lookups, each with the exact replacement code given.

**Type consistency:** `ChordMode` (3 values) used identically in Chord.h, ChordDetector (`setMode`/`getMode`/`mode`), MidiHandler (`setChordMode`/`chordMode`), SettingsWindow (`onChordModeChanged`), MainComponent. `recognizeSingleFinger(const std::set<int>&)` declared (Task 2 Step 3) and defined (Step 4) with matching signature. Persistence key `"ChordMode"` (int 0/1/2) consistent across Settings + MainComponent; legacy `"ChordFullKeyboard"` only read for migration.
