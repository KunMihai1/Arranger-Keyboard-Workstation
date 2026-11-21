# Arranger Phase 3 — Self-Contained Styles with Distinct Per-Section Content

**Date:** 2026-06-06
**Branch context:** `arranger-engine-phase1` (Phase 2 complete)
**Status:** Design approved, pending spec review → implementation plan

---

## 1. Goal

Let users **author and play self-contained arranger styles** where each section
(Intro / Variation / Fill / Ending …) has its own distinct content — replacing the
Phase-2 demo that fakes sections by slicing one Variation loop.

A style becomes a **self-contained snapshot** (Model B): it carries its own notes and
per-track basics, so it plays correctly when recalled regardless of what tracks are
currently loaded. It **coexists** with the classic player and the existing relational
style system, which remain untouched.

## 2. Decisions (locked during brainstorming)

| # | Decision | Rationale |
|---|----------|-----------|
| D1 | **Model B** — styles are self-contained (embed their notes), not references to live tracks | A style must survive track edits/deletion and be recallable/shareable. MIDI events are tiny (~tens–hundreds of KB), so size is a non-issue. |
| D2 | **Authoring approach A** — one recording → mark bar ranges → each becomes a section | Reuses existing `StyleSection` (startBar/endBar), recording flow, and `sliceBars`. The authoring workflow does not change the saved format, so B/C remain open later. |
| D3 | **Normalized storage** — file = `sourceTracks` (events once) + `sections` (bar-windows); slice on load | Self-contained AND re-editable (move a boundary without re-recording). Baked-sections rejected: loses editability for no real size gain. |
| D4 | **Scope = core + bar-marking UI** | Full end-to-end slice: format + builder + load/play + a visual timeline to mark bars. |
| D5 | **Coexist** with classic system | User wants both a non-arranger and arranger way of playing. Classic player feeds off the existing relational styles; arranger uses the new format. Zero risk to working features. |
| D6 | **Embed basics, defer effects** — embed notes + per-track instrument/channel/volume; reserve an optional per-track `effects` slot, unfilled | Matches what the engine actually emits today (`sendInstrumentSetup` = program-change + CC7 only). Per-hand `SoundSettings` are live-player config, not accompaniment. Device-specific effects are messy to freeze; defer. Reserving the slot keeps it additive (no migration). |
| D7 | **Visual timeline UI** (draggable regions) over numeric marking | Polished end state for an arranger authoring tool. Storage/engine identical either way, so it is purely a richer UI layer. |

## 3. Channel map (code-grounded, locked)

```
ch 1       → left hand            (live)        reserved   (firstHand SoundSettings)
ch 2..9    → melodic accompaniment (style)
ch 10      → drums                (style, GM)   reserved
ch 11..13  → spare melodic accompaniment
ch 14      → record player: left  (remap of 1)  reserved   (MidiRecordPlayer::remapChannel)
ch 15      → record player: right (remap of 16) reserved
ch 16      → right hand           (live)        reserved   (secondHand SoundSettings)
```

Allocator rule: **perc → 10; melodic → 2–9 then 11–13, skipping 10/14/15/16.** Up to
**11 melodic accompaniment tracks** before any clash; overflow warns, does not crash.

## 4. Architecture & data flow

```
 RECORD / IMPORT          AUTHOR (new UI)           SAVE                LOAD / PLAY
 one backing      →   Visual timeline:        →   <name>.style    →   slice windows →
 recording            drag section regions        JSON file            per-section loops →
 (TrackEntry)         over the bar ruler          (self-contained)     ArrangerEngine plays
                                                                        distinct sections
```

**Unchanged at runtime:** `ArrangerEngine`, `ArrangerScheduler`, `ArrangerSectionSequencer`
already play multi-section content. Phase 3 feeds them real sections instead of demo slices.

## 5. File format — `.style` (schemaVersion 3)

```jsonc
{
  "schemaVersion": 3,            // file format (runtime ArrangerModel stays v2)
  "kind": "arrangerStyle",
  "id": "uuid",
  "name": "Sunset Groove",
  "originalTempo": 120.0,
  "timeSigNum": 4, "timeSigDenom": 4,

  "sourceTracks": [              // events stored ONCE, beats measured from bar 1
    {
      "id": "uuid", "name": "Drums", "partType": "Drum",
      "channel": 10, "instrument": -1, "volume": 100.0,
      "effects": null,           // reserved (Phase 4+), additive, no migration
      "events": [ {"b":0.0,"m":"9A2464"}, {"b":0.5,"m":"8A2400"} ]
    }
  ],

  "sections": [                  // bar-windows into sourceTracks; carry no notes
    {"id":"intro_1","name":"Intro 1","type":"Intro","startBar":1,"lengthBars":4,"afterComplete":"FallThrough"},
    {"id":"var_1","name":"Variation 1","type":"Variation","startBar":5,"lengthBars":8,"afterComplete":"Loop"},
    {"id":"fill_1","name":"Fill 1","type":"Fill","startBar":13,"lengthBars":2,"afterComplete":"FallThrough"},
    {"id":"ending_1","name":"Ending 1","type":"Ending","startBar":27,"lengthBars":6,"afterComplete":"Stop"}
  ]
}
```

