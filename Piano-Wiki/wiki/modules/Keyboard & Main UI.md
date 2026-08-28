---
type: module
title: "Keyboard & Main UI"
path: "Source/UI/ (MainComponent, KeyboardUI, NoteLayer, displayGUI, Display, CustomBeatBar, + most view components)"
status: active
language: cpp
purpose: "Central app component wiring all subsystems, plus the on-screen keyboard and display"
depends_on:
  - "[[MIDI Handling]]"
  - "[[Audio & SFZ Playback]]"
  - "[[Track Playback]]"
  - "[[Arranger Engine]]"
  - "[[Styles System]]"
  - "[[Supabase Backend]]"
used_by: []
created: 2026-06-06
updated: 2026-06-25
tags:
  - module
  - ui
related: []
sources: []
---

# Keyboard & Main UI

> [!key-insight] The wiring hub
> `MainComponent` is where everything is owned and connected. Its includes are effectively the system diagram: `MidiHandler`, `KeyboardUI`, `NoteLayer`, `keyListener`, `MidiRecordPlayer`, `displayGUI`, `SectionsComponent`, `LoginComponent`, `PlaytimeTracker`, `SoundEffectWindowComponent`, `SFZLibraryUI`, `AudioHandler`.

## Purpose
The main application screen: hosts the on-screen keyboard, the display/transport, effect knobs, and owns + connects every subsystem.

## Key files
- `MainComponent.h/.cpp` — central component. Also defines custom UI: `SmoothRotarySlider`, `KnobLookAndFeel` (image-based rotary knobs from `BinaryData`). Owns the subsystem objects and wires their callbacks together.
- `KeyboardUI.h` (note: declared in `NoteLayer.h`) / `KeyboardUI.cpp` — visual piano keyboard; `MidiHandlerListener` that lights active notes over a configurable note range.
- `NoteLayer.h/.cpp` — overlay layer for note visuals.
- `displayGUI.h/.cpp` — the main display / transport readout.
- `CustomBeatBar.h/.cpp` — beat/bar position indicator (driven by `onElapsedBeats` from the players).
- Misc UI: `PlayScreenLookAndFeel.h`, `OverlayComponent`, `temporaryNotificationUI`, `CustomToolTip`, `SelectableLabel`.

## Loading overlay (`setLoadingOverlayVisible`)
Full-screen "working" overlay = the always-on-top `openingAudioLabel` (black 0.7 + centered white bold) covering `MainComponent`, with the GL note layer hidden while it's up.
- **Style change**: shows `"Preparing style..."` while SFZ streams (internal, `isOpenAudioOUT`) — and now **also for external MIDI out** (`isOpenOUT`), with a timed hide, so switching styles feels the same on both outputs (`displayInit` → `loadSettingsOnStyleChange`).
- **Arranger off-thread loads**: `display->onArrangerBusy` → `setLoadingOverlayVisible` shows it during background Load/Edit parsing (chain from [[Arranger Style Authoring]]). Note: this overlay can't cover the *editor* (the editor is parented to the top-level window, above `MainComponent`), so in-editor Save/Update use the editor's own internal overlay instead.

## How it works
`MainComponent` instantiates the MIDI/audio/playback/arranger/styles/backend objects, then connects them with `std::function` callbacks (e.g. section buttons → arranger, player elapsed-beats → beat bar, SFZ load → notifications). The `KeyboardUI` and `NoteLayer` subscribe to `MidiHandler` to render notes; the beat bar follows the active player.

## Phase 4 wiring & chord Settings
- **Live chord route:** `MidiHandler.onChordChanged` → `Display::setArrangerLiveChord` → `CurrentStyleComponent::setLiveChord` → `ArrangerEngine::setActiveChord`. See [[Chord Recognition & Transposition]].
- **Settings toggles** (`SettingsWindow` panel, grown to fit): **Bass Inversion** (`onChordBassInversionChanged` → `Display::setArrangerBassInversion`), **Full-Keyboard scan** (`onChordFullKeyboardChanged` → `MidiHandler::setChordScanArea`), **Chord Memory** (`onChordMemoryChanged` → `MidiHandler::setChordMemory`). Startup re-applies the stored values from app properties. `Display`/`CurrentStyleComponent` **remember** bass-inversion and re-apply it when the style component is rebuilt or on each Start (the component is created/destroyed dynamically, so a one-shot set would be lost).

## Keyboard-focus model (Korg/Yamaha "always listening")
The PC keyboard plays only while `MainComponent` holds keyboard focus, so focus must reliably return to the play surface after any UI interaction.
- **Buttons don't steal focus:** transport + section buttons use `setMouseClickGrabsKeyboardFocus(false)`.
- **Global reclaim:** `MainComponent` registers `addMouseListener(this, true)` and its `mouseDown` returns keyboard focus to itself on **any** click that isn't text-entry — skipping modal popups, the in-app overlay menu, and `TextEditor`s. This is the safety net that makes a lost-focus state recoverable by clicking anywhere.
- **Targeted grabs:** after the **Play** init completes (`paintOverChildren` → `playButtonOnClick`, which builds the play scene), and after the arranger **Start** (`CurrentStyleComponent::onRequestPlayFocus` → `Display::onRequestPlayFocus` → MainComponent, deferred), focus is re-grabbed — the scene rebuild could otherwise land focus on a freshly created child, leaving the PC keyboard silent until a stray screen click. Overlay/browser close also re-grabs (`onArrangerOverlayVisible`).
- **BPM lock:** in arranger mode the tempo slider is disabled on Start and re-enabled on Stop (`tempoSlider.setEnabled`), matching classic mode (you can't drag BPM mid-play).

## Connects to
- Depends on: essentially every other module (it owns them).

## Notes / gotchas
> [!gap] Large surface
> `MainComponent.cpp` is the biggest integration point; when tracing a feature, start from the callback wiring here.
