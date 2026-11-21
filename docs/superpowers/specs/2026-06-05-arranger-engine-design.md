# Arranger Engine — Design Spec

**Date:** 2026-06-05
**Status:** Phases 1-2 implemented (looping engine + Classic/Arranger toggle; section state machine + bar-synced transport; 108 Arranger unit-test assertions passing); Phases 3-7 pending
**Author:** Mihai + Claude

---

## 1. Context & Goal

The app today is a **MIDI playback system**: a "style" is a set of MIDI tracks
(`Track` / `TrackEntry`) played back linearly via `MultipleTrackPlayer` (a
`HighResolutionTimer` driving MIDI-out and an SFZ inject callback). There are
section buttons (Intro / Var / Fill / Break / Ending) in the UI, but they are
**not wired into playback** and their data is placeholder scaffolding.

The goal is to add a real **arranger engine** — specifically a
**keyboard-workstation arranger** in the style of Korg Pa / Yamaha PSR, *not* a
DAW timeline arranger. The classic playback system must keep working unchanged;
the arranger is a new mode selected by a setting.

### The two meanings of "arranger" (for future readers)

- **DAW timeline arranger** — drag audio clips onto tracks at timestamps,
  sample-accurate audio-callback scheduling, crossfades, time-stretch. **Not
  this project.** (A PDF reviewed during brainstorming described this; ~60–70%
  of it does not apply here.)
- **Keyboard-workstation arranger** — real-time auto-accompaniment: you hold a
  chord, a "style" of MIDI accompaniment patterns plays transposed to that
  chord, and you switch Intro/Main/Fill/Ending sections live. **This is the
  project.**

---

## 2. How a Korg arranger works (reference)

The end target is a full Korg-style engine. Its internals, for reference:

### Style data structure (3 levels)
```
Style
 └─ Style Elements (sections): Var1–4, Intro1–3, Fill1–3, Break, Ending1–3
     └─ Tracks (8): Drum, Perc, Bass, Acc1–5
         └─ Chord Variations (CV1–6): up to 6 recorded patterns per track
```
A **CV/Chord table** per section picks *which* CV plays based on the recognized
chord *type* (major-family → CV1, minor → CV2, dominant → CV3, …). Korg picks
the right hand-authored recording for the chord family, *then* transposes it.

### Per-track transposition parameters
- **Mode**: Drum / Perc / Bass / Acc. Drum & Perc are never pitch-transposed.
- **Original Key/Chord**: the key+chord each CV was recorded in (the reference).
- **NTT (Note Transposition Table)**: Parallel / Fixed / i-Series — converts each
  note so it fits the recognized chord *quality* (maj→min, correct 7th, etc.).
- **Wrap Around**: a semitone point where transposed notes fold back to stay in
  register.
- **Note Range**: hard low/high limits per track.
- **Retrigger**: behaviour of held notes when the chord changes.

### Chord recognition
- Scan area: Lower / Upper / Full (relative to the keyboard split).
- Recognition mode: Fingered 1/2/3, One Finger, Expert, …
- Output: `(root, type)` from ~17–30 chord types.
- Extras: Bass Inversion, Manual Bass, Memory.

### Real-time engine loop (per tick)
1. Chord recognizer reads scan zone → `(root, type)`.
2. For the active section + each track: CV/Chord table → pick CV.
3. Read CV events due this tick; for each note: transpose by
   `(recognizedRoot − originalRoot)`, run through NTT, apply Wrap Around +
   Note Range (Drum/Perc skip transposition).
4. Send to MIDI-out / SFZ sampler.
5. Pending section changes apply at the **next bar boundary**; Auto-Fill fires on
   variation change; Intros/Endings play once; Synchro Start begins on first
   chord.
6. On chord change, held notes obey **Retrigger**.

---

## 3. Current code — what exists vs what's missing

