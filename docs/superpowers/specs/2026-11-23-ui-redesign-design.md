# Arranger UI redesign: "Stage" panel, pages, keys, dust, finishes, sign-in — design

**Status:** owner's open questions answered (§14); awaiting approval of the whole spec · **Date:** 2026-11-23 · **Owner:** KunMihai
**Part 2** (every page in detail, the integration map): `2026-11-23-ui-redesign-design-part2.md`

**Accepted design (the visual source of truth):**
- Interactive boards: the owner's private canvas "Arranger UI directions",
  `https://claude.ai/artifact/S5iLYs2CyD1P6hZh1SJdQ4`, version `1795397183-44dd` or later. Only the boards
  listed in §1.3 are accepted; the rest of the canvas is history (rejected directions).
- Exact sources of those boards, committed with this spec:
  `docs/superpowers/specs/assets/2026-11-23-ui-redesign/*.dc.html`. Every colour, size, font and timing
  in this document comes from them. **When this document and a board disagree, the board wins**, and the
  document gets corrected.

---

## 0. Two rules the owner set for this work (read first)

These are the owner's explicit conditions. They override any shortcut a plan might suggest.

### 0.1 Behaviour first: the app must stay fully integrated

The redesign moves almost every control in the app. **Before building a page, the builder proves that
every existing behaviour behind it is wired into the new place**, and that nothing the app does today is
lost or changes meaning by accident.

- The integration map (part 2, §P.13) lists every control the app has today, where it goes and which code
  owns it. Before building each phase, the builder re-checks the rows for that phase **against the code**
  (not against this document) and adds any control the map missed.
- Each page is a new *view* over the existing logic. The logic (engine, sequencer, track player, MIDI,
  SFZ, settings, persistence) is reused through the callbacks and methods it already has. If a page needs
  a behaviour change, it is listed in §0.3; anything not listed must behave exactly as today.
- Persisted data keeps its format: the keys in `Source/Common/PersistedKeys.h` and the files in
  `%APPDATA%\Arranger Workstation` (`allStyles.json`, `myTracks.json`, `mySections.json`,
  `SFZLibrary.json`, `myDevices.json`, `Arranger Workstation.settings`) must load unchanged. New settings get
  new keys; no existing key changes text.
- Each phase ends with the full test run green (unit, arranger, integration, display) **and** a live check
  in the Debug app of every behaviour that phase touched (play, switch section, record, change instrument…),
  following the safety rules in `AGENTS.md` (back up and restore `%APPDATA%`, input only when the app is
  in front).

### 0.2 Visual check before finishing: 1:1 with the accepted design

**Nothing in this redesign is finished until the agent has looked at the running app and compared it,
screen by screen, with the accepted boards.** Passing tests are not enough.

Procedure for every screen state listed in §1.3:

1. Build the Debug app; back up `%APPDATA%\Arranger Workstation`.
2. Run the app at the **reference size**: a client area of 1920 × 1032 (1.6 × the 1200 × 645 boards). On a
   different screen, use any size with the same 1.86 aspect ratio and scale the comparison.
3. Put the app in the state of the board (same page, same selection, same example values where possible).
4. Take a screenshot of the client area and place it **side by side** with the board rendered at the same
   size (reference PNGs, see §0.4).
5. Check, region by region: position and size of every element (within 2 px at reference size), colours
   (the exact hex values of §3), fonts and sizes, LED / lamp / pressed states, text, alignment, spacing.
6. Every difference is either fixed, or written down as a deliberate deviation **and approved by the
   owner**. The phase is done when the list is empty.
7. Save the screenshots and the comparison notes in the phase's test plan
   (`docs/superpowers/test-plans/`), restore `%APPDATA%`, verify 0 differences.

The owner then does the final visual sign-off on the same screens.

### 0.3 Behaviour changes this redesign makes on purpose

Only these. Everything else behaves exactly as today.

