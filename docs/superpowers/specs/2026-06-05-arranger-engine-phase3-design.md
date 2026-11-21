# Arranger Engine — Phase 3 Design Spec (MIDI Sectioning Authoring Tool)

**Date:** 2026-06-05
**Status:** Designed (awaiting plan)
**Author:** Mihai
**Depends on:** Phase 1 + Phase 2 (`arranger-engine-phase1`, merged to `main`) — `ArrangerModel`,
`ArrangerScheduler`, `ArrangerSectionSequencer`, `ArrangerEngine`, `ArrangerPatternBuilder`,
Classic/Arranger toggle, wired section buttons.
**Parent roadmap:** `2026-06-05-arranger-engine-design.md` §4, Phase 3.

---

## 1. Context & Goal

Phases 1–2 gave the arranger a real transport: pressing Start loops the current style and the
Intro/Var/Fill/Ending buttons switch sections bar-synced, one-shots behave correctly, and no notes
hang at any seam. But there is still **no real per-section content** — `buildDemoMultiSectionStyle`
fakes four sections by slicing one loop, so every section sounds the same. The transport is honest;
the content is not.

**Goal:** give the user a tool to carve the **currently-loaded style's tracks** into genuinely
distinct, named, typed sections (Intro / Variation / Fill / Break / Ending), **save** them to a
self-contained file, **reload** them, and have the existing Phase-2 arranger switch between
*different-sounding* sections — **with no change to the sequencer or engine**, only richer data.

### Why this is the right Phase 3 (and what we deliberately deferred)

Roadmap Phase 3 originally bundled **two independent subsystems**: a MIDI-sectioning *authoring
tool* and a Yamaha/Korg *style importer*. We split them. This phase is the authoring tool only.
The importer is deferred to sit beside Phases 4–6, because its payload (CV tables, NTT, original
chord) is **inert until chord recognition and transposition exist** — importing it now would build
a parser for fields nothing reads. The authoring tool, by contrast, delivers the distinct,
switchable content Phase 2 was missing *right now*, depending on nothing downstream. The importer
also doubles as the "auto-sectioning" path (those files already carry their section boundaries), so
there is no need to build a fragile MIDI auto-detector: **manual marking** covers authoring from
scratch; the future importer covers already-sectioned content.

---

## 2. Scope

### In
- A small **authoring data model** (`ArrangerSectionDef` — one marking: name, type, start bar, length).
- A generalized slicer **`ArrangerPatternBuilder::buildStyleFromSections`** — turns the loaded
  tracks + a list of markings into a multi-section `ArrangerStyle` (reusing the proven Phase-2
  `sliceBars` logic).
- **`ArrangerStyleIO`** — JSON save/load of a **self-contained** authored style (embeds the source
  track events *and* the markings; re-slices on load), versioned via `schemaVersion`.
- **`StyleEditorWindow` / `StyleEditorComponent`** — a themed editor (table + live colour strip +
  per-section Preview ▶ + New / Load / Save), in the app's existing palette.
- **`ArrangerSectionColours.h`** — a shared section-type → accent-colour map.
- **Integration:** a launch button in the Current Style controls (Arranger mode), and an authored/
  loaded style replacing `buildDemoMultiSectionStyle` in `startPlaying()` when one is active.

### Out / deferred (decided with the user)
- **Yamaha/Korg `.sty`/`.prs`/`.set` importer** — later, with Phases 4–6.
- **Auto-detection of sections** — covered by the importer instead; manual marking only.
- **Phase-4 transposition metadata capture** — part-type override per track, original-chord
  tagging. Tracks keep the existing auto part-typing (perc → Drum/ch10, melodic → Acc/ch2+);
  `originalChord` stays at its default. Captured in Phase 4 when it is actually read.
- **Per-section track subsets** — a section slices *all* tracks together. Korg-style "each section
  has its own tracks/CVs" is later complexity.
- **Editing individual MIDI notes** — this tool sections existing material; note editing is the
  app's separate MIDI editor.

---

## 3. Decisions captured during brainstorming

