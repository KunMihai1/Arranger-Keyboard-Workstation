# UI redesign, phases 0–2 (foundation + Stage panel) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the 100 px header with the "Stage" panel of the accepted design (transport, section buttons, screen buttons, centred screen with beat lamps / messages / tempo, quick FX) while every existing behaviour stays reachable.

**Architecture:** Pure, unit-tested models (`StageScale`, `TempoModel`, `BeatLampModel`, `ScreenMessages`, `PageRouter`) under `Source/UI/Stage/`; a `StageLookAndFeel` + small widgets draw the materials; `StagePanel` assembles them and talks to `MainComponent` only through `std::function` callbacks wired to the handlers that exist today. Pages that are not redesigned yet open today's windows/components ("legacy page") so nothing is lost in between; phase 4 plans replace them one by one.

**Tech Stack:** JUCE 8.0.13, C++17, VS2022, `juce::UnitTest` (ci/Tests.jucer, category `Unit`).

**Spec:** `docs/superpowers/specs/2026-11-23-ui-redesign-design.md` (+ `-part2.md`). Boards: `docs/superpowers/specs/assets/2026-11-23-ui-redesign/StageRevised.dc.html` is the source of every value below.

## Global Constraints

- Owner rule §0.1: before each task touching a control, re-check its integration-map row (part 2 §P.13) against the code.
- Owner rule §0.2: phase ends only after the 1:1 visual check at 1920 × 1032 against `StageRevised` (Task 9).
- Names, not values: every number/string named (`ci/check_named_values.py` must stay at 0 problems).
- No file over 800 lines; new `Source/` files added to **both** `Arranger Workstation.jucer` and `ci/Tests.jucer` (edit jucers with Python, keep CRLF), Projucer re-save with `Start-Process -Wait`.
- Colours as named constants in `Source/Common/AppColours.h` (new `namespace StagePalette`), values exactly spec §3.1.
- Tests never write `%APPDATA%`; live checks back up/restore it (0 differences).
- Reference size 1200 × 645; scale `s = min(w / 1200, h / 645)`; panel 124, felt 4, keys 140, slip 12, note area = rest.
- Tempo range 60–200; hold: first step at once, repeat after 400 ms, then 130 ms − 10 ms × step (120, 110, …), floor 30 ms; wheel ±1 (Shift ±5); right-click = style tempo; locked only in arranger mode while playing.
- Messages show 2.5 s, upper case, top-left of the screen.

## Review Focus

1. Window narrower than 1200 px or very short (restore-down): panel must not overlap the keys; `StageScale` clamps (test in Task 2).
2. Tempo typed out of range / empty / non-digits ("abc", "", "999", "40") → clamped or unchanged, never a crash (Task 3).
3. No style open yet and Start / section button pressed → today's behaviour (nothing happens / existing message), no null dereference (Task 7).
4. Time signature 3/4 or 6/8 → lamp count follows the numerator (Task 3).
5. Two messages within 2.5 s → the newer replaces the older and gets a full 2.5 s (Task 3).

---

### Task 1: Fonts in BinaryData + `StageFonts`

**Files:**
- Create: `Resources/Fonts/BarlowSemiCondensed-Medium.ttf`, `BarlowSemiCondensed-SemiBold.ttf`, `ShareTechMono-Regular.ttf`, `OFL-Barlow.txt`, `OFL-ShareTechMono.txt` (from github.com/google/fonts `ofl/barlowsemicondensed`, `ofl/sharetechmono`)
- Create: `Source/UI/Stage/StageFonts.h/.cpp`
- Modify: both `.jucer` files (3 TTFs as `resource="1"`, the two `.cpp/.h` as sources)
- Test: `tests/unit/test_stage_fonts.cpp`

**Interfaces — Produces:** `namespace StageFonts { juce::Font silkscreen (float height, bool semiBold); juce::Font lcd (float height); }` (typefaces created once, function-local statics).

- [x] Test `StageFonts loads embedded faces`: `silkscreen(10.0f, true).getTypefaceName() == "Barlow Semi Condensed"`, `lcd(14.0f).getTypefaceName() == "Share Tech Mono"`.
- [x] Run unit tests → fails (no symbol).
- [x] Download fonts + licences, add to jucers, re-save both projects, implement via `juce::Typeface::createSystemTypefaceFor (BinaryData::…, size)`.
- [x] Run → passes; full unit run green. Commit `feat(ui): embed the Stage fonts`.