| # | Change | Where |
|---|---|---|
| B1 | Pop-up notifications ("Recording started!", "Playback stopped!"…) show on the panel screen for 2.5 s instead | §5.4 |
| B2 | Tempo: hold-to-repeat, click-to-type, mouse wheel, right-click = style tempo | §5.5 |
| B3 | New Dust particle motion (Sparks and Smoke unchanged) | §8 |
| B4 | Panel finishes (skins), chosen in Settings | §9 |
| B5 | Remember me (stay signed in), remembered email, Log out | §10 |
| B6 | Track picker: an explicit "Use in slot N" / "Open folder" button next to the existing double-click | part 2 §P.4 |
| B7 | Sections editor: inspector with exact start/length/type/"when it ends" controls, jump pads, zoom | part 2 §P.6 |
| B8 | Style list: Add / Rename / Delete as buttons (today hidden in a right-click menu, which stays too) | part 2 §P.3 |
| B9 | Particle style selector (Sparks / Dust / Smoke); today nothing calls `NoteLayer::setParticleStyle`, so it is always Dust | part 2 §P.2 |

### 0.4 Reference images

The boards render only inside the canvas. **Phase 0 of the plan exports each accepted board as a PNG at
2×** into `docs/superpowers/specs/assets/2026-11-23-ui-redesign/png/`: either the owner uses the canvas's
Share › Export, or the agent opens the canvas in the owner's browser (with the owner's permission) and
captures each board in its focused view. Those PNGs are what §0.2 compares against.

App screenshots for §0.2 are screen captures of the app window's client area (GDI copy of the screen
rectangle, the method already used for live checks), which includes the GL note area.

---

## 1. Intent

### 1.1 Why

The owner's words: the UI must **not look AI-generated**. It should look like real, human-designed music
gear, like a hardware arranger keyboard. Today it has generic tropes: coloured arcs with a glowing dot on rounded
cards (Effects), flat white key rectangles that turn green, purple/navy cards. The owner wants realistic
keys, real knobs, a refined look everywhere, decorative finishes in the spirit of Korg's limited
editions, and smooth particles.

### 1.2 Goals, non-goals, anti-goals

- **Goals:** a slim hardware-style top panel with a centred, bigger screen; every feature reachable from
  the panel or a page in the same style; realistic keys; real knobs, faders, rubber buttons with LEDs; a
  beat counter in the app's own colours; smoother dust; selectable panel finishes; stay-signed-in.
- **Non-goals:** new music features (engine, sequencer, SFZ, MIDI stay as they are); changing saved file
  formats; touch / tablet layouts; a light theme.
- **Anti-goals** (a failure even if it works): anything that reads as generic app UI (glow gradients, neon,
  uniform rounded cards, emoji icons); copying any brand's design or trade dress; losing or hiding a
  feature that exists today; a "futuristic" or ultra-modern look (owner: it must suit an arranger).

### 1.3 Accepted screens (one board each, all in the committed sources)

| Board (file) | What it shows |
|---|---|
| `StageRevised` | Play screen: panel, screen, beat lamps, falling-notes area, keys. Clickable: every page opens from it |
| `StageEffects`, `StageSettings`, `StageStyles`, `StageVoices` | The play screen with that page open |
| `PageTracks`, `PageNotes`, `PageSections`, `PageChords`, `PageFile` | Track picker, Notes information, Sections editor v2, Chord helper, Recorder |
| `StageLogin`, `PageLogin`, `StageHome`, `PageHome` | Log in / sign up (with Remember me) and Home, on the instrument |
| `Dust` | The new dust motion (with the current dust for comparison) |
| `SkinsFinish` | The finishes: rows 1-4 and 6 (row 5, Pearl accordion, is not built) |

Rejected and **not** to be built: directions B (vintage) and C (studio metal), skins v1 and v2, the
first sections editor, beat numbers "1 2 3 4" in the screen.

---

## 2. Approaches considered

