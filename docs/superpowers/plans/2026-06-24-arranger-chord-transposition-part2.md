> Part 2 of 2 of [2026-06-24-arranger-chord-transposition](2026-06-24-arranger-chord-transposition.md). Previous: [part 1](2026-06-24-arranger-chord-transposition.md).

### Task 6: Wire live MIDI input → detector → engine (scan-area seam + Chord Memory)

**Files:**
- Modify: `Source/Midi/MidiHandler.h`, `Source/Midi/MidiHandler.cpp`
- (No new unit test — `handleIncomingMidiMessage` is hardware/threaded; verified via the manual smoke below. The detector/transposer/engine logic is already unit-covered.)

**Interfaces:**
- Consumes: `ChordDetector` (Task 2), `ArrangerEngine::setActiveChord` (Task 5).
- Produces: `enum class ChordScanArea { Split, Full };` + `void MidiHandler::setChordScanArea(ChordScanArea);` and a pointer/reference to the active `ArrangerEngine` so the handler can push chords.

- [ ] **Step 1: Edit `MidiHandler.h`** — add scan area, detector, engine hook

```cpp
#include "Arranger/ChordDetector.h"
// forward declare or include ArrangerEngine
enum class ChordScanArea { Split, Full };
// ... in class:
    void setChordScanArea (ChordScanArea a) { chordScanArea = a; }
    void setArrangerEngine (ArrangerEngine* e) { arrangerEngine = e; }
private:
    std::atomic<ChordScanArea> chordScanArea { ChordScanArea::Split };  // UI writes, input thread reads
    ChordDetector  chordDetector;     // input-thread only — never shared
    ArrangerEngine* arrangerEngine = nullptr;  // set once at init
    bool inChordZone (int note) const;       // Split: note < rightHandBoundSetting; Full: always true
    void updateChordFromInput (const juce::MidiMessage& m);
```

- [ ] **Step 2: Edit `MidiHandler.cpp`** — feed the detector from `handleIncomingMidiMessage`

```cpp
bool MidiHandler::inChordZone (int note) const
{
    if (chordScanArea == ChordScanArea::Full) return true;
    return rightHandBoundSetting == -1 ? true : note < rightHandBoundSetting;  // left zone
}

void MidiHandler::updateChordFromInput (const juce::MidiMessage& m)
{
    if (arrangerEngine == nullptr) return;
    const int note = m.getNoteNumber();
    if (! inChordZone (note)) return;

    if (m.isNoteOn() && m.getVelocity() > 0)      chordDetector.noteOn  (note);
    else if (m.isNoteOff() || m.isNoteOn())       chordDetector.noteOff (note);  // note-on vel 0 = off

    Chord c = chordDetector.current();
    if (c.isValid())
        arrangerEngine->setActiveChord (c);   // Chord Memory: only push valid chords;
                                              // emptying the zone keeps the last chord (no push of none)
}
```
Call `updateChordFromInput(message)` near the top of `handleIncomingMidiMessage` (after note-number extraction), for note-on and note-off, **without** altering the existing right-hand pass-through behaviour.

- [ ] **Step 3: Wire the engine pointer** — in the owner that constructs both `MidiHandler` and the `ArrangerEngine` (search for where `ArrangerEngine` is created — `CurrentStyleComponent`/`MainComponent`), call `midiHandler.setArrangerEngine (&engine);` once during setup. Add the include.

- [ ] **Step 4: Build (twice if `.jucer` touched), run all arranger tests** — Expected: still all PASS (no test change; this is wiring).

- [ ] **Step 5: Manual smoke (Stage 1 acceptance)** — launch the app, arranger mode, load/Start a style, hold a left-hand note: the pitched backing shifts by the interval; drums don't; right-hand melody unaffected; lifting the hand keeps the last chord.