| Question | Decision |
|---|---|
| Sectioning method | **Manual region marking** (importer is the future auto path) |
| Source material | **The currently-loaded style's `TrackEntry` tracks** |
| Metadata scope | **Distinct content only**; defer part-type / original-chord to Phase 4 |
| Persistence | **Save to disk, reloadable**, as a **self-contained** JSON (embeds source + markings) |
| Marking UI | **Layout C** — editable table + read-only colour strip, themed in the app palette |
| Per-section preview | **Included** (▶ auditions one section in isolation) |
| Launch point | **Button A** — in the Current Style controls, Arranger mode |
| Slicing granularity | **All tracks together** per section |

---

## 4. Architecture

Five focused units plus wiring. The engine and sequencer are **untouched** — they already consume a
generic `std::vector<ArrangerSection>`; Phase 3 only produces richer ones.

### 4.1 Authoring model — `ArrangerSectionDef` (in `ArrangerModel.h`)
One "marking" the editor edits:
```cpp
struct ArrangerSectionDef
{
    juce::String         name   = "Variation 1";
    ArrangerSectionType  type   = ArrangerSectionType::Variation;
    int                  startBar   = 1;   // 1-based bar where this section begins
    int                  lengthBars = 1;   // section length in bars
};
```
Ranges are arbitrary labeled windows over the source: they **may overlap or leave gaps** — fully
free, no partition requirement.

### 4.2 The slicer — `ArrangerPatternBuilder::buildStyleFromSections`
```cpp
// sourceTracks: full-length, beat-based, channels already assigned
//   (exactly the tracks of buildSingleSectionStyle(...).sections.front()).
ArrangerStyle buildStyleFromSections (const std::vector<ArrangerTrack>& sourceTracks,
                                      const std::vector<ArrangerSectionDef>& sections,
                                      int timeSigNum, int timeSigDenom, double referenceBpm);
```
For each `ArrangerSectionDef`: clone `sourceTracks`, window each track's events to
`[(startBar-1)·bpb, (startBar-1+lengthBars)·bpb)`, rebase by `−startBeats`, set `lengthBars`,
`type`, `name`, `id`, and derive `afterComplete` from `type` (the Phase-2 rules):

| Type | `afterComplete` |
|---|---|
| Intro | FallThrough |
| Fill | FallThrough |
| Ending | Stop |
| Variation / Break | Loop |
| CountIn | FallThrough |

This is the existing `sliceBars` lambda generalized and lifted out of `buildDemoMultiSectionStyle`.
Held notes that straddle a slice edge are **not** repaired here — the engine's tested seam machinery
(`ArrangerScheduler::flushActiveNotes`) closes them at playback, identically to Phase 2. The source
tracks come from the already-proven `buildSingleSectionStyle` (channel mapping, beat conversion), so
`buildStyleFromSections` stays a pure windowing function.

### 4.3 Persistence — `ArrangerStyleIO`
A self-contained authored document and its JSON I/O:
```cpp
namespace ArrangerStyleIO
{
    struct AuthoredStyle
    {
        juce::String name;
        double originalTempo = 120.0;
        int    timeSigNum = 4, timeSigDenom = 4;
        std::vector<ArrangerTrack>       sourceTracks;  // full-length beat patterns (embedded)
        std::vector<ArrangerSectionDef>  sections;      // the markings
    };

    bool        save (const AuthoredStyle&, const juce::File&);   // false on write error
    bool        load (const juce::File&, AuthoredStyle& out);     // false on parse/version error
    ArrangerStyle toStyle (const AuthoredStyle&);                 // = buildStyleFromSections(...)
}
```
Storing both the **source** and the **markings** makes the file stand alone (open → play, nothing
else loaded) *and* re-editable (re-slice on load, adjust markings, re-save). MIDI is stored as raw
bytes; see §6.

