## Built With

- [JUCE](https://juce.com) — an open-source C++ framework for developing cross-platform audio applications.  
- Licensed under the [JUCE License (GPLv3 / Commercial)](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)

This project uses JUCE under the terms of the [GNU General Public License v3 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.html).


## Building

1. Install [JUCE](https://juce.com/get-juce).
2. Open the project in Projucer (`MyPlugin.jucer`).
3. Configure your exporter (Xcode / Visual Studio / CLion).
4. Build and run.


# Piano App — Arranger Keyboard

A software arranger keyboard built with JUCE — the desktop counterpart to a Yamaha PSR or Korg Pa.
You play, record and mix MIDI in real time, and an auto-accompaniment engine backs you: hold a chord
in the left hand and a recorded style follows you through intros, variations, fills and endings.

Sound comes from **sampled instruments (SFZ)**, not synthesis — there are no oscillators in here.

## Features

### Playing
- Real-time play from the on-screen keyboard, a connected MIDI controller, or the PC keyboard —
  mapped by physical scan code, so it works under any OS keyboard layout.
- Split keyboard — independent ranges and instruments for the left and right hand, for layered
  performance.
- Visual keyboard feedback showing which keys are active as you play.

### Arranger
- **Auto-accompaniment engine.** A 10 ms high-resolution timer drives a monotonic beat playhead.
  Section changes (Intro / Variation / Fill / Break / Ending) are queued and take effect exactly on
  the next bar line, and BPM changes alter the rate going forward without jumping musical position.
- **Chord following.** The chord held in the split zone is recognised — quality templates, sus2/sus4
  disambiguation, and hysteresis so passing melody notes don't churn the chord — and every
  accompaniment note is re-pitched at emit time. Per-track transposition modes role-remap chord tones
  (3rd / 5th / 7th), snap passing tones to scale, and handle bass inversion; drums stay fixed.
- **Style authoring.** Record a performance once, carve it into named sections on a draggable
  timeline, and save a self-contained versioned `.style` file. Older files migrate forward.

### Tracks
- Load MIDI files and pick which tracks to play along with.
- Non-destructive editing — adjust timestamps, velocities and notes without altering the original
  recorded data.

### Sound
- 16 SFZ samplers, one per MIDI channel, each with its own effects chain driven by MIDI CCs:
  low-pass filter (CC74 cutoff / CC71 resonance), distortion (CC80), chorus (CC93), reverb (CC91),
  tremolo (CC92), delay (CC94), random mod (CC95) and expression (CC11).
- Every stage is bypassed at its neutral value, so unused effects cost nothing per block, and
  derived values (LFO phase increment, distortion normalisation) are cached rather than recomputed
  per sample.
- Instruments load asynchronously, so the audio thread never blocks on file I/O.

### Backend
- Supabase-backed accounts, MIDI device registration (VID/PID) and playtime tracking, over a
  managed PostgreSQL database.

## Architecture notes

- The arranger runs **parallel** to the classic multi-track player rather than replacing it.
- Scheduling, section sequencing and time math are **pure functions**; the only threaded, impure
  piece is the engine's timer loop. That separation is what makes the engine testable.
- Audio, timer and GUI threads exchange state only through lock-guarded mailboxes — nothing mutates
  engine state directly from the message thread.

## Tech

- **Language / framework:** C++17, JUCE 8.0.13
- **Audio:** real-time SFZ sample playback (SFZero), per-channel MIDI-CC effects chain
- **Backend / API:** Supabase Edge Functions (HTTPS)
- **Database:** PostgreSQL (managed via Supabase)
- **Tests:** 1,300+ assertions across 30+ suites, run headlessly from a separate console target

## What I Learned

- Keeping audio, timer and GUI threads apart without locks in the hot path — and how much easier
  the engine is to test once the scheduling logic is pure.
- Writing per-sample effects by hand (waveshaping, LFOs, circular-buffer delay) and what it costs
  to run sixteen of them per block.
- Real chord/accompaniment transposition, the way hardware arrangers actually do it.
- Structuring a JUCE project so a test target and the shipped app can share one `Source/` tree.

## Roadmap

- Chord variation handling
- Yamaha style import
- Korg style import
- Richer visual feedback: velocity, timing, key animation
