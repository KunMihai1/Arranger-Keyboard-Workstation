---
type: concept
title: "GL Overlay Rendering"
created: 2026-06-07
updated: 2026-06-07
tags:
  - concept
  - ui
  - opengl
status: evergreen
related:
  - "[[Keyboard & Main UI]]"
  - "[[Arranger Style Authoring]]"
sources: []
---

# GL Overlay Rendering

Why on-screen overlays (loading label, ESC menu) need special handling in this app.

> [!key-insight] The GL note layer renders ON TOP of normal painting
> `NoteLayer` attaches a `juce::OpenGLContext` (`NoteLayer.cpp`). On Windows that gives it a native child surface composited **above** everything painted the normal (peer) way. So a centered label drawn in `MainComponent::paintOverChildren`, or any peer-painted child, is **hidden behind the note layer** wherever they overlap (the note layer spans the whole middle band of the screen).

## Symptoms this explains
- The "Preparing session…/style…" loading label showed only sometimes / not at all once you were "in" the scene (note layer visible covered it).
- The ESC menu had to be an OS-level top-level window (`addToDesktop`) just to float above the GL layer.

## Patterns adopted (2026-06-07, working tree)
1. **Loading label** (`MainComponent::setLoadingOverlayVisible`): while the label is up, **hide the note layer** (remember + restore its visibility). The label is peer-painted, so with the GL layer gone it's visible.
2. **Play-button overlay**: `playButtonOnClick()` blocks the message thread (opens devices + loads SFZ). You can't force-present a frame while blocked, so the work is **deferred until after the overlay actually paints** — set a `pendingPlayInit` flag + `repaint()`, and kick the blocking work from `paintOverChildren` via `callAsync` once it has drawn. Deterministic; no arbitrary delay.
3. **ESC menu** (`OverlayComponent`, now an **in-app child**, not `addToDesktop`): shown via `setOverlayMenuVisible`, which hides the note layer + keyboard so the menu paints on top, and restores them on close. Being in-app means a crash can't leave a stuck OS window. On close it also calls `NoteLayer::resetState()` so you don't return to stale frozen notes.
4. **Arranger authoring overlays** (style browser/editor, presented by `CurrentStyleComponent`): same problem — they punched through in the middle, showing the timeline as two strips. Fixed via the `onAuthoringOverlayVisible` callback chain → `MainComponent::setArrangerOverlaySceneHidden`, which hides the note layer while any authoring overlay is up (re-entrancy-guarded since browser→editor presents twice).

## Rule of thumb
Any overlay that must appear **over the live scene** either (a) hides the GL `NoteLayer` for its duration, or (b) is drawn inside the GL layer. Don't rely on `setAlwaysOnTop` or `performAnyPendingRepaintsNow` alone — they don't beat the GL child surface, and forcing a paint without yielding to the loop won't present it.

> [!warning] Never call OpenGL from `NoteLayer` component callbacks (message thread)
> `NoteLayer` attaches its own `juce::OpenGLContext`, which renders on a **dedicated render thread**. Only the render-thread hooks — `newOpenGLContextCreated` / `renderOpenGL` / `openGLContextClosing` — may touch GL. Component callbacks like `resized()` run on the **message thread**; calling `openGLContext.makeActive()` / `glViewport()` there makes the context current on the wrong thread and races the render thread, producing an intermittent `GL_DEBUG_TYPE_ERROR` that trips JUCE's `glDebugMessageCallback` assert (`juce_OpenGLContext.cpp:662`) during `swapBuffers` — looks like a crash under the debugger. It's also unnecessary: JUCE sets the viewport itself every frame before `renderOpenGL()` (`CachedImage::renderFrame` line 414). Fixed 2026-08-09 (`cfe100b`): `resized()` now only calls `triggerRepaint()`. The render vs message thread are otherwise mutually excluded — `renderFrame` holds the MessageManager lock across `renderOpenGL()` — so shared state like `particles` needs no extra locking. To diagnose a residual GL error, read the `message` variable at the line-662 break (the "…will use VIDEO memory…" NVIDIA lines are harmless `NOTIFICATION` noise from the per-frame `glBufferData`).