### Task 2: Palette, metrics and `StageScale`

**Files:** Modify `Source/Common/AppColours.h` (namespace `StagePalette`, every colour of spec §3.1); Create `Source/UI/Stage/StageMetrics.h` (reference sizes from the board: panel 124, felt 4, keys 140, slip 12, screen 368 × 106, panel columns `1fr | 600 | 1fr`, paddings 7 × 20, button sizes 26 × 14 / 40 × 17, section grid 6 columns × 3 rows (row gap 10), transport grid 3 × 44 (row gap 12), LED 5 + gap 3 left of the button, label gap 3, Quick FX block 130 with 2 knob cells of 62, knob ring 52 / skirt 36 / cap 24), `Source/UI/Stage/StageScale.h/.cpp`; Test `tests/unit/test_stage_scale.cpp`.

**Produces:** `struct StageRegions { juce::Rectangle<int> panel, felt, noteArea, keys, slip, panelContent; float scale; };` (`panelContent` = the 1200 × 124 board scaled by `s`, centred in the panel: same look on any window shape, spec §4.1); `StageRegions StageScale::regionsFor (juce::Rectangle<int> bounds);`

- [x] Tests: 1920 × 1032 → scale 1.6, panel height 198 (124 × 1.6 rounded), keys 224, slip 19, felt 6, note area = rest, regions stacked without gaps; 1200 × 645 → exact reference; 1000 × 900 → scale = 1000/1200 (width-limited); 1920 × 500 → scale = 500/645 and note area ≥ 0 (Review Focus 1).
- [x] Fail → implement → pass → commit `feat(ui): Stage palette, metrics and scaling`.

### Task 3: Pure models — `TempoModel`, `BeatLampModel`, `ScreenMessages`, `PageRouter`

**Files:** Create `Source/UI/Stage/StageModels.h/.cpp`; Test `tests/unit/test_stage_models.cpp`.

**Produces:**
- `class TempoModel { public: explicit TempoModel (double styleTempo); double value() const; void setLocked (bool); bool isLocked() const; void setStyleTempo (double); bool step (int direction); bool applyTyped (const juce::String&); bool wheel (int notches, bool coarse); bool resetToStyle(); static int holdDelayMs (int stepIndex); }` — mutators return true only when the value changed and refuse while locked; values round and clamp to 60–200.
- `struct BeatLampModel { static std::vector<LampState> lamps (int beatsPerBar, double elapsedBeats, bool playing, bool asleep); }` with `enum class LampState { off, downbeat, beat };`
- `class ScreenMessages { public: void post (const juce::String&, double nowMs); juce::String current (double nowMs) const; }` — upper-cases, shows 2500 ms, newer replaces older.
- `enum class StagePage { none, effects, settings, styles, voices, chords, file, home }; class PageRouter { public: StagePage toggle (StagePage); StagePage close(); StagePage open() const; };`

- [x] Tests (exact values): `step(+1)` from 120 → 121; 200 + step → 200; `applyTyped("140")` → 140; `applyTyped("999")` → 200; `applyTyped("40")` → 60; `applyTyped("")`/`("abc")` → false, value unchanged; `wheel(1,false)` → +1, `wheel(1,true)` → +5, `wheel(-2,false)` → −2; locked: all return false; `resetToStyle()` → style tempo; `holdDelayMs(0)` = 400, `(1)` = 120, `(5)` = 80, `(20)` = 30 (board: `max(30, 130 − 10 × step)`; the earlier "(1) = 130" contradicted "(5) = 80"). Lamps: 4/4 elapsed 0.0 playing → `{downbeat, off, off, off}`; elapsed 2.3 → index 2 `beat`; 3/4 → 3 lamps; not playing → all off; asleep → all off. Messages: post "Recording started!" at 0 → current at 100 = "RECORDING STARTED!", at 2600 = ""; post A at 0, B at 2000 → at 4000 = "B". Router: toggle(effects) → effects; toggle(effects) again → none; toggle(settings) while effects → settings; close() → none.
- [x] Fail → implement → pass → commit `feat(ui): Stage panel models (tempo, lamps, messages, pages)`.

### Task 4: `StageWidgets` (rubber buttons, knobs)

