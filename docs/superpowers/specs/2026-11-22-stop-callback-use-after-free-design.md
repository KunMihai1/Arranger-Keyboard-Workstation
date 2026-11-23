# Stop callbacks outlive their player (use-after-free) — design

**Status:** proposed, awaiting owner sign-off · **Date:** 2026-11-22

## Problem

`ArrangerEngine` and `TrackPlayer` post their beat-bar updates to the message thread with a lambda that
captures a raw `this`:

| Where | Code |
|---|---|
| `ArrangerEngine.cpp:417` (in `stop()`) | `callAsync ([this] { if (onElapsedBeats) onElapsedBeats (noBeats); })` |
| `ArrangerEngine.cpp:508` (while playing) | `callAsync ([this, beats] { if (onElapsedBeats) onElapsedBeats (beats); })` |
| `TrackPlayer.cpp:100` (in `stop()`) | `callAsync ([this]() { onElapsedUpdate (noTime); ... })` |
| `TrackPlayer.cpp:311` (timer tick) | `callAsync ([this, beatsElapsed, elapsed] { onElapsedUpdate (beatsElapsed); ... })` |
| `CurrentStyleComponent.cpp:235` | `callAsync ([this]() { isPlaying = false; ... })` |

If the object is destroyed before the queued message runs, the lambda reads freed memory.
`CurrentStyleComponent::~CurrentStyleComponent()` calls `arrangerEngine->stop()` and then destroys the
engine, so **every destruction of a style panel queues a callback into a dead engine**.

There is one style panel, created once (`Display.cpp:189`, only while none exists) and reused for every
style. Closing its tab, going Home, switching styles and Start/Stop do **not** destroy it. The only in-app
trigger is **deleting the style whose tab is open** (`Display.cpp:113-114` resets the panel). Quitting
also destroys it, but the queued message most likely never runs then (not verified).

## Evidence

- Running all test categories in one process (`Project Tests.exe` with no flag) crashed with
  0xC0000005, every time.
- **Bisected** to one unit test, `CurrentStyleComponent (Start with nothing to play)`, followed by any code
  that pumps the message queue. The display test does that, and so does a bare 1.5 s Win32 pump.
- JUCE's crash handler stack: `callAsync` `AsyncCallInvoker::messageCallback` →
  the lambda → `std::_Func_class<void,double>::operator()`. That is a `void(double)` callback, which can
  only be `onElapsedBeats` or `onElapsedUpdate`, called on a freed object.
- The per-category CI runs never pump the queue after this test, which is why CI stays green.
- **Reproduced live (2026-11-23)** in the Debug app: Play → open "Style 1" → right-click it in the
  Styles list → Delete → Remove. The process died within 3 s. Windows log: `0xc0000005` at offset
  `0x2bb6fa`, which the PDB resolves to `std::_Func_class<void,double>::operator()`, the same frame as
  the test-run crash.

## Options

1. **Liveness token** (recommended). Each object owns a `std::shared_ptr<bool>` created in its
   constructor, and each lambda captures a `std::weak_ptr` copy and returns early if it has expired.
   Copying a `weak_ptr` is thread-safe, so the timer-thread posts (`TrackPlayer.cpp:311`) are covered too.
   Small and local, with no change to what callers see.
2. `juce::WeakReference`. Idiomatic, but its master reference is created lazily and is not thread-safe
   when first taken from the timer thread.
3. Cancel pending messages in the destructor. JUCE has no API to cancel a `callAsync`; an `AsyncUpdater`
   rewrite would change the threading of every beat-bar update.

## Decision (proposed)

Option 1 in `ArrangerEngine`, `TrackPlayer` and `CurrentStyleComponent` (for its `callAsync` it can use
the existing `Component::SafePointer` pattern already used at `CurrentStyleComponent.cpp:563`).

## Acceptance criteria

- A new unit test sets `onElapsedBeats` / `onElapsedUpdate`, calls `stop()`, destroys the object,
  pumps the message queue and expects the callback **not** to run. It fails (crashes) before the fix
  and passes after.
- `Project Tests.exe` with no flag completes: unit 870+, arranger 770, integration 130, display 8, with
  0 failures.
- All per-category suites unchanged. Live check: remove the open style and quit the app, with no crash
  in a Debug build.