| Piece | Status |
|---|---|
| Style = grid of MIDI tracks | ✅ `Track`, `TrackEntry`, `CurrentStyleComponent` |
| Section buttons (Intro/Var/Fill/Break/Ending) | ✅ exist, **not wired** (handlers are stubs) |
| `StyleSection { startBar, endBar }` | ⚠️ **placeholder only** — `startBar`/`endBar` are always `-1`; sections are fake evenly-spaced 10s slots from `Display::initializeSectionsForStyle`. Free to replace. |
| Keyboard split (left/right zones) | ✅ exists (`leftHandBoundSetting`, `rightHandBoundSetting`) |
| MIDI-out + SFZ output path | ✅ exists (`weak_ptr<MidiOutput>` + `onMidiMessage` inject) |
| Linear player | ✅ `MultipleTrackPlayer` — wall-clock timer, events in absolute seconds, **stops at end, no looping** |
| Chord **browser** (name→notes + images) | ✅ exists (`ChordHelper::getNotesForChord`) — generative, reference only |
| **Chord recognition (notes→chord)** | ❌ missing |
| **Looping pattern engine** | ❌ missing |
| **Transposition (NTR/NTT)** | ❌ missing |
| **Section state machine (bar-synced + fills)** | ❌ missing |
| **Per-section / CV content** | ❌ missing (no real section data exists) |

---

## 4. Roadmap (phased path to full Korg)

Each phase is its own design → plan → implementation cycle, and each is audible
and testable on its own. The end target stays full Korg.

1. ✅ **Looping pattern engine + data model** — *implemented on branch `arranger-engine-phase1`*
2. ✅ **Section state machine + transport** — *implemented on branch `arranger-engine-phase1`*:
   section buttons wired; bar-synced switching; Intro-once→Variation, Ending-once→stop,
   manual Fill-once→return. (Auto-Fill and Synchro Start deferred to later phases.)
3. **Content & Sectioning pipeline** — two import front-ends into the same model:
   - **"Divide a MIDI into sections" authoring tool**: load a MIDI, mark regions
     (or auto-detect), tag each as Intro/Var/Fill/Break/Ending, assign tracks to
     parts, set loop length and original chord → per-section patterns.
   - **Yamaha/Korg style importer**: parse `.sty`/`.prs`/`.set` and map sections,
     parts, CVs, NTT, original chord straight into the model.
   - The author-vs-import emphasis and original-chord capture are decided here.
4. **Chord recognition** — `notes → (root, type)`, scan zone, Fingered mode.
5. **Transposition brain** — NTR (Root/Chord), NTT (Parallel/Fixed/i-Series),
   Wrap Around, Note Range, Retrigger. *(The big one.)*
6. **Chord Variation tables** — CV1–6 per track + CV/Chord table + Original Key.
7. **Polish** — Bass Inversion, Manual Bass, Memory, recognition modes, STS.

Both content paths in step 3 produce the same `ArrangerStyle → Section → Track →
pattern (+ CV/NTT later)` structure; the engine never knows where a style came
from.

---

## 5. Phase 1 Design — Looping Pattern Engine + Data Model

### 5.1 Goal
In a new **Arranger mode**, press play → the current style's tracks **loop
cleanly and in time** to the existing MIDI-out / SFZ path. No chord-following, no
section switching yet. Classic playback stays fully intact behind a toggle.

### 5.2 Scope
- **In:** new arranger data model; new `ArrangerEngine`; bar-accurate seamless
  looping; a mode toggle (`Classic` / `Arranger`); wrapping each existing style
  as a single default looped section; replacing the placeholder `StyleSection`
  scaffolding.
- **Out:** chord recognition, transposition, section switching / fills, the real
  sectioning / CV content pipeline (later phases).

### 5.3 Architecture
- **`ArrangerEngine`** — a new class, parallel to `MultipleTrackPlayer`. The old
  player is untouched.
- A **mode setting** (`Classic` / `Arranger`) decides which engine the play/stop
  controls and MIDI/SFZ output route to. Default **`Classic`** — existing
  behaviour unchanged until the user flips it.
- Both engines share the existing output plumbing
  (`std::weak_ptr<juce::MidiOutput>` + the `onMidiMessage` SFZ inject callback)
  and reuse `TrackEntry` for source MIDI.

