# Note rendering performance — research & benchmark spec

Status: **implemented** on `perf/note-layer-gpu` (decision: `sdf-dwm`, approved 2026-11-21)

## Question
Is `NoteLayer`'s falling-note rendering a real performance problem, and if so which rendering
approach is the best fit for this app?

## How it works today (verified in code)
- `NoteLayer` attaches a `juce::OpenGLContext` **with component painting enabled** (the default).
- Notes are drawn in `paint()` with `juce::Graphics`: per note a gradient `fillRoundedRectangle` +
  `drawRoundedRectangle`, and for held notes two more gradient/rounded fills (amber pulse).
  Particles are drawn with raw GL points in `renderOpenGL()`.
- Animation runs on a 60 Hz `juce::Timer` with a **fixed** `dt = 1/60`, and positions are rounded to
  `int` pixels.
- JUCE 8.0.13 `CachedImage::renderFrame`: every frame triggered by `repaint()`/`triggerRepaint()`
  takes the **MessageManager lock** and holds it across `renderOpenGL()` + `paintComponent()`
  (JUCE's GL 2D renderer rasterises each rounded path into an edge table on the CPU).
  So paint cost lands on the message thread too, which is where all of the UI and arranger
  callbacks run.

## Hypotheses to test (each must be confirmed or rejected by measurement)
- H1: per-frame lock hold time grows with note count and blocks the message thread enough to matter.
- H2: timer-driven fixed-`dt` animation produces uneven motion (frames with 0 or 2 animation steps).
- H3: rounded paths are the dominant part of the paint cost (vs. rectangles).

## Variants
| id | description |
|---|---|
| `paint` | Faithful copy of the current `NoteLayer` (baseline). |
| `rects` | Same pipeline, but plain rects instead of rounded paths. Isolates H3. The cheapest possible fix. |
| `sdf` | Component painting **off**; notes drawn in `renderOpenGL()` as one batched draw with a rounded-rect SDF shader reproducing the same look; simulation on the render thread with measured `dt`, vsync-paced continuous repaint while anything is alive; no MessageManager lock. |

## Scenarios
- `typical` — melody ~6 notes/s plus a 3–4 note left-hand chord every second.
- `stress` — 20 notes/s melody, 10-note chords twice a second, and a full 88-key glissando every 3 s.
Same deterministic (seeded) script for every variant. Notes are injected from a separate thread
via `callAsync`, the way `MidiHandler` → `NoteLayer` does it. Particles (dust) are on.

## Metrics
- **Message-thread latency**: a probe thread posts `callAsync` every 2 ms; p50 / p99 / max delay.
- **Lock / render CPU per frame**: `renderOpenGL` start → `paint` end (`paint`, `rects`), or
  `renderOpenGL` duration (`sdf`); mean / p99.
- **Frame pacing**: presented-frame interval mean / p99 and count > 25 ms.
- **Animation evenness**: animation time advanced per presented frame; stddev and the share of
  frames that advanced 0 or ≥ 2 ticks.
- **Process CPU**: process CPU time ÷ wall time over the measured window.
- **Visual parity**: screenshot of each variant frozen at the same scripted instant, without particles.

Release build, 3 repetitions per cell, first second discarded as warm-up.

## Acceptance for a recommendation
A variant is only worth adopting if it improves a metric the user can feel (message-thread
latency, dropped/uneven frames) by a clear margin *and* the parity screenshots look the same.
If the baseline is already well within budget on realistic loads, the recommendation is to leave it alone.

## Results (2026-11-21)

Machine: Ryzen 7 7435HS, RTX 4060 Laptop, 1920×1080 @ 60 Hz, 125 % scaling; bench window 1536×546
logical (1920×682 px). Harness: `notebench` (JUCE 8.0.13, CMake), kept outside the repo. Added
variants beyond the spec: `paint-swap0` (baseline + `setSwapInterval(0)`), `sdf-vblank`
(frames triggered by `juce::VBlankAttachment`), `sdf-dwm` (render thread paces itself with `DwmFlush()`).

**Session lock:** the first Release matrix most likely ran with the Windows session locked. It was
re-run with the session unlocked (same harness, 3 reps per cell, `paint-swap0` included) and
confirms every conclusion; those unlocked numbers are the ones below.

### Release, unlocked session, median of 3 runs (stress = ~93 live notes)
| variant | CPU % of a core (typ / stress) | frame work ms p99 (typ / stress) | msg-thread latency p99 ms (typ / stress) |
|---|---|---|---|
| paint (current) | 28 / 31 | 3.1 / 9.4 | 4.6 / 8.8 |
| paint-swap0 | 15 / 28 | 3.3 / 9.2 | 5.6 / 8.8 |
| rects | 27 / 36 | 2.3 / 4.4 | 4.5 / 7.1 |
| sdf (continuous) | 31 / 27 | 0.14 / 0.19 | 0.22 / 0.14 |
| sdf-vblank | 27 / 28 | 0.14 / 0.19 | 0.16 / 0.15 |
| **sdf-dwm** | **2.1 / 1.9** | **0.68 / 0.47** | **0.20 / 0.23** |

All variants hold 60 fps in Release on this machine. Animation-step error is not tabulated: for
the `sdf*` variants it is zero by construction (they step by measured `dt`), so it is not evidence.

### Debug build (how the app is run day to day), 2 runs each
| variant | typical: fps / work p99 / msg-lat max | stress: fps / work p99 / msg-lat max |
|---|---|---|
| paint (current) | 59.5 / 11 ms / 14–23 ms | **17.5–18.2 fps / 88 ms / 91–96 ms** |
| sdf-dwm | 60 / 0.4 ms / 0.8 ms | 59.6–59.8 / 0.7 ms / 0.8 ms |

### Findings
- **H1 confirmed.** Frame cost scales with note count and is paid while holding the MessageManager
  lock. Release: ≤ 9 ms p99 (not user-visible on this machine). Debug: 50–88 ms per frame under stress
  → 18 fps and a UI frozen for up to ~90 ms at a time. Audio/arranger timing is **not** affected:
  `ArrangerEngine` and `MultipleTrackPlayer` run on `HighResolutionTimer` threads.
- **H2 weakly confirmed.** The fixed-`dt` timer gives 1–3 repeated/skipped animation steps per 9 s, plus
  integer-pixel snapping. Minor.
- **H3 confirmed but not sufficient.** Plain rects halve the paint cost but keep the lock and change the look.
- **Unexpected main CPU cost: the swap wait, not the drawing.** Every variant that blocks in
  `SwapBuffers` for vsync costs ~25–35 % of a core, even when its drawing takes 0.06 ms (the NVIDIA
  driver spin-waits). Only pacing with `DwmFlush()` before rendering removes it (→ 2–3 %).
  `setSwapInterval(0)` alone helps only at light load.
- **Visual parity.** GL read-back of `paint` vs `sdf-dwm` frozen at the same scripted instant: same
  gradient, outline, corner radius, amber glow and top line; 1.3 % of pixels differ by > 8/255, mostly
  sub-pixel position (sdf moves in float pixels, the current code snaps to ints).

### Decision
Adopt **`sdf-dwm`**. Notes drawn in `renderOpenGL()` with the rounded-rect SDF shader, component
painting off, note events through a small mutex-guarded queue, simulation on the render thread with
measured `dt`, `DwmFlush()` pacing on Windows (vsync swap elsewhere), and rendering stops when idle.
Implementation outline (TDD):
1. Extract the note simulation (events → geometry, `step(dt)`) into a pure class, unit-tested in
   `ci/Tests.jucer` (rise, clamp, release, fade, erase, reset, out-of-range notes), following ADR-001.
2. `NoteLayer` keeps its public API; setters (`resetState`, particle colour/style/toggle) go through
   the queue, because the render thread no longer holds the message lock.
3. Manual test plan for the GL side: look, overlays (loading label, ESC menu, authoring overlays),
   resize, hide/show, particle styles.

Smaller alternatives, if a rewrite isn't wanted: `paint-swap0` (one line, halves CPU at light load,
fixes nothing else); `rects` (half the lock time, loses the rounded look).

## Verification of the implementation
- `tests/unit/test_note_field.cpp` — the motion rules (41 assertions), red against a stub first.
- `tests/integration/test_note_layer_integration.cpp` (local, needs a display) — the real
  `KeyboardUI` + `NoteLayer` on screen, pixels read back: notes in the right key columns, cleared
  after release, still drawing after the hide/show cycle that re-creates the GL context.
- Manual: `docs/superpowers/test-plans/2026-11-21-note-layer-gpu-testing.md`.
