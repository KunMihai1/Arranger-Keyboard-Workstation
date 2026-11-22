> Part 2 of 2 of [2026-06-26-arranger-phase7a-ntt-engine](2026-06-26-arranger-phase7a-ntt-engine.md). Previous: [part 1](2026-06-26-arranger-phase7a-ntt-engine.md).

## Task 4: Wrap-around register guard

**Files:**
- Modify: `Source/Arranger/ChordTransposer.cpp` (`wrapToOctave`)
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `wrapToOctave(int origin, int n)` declared in Task 3.
- Produces: same signature; now folds results that land more than one octave from `origin` back toward it, then MIDI-clamps.

- [ ] **Step 1: Write the failing test** — append to `tests/unit/test_chord_transposer.cpp`:

```cpp
        beginTest ("wrap-around folds a >1-octave result back toward the origin");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Dim, 0 });   // C dim {0,3,6}
            t.setActiveChord   ({ 11, ChordQuality::Aug, 11 }); // B aug {0,4,8}, interval +11
            // F# is the dim 5th (deg 6, idx 2); active aug 5th is deg 8 -> +2. result = 60+11+2 = 73,
            // which is 13 semitones above the origin 60 -> fold down an octave to 61 (same pitch class).
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord), 61);
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord) % 12, 1);
        }

        beginTest ("result still clamps to 0..127 for Chord");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            for (int n = 0; n <= 127; ++n)
            {
                const int x = t.transpose (n, PartKind::Acc, NttType::Chord);
                expect (x >= 0 && x <= 127);
            }
        }
```

- [ ] **Step 2: Run to verify failure**

Run: build + `--arranger-tests` → expect the fold test to FAIL (current stub returns 73, not 61).

- [ ] **Step 3: Implement the guard** — replace the `wrapToOctave` body in `ChordTransposer.cpp`:

```cpp
int ChordTransposer::wrapToOctave (int origin, int n) const
{
    while (n - origin > 12) n -= 12;   // more than an octave above -> fold down
    while (origin - n > 12) n += 12;   // more than an octave below -> fold up
    return clampMidi (n);
}
```

- [ ] **Step 4: Run the tests — expect PASS**

Run: `--arranger-tests`
Expected: PASS, including the new fold test and the full-range clamp; all earlier blocks remain green (Parallel intervals are ≤ 11 semitones, so they never fold and stay truly parallel).

- [ ] **Step 5: Commit** (ask first)

```bash
git add Source/Arranger/ChordTransposer.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): NTT register guard — fold >1-octave results back toward origin"
```

---

## Task 5: Master "Chord Transposition" bypass — engine forwarder + Settings toggle

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h/.cpp` (`setNttEnabled`)
- Modify: `Source/Styles/CurrentStyleComponent.h/.cpp` + `Source/UI/Display.h/.cpp` (pass-through + re-apply on Start — mirror `setArrangerBassInversion`)
- Modify: `Source/UI/SettingsWindow.h/.cpp` (toggle, property `"NttEnabled"`, default true)
- Modify: `Source/UI/MainComponent.cpp` (callback wiring + startup restore)
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `ChordTransposer::setNttEnabled(bool)` (Task 3).
- Produces: `ArrangerEngine::setNttEnabled(bool)`; `CurrentStyleComponent::setArrangerNttEnabled(bool)` + remembered `arrangerNttEnabled`; `Display::setArrangerNttEnabled(bool)`; `SettingsWindow::onNttEnabledChanged` (`std::function<void(bool)>`) + `nttEnabledToggle`.

- [ ] **Step 1: Write the failing transposer test** — append to `tests/unit/test_chord_transposer.cpp`:

```cpp
        beginTest ("master bypass: disabled NTT returns input notes for all parts/types");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Min, 5 });
            t.setNttEnabled (false);
            expectEquals (t.transpose (60, PartKind::Acc,  NttType::Chord),    60);
            expectEquals (t.transpose (64, PartKind::Acc,  NttType::Parallel), 64);
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 43);
            t.setNttEnabled (true);
            expect (t.transpose (60, PartKind::Acc, NttType::Chord) != 60);   // active again
        }