### 5.4 Data model (replaces the scaffolding)
```
ArrangerStyle {
    String id, name;
    double originalTempo;        // BPM the patterns were authored at
    int    timeSigNum, timeSigDenom;
    std::vector<ArrangerSection> sections;
}

ArrangerSection {
    String id, name;
    SectionType type;            // Intro | Variation | Fill | Break | Ending | CountIn
    int    lengthBars;           // this section's own loop length
    std::vector<ArrangerTrack> tracks;
}

ArrangerTrack {
    String id, name;
    PartType partType;           // Drum | Perc | Bass | Acc   (used Phase 4+)
    int      instrument, channel;
    double   volume;
    MidiPattern pattern;         // MIDI in musical time (ticks/beats)
    // --- reserved, inert in Phase 1 ---
    // originalChord;                         // Phase 4/5
    // nttType, ntrMode, wrapAround, noteRange, retrigger;  // Phase 4
    // std::vector<ChordVariation> chordVariations;          // Phase 5
}
```
- Phase 1 fills `pattern` only; the reserved fields exist but are inert.
- Patterns stored in **musical time (ticks/beats)**, tempo-independent; converted
  to wall-clock time at the current BPM during playback.
- `originalChord` must exist in the model from Phase 1 even though nothing reads
  it until Phase 4 (the styles are in their original keys, so the transposition
  reference has to be captured eventually).
- Include a **schema/version field** on `ArrangerStyle` for forward-compatible
  persistence.

### 5.5 Clock & seamless looping (the core technical work)
- A musical clock tracks **bar:beat:tick**;
  `loopLengthBeats = lengthBars × beatsPerBar`.
- Each tick: compute elapsed beats since start, `mod` into the loop window,
  convert due events to the current BPM, and send them.
- **Loop-seam correctness is the key risk.** At the wrap boundary the engine must
  ensure no notes hang — send proper note-offs (pair each note-on with its
  note-off, or emit all-notes-off per channel at the seam). Notes that straddle
  the boundary must be handled so nothing sticks.
- Phase 1 loop length = the style's content length quantized **up** to whole bars.
- Tempo changes mid-loop must keep the musical position continuous (reuse the
  approach already in `applyBPMchangeDuringPlayback`, adapted to musical time).

### 5.6 Integration
- The mode toggle lives in the existing settings. The play/stop controls in
  `CurrentStyleComponent` (or its owner) consult the mode and drive the right
  engine.
- A small **adapter** builds an `ArrangerStyle` with a single `ArrangerSection`
  from the current style's `TrackEntry` list (channel/instrument/volume mapping
  reused from `MultipleTrackPlayer`'s existing logic: percussion → ch10, melodic
  → ch2+).
- Replace `Display::initializeSectionsForStyle` and the `StyleSection` placeholder
  with the real model (or adapt the section-button wiring to it; full wiring is
  Phase 2).

### 5.7 Testing
- **Unit:** clock/loop math (beats↔bars↔time, wrap); loop-seam note-off
  correctness (no hung notes); tempo change mid-loop preserves position.
- **Integration:** a style with multiple `TrackEntry`s loops N times with zero
  hung notes; mode toggle routes to the correct engine and never disturbs classic
  playback.

### 5.8 Risks
- **Loop seam / hung notes** — primary risk; covered by 5.5 and tested in 5.7.
- **Timer accuracy / drift** — reuse the high-resolution timing already proven in
  `MultipleTrackPlayer`; musical position derived from elapsed time, not
  accumulated per-tick.
- **Existing styles may be full songs, not clean loops** — acceptable for Phase 1
  (looping the whole thing still demonstrates the engine); clean per-section loops
  are the Content & Sectioning phase's job.

---

## 6. Future decisions (deferred, recorded here)

- **Author vs import emphasis** for content (Phase 3) — both supported; which is
  primary affects tooling priority.
- **Original-chord capture** (Phase 4/5) — manual tag per style/section,
  auto-detect on import, or normalize patterns to C at import.
- **NTT depth** (Phase 5) — target is full Korg-grade (multiple NTT types); built
  incrementally.

---

## 7. Out of scope (whole project)
- DAW-style audio-clip timeline, crossfades, time-stretch, `MixerAudioSource`.
- Anything driven by an audio callback over audio files (this engine drives MIDI
  / SFZ).