- [ ] **Step 6: Commit**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h
git commit -m "feat(arranger): drive accompaniment transposition from left-hand chord input"
```

**Stage 1 complete: end-to-end root transposition is audible.**

---

# STAGE 2 — Fingered chord recognition

Goal: `ChordDetector` returns true quality + extensions, inversion-correct.

---

### Task 7: Interval-template chord recognition

**Files:**
- Modify: `Source/Arranger/ChordDetector.cpp` (and `.h` if adding helpers)
- Test: `tests/unit/test_chord_detector.cpp` (extend)

**Interfaces:**
- Produces: `current()` now returns the real `ChordQuality` by matching the held pitch-class set against root-relative interval templates; `root` = template root (not lowest note); `bassNote` = lowest held pitch class.

- [ ] **Step 1: Write the failing test** — append cases

```cpp
        beginTest ("recognizes common triads and sevenths");
        {
            auto detect = [] (std::initializer_list<int> notes) {
                ChordDetector d; for (int n : notes) d.noteOn (n); return d.current();
            };
            expect (detect({60,64,67}).quality == ChordQuality::Maj);   // C E G
            expect (detect({60,63,67}).quality == ChordQuality::Min);   // C Eb G
            expect (detect({60,64,67,70}).quality == ChordQuality::Dom7);// C E G Bb
            expect (detect({60,63,67,70}).quality == ChordQuality::Min7);// C Eb G Bb
            expect (detect({60,63,66}).quality == ChordQuality::Dim);   // C Eb Gb
            expect (detect({60,65,67}).quality == ChordQuality::Sus4);  // C F G
            expectEquals (detect({60,64,67}).root, 0);                  // C
        }

        beginTest ("inversions resolve to the same chord and root");
        {
            auto detect = [] (std::initializer_list<int> notes) {
                ChordDetector d; for (int n : notes) d.noteOn (n); return d.current();
            };
            // E G C and G C E are both C major, root C
            expect (detect({64,67,72}).quality == ChordQuality::Maj);
            expectEquals (detect({64,67,72}).root, 0);
            expectEquals (detect({64,67,72}).bassNote, 4);   // lowest = E
            expectEquals (detect({67,72,76}).root, 0);
        }

        beginTest ("ambiguous/sparse set -> None");
        {
            ChordDetector d; d.noteOn (60); d.noteOn (61);   // C + C#: not a known chord
            expect (d.current().quality == ChordQuality::None || ! d.current().isValid());
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: FAIL (stage-1 returns Maj for everything).

- [ ] **Step 3: Implement template matching in `ChordDetector.cpp`**

```cpp
namespace
{
    struct Template { ChordQuality q; std::vector<int> intervals; };  // root-relative semitones, sorted, incl 0
    // Order matters: richer (more-note) templates first so 4-note chords win over their 3-note subsets.
    const std::vector<Template>& templates()
    {
        static const std::vector<Template> t = {
            { ChordQuality::Maj7,    {0,4,7,11} },
            { ChordQuality::Dom7,    {0,4,7,10} },
            { ChordQuality::Min7,    {0,3,7,10} },
            { ChordQuality::HalfDim, {0,3,6,10} },
            { ChordQuality::Maj,     {0,4,7}    },
            { ChordQuality::Min,     {0,3,7}    },
            { ChordQuality::Dim,     {0,3,6}    },
            { ChordQuality::Aug,     {0,4,8}    },
            { ChordQuality::Sus2,    {0,2,7}    },
            { ChordQuality::Sus4,    {0,5,7}    },
        };
        return t;
    }

    // Does the pitch-class set, rooted at `root`, equal this interval template?
    bool matches (const std::set<int>& pcs, int root, const std::vector<int>& intervals)
    {
        if ((int) pcs.size() != (int) intervals.size()) return false;
        for (int iv : intervals)
            if (pcs.find ((root + iv) % 12) == pcs.end()) return false;
        return true;
    }
}

Chord ChordDetector::current() const
{
    if (heldNotes.empty()) return {};

    std::set<int> pcs;
    for (int n : heldNotes) pcs.insert (n % 12);
    const int lowestPc = (*heldNotes.begin()) % 12;

    for (const auto& tpl : templates())
        for (int root = 0; root < 12; ++root)
            if (matches (pcs, root, tpl.intervals))
                return { root, tpl.q, lowestPc };

    return { -1, ChordQuality::None, lowestPc };   // unrecognized
}
```

- [ ] **Step 4: Run tests to verify they pass** — Expected: new + Stage-1 detector tests PASS. (Stage-1 "root = lowest" test from Task 2 now only holds for actual chords; if that earlier single-note test conflicts, update it to assert `isValid()==false` for a lone note, matching the new contract — a lone note matches no template.)
> **Implementer note:** the Task-2 test "root is the pitch class of the lowest held note (stage 1)" used a full C-major triad, so it still passes (root C). The single-note expectations remain `!isValid()`.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ChordDetector.cpp Source/Arranger/ChordDetector.h tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): fingered chord recognition via interval templates (quality + inversions)"
```

---

# STAGE 3 — NTT mapping (real Option 3)

Goal: replace block shift with per-note role → chord-tone mapping.

---

### Task 8: NTT transposition in `ChordTransposer`

**Files:**
- Modify: `Source/Arranger/ChordTransposer.h`, `Source/Arranger/ChordTransposer.cpp`
- Test: `tests/unit/test_chord_transposer.cpp` (extend)

**Interfaces:**
- Produces: `transpose(note, true)` now maps the note by its scale-degree role in `original` to the corresponding tone of `active` (chord-tone snapping), preserving octave register; non-transposable + invalid paths unchanged from Stage 1.

**Design of the mapping (implement exactly):**
1. Build the **chord-tone pitch-class set** for a `Chord` from its quality's intervals (reuse the same interval lists as the detector — extract them to a shared helper `chordIntervals(ChordQuality)` in `Chord.h` so detector and transposer agree).
2. For an incoming note: `pc = note % 12`, `octave = note / 12`. Compute `degreeOffset = (pc - original.root + 12) % 12`.
3. Find the **chord tone of `original`** nearest to `pc` (by pitch-class distance). Its index `k` in the original chord-tone list is the note's "role".
4. Map to `active`'s chord tone at the **same role index** `k` (clamp/wrap index to the active chord's tone count). Target pc = that active chord tone.
5. Reattach the original octave, then shift by whole octaves to land nearest the original `note` (preserve register); clamp 0..127.
6. **Bass Inversion (Stage 5 will toggle):** if the note's role is the root (k==0) and the part is bass and inversion is enabled, target `active.bassNote` instead of `active.root`. (Stage 3 ignores the bass flag; add the hook but default off.)

- [ ] **Step 1: Write the failing test** — append

```cpp
        beginTest ("NTT maps the 3rd differently from root/5th (C maj -> A min)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C E G
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A C E
            // root C(60)->A, 3rd E(64)->C (minor 3rd), 5th G(67)->E
            expectEquals (t.transpose (60, true) % 12, 9);   // A
            expectEquals (t.transpose (64, true) % 12, 0);   // C  (the minor 3rd, NOT C#)
            expectEquals (t.transpose (67, true) % 12, 4);   // E
        }

        beginTest ("identity when active == original");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 0, ChordQuality::Maj, 0 });
            expectEquals (t.transpose (64, true), 64);
        }

        beginTest ("drums still untouched, result in range");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            expectEquals (t.transpose (36, false), 36);
            const int x = t.transpose (64, true);
            expect (x >= 0 && x <= 127);
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: FAIL (block shift gives C#, not C, for the 3rd).

- [ ] **Step 3: Add `chordIntervals` helper to `Chord.h`**

```cpp
inline std::vector<int> chordIntervals (ChordQuality q)
{
    switch (q) {
        case ChordQuality::Maj:     return {0,4,7};
        case ChordQuality::Min:     return {0,3,7};
        case ChordQuality::Dom7:    return {0,4,7,10};
        case ChordQuality::Maj7:    return {0,4,7,11};
        case ChordQuality::Min7:    return {0,3,7,10};
        case ChordQuality::Dim:     return {0,3,6};
        case ChordQuality::HalfDim: return {0,3,6,10};
        case ChordQuality::Aug:     return {0,4,8};
        case ChordQuality::Sus2:    return {0,2,7};
        case ChordQuality::Sus4:    return {0,5,7};
        case ChordQuality::None:    return {0,4,7};   // safe default
    }
    return {0,4,7};
}
```
(Then refactor `ChordDetector`'s templates to call `chordIntervals` so the two never drift — optional but recommended; keep template ordering for match priority.)

- [ ] **Step 4: Implement NTT in `ChordTransposer.cpp`** (replace the block-shift body)

```cpp
#include <vector>
#include <cmath>

static int nearestChordToneIndex (int pc, int root, const std::vector<int>& iv)
{
    int best = 0, bestDist = 99;
    for (int k = 0; k < (int) iv.size(); ++k)
    {
        const int tonePc = (root + iv[k]) % 12;
        int d = std::abs (tonePc - pc); d = std::min (d, 12 - d);
        if (d < bestDist) { bestDist = d; best = k; }
    }
    return best;
}

int ChordTransposer::transpose (int noteNumber, bool transposable) const
{
    if (! transposable || ! active.isValid() || ! original.isValid())
        return noteNumber;

    const auto origIv = chordIntervals (original.quality);
    const auto actIv  = chordIntervals (active.quality);
    const int pc = noteNumber % 12;

    const int role     = nearestChordToneIndex (pc, original.root, origIv);
    const int actIndex = juce::jmin (role, (int) actIv.size() - 1);
    const int targetPc = (active.root + actIv[actIndex]) % 12;

    // place targetPc nearest to the original note's register
    int candidate = (noteNumber / 12) * 12 + targetPc;
    while (candidate - noteNumber >  6) candidate -= 12;
    while (noteNumber - candidate >  6) candidate += 12;
    while (candidate < 0)   candidate += 12;
    while (candidate > 127) candidate -= 12;
    return candidate;
}
```

- [ ] **Step 5: Run tests to verify they pass** — Expected: new NTT tests + Stage-1 transposer tests PASS (re-check the Stage-1 block-shift tests: the `C->D Maj` cases still map root→root etc. — for same-quality chords NTT reduces to a shift, so they remain green; if a Stage-1 case asserted a non-chord-tone note, adjust it to a chord tone).

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp Source/Arranger/Chord.h tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): NTT note-role to chord-tone transposition"
```

**Stage 3 complete: chord-aware transposition is live.**

---

# STAGE 4 — Original chord: persistence + auto-detect + editor

---

### Task 9: Style-level original chord in model + schema v3→v4 persistence

**Files:**
- Modify: `Source/Arranger/ArrangerModel.h`, `Source/Arranger/ArrangerStyleFile.h`, `Source/Arranger/ArrangerStyleIOHelper.cpp`
- Test: `tests/unit/test_arranger_style_io.cpp` (extend)

**Interfaces:**
- Produces: `ArrangerStyle` and `ArrangerStyleFile` gain `int originalRoot = 0; ChordQuality originalQuality = ChordQuality::Maj;`. `ArrangerStyleFile::schemaVersion` default 4. IO writes both; reading a file without them defaults to C major.

- [ ] **Step 1: Write the failing test** — append to `test_arranger_style_io.cpp`

```cpp
        beginTest ("v4 round-trips original chord");
        {
            ArrangerStyleFile f = makeMinimalStyleFile();   // existing helper in this test file
            f.schemaVersion = 4;
            f.originalRoot = 9; f.originalQuality = ChordQuality::Min;   // A minor
            auto json = ArrangerStyleIOHelper::toJson (f);
            ArrangerStyleFile back;
            expect (ArrangerStyleIOHelper::fromJson (json, back));
            expectEquals (back.originalRoot, 9);
            expect (back.originalQuality == ChordQuality::Min);
        }

        beginTest ("v3 file (no original chord) loads as C major");
        {
            ArrangerStyleFile f = makeMinimalStyleFile();
            auto json = ArrangerStyleIOHelper::toJson (f);
            // simulate an old file: remove the new keys
            auto* obj = json.getDynamicObject();
            obj->removeProperty ("originalRoot");
            obj->removeProperty ("originalQuality");
            obj->setProperty ("schemaVersion", 3);
            ArrangerStyleFile back;
            expect (ArrangerStyleIOHelper::fromJson (json, back));
            expectEquals (back.originalRoot, 0);
            expect (back.originalQuality == ChordQuality::Maj);
        }
```
> **Implementer note:** match the actual IO entry points in `ArrangerStyleIOHelper` (the existing tests already call them — reuse those exact names; `toJson`/`fromJson` above are illustrative).

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error (fields missing) / FAIL.

- [ ] **Step 3: Add fields** — `ArrangerModel.h` `ArrangerStyle` and `ArrangerStyleFile.h` (`#include "Chord.h"`), both:

```cpp
    int          originalRoot    = 0;                  // 0=C..11=B
    ChordQuality originalQuality = ChordQuality::Maj;  // recorded key, default C major
```
Bump `ArrangerStyleFile::schemaVersion = 4;`. Remove the now-dead `ArrangerTrack::originalChord` string (and any reference).

- [ ] **Step 4: Edit `ArrangerStyleIOHelper.cpp`** — serialize + default

```cpp
// writing:
obj->setProperty ("originalRoot", f.originalRoot);
obj->setProperty ("originalQuality", toString (f.originalQuality));
// reading (with v3 default):
out.originalRoot    = (int) obj->getProperty ("originalRoot");          // absent -> 0
out.originalQuality = chordQualityFromString (obj->getProperty ("originalQuality").toString());
if (out.originalQuality == ChordQuality::None) out.originalQuality = ChordQuality::Maj;  // v3/default
```
Map the file fields into the runtime `ArrangerStyle` wherever the helper converts `ArrangerStyleFile` → `ArrangerStyle` (carry `originalRoot`/`originalQuality` across).

- [ ] **Step 5: Run tests to verify they pass** — Expected: new + existing IO tests PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerModel.h Source/Arranger/ArrangerStyleFile.h Source/Arranger/ArrangerStyleIOHelper.cpp tests/unit/test_arranger_style_io.cpp
git commit -m "feat(arranger): persist style-level original chord (schema v4, v3 defaults to C major)"
```

---

### Task 10: Engine reads style original chord; auto-detect helper

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.cpp` (`setStyle`), `Source/Arranger/ChordDetector.h`/`.cpp` (add a static key-finder) OR a small free function in `Chord.h`
- Test: `tests/unit/test_chord_detector.cpp` (extend for the key-finder)

**Interfaces:**
- Produces:
  - In `ArrangerEngine::setStyle`: `transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });`
  - `Chord detectKeyFromEvents (const std::vector<TimedBeatEvent>& pitchedEvents);` — pitch-class histogram → best root+quality; empty/ambiguous → C major. Used at authoring time (Task 11).

- [ ] **Step 1: Write the failing test** — append to `test_chord_detector.cpp`

```cpp
        beginTest ("key finder picks C major from a C-major-heavy pattern");
        {
            std::vector<TimedBeatEvent> evs;
            for (int n : { 60,64,67, 60,64,67, 60, 67 })
                evs.push_back ({ 0.0, juce::MidiMessage::noteOn (2, n, (juce::uint8)100) });
            Chord k = detectKeyFromEvents (evs);
            expectEquals (k.root, 0);
            expect (k.quality == ChordQuality::Maj);
        }

        beginTest ("key finder falls back to C major on empty input");
        {
            Chord k = detectKeyFromEvents ({});
            expectEquals (k.root, 0);
            expect (k.quality == ChordQuality::Maj);
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error / FAIL.

- [ ] **Step 3: Implement `detectKeyFromEvents`** (declare in `ChordDetector.h`, define in `.cpp`)

```cpp
Chord detectKeyFromEvents (const std::vector<TimedBeatEvent>& events)
{
    int hist[12] = {0};
    for (const auto& e : events)
        if (e.message.isNoteOn() && e.message.getVelocity() > 0)
            hist[e.message.getNoteNumber() % 12]++;

    int total = 0, root = 0, best = -1;
    for (int i = 0; i < 12; ++i) { total += hist[i]; if (hist[i] > best) { best = hist[i]; root = i; } }
    if (total == 0) return { 0, ChordQuality::Maj, 0 };   // fallback C major

    // major vs minor: compare the third above the root
    const int majThird = hist[(root + 4) % 12];
    const int minThird = hist[(root + 3) % 12];
    const ChordQuality q = (minThird > majThird) ? ChordQuality::Min : ChordQuality::Maj;
    return { root, q, root };
}
```

- [ ] **Step 4: Edit `ArrangerEngine::setStyle`** — after building, set the original chord:

```cpp
transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });
```

- [ ] **Step 5: Run tests to verify they pass** — Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp Source/Arranger/ArrangerEngine.cpp tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): engine reads style original chord; add key-finder for auto-detect"
```

