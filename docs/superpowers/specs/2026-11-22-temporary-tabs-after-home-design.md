# Style tab disappears after Home — design

**Status:** bug found live (2026-11-22); fixed on `fix/tab-state-after-home`.

## Problem
The style panel has a "Styles" tab, the open style's tab, and two **temporary** tabs that open on demand
after it: "Playback Settings" (style dropdown → Settings) and "My tracks" (picking a track for a slot).
`Display` remembers that a temporary tab is open in two flags (`createdPlaybackSettingsTab`,
`createdTracksTab`) and closes it by removing **the last tab**.

`Display::homeButtonInteraction` (Home pressed with three tabs) removes the last tab but never clears
those flags. Back on the play screen the stale flag fires on the next tab change and "closes the
temporary tab" again — removing the **style tab** instead. Then `created` is still true, so clicking the
style in the list renames the remaining tab ("Styles") to the style's name.

## Evidence (live, 2026-11-22)
Open Style 1 → dropdown → Settings → Home → Play → click the Styles tab: the Style 1 tab is gone; clicking
Style 1 in the list relabels the only tab "Style 1" while it still shows the list; the style can no longer
be opened until the app restarts. The same happens with "My tracks" as the third tab.

## Options
1. Clear the flags in `homeButtonInteraction`. Fixes this path only; "remove the last tab" stays fragile.
2. **Move the temporary-tab rules into one small class (`TemporaryTabs`, next to `MyTabbedComponent`)
   that closes tabs by name and keeps its own state with the tabs.** Testable without `Display` (whose
   constructor reads and writes app data, so it cannot be built in a test).

## Decision
Option 2. First move the existing logic unchanged (refactor), then a failing test replays the live
sequence, then the fix: Home closes every temporary tab by name and clears its state; tab changes close
a temporary tab by name, never by position. The rules for *when* a tab closes stay as they are ("My
tracks" closes when another tab is selected; "Playback Settings" when "Styles" is selected).

## Acceptance
- Home after "Playback Settings" or "My tracks", then selecting "Styles": the style tab stays.
- Existing closing rules unchanged (tests pin them).
- Unit + arranger + integration suites green; app builds; the live sequence above works.

## Not in scope (noted)
`MyTabbedComponent::mouseDown` ("Close Tab" on right-click) removes tabs without telling `Display`, and
computes the clicked tab from `x` although the tabs are vertical. Possibly the owner's TODO "glitch right
click on a style while playing"; to be reproduced separately.
