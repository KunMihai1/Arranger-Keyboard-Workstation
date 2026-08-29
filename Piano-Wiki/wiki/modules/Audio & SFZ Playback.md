---
type: module
title: "Audio & SFZ Playback"
path: "Source/Audio/ (AudioHandler, SFZlibrary, SFZLibraryUI)"
status: active
language: cpp
purpose: "Renders MIDI to audio via per-channel SFZ synths with a DSP effects chain"
depends_on:
  - "[[MIDI Handling]]"
used_by:
  - "[[Keyboard & Main UI]]"
created: 2026-06-06
updated: 2026-08-29
tags:
  - module
  - audio
  - sfz
related:
  - "[[MIDI Handling]]"
sources: []
---

# Audio & SFZ Playback

## Purpose
Turns MIDI into sound. Hosts 16 SFZ synths (one per MIDI channel), applies a per-channel DSP effects chain, and manages the library of SFZ instruments and their assignment to styles.

## Key files
- `AudioHandler.h/.cpp` — `juce::AudioIODeviceCallback`. Owns `sfzero::Synth sfzSynths[16]` (one per channel), renders the audio block, and applies a `ChannelDSP` per channel. `loadSfz(file, channel)` loads instruments asynchronously (`pendingLoads`, `onSfzLoadStart/Complete`, `onNoSfzForChannels`). Takes a `MidiHandler&` to pull MIDI.
- `ChannelDSP` (struct in `AudioHandler.h`) — per-channel effects: state-variable filter (CC74 cutoff / CC71 resonance), `juce::Reverb`, `juce::dsp::Chorus`, tremolo, delay line (L/R), distortion (tanh drive), random mod, expression. Caches derived values (e.g. tremolo phase increment, distortion norm factor).
- `SFZlibrary.h/.cpp` — `SFZLibraryManager`: persistent library of `SFZLibraryEntry` (id, name, path) plus `SFZStyleMappings` (style id → {instrument number → entry id}). Add/remove files, assign SFZ to a style+instrument, import mappings between styles, resolve `getSfzForStyleInstrument()`. Also owns the shared GM name table — `getGMInstrumentNames()` / `getGMInstrumentName(int)` (free functions, since instrument numbers are the keys mappings are stored under).
- `SFZLibraryUI.h/.cpp` — UI for managing the SFZ library and assignments. `buildInstrumentList()` just points at `getGMInstrumentNames()`.

## How it works
The audio device drives `AudioHandler`'s callback. It pulls MIDI (live keyboard, [[Arranger Engine]] / [[Track Playback]] inject via `onMidiMessage`, or [[MIDI Handling]]'s record player), feeds each channel's `sfzero::Synth`, then runs that channel's `ChannelDSP` chain before mixing to output. Which SFZ loads on which channel is resolved through `SFZLibraryManager` style/instrument mappings.

## Connects to
- Depends on: [[MIDI Handling]] (`MidiHandler&`, channel routing).
- Used by: [[Keyboard & Main UI]] (`MainComponent` owns `AudioHandler` + `SFZLibraryUI`); inject callbacks from [[Arranger Engine]] and [[Track Playback]].

## Notes / gotchas
> [!key-insight] 16 channels, 16 synths
> One `sfzero::Synth` and one `ChannelDSP` per MIDI channel (index 0–15). CC74/CC71 map to the filter; neutral values (127 cutoff / 0 resonance) mean no filtering.

> [!key-insight] Async loads
> SFZ loading is asynchronous and tracked with atomics; UI reacts via the `onSfzLoad*` callbacks. SFZero lives under `modules/SFZero/`.

> [!key-insight] SFZs are mapped per style + instrument — channels are only routing
> Nothing in `SFZLibraryManager` stores a channel: the map is `styleId → {instrumentNumber → entryId}`. Channels appear one layer down, where `MainComponent::loadSfzForCurrentStyle` resolves a file and routes it — **ch 1 = left hand, ch 16 = right hand, other channels = style tracks** (from `display->getTrackChannelInstruments()`). By the time `AudioHandler` has the file, the style/instrument identity is gone. Keep this in mind for anything user-facing: the user assigns by style + instrument and never sees a channel number.