```

(This already passes if Task 3's `if (! nttEnabled) ntt = NoTranspose;` is in place — it locks the contract before the UI wiring. Run it to confirm green, then proceed to wire the UI, which has no unit test and is verified by build + manual smoke.)

- [ ] **Step 2: Run it — expect PASS** (`--arranger-tests`). If it fails, fix Task 3's bypass line.

- [ ] **Step 3: Engine forwarder** — `ArrangerEngine.h` next to `void setBassInversion (bool);` add:
```cpp
    void setNttEnabled (bool enabled);
```
`ArrangerEngine.cpp` next to `setBassInversion`:
```cpp
void ArrangerEngine::setNttEnabled (bool enabled)
{
    transposer.setNttEnabled (enabled);
}
```

- [ ] **Step 4: Pass-through + re-apply on Start** — mirror Bass Inversion exactly.

`CurrentStyleComponent.h`: add `void setArrangerNttEnabled (bool enabled);` and member `bool arrangerNttEnabled = true;`.
`CurrentStyleComponent.cpp`: add (modeled on `setArrangerBassInversion`, ~line 376):
```cpp
void CurrentStyleComponent::setArrangerNttEnabled (bool enabled)
{
    arrangerNttEnabled = enabled;
    if (arrangerEngine != nullptr)
        arrangerEngine->setNttEnabled (enabled);
}
```
In `startPlaying()` where `arrangerEngine->setBassInversion (arrangerBassInversion);` is re-applied (~line 36), add directly after:
```cpp
        arrangerEngine->setNttEnabled (arrangerNttEnabled);   // re-apply; setStyle/rebuild may reset
```
`Display.h`: add `void setArrangerNttEnabled (bool enabled);` and `bool arrangerNttEnabled = true;`.
`Display.cpp`: mirror `setArrangerBassInversion` (~line 456):
```cpp
void Display::setArrangerNttEnabled (bool enabled)
{
    arrangerNttEnabled = enabled;
    if (currentStyleComponent != nullptr)
        currentStyleComponent->setArrangerNttEnabled (enabled);
}
```
And where `currentStyleComponent->setArrangerBassInversion(arrangerBassInversion);` is applied on (re)creation (~line 246), add:
```cpp
        currentStyleComponent->setArrangerNttEnabled (arrangerNttEnabled);
```

- [ ] **Step 5: Settings toggle** — `SettingsWindow.h`: next to `onChordBassInversionChanged` add:
```cpp
    std::function<void(bool)> onNttEnabledChanged;       // Phase 7a: master Chord Transposition on/off
```
next to `chordBassInversionToggle` add:
```cpp
    juce::ToggleButton nttEnabledToggle { "Chord transposition (NTT)" };   /**< Phase 7a master bypass */
```
`SettingsWindow.cpp`: in the layout method, after `chordBassInversionToggle.setBounds(...)` (~line 339):
```cpp
    y += rowH + 6;
    nttEnabledToggle.setBounds (pad, y, 320, rowH);
```
In the setup method, after the `chordBassInversionToggle` block (~line 495):
```cpp
    settingsPanel.addAndMakeVisible (nttEnabledToggle);
    nttEnabledToggle.setToggleState (propertyFile != nullptr && propertyFile->getBoolValue ("NttEnabled", true),
                                     juce::dontSendNotification);
    nttEnabledToggle.onClick = [this]()
    {
        const bool on = nttEnabledToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue ("NttEnabled", on); propertyFile->saveIfNeeded(); }
        if (onNttEnabledChanged) onNttEnabledChanged (on);
    };
```
If the panel has a fixed `setSize(...)` height, bump it by `rowH + 6` so the new row fits (match how the chord-zone-mute row bumped it earlier).

- [ ] **Step 6: MainComponent wiring** — in the block that sets `midiWindow->onChordBassInversionChanged` (~line 999):
```cpp
            midiWindow->onNttEnabledChanged = [this](bool on)
            {
                if (display != nullptr) display->setArrangerNttEnabled (on);
            };
