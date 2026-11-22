# Auto Fill plays the fill of the variation you leave — design

**Status:** approved by the owner (2026-11-22: "fill 2 should play"); implemented on `fix/autofill-outgoing-variation`.

## Problem
With Auto Fill on, switching from one variation to another inserts a one-bar fill before the new
variation. `ArrangerSectionSequencer::queue` picks that fill by the number of the variation being
switched **to**: Var 2 → Var 1 plays Fill 1. The owner expects the fill that belongs to the variation
being **left**: Var 2 → Var 1 plays Fill 2 (Var 1 → Var 2 plays Fill 1).

## Evidence
`ArrangerSectionSequencer.cpp`, `queue`: `findSection (ArrangerSectionType::Fill, name)` where `name`
is the target's label, so the fill's trailing number matches the target variation. No test covers
Auto Fill today (no `setAutoFillEnabled` in `tests/`), so the new tests also pin the rest of it.

## Options
1. **Pick the fill by the active (outgoing) variation's number.** Look up the Fill whose trailing number
   matches the section currently playing; fall back to any Fill as today.
2. Make it a user setting (fill of source vs. target). Not asked for; more UI and state.

## Decision
Option 1, the owner's rule. Only the fill lookup in `queue` changes; the landing variation (the
target) and every other transition stay as they are.

## Acceptance
- Var 2 playing, queue Var 1 → Fill 2 plays one bar, then Var 1.
- Var 1 playing, queue Var 2 → Fill 1 plays, then Var 2.
- No fill with the outgoing number → the first Fill of the style; no Fill at all → direct switch.
  (A variation whose name has no number therefore always gets the first Fill; before, the target's
  number could still pick one.)
- Auto Fill off, or switching from a non-variation → unchanged.
- Unit + arranger suites green; app builds.