> [!key-insight] `onNoSfzForChannels` ships a bitmask, not a list
> Detection happens on the **audio thread** (`audioDeviceIOCallbackWithContext`), where allocating is not allowed. So unmapped channels accumulate lock-free in a `std::atomic<int>` via `fetch_or(1 << ch)` (0-based bits), and the message thread claims the batch with `exchange(0)`. `MainComponent` unpacks the mask back into channel numbers.

> [!key-insight] `describeSfzSlot` reverses the routing for user-facing messages (2026-08-11, `b5765e9`)
> The missing-SFZ popup used to print the raw channel mask ("No SFZ loaded for channel: 16") — engine vocabulary the assign dialog never shows. `MainComponent::describeSfzSlot(channel)` now inverts the routing above and reports "Violin (right hand) in style Pop Ballad". It re-reads the **live** state that decided the routing (`midiHandler.getProgramNumberLeft/RightHand()` for ch 1/16, live track objects otherwise) rather than caching anything at load time, so it cannot go stale. **Caveat:** the load path reads ch1/16 program numbers from `propertiesFile` while `describeSfzSlot` reads `midiHandler` — two stores kept in one-way sync by `loadSettings` (`MainComponent.cpp:682`). Any future write to the properties key that bypasses `loadSettings` would make the popup name a stale instrument.

## The `ChannelDSP` chain (mapped 2026-08-29)

`process()` is a **serial, in-place chain**, not a selection. Every block, each stage whose parameter
is off-neutral processes `tempBuffer`; order is fixed and audible:

```
sfzero output → filter (CC74/71) → distortion (CC80) → chorus (CC93) → reverb (CC91)
              → tremolo (CC92) → delay (CC94) → randomMod (CC95) → expression (CC11)
              → gain + constant-power pan → mainBuffer
```

> [!key-insight] Which effects are hand-written vs JUCE's
> **Hand-written, per-sample:** tanh waveshaper distortion with gain compensation (`1/tanh(drive)`
> cached as `distortionNormFactor`), tremolo (5 Hz LFO, manual phase accumulator), delay (raw
> circular `std::vector<float>` L/R with modulo read/write, *not* `juce::dsp::DelayLine`), random mod
> (one-pole smoothed noise). **JUCE's, configured and CC-mapped:** `StateVariableTPTFilter`,
> `juce::Reverb`, `juce::dsp::Chorus`. Useful to know before describing this work anywhere.

> [!key-insight] `> 0.005f` is a neutral-value bypass, not a CC check
> Params are `value / 127.0f`, so the smallest nonzero CC gives ≈0.0079 — the threshold sits just
> under it, cleanly separating CC 0 (skip) from CC 1 (process). Tightest margin is reverb
> (`norm * 0.85` → 0.0067); drop that factor below ~0.64 and CC 1 would silently fall under.
> The filter's check is different on purpose — `filterCutoffCC < 127 || filterResonanceCC > 0` —
> because its neutral is **127** (fully open), not 0. The rule: each bypass encodes what neutral
> means for *that* parameter.

> [!key-insight] Control rate vs audio rate
> `updateCC` (CC number → float, `switch (ccNumber)`) runs only when a MIDI CC arrives.
> `process` runs every block regardless and reads **state**, never events — there is no CC message
> to inspect at that point. That split is why `process` tests floats instead of CC numbers.

> [!key-insight] Post-render effects → no per-voice filtering
> The chain operates on the **summed** audio of every voice on that channel, so there is no per-note
> filter envelope and no key tracking. This is exactly why `AudioHandler.cpp:118-121` documents
> CC76 (filterTrack) and CC73/75/72 (attack/decay/release) as no-ops: they are per-voice concepts
> that post-render DSP cannot reach. True per-voice filtering would mean going inside sfzero.