| Topic | Options shown | Chosen (owner) | Why |
|---|---|---|---|
| Overall look | A stage workstation (matte charcoal) · B vintage console (cream + walnut) · C studio metal | **A, revised** | Owner: "I want A but a bit different, better": slimmer panel, centred bigger screen |
| Where effects live | All on the panel · quick knobs on the panel + full page | **4 quick knobs + Effects page** | How real arrangers do it; panel stays slim |
| Beat counter | Squares in the screen · digits 1-2-3-4 · coloured lamps | **Lamps in the screen bezel, app's colours** | Owner rejected digits and the dull squares; wants the look of today's beat bar |
| Skins | v1 contour/walnut/woven · v2 riso/terrazzo/signal/livery · v3 instrument finishes | **v3** (owner: "decent") | Korg-like quality: real finishes, not printed art; not futuristic |
| Sections editor | Blocks in sequence · windows over the recording | **Windows over the recording (v2)** | v1 did not match how the editor works (`SectionWindow` = start bar + length over the recording) |
| Tempo | ± buttons only · hold · type · wheel · tap · reset | **Hold, type, wheel, reset** (no tap) | Owner: "1 2 3 and 5" |
| Remember me | Remember email only · stay signed in | **Stay signed in; email always remembered** | Owner: "like it works in a browser" |

---

## 3. Design system

All values at the **reference size 1200 × 645**; the app multiplies every length by the scale factor of
§4.1. Colours are exact; each becomes a named constant in `Source/Common/AppColours.h` (new section
"Stage panel"), never an inline value (names-not-values rule).

### 3.1 Colours

| Role | Value |
|---|---|
| Panel surface | vertical gradient `#2a2b2e` → `#242528`; bottom edge 1 px `#0b0b0c` |
| Page surface (pages over the note area) | vertical gradient `#26272a` → `#212225` |
| Silkscreen text / dim text | `#c8cacd` / `#a3a5aa` |
| Rule lines | `#4a4c51` |
| Rubber button | gradient `#3d3e42` → `#2c2d30`, border `#111214`, drop edge `0 2px 0 #0d0e0f`, top highlight 1 px white 9 % |
| Lit button ("primary") | gradient `#56463a` → `#3f3127`, border `#2a1d14`, text `#ffe2cf` |
| LED off / red / green | `#4a2a24` / `#ff5a36` (glow 3 px, 70 %) / `#5fd35a` (glow 3 px, 70 %) |
| Screen bezel / LCD / LCD ink | `#101112` / `#a7b394` / `#1b2216`; selected rows = ink background, LCD-colour text |
| Beat lamp off | `#2b2c2f` |
| Beat lamp downbeat (red) | gradient `#ff7a66` → `#e3361f`, glow 7 px rgba(255,80,50,.55) |
| Beat lamp other beats (yellow) | gradient `#ffe77a` → `#f0b70c`, glow 7 px rgba(255,205,40,.5) |
| Knob skirt (knurled) | alternating `#18181a` / `#2b2b2e` segments, 5–6° each |
| Knob cap | radial `#55565a` (at 38 % / 30 %) → `#2e2f32` (55 %) → `#1f2022`; pointer line `#f2f2f0` |
| Knob scale ticks | `#8d8f94` |
| Felt strip | `#5e1a1d` → `#43121a` |
| Note area / octave guide lines / falling notes | `#050506` / `#15161a` / `#4f8f86` |
| Timeline lanes background (Sections) | `#121315`; bar lines `#1e2024`, every 4th `#2c2f34` |
| Section type colours | Intro `#c99a3e`, Var `#5f8f84`, Fill `#c8743f`, Break `#8a8f99`, Ending `#b0534a`, Count-in `#7a7aa8` |
| Tempo / playhead accent | `#ffa63a` |

### 3.2 Typography

- **Silkscreen labels:** Barlow Semi Condensed, 600 (labels) and 500 (dim), uppercase, letter spacing
  0.10–0.14 em, 9–11 px; page titles 14 px with 0.2 em spacing.
- **Screen and value text:** Share Tech Mono.
- Both fonts are embedded in the app (BinaryData) and loaded as typefaces at start-up. Both are SIL Open
  Font License fonts: take the TTFs and their `OFL.txt` from the official `google/fonts` repository, commit
  them under `Resources/Fonts/` (licence next to them), and add the TTFs to BinaryData in both `.jucer`s.
  The builder confirms the licence text before committing.

### 3.3 Materials (reusable components)

Each is one small JUCE class or LookAndFeel method, reused everywhere:

- **Rubber button + LED** (`.btn`, `.led`): sizes 26 × 14 (section buttons), 40 × 17 (screen buttons);
  pressed = moves down 1 px, drop edge 1 px. LED above (section buttons) or beside (screen buttons).
