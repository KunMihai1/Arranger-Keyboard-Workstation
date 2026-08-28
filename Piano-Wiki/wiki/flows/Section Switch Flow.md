---
type: flow
title: "Section Switch Flow"
created: 2026-06-06
updated: 2026-06-06
tags:
  - flow
  - arranger
status: developing
related:
  - "[[Arranger Engine]]"
  - "[[Bar-Synced Section Switching]]"
  - "[[Loop-Seam Note-Off]]"
---

# Flow: pressing a section button mid-playback

Traces one user action — tapping **Fill** while a Variation loops — from UI to sound.

```
[User taps "Fill" button]
        │
        ▼
StyleSectionComponent  (Styles System)
  runs the button's std::function callback
        │  queueSection(Fill, "Fill 1")
        ▼
ArrangerEngine::queueSection
        │  sequencer.queue(Fill, "Fill 1")
        ▼
ArrangerSectionSequencer
  pendingIndex = findSection(Fill,"Fill 1")   ← NOT applied yet
        │
        │   ... timer keeps ticking ...
        ▼
ArrangerEngine::hiResTimerCallback (every 10ms)
  delta → beats; renderRange(from, to)
        │
        ▼
ArrangerSectionSequencer::advance(from, to)
  walks to the next GLOBAL BAR LINE
  at the boundary → applyBoundary():
     pending wins → activeIndex = Fill
     (Fill from a Variation ⇒ returnIndex = that Variation)
  emits SectionSegment{ sectionIndex=Fill, sectionChanged=true }
        │
        ▼
ArrangerEngine::renderRange
  sectionIndex changed ⇒
     outgoing.flushActiveNotes() → noteOffs dispatched   (Loop-Seam Note-Off)
     outgoing.reset(); incoming.reset()
  incoming(Fill).advance(local beats) → events
        │  dispatch(m)
        ▼
   ├─► MIDI-out device  (sendMessageNow)
   └─► onMidiMessage ──► Audio & SFZ Playback (sfzero::Synth + ChannelDSP) ──► speakers
        │
        ▼
[Fill plays for its 1 bar]
  afterComplete = FallThrough ⇒ at next boundary
  applyBoundary returns to returnIndex (the Variation)
```

## Key points
- The switch is **deferred to the next bar line** — never mid-bar. See [[Bar-Synced Section Switching]].
- Hung notes from the outgoing section are closed by `flushActiveNotes` before the new section sounds — see [[Loop-Seam Note-Off]].
- An **Ending** instead sets `stopRequested`, and the engine `haltAudio()`s + stops the timer via `callAsync`.
- Every event goes to both MIDI-out and the SFZ synth via `dispatch`.