> [!done] The delay bypass freeze is fixed (2026-08-30, `fix/dsp-quality`)
> The **write** used to sit *inside* `if (delayMix > 0.005f)`, so with the delay off nothing was
> written and `delayWritePos` stopped advancing — re-enabling replayed whatever was in the buffer
> when it was switched off, a stale echo of old material at full mix. The write is now
> unconditional and only the read/mix is gated, on the reasoning that a delay line is a record of
> recent history and has to stay one whether or not anyone is listening. The `%` operations were
> replaced with compare-and-wrap in the same change, which pays for most of the cost of running
> the loop every block. Covered by `test_channel_dsp.cpp` ("bypassing does not freeze the line").

> [!key-insight] Every CC-driven parameter is smoothed per sample (2026-08-30)
> `updateCC` now sets a **target**; `process` walks each `SmoothedParam` toward it one sample at a
> time. Previously a CC was applied as a constant for the whole block, stepping the parameter up to
> 750 times a second — every step a waveform discontinuity. Measured on a modulated filter, that
> plants sidebands at carrier ± n·blockrate roughly 98 dB above where per-sample updating leaves
> them, and because the artifact frequencies track the *host's* block size, the same code sounds
> different at 64 and 128 samples. Time constants are per parameter (expression 8 ms, filter and
> drive 20 ms, mixes and depths 25 ms) — see the reasoning in `prepare()`.
>
> Two things this exposed that were not obvious:
> - **Cutoff is smoothed as `log2(Hz)`, not Hz.** A one-pole covering hertz proportionally to the
>   distance remaining crashes through the top octaves at ~1/(τ·ln2) ≈ 70 octaves/second on a large
>   move, then crawls. That is a lurch, not a sweep, and it registers as a discontinuity.
> - **The filter is crossfaded in, not switched in.** A bypassed filter holds no state, and a
>   two-pole TPT lowpass with zero integrator state returns ≈`a2·x` on its first sample — about
>   0.18 of the input at a 20 kHz cutoff. That click predates the smoothing work; smoothing
>   everything else is what made it audible.

> [!key-insight] The distortion is antialiased with first-order ADAA (2026-08-30)
> `tanh` applied pointwise generates harmonics without limit; the ones above Nyquist fold back to
> `|fs − n·f|`, which are not harmonics of anything and which *descend* as the played note rises.
> That is why heavy drive sounded metallic in the top octaves. `Adaa1Tanh` evaluates the average of
> the curve across each sample interval instead — a difference of `log(cosh)`, its antiderivative.
> Measured in `test_channel_dsp.cpp` on a 5 kHz tone at CC80 = 127: alias-to-signal improves from
> **−13.07 dB to −19.25 dB**, for roughly 20% more CPU in that stage.
>
> Oversampling does far better (−51.7 dB at 4×) but costs ~23×, and **sixteen of these run in
> parallel** — about 38% of a core for one stage in a chain that also runs filters, chorus, reverb,
> delay and a sampler. ADAA is ~1.9% for all sixteen. This is a budget decision, not a claim that
> ADAA is the better algorithm.

> [!bug] The delay read offset still jumps
> `updateCC` case 94 sets `delayReadOffset` instantly. Moving a delay read head is a pitch
> discontinuity — it clicks. Smoothing it needs fractional interpolation on the read position
> (Lagrange), which is a separate change from the mix smoothing already done. Noted at
> `AudioHandler.cpp` case 94.

> [!key-insight] Effects apply to BOTH engines, but different code does the work
> One knob fires both paths, gated on which output is open (`MainComponent.cpp:770-776`, and again
> in `sendEffectsBeforePlaying` `:902-916`): `MidiDevice::sendMidiCC` → external device (its own
> effects engine), `MidiHandler::injectCC` → `ChannelDSP` (yours). CC7/10/91/92/93 follow GM2 so
> both engines agree. **CC80 (distortion), CC94 (delay), CC95 (random mod) diverge** — GM2 reads
> 94 as celeste/detune and 95 as phaser, so those three knobs do something different, or nothing,
> on external gear. See [[MIDI Handling]].