- **Segment button** (`.seg`): 22 px high (30 px on Login), text inside, optional LED; lit variant.
- **Knob, quick size:** ring 52, knurled skirt 36, cap 24, 11 ticks from −135° to +135°.
- **Knob, effects size:** ring 72, skirt 50, cap 34, 11 ticks. Value text under it (Share Tech Mono).
  The knob turns with a vertical drag (JUCE rotary-vertical-drag) and the mouse wheel, as JUCE sliders do.
- **Fader** (Style page): slot 4 × 124, cap 28 × 16 with a centre groove, 5 scale marks.
- **LCD list / field:** LCD background, inset shadow, 13–14 px mono text, selected row inverted; fields
  show ◂ ▸ for stepping.
- **Screen:** bezel 368 × 106, LCD inside, lamp row under the LCD (§5.3).

---

## 4. Layout

### 4.1 Regions and scaling

Reference 1200 × 645 (top to bottom): **panel 124 · felt 4 · note area 365 · keys 140 · key slip 12.**

- Scale factor `s = min(width / 1200, height / 645)`. The panel, felt, keys and slip are their reference
  height × `s`; the note area takes the rest. Horizontal positions inside the panel scale by `s` and the
  three panel columns stay centred (§4.2).
- At 1920 × 1032 (`s` = 1.6) the result matches the boards exactly; this is the size of the visual check.
- The restored-down window keeps today's rule (work-area sized; owner decision, see the wiki).

### 4.2 The panel (board `StageRevised`)

Three columns: `1fr | 600 | 1fr`, padding 7 × 20.

- **Left:** transport 2 × 2 (Rec, File, Stop, Start), a 1 px rule, then the section buttons 6 × 3:
  `Intro 1-3, End 1-3 / Var 1-4, Break, Auto Fill / Fill 1-4, Sync Start`. LED red = active section,
  green = an on/off function (Auto Fill, Start). Section-button behaviour = today's buttons exactly.
- **What the left-column buttons do:** **Rec** toggles recording (today's separate record ● / stop ■
  buttons become one toggle; LED red while recording). **File** opens the Recorder page, and pressed again
  closes it (every page button toggles its page). **Stop / Start** = today's style Start / Stop (Start LED
  green while playing). **Sync St.** toggles the existing Synchro start setting (same key as Settings ›
  Arranger; both show the same state). Section-button LEDs follow today's highlighting exactly: only the
  playing section's LED is lit, never two (`MainComponent::highlightArrangerSection`,
  `MainComponentOverlays.cpp:327`); none during a Count-in; the chosen start section is lit while stopped
  (`CurrentStyleComponent::highlightPendingStartSection`). Auto Fill LED = the auto-fill setting.
- **Centre:** four screen buttons on each side of the screen, label + button + LED.
  Left: **Home, Styles, Voices, Tempo −**. Right: **Effects, Settings, Chords, Tempo +**. The LED of the
  page that is open is lit.
- **Right:** "Quick FX" with Part buttons **Left / Right** (= today's left-hand / right-hand instrument
  toggles) and four quick knobs **Volume, Reverb, Chorus, Bright(ness)** acting on the selected part, the
  same controllers as the Effects page.

### 4.3 Note area and keys

The note area is today's GL `NoteLayer` (falling notes, particles), on `#050506` with octave guide lines
at every C. **Pages open over this area** (§6) and the `NoteLayer` must be hidden while a page is open
(AGENTS.md: anything over the live scene hides the GL layer).

---

## 5. The screen

### 5.1 Rows

Top row (11 px): left = context or a message (§5.4), centre = page name (`PLAY`, `EFFECTS`, `SETTINGS`,
`STYLE EDIT`, `VOICES`, `TRACKS`, `NOTES`, `SECTIONS`, `CHORDS`, `RECORDER`, `HOME`, `SIGN IN`), right =
time signature and "TEMPO". Middle row: style name (28 px) and tempo (24 px). Bottom row (13 px):
variation, chord, `BAR nn`.

### 5.2 Content source

Style name, tempo, variation, chord, bar and beat come from the same sources as today's style tab and beat
bar (`CurrentStyleComponent`, `CustomBeatBar` / engine elapsed-beats callback).

