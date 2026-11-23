# UI redesign phase 1 — 1:1 visual check against `StageRevised`

Plan `docs/superpowers/plans/2026-11-23-ui-redesign-phase1-panel.md`, Task 9; spec §0.2. Done 2026-11-23 on
branch `feat/ui-stage-panel`.

## How it was checked

- **App:** Debug build, the owner's machine, maximised window = **1920 × 991** client (the spec assumed
  1920 × 1032; the owner's taskbar leaves 991). Stage scale = min(1920 / 1200, 991 / 645) = **1.536**.
  The agent drove the running app itself: screenshots of the window's client area and synthetic mouse /
  keyboard input (largest window of the process: the Sound Effects window is a separate desktop window
  with the same title). `%APPDATA%` backed up before and restored after (0 differences).
- **Board:** `specs/assets/2026-11-23-ui-redesign/render/render.sh` renders `StageRevised` at the same
  scale (headless Edge + the stand-in canvas runtime), so app and board are compared pixel for pixel.
- **Offscreen renders** of `StagePanel` (the same component code, software renderer) were used where the
  app could not reach a state: the beat lamps while playing (the owner's styles have no tracks).

## Results

| Check | Result | Image |
|---|---|---|
| Whole panel, live app vs board | positions, sizes, fonts and colours match; differences are state only (no style open, not playing, knob values) | `01-live-panel-vs-board.png` |
| Panel zoomed, same state | match to the pixel after the fixes below | `02-render-vs-board-zoomed.png` |
| Beat lamps, beats 1 and 2 | red downbeat, yellow beat 2, same glow | `03-lamps-beat1-beat2.png` |
| Screen message | short message where STYLE was, PLAY and 4/4 · TEMPO stay (as the board); a message too long for that takes the row and scrolls | `04-message.png` |
| Tempo editor | light digits on a rounded dark field; the selection is a faint tint | `05-tempo-editor.png` |
| Window sizes 1280 × 720, 1024 × 768, 800 × 600, 1900 × 500 | everything visible, nothing overlapping, panel scaled as one piece and centred | `06-window-sizes.png` |
| LED glow (board left, app right of each pair) | same fall-off; energy in a 5–10 px ring within 3 % (Var 1) | `07-led-glow.png` |

Behaviour checked live: every section button (LED + screen), Auto Fill, Sync Start, Rec on / off (messages),
Start / Stop (no style: today's "no tracks" box), tempo + / hold (121 → 152 in 1.5 s, the planned speed-up) /
wheel / right-click (back to 120 + message) / type 95 + Enter / −, each page button and its LED and screen
mode, Esc closing pages (also from the chord search box), knob turning both ways, Part toggles.

## Differences found and fixed

1. Knob turned against the hand (vertical drag fought circular motion) → relative circular turning.
2. LED glow cut off on the left → every button keeps room for its glow.
3. LED glow ~3× the board's CSS blur → Gaussian fall-off (CSS blur radius = 2 σ).
4. Every silkscreen label one letter-spacing too wide (JUCE's measured width already holds the spacing
   after the last letter) → Part row and screen-button labels were up to 5 px off; now exact.
5. Tempo editor showed an inverted light block → board look.
6. Chord helper opened as a call-out box over the panel and kept Esc → Chords page.
7. A screen message took the whole top row → only when it does not fit beside the page name.

## Not a difference

- Text looks a touch heavier on the board: Chrome draws ClearType colour fringes, JUCE greyscale. Same
  colour peak (200, 202, 205) and stem width.

## Repeatable manual check (Stage wiring, `MainComponentStage.cpp`)

The wiring needs the whole app (login, audio, MIDI), so it has no unit test; repeat this after changing it.
Back up `%APPDATA%\Arranger Workstation` first and restore it after.

1. Log in, Play. Screen: `No style`, `PLAY`, `4/4 · TEMPO`, `120`, `BAR 01`; all LEDs whole (glow not cut).
2. Start with no style open → "OPEN A STYLE FIRST (STYLES)" on the screen.
3. Styles → page opens, Styles LED on, screen `STYLE EDIT`, notes hidden; open a style → its name on the screen.
   Esc → page closes, notes back, the PC keyboard plays at once.
4. Var 2, Auto Fill, Sync Start → their LEDs; Var 2 also on the screen's bottom row.
5. Rec on / off → red LED; "RECORDING STARTED!" / "STOPPED!" beside `PLAY` (not over it).
6. Tempo +: one press = +1; hold 1.5 s → the screen climbs (~+30) and stays after release; the Styles page
   shows the same BPM. Wheel over the tempo ±1 per notch; right-click → style tempo + message; click,
   type 95, Enter → 95.
7. Part Right → LED, knobs jump to the right hand's values; turn a knob around its centre both ways.
   Part off → turning a knob says "CHOOSE A PART" and the knob springs back.
8. Effects / Settings / Chords / File → each opens, LED on, pressing again closes; in Chords type in the
   search box then Esc → the page closes. Voices → instrument list. Home → home screen.
9. Restore down and resize (e.g. 800 × 600, 1900 × 500) → everything visible, nothing overlapping.
10. With a hardware keyboard: play chords while a style runs → the chord shows on the screen, no assert.

## Not checked

- Real playback (lamps / bar moving with the engine): the owner's library has no tracks. Lamps were checked
  on offscreen renders of the same component.