```
In the startup-restore block near `display->setArrangerBassInversion(propertiesFile->getBoolValue("ChordBassInversion", false));` (~line 2108):
```cpp
            display->setArrangerNttEnabled (propertiesFile->getBoolValue ("NttEnabled", true));
```

- [ ] **Step 7: Build + manual smoke**

Build clean; launch; toggle "Chord transposition (NTT)" off → accompaniment stays in the recorded home key while sections/fills/synchro and chord recognition still run; toggle on → it follows held chords again. Confirm the setting persists across a restart.

- [ ] **Step 8: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp \
        Source/Styles/CurrentStyleComponent.h Source/Styles/CurrentStyleComponent.cpp \
        Source/UI/Display.h Source/UI/Display.cpp Source/UI/SettingsWindow.h Source/UI/SettingsWindow.cpp \
        Source/UI/MainComponent.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): master Chord-Transposition on/off bypass (Settings + engine)"
```

---

## Task 6: Minor-scale selector — engine forwarder + Settings 3-way

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h/.cpp` (`setMinorScale`)
- Modify: `Source/Styles/CurrentStyleComponent.h/.cpp` + `Source/UI/Display.h/.cpp` (pass-through + re-apply on Start)
- Modify: `Source/UI/SettingsWindow.h/.cpp` (3-way selector, property `"NttMinorScale"` int 0/1/2)
- Modify: `Source/UI/MainComponent.cpp` (callback + startup restore)

**Interfaces:**
- Consumes: `ChordTransposer::setMinorScale(MinorScaleChoice)` (Task 3).
- Produces: `ArrangerEngine::setMinorScale(MinorScaleChoice)`; `CurrentStyleComponent::setArrangerMinorScale(MinorScaleChoice)` + remembered member; `Display::setArrangerMinorScale(MinorScaleChoice)`; `SettingsWindow::onNttMinorScaleChanged` (`std::function<void(int)>`, 0=Dorian/1=Aeolian/2=HarmonicMinor).

This task has no new pure logic to unit-test (Task 3's "minor-scale selector changes the snap target" already covers the engine behavior). It is a wiring task verified by build + smoke + the existing minor-scale transposer test.

- [ ] **Step 1: Engine forwarder** — `ArrangerEngine.h`:
```cpp
    void setMinorScale (MinorScaleChoice choice);
```
`ArrangerEngine.cpp`:
```cpp
void ArrangerEngine::setMinorScale (MinorScaleChoice choice)
{
    transposer.setMinorScale (choice);
}
```

- [ ] **Step 2: Pass-through + re-apply on Start** — mirror Task 5.

`CurrentStyleComponent.h`: `void setArrangerMinorScale (MinorScaleChoice c);` + `MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian;`.
`CurrentStyleComponent.cpp`:
```cpp
void CurrentStyleComponent::setArrangerMinorScale (MinorScaleChoice c)
{
    arrangerMinorScale = c;
    if (arrangerEngine != nullptr)
        arrangerEngine->setMinorScale (c);
}
```
In `startPlaying()` after the `setNttEnabled` re-apply line:
```cpp
        arrangerEngine->setMinorScale (arrangerMinorScale);   // re-apply on Start
```
`Display.h`: `void setArrangerMinorScale (MinorScaleChoice c);` + `MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian;`.
`Display.cpp`:
```cpp
void Display::setArrangerMinorScale (MinorScaleChoice c)
{
    arrangerMinorScale = c;
    if (currentStyleComponent != nullptr)
        currentStyleComponent->setArrangerMinorScale (c);
}
```
And in the currentStyleComponent (re)creation block:
```cpp
        currentStyleComponent->setArrangerMinorScale (arrangerMinorScale);