### 5.3 Beat lamps

A row of lamps under the LCD, one per beat of the time signature (4 in 4/4, 3 in 3/4), each 8 px high,
4 px apart. The current beat is lit: **red on the downbeat while playing, yellow on the other beats**, off
otherwise. This is the colour logic of today's `BeatBar::paint` (`CustomBeatBar.cpp:53-69`) in hardware
form. The lamps stay dark on Home and Login.

### 5.4 Messages (B1)

Every short notification the app shows today (the `temporaryNotificationUI` toasts that `MainComponent`
raises; the texts live in `MainComponentInit.cpp` / `MainComponentPlayback.cpp`: "Recording started!",
"Recording stopped!", "Playback started/stopped!", device messages…; the plan lists every call site) appears **in the top-left of the
screen for 2.5 s** in upper case, then the normal text returns. Error dialogs that need an answer stay
dialogs, restyled with the page materials.

### 5.5 Tempo (B2)

Range **60–200 BPM** (today's `minTempoBpm` / `maxTempoBpm`, `CurrentStyleComponent.cpp:24-25`).

- **Tempo − / +:** one press = ±1. Holding: one step at once, repeats after **400 ms**, then each next step
  comes faster (130 ms, minus 10 ms per step, never under 30 ms). 120 → 140 ≈ 1.5 s.
- **Click the tempo on the screen:** it becomes an input (LCD-inverted); type digits, **Enter** applies,
  **Esc** cancels, clicking away applies. Values are clamped to the range.
- **Mouse wheel over the tempo:** ±1 per notch, **±5 with Shift**. Vertical drag on the number works too.
- **Right-click the tempo:** back to the style's own tempo; the screen shows `TEMPO BACK TO STYLE: nnn`.
  (Pressing both on-screen buttons at once is impossible with one mouse; this replaces it.)
- **Rule kept from today, exactly:** tempo is locked only **in arranger mode while the arranger plays**,
  and free otherwise (`CurrentStyleComponent.cpp:181`, `:212`, `:225`, each behind
  `if (arrangerModeEnabled)`; switching modes unlocks it, `:366`). The owner's TODO "change BPM while
  playing" is a separate item. Today the slider is just disabled; while locked, the new controls do nothing
  and show the screen message `TEMPO LOCKED WHILE PLAYING` (the only new part).

---

## 6. Pages

A page fills the note area (same size, page surface colour), with a title row: page name in silkscreen,
a rule, page actions, **Exit** on the right. Opening another page replaces it; Exit returns to the play
view. Esc also closes the open page. The open page's screen button LED is lit. Full contents of every
page: **part 2**.

---

## 7. Keys (board `StageRevised`, keys region)

- 88 keys (52 white), white key width = width / 52, 1 px gap; black keys 14 × 88 at reference
  (0.6 × white width, 0.63 × key height), offset like a real keyboard: C# and F# 2 px left, D# and A#
  2 px right, G# centred.
- **White key:** gradient `#e9e6de` (0) → `#f8f7f2` (12 %) → `#fbfaf6` (80 %) → `#efece4`, front lip
  6 px `#d8d3c6`, right edge 1 px black 10 %, bottom corners 3 px.
- **Black key:** gradient `#141415` → `#1e1e20` (82 %) then a lit front slope `#3b3b3f` (84 %) →
  `#2a2a2d`; side bevels 1 px `#2c2c2f` / `#070707`; shadow 1 px right, 3 px down, 4 px blur, 45 %.
- **Pressed** (no green any more): white `#cfcbc1` → `#e6e3da` → `#eeebe3` → `#e4e0d6`, lip 3 px
  `#cfc9bb`, inner top shadow; black `#0e0e0f` → `#18181a` with a shorter slope. The key looks pushed down.
- Kept from today: dimmed keys outside the device range (`min_draw` / `max_draw`, same alpha), note names
  when "Note names on keys" is on.

---

## 8. Dust particles (board `Dust`, B3)

Today (`NoteLayerParticles.cpp:288-292`) every particle gets a new random sideways kick each frame (jitter),
fades linearly then disappears, never changes size, and comes only as one burst per key press.

New Dust (Sparks and Smoke stay as they are):

- **Emission:** a burst of 26 particles at note-on, then **60 particles/s per held key** while the key is
  held; x spread over 90 % of the key width, start at the top edge of the key.
- **Two kinds:** 86 % fine specks (size 0.7–1.8 growing to 1.4–3.0, drawn radius × 2.2, alpha 0.45–0.9),
  14 % soft motes (size 5–9 growing to 11–19, alpha 0.05–0.11). Life 2.2–4.0 s (specks), 1.8–3.2 s (motes).
- **Motion:** a smooth flow field instead of per-frame random kicks:
  `flowX = 0.65 sin(0.011 y + 0.55 t + seed) + 0.35 sin(0.029 y − 0.9 t + 0.004 x)`,
  `flowY = 0.5 cos(0.009 x − 0.45 t) + 0.3 cos(0.023 x + 0.006 y + 0.8 t)`;
  `v = v · e^(−1.4 dt) + (22 flowX, 9 flowY − 16 (1 − life fraction)) dt` (pixels at reference size,
  scaled by `s`). Initial velocity: x ±13, up 14–44 px/s (burst particles × 1.8).
- **Fade and growth:** alpha × smoothstep(0, 0.12, p) × (1 − smoothstep(0.3, 1, p)), with p = age / life;
  size grows with ease-out `1 − (1 − p)²`.
- **Drawing:** additive blending, a soft round sprite (white core → particle colour at 18 % → 40 % alpha at
  45 % → transparent). Particle colour = the user's particle colour (today's colour picker).