**Files:** Create `Source/UI/Stage/StageWidgets.h/.cpp`; Modify `StageFonts` (sizes are CSS px = JUCE point height, letter spacing in CSS em), `StageMetrics.h`; Test `tests/unit/test_stage_widgets.cpp` (state and geometry, no pixels).

**Produces (as built):**
- `class RubberButton : public juce::Button` — `Style { key, screen, ledToggle }`, `LedColour { red, green }`, `LabelTone { bright, dim }`, `LabelSide { left, right }`; `setLedOn / isLedOn`, `setScale`, `getPreferredWidth/Height`, `getCapBounds / getLedBounds / getLabelBounds`. Each draws its own LED, cap and label (board `.btn` / `.led` / `.silk`); a key button is centred on its cap and only the cap takes clicks.
- `class StageKnob : public juce::Slider` — `Size { quick, effects }`, `RotaryVerticalDrag`, 0–127 step 1, ±135°, draws ticks, knurled skirt, cap, pointer and label.
- No separate `StageLookAndFeel`: nothing in phase 1 needs one (each widget paints itself); add one when a page needs JUCE-drawn controls.
- [x] Tests: LED state; style / colour / label lines; key geometry (cap centred, LED 3 px left, label 3 px under); only the cap takes clicks; knob range, ±135°, vertical drag. Fonts: point height = CSS size, kerning = CSS em.
- [x] Implemented; a deliberate break of the cap position made 2 checks fail, then reverted → pass → commit `feat(ui): Stage rubber buttons and knobs`.

### Task 5: `StageScreen` (LCD rows, lamps, tempo interactions, messages)

**Files:** Create `Source/UI/Stage/StageScreen.h/.cpp`; Test `tests/unit/test_stage_screen.cpp`.

**Consumes:** Task 3 models, Task 4 fonts/palette.
**Produces:** `class StageScreen : public juce::Component, private juce::Timer { public: struct Content { juce::String styleName, mode, variation, chord, timeSignature; int bar; int beatsPerBar; double elapsedBeats; bool playing; bool asleep; }; void setContent (const Content&); void postMessage (const juce::String&); TempoModel& tempo(); std::function<void (double)> onTempoChanged; };` Mouse: click tempo → inline `juce::TextEditor` (digits only, Enter applies, Esc cancels, focus lost applies); wheel / vertical drag on tempo; right-click → `resetToStyle` + message "TEMPO BACK TO STYLE: nnn"; locked → message "TEMPO LOCKED WHILE PLAYING".
- [ ] Tests (component, no window): `postMessage` then the top-left text = upper case; simulated `mouseWheelMove` over the tempo bounds raises `onTempoChanged(121)`; right-click calls `onTempoChanged(style tempo)`; locked tempo → callback not called and message shown.
- [ ] Fail → implement paint per board (bezel 368 × 106, LCD rows 11/28/13 px, lamp row 8 px, 4 px gaps) → pass → commit `feat(ui): Stage screen`.

### Task 6: `StagePanel` assembly

**Files:** Create `Source/UI/Stage/StagePanel.h/.cpp`; Test `tests/unit/test_stage_panel.cpp`.

**Layout:** every control is placed inside `StageRegions::panelContent` with `StageMetrics` × `s`, never against the panel edges (spec §4.1, same look on every window shape).
**Produces:** `class StagePanel : public juce::Component { public: StageScreen& screen(); void setSectionLit (const juce::String& buttonName); void setAutoFill (bool); void setSyncStart (bool); void setRecording (bool); void setPlaying (bool); void setPageLed (StagePage); void setPartLeds (bool left, bool right); std::function<void (const juce::String& sectionButton)> onSection; std::function<void()> onRec, onFile, onStop, onStart, onAutoFill, onSyncStart; std::function<void (StagePage)> onPage; std::function<void (int direction, bool pressed)> onTempoHold; std::function<void (bool left)> onPart; std::function<void (int controller, int value)> onQuickKnob; };`
Section button names = today's `SectionGroupNames` labels (Intro 1-3, Ending 1-3, Variation 1-4, Break, Fill 1-4) so `MainComponent::handleIntro/handleEnding/handleVar/handleFill/handleBreak` and `highlightArrangerSection` keep working unchanged. Grid order per spec §4.2. Auto Fill sits after Fill 4 in the left section grid; Rec, File, Sync Start, Stop, Start sit in the right-hand transport grid (spec §4.2, changed 2026-11-23). Two quick knobs send `EffectControllers::volume / reverb`.
- [ ] Tests: clicking each section button raises `onSection` with the exact existing label; `setSectionLit("Variation 2")` lights only that LED (all others off); page button for the open page lit; tempo button press/release raise `onTempoHold(dir, true/false)`.
- [ ] Fail → implement layout from `StageMetrics` scaled by `StageScale` → pass → commit `feat(ui): Stage panel`.