```

- [ ] **Step 3: Settings 3-way selector** — model on the existing 3-way chord-mode button group in `SettingsWindow.cpp` (the Phase-5 selector with `accent1`-on / dark-off colours added this session). Add three `juce::TextButton` (or reuse the existing radio-group helper) labelled **Dorian / Aeolian / Harmonic**, mutually exclusive, default Dorian.

`SettingsWindow.h`:
```cpp
    std::function<void(int)> onNttMinorScaleChanged;     // 0=Dorian, 1=Aeolian, 2=HarmonicMinor
    juce::TextButton nttMinorDorianBtn   { "Dorian" };
    juce::TextButton nttMinorAeolianBtn  { "Aeolian" };
    juce::TextButton nttMinorHarmonicBtn { "Harmonic" };
```
`SettingsWindow.cpp` setup (mirror the chord-mode group's colours/radio behaviour exactly — same `setColour` calls so it matches the app theme):
```cpp
    auto initMinorBtn = [this](juce::TextButton& b, int idx)
    {
        settingsPanel.addAndMakeVisible (b);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (7701);
        b.setColour (juce::TextButton::buttonColourId,   cardBg);
        b.setColour (juce::TextButton::buttonOnColourId, accent1);
        b.setColour (juce::TextButton::textColourOffId,  subtitleText);
        b.setColour (juce::TextButton::textColourOnId,   background);
        b.onClick = [this, idx]()
        {
            if (propertyFile != nullptr) { propertyFile->setValue ("NttMinorScale", idx); propertyFile->saveIfNeeded(); }
            if (onNttMinorScaleChanged) onNttMinorScaleChanged (idx);
        };
    };
    initMinorBtn (nttMinorDorianBtn,   0);
    initMinorBtn (nttMinorAeolianBtn,  1);
    initMinorBtn (nttMinorHarmonicBtn, 2);
    const int savedMinor = propertyFile != nullptr ? propertyFile->getIntValue ("NttMinorScale", 0) : 0;
    (savedMinor == 1 ? nttMinorAeolianBtn : savedMinor == 2 ? nttMinorHarmonicBtn : nttMinorDorianBtn)
        .setToggleState (true, juce::dontSendNotification);
```
Layout: lay the three side by side on a new row below the `nttEnabledToggle` (match the chord-mode row's geometry). Bump the panel height again if it's fixed.

- [ ] **Step 4: MainComponent wiring** — near the other arranger settings callbacks:
```cpp
            midiWindow->onNttMinorScaleChanged = [this](int idx)
            {
                const auto c = idx == 1 ? MinorScaleChoice::Aeolian
                             : idx == 2 ? MinorScaleChoice::HarmonicMinor
                                        : MinorScaleChoice::Dorian;
                if (display != nullptr) display->setArrangerMinorScale (c);
            };
```
Startup restore (near the `NttEnabled` restore):
```cpp
            {
                const int m = propertiesFile->getIntValue ("NttMinorScale", 0);
                const auto c = m == 1 ? MinorScaleChoice::Aeolian
                             : m == 2 ? MinorScaleChoice::HarmonicMinor
                                      : MinorScaleChoice::Dorian;
                if (display != nullptr) display->setArrangerMinorScale (c);
            }
```

- [ ] **Step 5: Build + smoke** — build clean; play a minor chord with a style that has audible passing tones; switch Dorian ↔ Aeolian ↔ Harmonic and hear the passing-tone colour change; confirm persistence across restart.

- [ ] **Step 6: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp \
        Source/Styles/CurrentStyleComponent.h Source/Styles/CurrentStyleComponent.cpp \
        Source/UI/Display.h Source/UI/Display.cpp Source/UI/SettingsWindow.h Source/UI/SettingsWindow.cpp \
        Source/UI/MainComponent.cpp
git commit -m "feat(arranger): curated minor-scale selector for NTT passing-tone snap"
```

---

## Task 7: Per-track NTT-type selector in the style editor

**Files:**
- Modify: `Source/Arranger/ArrangerStyleEditor.h/.cpp` (track rows)
- Test: manual (editor UI); persistence already covered by Task 1's IO round-trip.