### 4.4 The editor — `StyleEditorComponent` + `StyleEditorWindow`
- **`StyleEditorComponent`** holds the working `AuthoredStyle`, renders **layout C**: the
  **section table** (Name | Type ▾ | Start bar | Length | ✕, plus "+ Add section"), the **read-only
  colour strip** above it (segments sized by `lengthBars`, coloured by §5, redrawn on every edit),
  the **New / Load… / Save** actions, and a **Preview ▶** per row. Themed with `AppColours` + a
  small `LookAndFeel` consistent with `PlayScreenLookAndFeel`.
- **`StyleEditorWindow`** is a themed `juce::DocumentWindow` wrapper owning one
  `StyleEditorComponent` — the same idiom as `MIDIWindow` / `SoundEffectWindow`.
- **Ownership:** the window is owned by `CurrentStyleComponent` (which already holds the loaded
  tracks and the `ArrangerEngine`), created lazily on first open and shown with `setVisible(true)`.
  Optionally registered with `MainComponent`'s foreground-hide handler for parity with the other
  themed windows.

### 4.5 Shared colour map — `ArrangerSectionColours.h`
```cpp
namespace ArrangerSectionColours
{
    inline juce::Colour forType (ArrangerSectionType t)
    {
        switch (t) {
            case ArrangerSectionType::Intro:     return AppColours::accent3; // teal
            case ArrangerSectionType::Variation: return AppColours::accent4; // blue
            case ArrangerSectionType::Fill:      return AppColours::accent5; // golden
            case ArrangerSectionType::Ending:    return AppColours::accent6; // coral
            case ArrangerSectionType::Break:     return AppColours::accent8; // violet
            case ArrangerSectionType::CountIn:   return AppColours::rowLabel;
        }
        return AppColours::accent4;
    }
}
```
Used by the strip now; reusable to colour the Phase-2 transport buttons later (UI cohesion).

### 4.6 Wiring (`CurrentStyleComponent`, classic mode untouched)
- **Launch (button A):** an "Edit Sections…" button in the Current Style controls, visible in
  Arranger mode, calls `openStyleEditor()` — which builds `sourceTracks` from the current
  `selectedTracks` via `buildSingleSectionStyle(...).sections.front().tracks` and opens the window.
- **Apply:** the editor sets the active authored style on `CurrentStyleComponent`
  (`std::optional<AuthoredStyle> activeAuthoredStyle`) whenever the user **Saves** or **Loads** a
  file in the window, so the next Start uses what's on screen.
- **`startPlaying()`** (currently lines ~55-63): in Arranger mode, if `activeAuthoredStyle` is set,
  play `ArrangerStyleIO::toStyle(*activeAuthoredStyle)`; otherwise keep
  `buildDemoMultiSectionStyle` as the fallback. Classic mode unchanged.
- **Preview ▶:** the editor asks `CurrentStyleComponent` to build a **single-section** style from
  that one `ArrangerSectionDef` (via `buildStyleFromSections` with a one-element vector, looped) and
  start the engine; Stop ends the preview.

---

## 5. Section type → colour (normative)

Intro = **teal** (`accent3`), Variation = **blue** (`accent4`), Fill = **golden** (`accent5`),
Ending = **coral** (`accent6`), Break = **violet** (`accent8`), CountIn = **muted** (`rowLabel`).

---

## 6. Persistence file format (`.json`, `schemaVersion = ARRANGER_SCHEMA_VERSION`)

```json
{
  "schemaVersion": 2,
  "name": "Sunset Groove",
  "originalTempo": 120.0,
  "timeSigNum": 4,
  "timeSigDenom": 4,
  "sourceTracks": [
    {
      "id": "…", "name": "Drums", "partType": "Drum",
      "channel": 10, "instrument": -1, "volume": 100.0,
      "events": [ { "b": 0.0, "m": "9A2464" }, { "b": 0.5, "m": "8A2400" } ]
    }
  ],
  "sections": [
    { "name": "Intro 1",     "type": "Intro",     "startBar": 1,  "lengthBars": 4 },
    { "name": "Variation 1", "type": "Variation", "startBar": 5,  "lengthBars": 8 },
    { "name": "Fill 1",      "type": "Fill",      "startBar": 13, "lengthBars": 2 },
    { "name": "Ending 1",    "type": "Ending",    "startBar": 27, "lengthBars": 6 }
  ]
}
```
- `m` = the MIDI message's raw bytes as hex (`MidiMessage::getRawData`); load via
  `juce::MidiMessage(const void*, int)`.