---

### Task 11: Editor — original-chord combos, auto-detect pre-fill, persistence

**Files:**
- Modify: `Source/Arranger/ArrangerStyleEditor.h`/`.cpp`, `Source/Styles/CurrentStyleComponent.cpp`
- (No new unit test — UI; covered by manual smoke. Logic — key finder — is unit-tested in Task 10.)

**Interfaces:**
- Consumes: `detectKeyFromEvents` (Task 10), style-level fields (Task 9).
- Produces: editor exposes root + quality combos bound to the working style's `originalRoot`/`originalQuality`, pre-filled by auto-detect on `loadRecording`/Update Tracks, saved through the existing save path.

- [ ] **Step 1: Add combos to `ArrangerStyleEditor.h`** — `juce::ComboBox originalRootBox, originalQualityBox;` add to the `addAndMakeVisible` toolbar list; lay out in `resized()` next to `nameEditor` (drop to a second toolbar row if width is tight).

- [ ] **Step 2: Populate + bind** — fill `originalRootBox` with C..B (ids 1..12), `originalQualityBox` with maj/min (+ the other qualities, ids matching `ChordQuality`). On change, write into the working style's `originalRoot`/`originalQuality`.

- [ ] **Step 3: Pre-fill from auto-detect** — in `loadRecording` and after Update Tracks, call `detectKeyFromEvents` over the pitched (non-drum) source-track events, set the combos (fallback C major), and store into the working style.