**Interfaces:**
- Consumes: `SourceTrackFile.nttType` (Task 1), `ArrangerEnums::toString/nttFromString` (Task 1).
- Produces: a per-track 4-way control writing `sourceTracks[i].nttType`; the value flows out through the existing `toStyleFile()` → save path and into the live preview through `buildStyle()` (Task 8 consumes it at emit).

- [ ] **Step 1: Inspect the existing track-row rendering** — in `ArrangerStyleEditor.cpp`, find where each `sourceTracks[i]` row is drawn/laid out (the loop near `for (const auto& st : sourceTracks)` ~line 115, and wherever per-track controls like name/part are shown). Identify whether rows use child Components or are custom-painted; follow that pattern.

- [ ] **Step 2: Add a 4-way selector per track** — a `juce::ComboBox` per row (items: `No Transpose`, `Parallel`, `Chord`, `Fixed`; ids 1..4 mapping to `NttType` 0..3), themed to match the editor. Initialize each from `sourceTracks[i].nttType`. On change:
```cpp
    // idx0 = NttType order: NoTranspose, Parallel, Chord, Fixed
    sourceTracks[trackIndex].nttType = static_cast<NttType> (comboSelectedId - 1);
    // keep the live preview in sync, exactly like a region/key edit does:
    onSourceTracksChanged();   // or the existing method that calls engine.setStyle(buildStyle())
```
Use the editor's existing "something changed → rebuild preview" entry point (the same one region moves use, which calls `engine.setStyle(buildStyle())`). Do **not** introduce a new rebuild path.

- [ ] **Step 3: Persist** — no new code: `toStyleFile()` already copies `sourceTracks` wholesale (`f.sourceTracks = sourceTracks;`), and Task 1 serializes `nttType`. Saving the style now writes per-track NTT; reloading restores it.

- [ ] **Step 4: Build + smoke** — open the editor, change a track's type (e.g. set the bass to Parallel, a pad to Fixed), save, reload → the selectors restore; play and confirm the audible behavior differs per track. Confirm drum tracks default to / show **No Transpose**.