Rules:
- `events[].m` = raw MIDI bytes in hex; `events[].b` = beats from the start of the recording. **Events are note-on/off only** (the builder filters to notes; instrument/volume travel as track fields).
- Bar *N* starts at beat `(N-1) × beatsPerBar`. Sections may overlap or leave gaps.
- A section is defined **purely by its window** — it spans **all** `sourceTracks` over its bar range. (Subsetting tracks per section is approach B/C territory, out of scope here.)
- `afterComplete` ∈ `Loop | FallThrough | Stop`, defaulted by section type in the UI, overridable.
- File-model structs (`ArrangerStyleFile`, `SourceTrackFile`, `SectionWindow`) are kept
  distinct from the runtime `ArrangerModel`, so disk format and engine model evolve independently.

## 6. Components

### 6.1 `ArrangerStyleIOHelper` (new)
`saveToFile(file, ArrangerStyleFile)` / `loadFromFile(file, ArrangerStyleFile&)` using JUCE
`DynamicObject`/JSON, mirroring `TrackIOHelper`/`SectionIOHelper`. Handles MIDI hex
(de)serialization. One `.style` file per style in a new `IOHelper::getFolder("ArrangerStyles")`.

### 6.2 `ArrangerPatternBuilder::buildStyleFromWindows` (new)
Generalizes `buildDemoMultiSectionStyle`. Builds one master `ArrangerSection` from
`sourceTracks` (as `buildSingleSectionStyle` does), then for each window calls the existing
`sliceBars(master, (startBar-1)*bpb, lengthBars)`, tagging type/afterComplete. Includes the
channel allocator (§3). The demo builder is retired once the UI lands.

### 6.3 `ArrangerTimelineComponent` (new, pure UI)
Bar ruler + draggable/resizable colored section regions. Given *(recording length in bars,
time signature, regions)* it renders and edits regions, snapping edges to bar lines, and emits
`onSectionChanged / onSectionAdded / onSectionRemoved / onSectionTypeChanged`. Knows nothing
about files or the engine. Colors reuse `AppColours.h` + the section-button palette. A live
playhead is fed by the engine's `onElapsedBeats` (same signal as `CustomBeatBar`).
Constraints: `lengthBars ≥ 1`; overlaps/gaps allowed. Snap/hit-test math kept separate from
painting for unit testing.

### 6.4 `ArrangerStyleEditor` (new, wiring/controller)
Owns the recording + timeline + section list. Pulls the recording (current `TrackEntry`
tracks or imported MIDI) → `sourceTracks` with channels per §3; feeds/collects regions;
**preview** by building the style and `engine.setStyle` so sections can be triggered live;
**save** via builder + `ArrangerStyleIOHelper`.

### 6.5 Mode & style selection
Plugs into existing arranger mode (section buttons already wired to the engine; output
re-routed on engine switch). A dedicated arranger-style list reads `.style` files from the
`ArrangerStyles` folder → load → `engine.setStyle`. A Create/Edit entry opens
`ArrangerStyleEditor`. Classic mode and existing relational styles are untouched.

## 7. Load / save paths

- **Load:** parse JSON → master `ArrangerSection` from `sourceTracks` → per window
  `sliceBars` → assemble `ArrangerStyle` → `engine.setStyle`.
- **Save:** serialize authoring state (`sourceTracks` + windows) straight to JSON. Save
  source + windows, never baked slices — preserves re-editability.

## 8. Testing

- **Builder unit tests** (extend `tests/unit/test_arranger_pattern_builder.cpp`): real windows
  slice & rebase correctly; gaps and overlaps; channel allocator skips reserved channels;
  window past end-of-recording clamps cleanly.
- **Round-trip IO test:** `ArrangerStyleFile → JSON → ArrangerStyleFile` preserves
  events/sections; MIDI hex encode/decode for note-on/off.
- **Timeline logic tests:** snap-to-bar and region hit/resize math (separated from painting).
- **Pure scheduler/sequencer:** existing `test_arranger_*` unchanged.
- **Manual integration:** record → mark → save → reload → play, switching sections live.

## 9. Risks / edge cases

- Empty/zero-bar recording; section extending beyond recording end (clamp).
- Channel-budget overflow (>11 melodic tracks) — warn, don't crash.
- `schemaVersion` forward-compat guard on load.
- Events note-only keeps hex serialization simple (no CC/program-change in `events`).

## 10. Work breakdown (slices for the implementation plan)

1. **Format + IO** — file-model structs, `ArrangerStyleIOHelper`, hex (de)serialization, round-trip tests.
2. **Builder + load path** — `buildStyleFromWindows` + channel allocator + tests; wire file → `ArrangerStyle` → `engine.setStyle`.
3. **Timeline component** — `ArrangerTimelineComponent` (render + drag/resize/snap) + logic tests.
4. **Editor + wiring** — `ArrangerStyleEditor` (recording → preview → save), arranger-style list, mode entry points.
5. **Polish/integration** — theming to app palette, live playhead, end-to-end pass.

## 11. Out of scope (later phases)

- Separate-phrase-per-section / assemble-from-tracks authoring (approaches B/C) — format already supports them.
- Per-track effects baking (`effects` slot reserved).
- Chord/key transposition (`ArrangerPartType`, `ArrangerTrack::originalChord`) — Phase 4/5.
