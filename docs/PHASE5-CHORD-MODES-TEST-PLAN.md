# Phase 5 & 6 — Manual Test Plan (Chord Modes + Synchro Start + Count-In)

> **Phase 5** = §1–§4 (chord recognition modes). **Phase 6** = §5 (Synchro Start + Count-In). §6 = other fixes.

What changed: a **Chord Mode** selector (Settings) with three modes — **Fingered** (unchanged),
**Single-Finger** (new, Korg one-finger), **Full-Keyboard** (re-tuned). All on the chord detector;
the accompaniment that flows from it is the existing Phase-4 transposition.

> Everything below is **uncommitted** (working tree). Build is clean; unit/arranger tests pass. This
> file is for the **by-ear / by-hand** checks I can't run.

---

## 0. Setup (once)

- [ ] Close the app if open, then launch `Builds/VisualStudio2022/x64/Debug/App/Project Synth2.exe`
      normally (double-click / Run). The test-runner flag is harmless to normal launch.
- [ ] Enter the play scene, **Arranger mode ON**, load/select a **style** (so there's accompaniment).
- [ ] Make sure a **split point** is set so there's a left-hand chord zone (the chord zone = keys
      **below** the right-hand split). If no split is set, the whole keyboard is the chord zone.
- [ ] Open **Settings** → find the new **"Chord Mode"** row: `Fingered | 1-Finger | Full-Kbd`
      (replaces the old "Full-keyboard chord scan" checkbox). Confirm one is pre-selected.

**Reading the result:** in every test, "you should hear X" = the **accompaniment** (bass/chords of the
style) plays in chord X. Watch/listen to the backing, not the note you pressed.

---

## 1. Single-Finger mode (Korg "One Finger")

Select **Chord Mode → 1-Finger**. Press **Start** (or just hold the keys if the style follows live).
Play these **in the left/chord zone**. The **root is the highest key**; modifiers go to its **left**.

| Play (root = C) | Expect accompaniment |
|---|---|
| - [ ] **C alone** | **C major** |
| - [ ] **C + B** (white key just left of C) | **C7** (dominant 7) |
| - [ ] **C + B♭** (black key just left of C) | **C minor** |
| - [ ] **C + B + B♭** (both, to the left) | **C minor 7** |

- [ ] Repeat rooted on **F** and **G** (F alone → F; F + E → F7; F + E♭ → Fm). Roots should track the
      **highest** key you hold.
- [ ] Sanity: it should feel like a Korg/Yamaha arranger — one or two fingers, instant chord changes,
      **no lag**.

---

## 2. Full-Keyboard mode (bass-driven, anti-lurch)

Select **Chord Mode → Full-Kbd**. Now the **whole keyboard** is scanned (no split zone).

- [ ] Hold a **low C major triad** (left hand) → accompaniment = **C major**.
- [ ] **Keep holding it** and play a **melody on top** (right hand), including notes outside C major
      (e.g. B♭, D, F#) → the backing **stays on C major** (does **not** lurch to C7/other).
- [ ] Move the **left hand** to a **low F major triad** → backing switches to **F** (this should be
      **immediate** — no delay).
- [ ] Play fewer than 3 notes total → backing **holds the last chord** (doesn't drop out).

The point: the **lowest notes (bass hand) drive the chord**; a melody on top never changes it; changes
are instant (no debounce).

---

## 3. Fingered mode (regression — should be unchanged)

Select **Chord Mode → Fingered**.

- [ ] Play full chords in the **left zone** as before: C-E-G → C; C-E♭-G → Cm; C-E-G-B♭ → C7;
      C-E-G-B → Cmaj7; C-E♭-G-B♭ → Cm7. All should behave exactly like before Phase 5.

---

## 4. Persistence & migration

- [ ] Set a mode (e.g. **1-Finger**), **close and relaunch** the app → Settings shows **1-Finger**
      still selected, and it's active.
- [ ] (If you had the old "Full-keyboard chord scan" **on** before this build) on first launch it
      should come up as **Full-Kbd**; otherwise **Fingered**. After that it remembers your choice.

---

## 5. Phase 6 — Transport feel (Synchro Start + Count-In)

Both are toggles in **Settings** (next to Chord Memory / Bass Inversion). Test each on its own first,
then together.

### 5a. Synchro Start
- [ ] Settings → enable **Synchro Start**, disable Count-In. Make sure a style is loaded, Arranger ON.
- [ ] Press **Start with both hands off the keys** → **nothing plays** (the engine is *armed*, waiting).
- [ ] Now **play a chord** in the chord zone → the groove **starts immediately on that chord**.
- [ ] Stop. Press **Start while already holding a chord** → it starts **at once** on the held chord
      (the held-chord seed fires the gate). 
- [ ] With Synchro Start **off** (regression): Start plays immediately as before.

### 5b. Count-In
- [ ] Settings → enable **Count-In**, disable Synchro Start. Press **Start**.
- [ ] You should hear **exactly one bar** of metronome clicks — **louder on beat 1**, softer on 2-3-4 —
      then the **groove begins** on the very next downbeat (clicks are a side-stick on the drum channel).
- [ ] Confirm the groove starts **in time** (no gap/jump after the count-in bar).

### 5c. Both together (Korg order: chord → count-in → groove)
- [ ] Enable **both**. Press Start with hands off → silent. Play a chord → you hear the **count-in bar**,
      then the **groove** comes in on that chord.

---

## 6. Other fixes shipping in this build (quick checks)

Not Phase 5/6, but in the same uncommitted pile:

- [ ] **Frozen notes fix:** play some notes, then **click a different style** → the on-screen notes
      **clear** (no frozen notes stuck on screen); playing afterwards still animates.
- [ ] **Held-note glow:** hold a note until it grows to full height → a **soft amber pulse** breathes
      on its **top edge** (it reads as "still held", not stuck).
- [ ] **Held chord at Start:** hold a chord, press **Start** → it **stays audible** (doesn't go silent).
- [ ] **Tooltip:** hover the **Auto-Fill** toggle → the tooltip shows the **full** text (the leading
      "E" of "Enable…" is no longer clipped).

---

## If something's wrong

Note which test, what you played, and what you heard vs. expected. The detector is pure and unit-tested,
so by-ear mismatches usually point to: split-point/zone setup, the mode not applied, or a chord-memory
interaction. Report it and I'll trace it.