- [ ] **Step 5: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerStyleEditor.h Source/Arranger/ArrangerStyleEditor.cpp
git commit -m "feat(arranger): per-track NTT-type selector in the style editor"
```

---

## Task 8: Engine wiring — thread per-track `NttType` to emit time

**Files:**
- Modify: `Source/Arranger/ArrangerScheduler.h/.cpp` (carry `NttType` parallel to `PartKind`)
- Modify: `Source/Arranger/ArrangerEngine.cpp` (`rebuildFromStyle` builds the `NttType` vector; dispatch passes it to `transpose`)
- Test: `tests/unit/test_arranger_engine.cpp` (or `test_arranger_scheduler.cpp`)

**Interfaces:**
- Consumes: `ArrangerTrack.nttType` (Task 1), `ChordTransposer::transpose(note, part, ntt)` (Task 3), the existing `EmittedEvent.part` / `sortedParts` / `activeNoteParts` threading.
- Produces: `EmittedEvent.ntt`; `ArrangerScheduler::setLoop(events, parts, ntts, loopLengthBeats)` (3-vector overload); `sortedNtt` + `activeNoteNtt` mirroring the part vectors so note-offs carry the same type as their note-ons.

- [ ] **Step 1: Write the failing test** — in `tests/unit/test_arranger_engine.cpp` (or the scheduler test), assert a non-default per-track type reaches `transpose`. Concretely, build a one-track style whose single Acc track is set to `NttType::Parallel`, set original C / active F, advance the engine over the note, and assert the emitted note is the **parallel** shift (note+5), not the Chord remap. Use the test harness's existing engine-construction helper in that file. Sketch:

```cpp
beginTest ("per-track NttType reaches emit-time transposition");
{
    ArrangerStyle style = /* one section, one Acc track, single note C4 (60) at beat 0 */;
    style.originalRoot = 0; style.originalQuality = ChordQuality::Maj;
    style.sections[0].tracks[0].nttType = NttType::Parallel;

    ArrangerEngine engine (/* null/loopback out per the file's pattern */);
    engine.setStyle (style);
    engine.setActiveChord ({ 5, ChordQuality::Maj, 5 });   // F
    // capture emitted notes via the file's existing onMidiMessage hook
    // advance one tick covering beat 0
    // EXPECT a note-on for 65 (Parallel: 60 + 5), NOT 65-from-Chord-coincidence:
    //   choose a note where Parallel and Chord differ, e.g. original note 62 (D passing):
    //     Parallel -> 67 ; Chord over F maj would snap 67 (G) -> in F scale, also 67 — pick instead
    //   original note 61 (C#, passing): Parallel -> 66 ; Chord -> snaps 66 to F-Ionian (65 or 67).
    // Use note 61 so Parallel(66) != Chord(65/67).
    expect (capturedContains (66));   // proves Parallel type was used
}
```
Pick the seed note so Parallel and Chord give different results (e.g. a chromatic passing tone like MIDI 61 over C→F: Parallel→66, Chord snaps to 65 or 67). Match the file's actual engine/dispatch capture helpers.

- [ ] **Step 2: Run to verify failure** — expect FAIL (engine currently ignores `nttType`; emits the Chord-type result).

- [ ] **Step 3: Carry `NttType` through the scheduler** — `ArrangerScheduler.h`:
  - In `struct EmittedEvent`, after `PartKind part = PartKind::Fixed;` add:
    ```cpp
    NttType ntt = NttType::Chord;   // Phase 7a: how to transpose this note
    ```
  - Add a 3-vector overload:
    ```cpp
    void setLoop (std::vector<TimedBeatEvent> events, std::vector<PartKind> parts,
                  std::vector<NttType> ntts, double loopLengthBeats);
    ```
  - Add parallel storage mirroring `sortedParts` / `activeNoteParts`:
    ```cpp
    std::vector<NttType>                   sortedNtt;            // aligned with sortedEvents
    std::map<std::pair<int,int>, NttType>  activeNoteNtt;        // ntt of each sounding note (seam offs)
    ```
  - `trackActiveNote` signature gains the ntt:
    ```cpp
    void trackActiveNote (const juce::MidiMessage& m, PartKind part, NttType ntt);
    ```

  `ArrangerScheduler.cpp`:
  - Keep the existing 2-vector `setLoop` delegating; have it pass an empty ntt vector to the 3-vector form:
    ```cpp
    void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events,
                                     std::vector<PartKind> parts, double loopLengthBeats)
    {
        setLoop (std::move (events), std::move (parts), {}, loopLengthBeats);
    }
    ```
  - In the 3-vector `setLoop`, fill `sortedNtt` exactly as `sortedParts` is filled, defaulting missing entries to `NttType::Chord`:
    ```cpp
        sortedNtt.push_back (i < ntts.size() ? ntts[i] : NttType::Chord);
    ```
    Clear `activeNoteNtt` everywhere `activeNoteParts` is cleared (reset paths + both `clear()` sites).
  - In `advance`, where it reads `const PartKind part = sortedParts[i];`, also read `const NttType ntt = sortedNtt[i];`, set `ev.ntt = ntt;` on the emitted note-on, and call `trackActiveNote (m, part, ntt);`.
  - In `trackActiveNote`, store `activeNoteNtt[key] = ntt;` on note-on and `erase(key)` on note-off (mirror the part map).
  - In `flushActiveNotes` and the loop-seam off path that looks up `activeNoteParts`, also look up `activeNoteNtt` (default `NttType::Chord`) and set `ev.ntt` on the synthesized note-off so the off transposes identically to its on.

- [ ] **Step 4: Build the `NttType` vector in `rebuildFromStyle`** — `ArrangerEngine.cpp:29-43`, alongside `parts`:
```cpp
        std::vector<TimedBeatEvent> merged;
        std::vector<PartKind>       parts;
        std::vector<NttType>        ntts;
        for (const auto& tr : sec.tracks)
        {
            const PartKind pk = (tr.partType == ArrangerPartType::Bass) ? PartKind::Bass
                              : (tr.partType == ArrangerPartType::Acc)  ? PartKind::Acc
                                                                        : PartKind::Fixed;
            for (const auto& ev : tr.pattern)
            {
                merged.push_back (ev);
                parts.push_back (pk);
                ntts.push_back (tr.nttType);
            }
        }
        ...
        s.setLoop (std::move (merged), std::move (parts), std::move (ntts), bars * bpb);