- [ ] **Step 4: Persist** — ensure the working style's `originalRoot`/`originalQuality` are written when building the `ArrangerStyleFile` for save (Task 9 added the fields; confirm the editor's save path carries them).

- [ ] **Step 5: Build (twice if `.jucer` touched), run all arranger tests** — Expected: PASS (unchanged).

- [ ] **Step 6: Manual smoke** — author a config in a non-C recording → combos pre-fill to the detected key; edit them; Save; reload → key persists; Start → transposition uses the saved original chord.

- [ ] **Step 7: Commit**

```bash
git add Source/Arranger/ArrangerStyleEditor.h Source/Arranger/ArrangerStyleEditor.cpp Source/Styles/CurrentStyleComponent.cpp
git commit -m "feat(arranger): editor original-chord combos with auto-detect and persistence"
```

---

# STAGE 5 — Extras (Chord Memory toggle, Bass Inversion)

---

### Task 12: Bass Inversion (engine + transposer hook + toggle)

**Files:**
- Modify: `Source/Arranger/ChordTransposer.h`/`.cpp` (bass-aware target), `Source/Arranger/ArrangerScheduler.*` + `ArrangerEngine.*` (carry an `isBass` distinction), `Source/Styles/CurrentStyleComponent.*` (toggle)
- Test: `tests/unit/test_chord_transposer.cpp` (extend)