- **Budget:** at most 3200 particles; the oldest go first. The simulation stays on the GL render thread
  as today (no allocation in the frame loop: reserve the vector once).

---

## 9. Finishes (skins, board `SkinsFinish`, B4)

- **Default:** the matte charcoal panel of §3 ("Stage"). **Five** optional finishes (owner, 2026-11-23),
  all original: **Rosso burst** (red lacquer, maple sides), **Champagne** (brushed gold, walnut sides),
  **Tobacco burst** (flamed-maple sunburst, dark sides), **Midnight flake** (navy metal flake, chrome trim,
  rosewood sides), **Concert black** (piano gloss, gold pinstripe).
- **Not built:** Pearl accordion (5th row of the `SkinsFinish` board): the owner left it out.
- **Where it applies:** the panel surface and the key slip. Controls, screen and keys keep their own
  materials (as on the board). The board's side strips ("sides") are drawn as 26 px strips at the left and
  right ends of the **panel** only; the keys keep the full width.
- **How:** the textures (wood grain, flame, brushed lines, flake) are generated procedurally with
  fractal noise, using the board's parameters (`feTurbulence` base frequencies, octaves, seeds and colour
  matrices), **once per finish and size** into a cached `juce::Image`, never per frame.
- **Choice:** Settings › Display › **Finish**, saved under a new settings key `PanelFinish` (text id, e.g.
  `stage`, `rosso-burst`). Default `stage`.

---

## 10. Sign-in: Remember me, remembered email, Log out (B5)

Agreed with the owner (2026-11-23), "like it works in a browser":

- **The last email used is always remembered** and filled in on the login screen, with
  "Remembered from last time · *Not you?*". *Not you?* empties the field and forgets the saved email (on
  this computer only; no account is touched).
- **Remember me** (LED toggle under the password, off by default): after a successful login the app keeps
  the session and **logs in by itself on the next starts until the user logs out**.
- **Log out** on Home ("Signed in as …" + Log out): ends the session and the automatic login; the email
  stays remembered.
- **Storage:** the email and the session's refresh token in the settings file (new keys `LastEmail`,
  `RememberedSession`); **never the password**. The token is encrypted with the Windows user's data
  protection (DPAPI) before it is written. If the refresh fails, the login screen shows with the email
  filled in.

**Dependencies (must be done first, they are bugs in the backlog):**
1. `LoginComponent::handleLogin()` returns before checking anything: Login always succeeds. Remember me is
   meaningless until login is real.
2. The auth proxy (`SupabaseClient.cpp:16`, `…/functions/v1/auth-proxy?grant_type=password`) must also
   accept `grant_type=refresh_token`. Its code is **not in this repo**; the owner updates the edge function.