### Task 7: Wire the panel into `MainComponent` (behaviour integration)

**Files:** Create `Source/UI/MainComponentStage.cpp` (keeps `MainComponent.cpp` under 800 lines); Modify `MainComponent.h` (member `std::unique_ptr<StagePanel> stagePanel; PageRouter pageRouter;`), `MainComponent.cpp::resized` (use `StageScale::regionsFor`; header controls replaced), `MainComponentInit.cpp` (old header buttons not added to the header any more; their objects stay for the legacy pages).

Wiring (each = today's handler, integration map rows):
- `onSection` → `handleIntro/handleEnding/handleVar/handleFill/handleBreak` (by prefix); `highlightArrangerSection` → `stagePanel->setSectionLit`.
- `onStart/onStop` → `display`'s current style `triggerStartClick/triggerStopClick`; no style open → today's behaviour (no-op).
- `onRec` → today's `startRecording` / `stopRecording` click handlers toggled; `onFile` → `pageRouter.toggle(file)`, legacy page = today's save-recording + play-file buttons and play/stop of the recording in a page frame.
- `onAutoFill` → today's auto-fill toggle (settings key `ArrangerAutoFillEnabled`); `onSyncStart` → `display->setArrangerSynchroStart` + key `SynchroStart`.
- Screen: style name/tempo/variation/chord/bar/beat from the current style component and the engine elapsed-beats callback that feeds `BeatBar` today; `screen.onTempoChanged` → `CurrentStyleComponent::setTempo`; lock = `isArrangerModeEnabled() && getIsPlaying()`.
- Every `temporaryPopup` call site (MainComponent.cpp:573, MainComponentInit.cpp:150/173/214/241, MainComponentPlayback.cpp:291) → `stagePanel->screen().postMessage(text)`.
- `onPage`: home → `homeButtonOnClick`; styles → legacy page hosting `display` (today's tabs) ; voices → `showInstrumentSelector`; effects → today's Sound Effects window; settings → today's MIDI settings window; chords → today's chord helper. Pages hide the `NoteLayer` (same pattern as `setArrangerOverlaySceneHidden`). Esc closes an open page before the Esc menu.
- `onPart` → left/right hand instrument toggles; `onQuickKnob` → `applyEffectCC(controller, value, channel of the selected part)`.
- [ ] Before coding: re-check every row above against the code; list any control still only in the old header and wire it.
- [ ] Build app + run all four test categories; numbers ≥ before (unit 870+, arranger 770, integration 130, display 8).
- [ ] Live check (backup/restore %APPDATA%): login, Play, open style, Start/Stop, every section button, Auto Fill, Rec start/stop (message on screen), tempo hold/type/wheel/right-click, each page button opens its legacy page, Esc. Commit `feat(ui): Stage panel replaces the header`.

### Task 8: Reference PNGs (spec §0.4)

- [ ] With the owner's permission, open the canvas in the owner's browser and capture `StageRevised` and the page boards at 2× into `docs/superpowers/specs/assets/2026-11-23-ui-redesign/png/`; otherwise ask the owner to export them. Commit `docs: reference PNGs of the accepted boards`.

### Task 9: 1:1 visual check, review, wiki

- [ ] Run the app at 1920 × 1032, screenshot the play view (and lamps on beats 1 and 2, a screen message, the tempo editor), compare side by side with `StageRevised` per spec §0.2; fix every difference; save screenshots + notes to `docs/superpowers/test-plans/2026-11-23-ui-redesign-phase1-visual.md`.
- [ ] Sonnet review of the branch; verify each claim; fix confirmed ones.
- [ ] Wiki sync (log, hot, backlog, Keyboard & Main UI page); PR; CI green; merge.

Later phases (keys, each page, Dust, finishes, sign-in) get their own plans, written against the code this plan leaves.