**Interfaces:**
- Produces: `ChordTransposer::setBassInversion(bool)`; when on, a **bass-part root-role** note targets `active.bassNote` instead of `active.root`. Requires distinguishing Bass from Acc at emit — extend the scheduler tag from `char transposable` (0/1) to a small code: `0=fixed, 1=acc, 2=bass`, and pass the part kind into `transpose`.

- [ ] **Step 1: Write the failing test**

```cpp
        beginTest ("bass inversion targets the played bass note for the bass root");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 0, ChordQuality::Maj, 4 });   // C major, bass = E (C/E)
            t.setBassInversion (true);
            // a bass root note C should now sound E
            expectEquals (t.transposeBass (36, /*isBassRoot…*/ true) % 12, 4);  // see note
        }
```
> **Implementer note:** choose the exact API — either add a `PartKind` param to `transpose(note, PartKind)` (`Fixed/Acc/Bass`) and branch internally, or a dedicated `transposeBass`. Keep one path; update Task-5/8 call sites and the scheduler tag type accordingly. Update earlier transposer tests to pass `PartKind::Acc` where they passed `true`.

- [ ] **Step 2: Run test to verify it fails.**

- [ ] **Step 3: Implement** — thread a `PartKind` from `rebuildFromStyle` (Bass→bass, Acc→acc, Drum/Perc→fixed) through the scheduler tag and `EmittedEvent`, into `ChordTransposer`. When bass inversion is on and the note's role is the root of a bass part, set target pc = `active.bassNote` (fallback to `active.root` when `bassNote < 0`).

