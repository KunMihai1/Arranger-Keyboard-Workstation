# Names, not values

The owner's rule (2026-11-22): every value in the C++ code has a name that says what it means, and
every name says what it holds. `AGENTS.md` carries the short version; this is the full one.

## Numbers
- **No bare numbers, not even 0 or 1.** Name the value, or use an idiom that needs no number:
  - `{}` value-initialisation for "start at zero": `int passes {};`, `for (int trackIndex {}; ...)`
  - `empty()`, `front()` / `back()`, `begin()` / `end()`, `++` / `--`
- Name facts by meaning, not by digit: `maxMidiVelocity = 127`, `drumChannel = 10`, `middleC = 60`,
  `blackKeyLengthRatio = 0.6f`, `semitonesPerOctave = 12`. Never `kOne`, `kTwelve`.
- A table of related values (a chord's intervals, a lookup table) is one named constant.

## Strings
- **No inline strings**: keys, IDs, file names, JSON and settings keys, URLs, user-visible text, test
  names, test categories and test messages are all named constants.
- **Persisted text never changes.** Renaming the *constant* is fine; changing the *string* breaks files
  users already have (`myTracks.json`, `allStyles.json`, settings keys...).

## Identifiers
- Names say what they hold: `trackEntry`, `entryForUuid`, `resultIndex` - never `tr`, `it`, `cfg`, `tmp`,
  `laf`, `c`, `x2`, and **no single-letter names, loop indices included** (`trackIndex`, not `i`).
- The only exception is JUCE's own convention: `g` for the `juce::Graphics&` in `paint()`.

## Where names live
- Used in one file: a namespace at the top of that file (`namespace NoteNaming { ... }`).
- Shared by a subsystem: that subsystem's constants header (e.g. MIDI facts for `Source/Midi`).
- No single global "constants" file.

## Enforcement
- `python ci/check_named_values.py` (first CI step, about a second) flags literals outside a
  named-constant definition and single-letter names other than `g`. Comments, `#include`, enumerator
  values, a pure-virtual `= 0` and `true` / `false` / `nullptr` are fine.
- `ci/named-values-allowlist.txt` lists the files not converted yet. It only shrinks: a file that becomes
  clean must leave it (the check fails otherwise) and new files never join it.
- Conversion happens in dedicated PRs, one subsystem each, with behaviour and file formats unchanged.
  This rule beats "minimal diff" only there; in any other change, lines you add or touch must comply.