```

- [ ] **Step 5: Pass `ntt` into `transpose` at dispatch** — find `dispatchEmitted` (where it currently calls `transposer.transpose (note, ev.part)` / `(note, part)`), and change the call to:
```cpp
        const int outNote = transposer.transpose (note, ev.part, ev.ntt);
```
Apply at every transpose call site that handles emitted note-on/off (note-on and the seam/flush note-off must both use `ev.ntt`).

- [ ] **Step 6: Run the test — expect PASS**

Run: `--arranger-tests`
Expected: PASS — the Parallel-typed track emits the parallel shift; all other arranger-engine tests stay green (default `NttType::Chord` reproduces prior behavior for untyped paths).

- [ ] **Step 7: Full regression + smoke**

Run all suites: `--unit-tests`, `--arranger-tests`, `--integration-tests` → 0 failures. Then a manual smoke: load a real style, set per-track types in the editor, play held chords, toggle the master bypass and minor scale — confirm each track transposes per its type and drums never move.

- [ ] **Step 8: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp \
        Source/Arranger/ArrangerEngine.cpp tests/unit/test_arranger_engine.cpp
git commit -m "feat(arranger): thread per-track NttType through scheduler to emit-time transposition"
```

---

## Self-Review (against the spec)

**Spec coverage:**
- §4 four types → Task 3 (NoTranspose/Parallel/Chord/Fixed) + Task 4 (wrap). ✓
- §4.1 role remap → Task 3 (`act[idx]-orig[idx]`). ✓
- §4.2 passing-tone snap + downward tie → Task 3 `snapToScale`. ✓
- §5 data-driven scale table → Task 2 (`NttScales.h`). ✓
- §5.1 curated minor selector → Task 2 (table) + Task 6 (Settings). ✓
- §6 wrap-around → Task 4. ✓
- §7 master bypass → Task 5. ✓
- §8 enums + per-track field + schema bump + migration → Task 1. ✓
- §9 UI (per-track selector; minor + master in Settings) → Task 7 + Tasks 5/6. ✓
- §10 engine wiring + off-audio-path setters + purity → Task 8 + Tasks 5/6 forwarders. ✓
- §11 tests → Tasks 1–8 cover NoTranspose, Parallel, role, snap, minor-switch, Fixed, wrap, bypass, migration; Bass-Inversion regression preserved by keeping the existing inversion test green (Task 3 keeps the inversion branch ahead of type dispatch). ✓

**Type consistency:** `NttType` order `{NoTranspose, Parallel, Chord, Fixed}` is fixed in Task 1 and used as `static_cast<NttType>(id-1)` (Task 7) and combo ids 1..4; `MinorScaleChoice {Dorian, Aeolian, HarmonicMinor}` ↔ ints 0/1/2 consistently in Tasks 2/6. `setLoop` 3-vector signature defined in Task 8 header matches its `.cpp` and engine call. `transpose(note, part, ntt)` signature identical across Tasks 3/8.

**Placeholder scan:** none — every code step shows the code; the only deliberately deferred body is `wrapToOctave` (stubbed in Task 3, implemented in Task 4) with its own failing test gating it.

**Open confirmations for the implementer (resolve by reading the cited code, not by guessing):**
- Task 2: confirm how `Project Synth2.jucer` lists test files and add the new one the same way.
- Task 7: confirm the editor track-row construction pattern before choosing ComboBox vs. the editor's own control idiom; reuse the existing preview-rebuild entry point.
- Task 8: confirm the exact name of the dispatch method and the test file's engine-capture helper before writing the call/test.