- [ ] **Step 4: Add the UI toggle** — a "Bass Inversion" switch in the arranger controls calling `engine.setBassInversion(bool)` (off by default). Persist the preference like `ArrangerAutoFillEnabled` if desired.

- [ ] **Step 5: Run tests + manual smoke** — play an inversion with bass inversion on → slash-chord bass; off → root bass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): bass inversion (slash chords) with UI toggle"
```

---

### Task 13: Chord Memory toggle (off = revert to original chord on release)

**Files:**
- Modify: `Source/Midi/MidiHandler.h`/`.cpp`, `Source/Styles/CurrentStyleComponent.*`
- (No unit test — input/threaded; manual smoke.)

**Interfaces:**
- Produces: `MidiHandler::setChordMemory(bool)` (default true). When OFF and the chord zone empties, push the style's original chord (identity) instead of holding the last chord.

- [ ] **Step 1: Implement** — in `updateChordFromInput`, when the zone becomes empty: if memory on → push nothing (hold); if memory off → `arrangerEngine->setActiveChord(originalChordOfActiveStyle)` (expose the style's original chord to the handler, or send an "identity" sentinel the engine treats as original).

- [ ] **Step 2: Add the toggle UI** — a "Chord Memory" switch (default on), wired to `setChordMemory`.

- [ ] **Step 3: Build, run all arranger tests** — Expected: PASS (unchanged).

- [ ] **Step 4: Manual smoke** — memory on: lift hand → backing holds the chord; memory off: lift hand → backing returns to the recorded key.

- [ ] **Step 5: Commit**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h
git commit -m "feat(arranger): chord memory toggle (hold vs revert on release)"
```