- `type` / `partType` serialized as stable strings.
- `load` returns `false` on malformed JSON or unknown `schemaVersion` (callers show an alert and
  leave the current state untouched).

---

## 7. Testing

**Unit — `buildStyleFromSections`:**
- N markings → N sections with matching `type`/`name`/`lengthBars`;
- per-type `afterComplete` mapping correct;
- slice windows events and rebases to beat 0 (an event at bar 6 in a `startBar=5` section lands at
  the right local beat);
- overlapping ranges and a range past the source length behave (empty/clamped, no crash);
- channel/part mapping preserved from the source tracks.

**Unit — `ArrangerStyleIO`:**
- `save` then `load` round-trips an `AuthoredStyle` to equality — markings **and** every source MIDI
  event (bytes + beat);
- `toStyle(load(save(x))) == buildStyleFromSections(x…)`;
- unknown `schemaVersion` and malformed JSON return `false` without throwing.

**Unit — `ArrangerSectionColours`:** each type maps to its specified accent.

**Manual smoke (the real proof):** load a style, open the editor, mark four distinct sections, Save.
Start → Variation loops; press Fill → one fill → back; press Ending → ending → silence — all
**audibly different** and with **zero hung notes**. Restart the app, Load the file, press play →
identical result with nothing else loaded. Classic mode unchanged.

---

## 8. Risks

- **Slice-boundary correctness** — same family as Phases 1–2; mitigated by reusing the tested
  `sliceBars`/seam machinery and the unit tests above.
- **JSON MIDI-byte fidelity** — covered by the explicit round-trip test.
- **Editor ↔ engine coupling for Preview** (starting/stopping the live engine from the editor) —
  keep it behind the existing `start()`/`stop()` API; the editor never touches the timer directly.
- **Not disturbing classic mode** — all integration stays inside the `arrangerModeEnabled` branches,
  exactly as Phase 2.
- **Source embedding size** — MIDI is tiny; a full style is a few KB of JSON. Acceptable.

---

## 9. Files

**New:**
- `Source/Arranger/ArrangerStyleIO.h` / `.cpp`
- `Source/Arranger/ArrangerSectionColours.h`
- `Source/StyleEditorComponent.h` / `.cpp`
- `Source/StyleEditorWindow.h` (themed `DocumentWindow` wrapper)
- `tests/unit/test_arranger_style_io.cpp`

**Modified:**
- `Source/Arranger/ArrangerModel.h` — add `ArrangerSectionDef` (schema already at v2).
- `Source/Arranger/ArrangerPatternBuilder.h` / `.cpp` — add `buildStyleFromSections`; refactor the
  `sliceBars` lambda into the shared slice helper (demo builder keeps working via it).
- `Source/CurrentStyleComponent.h` / `.cpp` — launch button, `activeAuthoredStyle`, `openStyleEditor`,
  preview hook, `startPlaying()` authored-style branch.
- `Source/MainComponent.cpp` — (optional) foreground-hide parity for the editor window.
- `tests/unit/test_arranger_pattern_builder.cpp` — extend for the lifted slice helper **and the
  `buildStyleFromSections` tests** (§7).
- `Project Synth2.jucer` — register the new files.

---

## 10. How this sets up later phases
The importer (deferred) targets the **same** `AuthoredStyle`/`ArrangerStyle` shape, so it drops in
beside this tool with no engine change. Phase 4 adds the inert fields (part-type override, original
chord) to the **same editor** and serializer — additive, schema-compatible. The sequencer/engine
never learn where a style came from.

## 11. Out of scope (whole project — unchanged from parent spec)
- DAW-style audio-clip timeline, crossfades, time-stretch.
- Chord recognition, transposition (NTR/NTT), CV tables — Phases 4–6.
- The Yamaha/Korg binary importer — deferred to the Phase 4–6 window.