3. `SupabaseClient` builds JSON by string concatenation (a `"` in a password breaks it) and `signup()`
   ignores the reply. Fix both with the sign-in work.
4. "Forgot password?" sends nothing today; it must send the reset request for the email in the field.

**If dependency 2 is not ready** when phase 7 starts, ship the rest first (real login, remembered email,
*Not you?*, Log out) and hide the Remember me toggle until the proxy accepts refresh tokens.

---

## 11. Constraints for the build (from AGENTS.md)

- **Names, not values:** every colour, size, timing and text above becomes a named constant; CI enforces it.
- **800-line cap:** one class per page, split by responsibility (ADR-003). `MainComponent` loses its header
  layout code to a new `StagePanel` component.
- **GL:** pages hide the `NoteLayer`; particles stay on the render thread; no GL calls elsewhere.
- **Threading:** the screen is fed by the existing message-thread callbacks; the audio thread is not touched.
- **Tests never write to `%APPDATA%`.** New logic (tempo stepping and clamping, hold-repeat timing, beat
  lamp state, page routing, finish persistence key, remembered-email / session state machine, dust
  emission maths) gets unit tests first (spec → failing test → code).
- **Fonts and finishes** are loaded once; no image or font loading in `paint()`.

---

## 12. Phasing (input for the plan)

0. Reference PNGs (§0.4); integration map re-checked against the code (§0.1).
1. Design system: colours, fonts, materials (§3), unit-tested where logic exists.
2. Panel + screen + beat lamps + messages + tempo (§4, §5).
3. Keys (§7).
4. Pages, one per PR: Effects, Settings, Style (with track picker + notes), Voices, Chords, Recorder,
   Esc menu, then Sections v2 (part 2).
5. Dust (§8).

Sections v2 and Dust are approved by the owner (2026-11-23, §14); they stay late only because they build on
the earlier phases.
6. Finishes (§9).
7. Sign-in fixes, then Remember me / Log out (§10).

Each phase: integration rows checked (§0.1) → tests → build → live behaviour check → **visual 1:1 check
(§0.2)** → Sonnet review → wiki sync → PR.

---

## 13. Acceptance criteria

- Every row of the integration map (part 2 §P.13) is reachable in the new UI and behaves as before (or as
  §0.3 says), checked live.
- All four test categories green with no fewer tests than before; new unit tests for §11's logic.
- Every screen state of §1.3 passes the visual check of §0.2 with **zero unapproved differences**;
  screenshots and notes saved in the test plans; the owner signs off.
- Saved data from before the redesign loads unchanged; new keys only add.
- No file over 800 lines; names-not-values check clean.

---

## 14. Owner decisions (2026-11-23)

1. **Home and Login:** the **instrument look** of the boards. The forest-and-piano picture
   (`BinaryData::MainWindow_png`) is no longer shown behind Home or Login.
2. **Finishes:** five ship (all but Pearl accordion), §9.
3. **Sections editor v2 and the new Dust:** approved as shown on the boards.
4. Still assumed, not asked: Remember me is **off** by default (like most browsers).

## 15. Calls made in this document (owner, please check)

- Scale factor and the reference size 1920 × 1032 (§4.1).
- Finishes apply to the panel and key slip; side strips only at the panel ends (§9).
- Tempo keeps today's lock rule (arranger mode + playing only) and adds the "locked" screen message (§5.5).
- Rec becomes one toggle; every page button toggles its page; Sync St. on the panel mirrors the setting (§4.2).
- Fonts committed under `Resources/Fonts/` with their OFL licence (§3.2).
- Without refresh-token support in the proxy, Remember me stays hidden and the rest of §10 ships (§10).
- Esc closes the open page; the Esc menu itself stays (part 2 §P.12).
- Error dialogs that need an answer stay dialogs (§5.4).
- The saved session token is encrypted with Windows DPAPI; new settings keys `PanelFinish`,
  `ParticleStyle`, `LastEmail`, `RememberedSession` (§9, §10, part 2 §P.2).
- "Style settings…" on the Style page opens the Voices page at its range / split / transpose block, which
  is where today's playback-settings tab moves (part 2 §P.3, §P.7).