---

# STAGE 6 — Full-Keyboard mode (chord/melody inference, tuned by ear)

---

### Task 14: Full-Keyboard inference layer

**Files:**
- Modify: `Source/Arranger/ChordDetector.h`/`.cpp` (or a new `FullKeyboardChordFilter`), `Source/Midi/MidiHandler.cpp`
- Test: `tests/unit/test_chord_detector.cpp` (extend for the deterministic parts)

**Interfaces:**
- Produces: when `ChordScanArea::Full`, an inference step decides which held notes are chord vs melody before recognition: minimum chord size (≥3 notes forming a known chord), held-duration/recency weighting, and hysteresis (don't change the recognized chord unless evidence is strong).

- [ ] **Step 1: Write the failing test (deterministic rules only)**

```cpp
        beginTest ("full-keyboard: a lone melody note does not change the chord");
        {
            // Hold a C major chord, then add one high passing note: chord stays C major.
            ChordDetector d;
            d.setFullKeyboardMode (true);
            for (int n : {48,52,55}) d.noteOn (n);     // C major low
            Chord before = d.current();
            d.noteOn (84);                              // high melody note
            expect (d.current() == before);            // unchanged
        }

        beginTest ("full-keyboard: requires >=3 notes forming a known chord");
        {
            ChordDetector d; d.setFullKeyboardMode (true);
            d.noteOn (60); d.noteOn (84);
            expect (! d.current().isValid());          // not enough chord evidence
        }
```

- [ ] **Step 2: Run test to verify it fails.**

- [ ] **Step 3: Implement the inference** — in Full mode, before template matching: require ≥3 notes; try recognizing the **lowest 3–4 sustained notes** first; keep the previously recognized chord (hysteresis) unless a new full chord is clearly present. Add `setFullKeyboardMode(bool)` and the held-note metadata (timestamps via a monotonic counter incremented per `noteOn`).

- [ ] **Step 4: Wire the mode** — `MidiHandler::setChordScanArea(Full)` forwards **all** notes and calls `chordDetector.setFullKeyboardMode(true)`.

- [ ] **Step 5: Run tests + extended manual tuning** — play melody+chord together; adjust thresholds (min notes, hysteresis strength) by ear until the accompaniment doesn't lurch on melody notes but tracks real chord changes.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp Source/Midi/MidiHandler.cpp tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): full-keyboard chord/melody inference mode"
```

**Phase 4 complete.** Final step: update the wiki (`Arranger Phase Roadmap` → Phase 4 done, Phase 5/6 next) and `hot.md`/`log.md` per the wiki sync process — separate from this plan, on user request.

---

## Self-review notes (coverage map)

- Spec §4.1 Chord → Task 1; §4.2 ChordDetector → Tasks 2,7,14; §4.3 ChordTransposer → Tasks 3,8,12; §4.4 tagging → Tasks 4,5; §4.5 mailbox + emit + note-off matching → Task 5; §4.6 input wiring + scan area + memory → Tasks 6,13,14; §4.7 persistence v3→v4 → Task 9; §4.8 editor + auto-detect → Tasks 10,11; §5 threading → Task 5 (mailbox) + Task 6 (input thread only writes); §7 testing → per-task tests; §8 risk (note-off pitch) → Task 5 test "note-off matches transposed pitch"; §10 stages → Stage headers 1–6.
- All six stages have at least one independently testable deliverable. UI-only tasks (6,11,13) are explicitly marked manual-smoke with their underlying logic unit-tested elsewhere.
