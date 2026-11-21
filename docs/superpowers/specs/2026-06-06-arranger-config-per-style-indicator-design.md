# Per-style arranger configuration: fix stale state + relocate indicator

**Date:** 2026-06-06
**Branch:** arranger-engine-phase1

## Problem

A single `CurrentStyleComponent` instance is reused for every style tab
(`Display::showCurrentStyleTab`, the `else` branch renames and reloads rather than
recreating). The active arranger configuration is held in three fields on that
reused component:

- `hasActiveArrangerConfig`
- `activeArrangerConfig`
- `activeArrangerConfigName`

When the user switches from Style 1 (which has a loaded `.style` config) to Style 2,
`CurrentStyleComponent::loadJson()` only **sets** these fields when the incoming style
has a saved `arrangerConfig`. It never **clears** them. So Style 1's configuration
lingers and the play-settings dropdown keeps showing it as the selected item (item
id 6) while the user is on Style 2.

Two things are wrong:

1. **Stale state.** Configuration selection must be strictly per-style. Style 2 with
   no saved config must show *no* config selected.
2. **Placement.** The active configuration name should not live in the play-settings
   dropdown as the selected item at all. It belongs in the section-configuration
   browser.

## Goals

- Configuration selection is per-style; switching styles never leaks the previous
  style's configuration.
- The play-settings dropdown no longer contains or displays the active configuration.
- The active configuration is shown only inside the browser
  (`ArrangerStyleListComponent`), marked with a check + "active" label and a subtle
  row highlight.
- The user returns to live tracks by picking "All tracks" / "Solo" in the dropdown
  (existing behavior, kept).

## Non-goals

- No change to how configurations are authored, saved, or played back.
- No change to `ArrangerEngine` / `startPlaying()` selection logic.
- No new "live tracks (none)" row in the browser (rejected during design — the
  dropdown already deactivates).

## Design

### 1. Clear stale config on style load — `CurrentStyleComponent::loadJson`

At the start of the arranger-config restore block, unconditionally reset the three
fields *before* the existing `cfgName.isNotEmpty()` restore runs:

```cpp
hasActiveArrangerConfig  = false;
activeArrangerConfig     = {};
activeArrangerConfigName = {};
// ... existing: if (cfgName.isNotEmpty()) { restore + setStyle + ... }
```

Call `rebuildPlaySettingsItems()` after the block in **both** paths (config restored
or not) so the dropdown selection/tooltip reflect the freshly-loaded style. Today the
rebuild only runs when a config is present.

### 2. Remove the config row from the dropdown

`rebuildPlaySettingsItems()`:

- Delete the `if (hasActiveArrangerConfig) addItem(..., 6)` row. The dropdown is
  exactly: All tracks (1) / Solo (2) / separator / Section Configurations (4) /
  Settings (3).
- Selection always = `lastPlayModeId` (drop the `hasActiveArrangerConfig ? 6 : ...`).
- Tooltip no longer names the configuration (it lives in the browser).

`comboBoxChanged()`:

- id 1 / id 2: keep `lastPlayModeId`; if a config was active, clear it
  (`hasActiveArrangerConfig = false`, name cleared), rebuild, and update the browser's
  active marker.
- id 4 (open browser) and id 3 (settings): reset selection to `lastPlayModeId` (drop
  the `? 6 :`).
- Remove the id-6 no-op branch.

`comboBoxChangeIndex()`: selection = `lastPlayModeId` (drop the `? 6 :`).

### 3. Active config shown in the browser — `ArrangerStyleListComponent`

- Add `void setActiveConfigName (const juce::String& name);` storing
  `juce::String activeName;` and repainting.
- `paintListBoxItem`: when `files[row].getFileNameWithoutExtension() == activeName`,
  draw a leading check mark and a trailing "active" label, plus a subtle highlight
  distinct from the row-selection highlight.

`CurrentStyleComponent` pushes the current active name into the list:

- When opening the browser (`showStyleList(true)` / before `presentOverlay`): call
  `arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String())`.
- After loading a config (`loadStyleFileIntoEngine`, editor `onSaved`): set the active
  name on the list.
- After clearing a config (dropdown id 1/2): set the active name to empty on the list.

### 4. Deactivation

Unchanged in spirit: picking All tracks / Solo clears the active config and the browser
marker. `startPlaying()` is untouched — it plays the active config when present, else
live tracks.

## Files touched

- `Source/Styles/CurrentStyleComponent.cpp` — `loadJson`, `rebuildPlaySettingsItems`,
  `comboBoxChanged`, `comboBoxChangeIndex`, `showStyleList`, `loadStyleFileIntoEngine`,
  editor `onSaved` lambdas.
- `Source/Arranger/ArrangerStyleListComponent.h` / `.cpp` — `setActiveConfigName`,
  `paintListBoxItem`.

## Testing & verification

This is `CurrentStyleComponent` / JUCE `ComboBox` / `ListBox` UI-state logic, which the
headless unit harness does not reach (existing arranger tests cover IO/builder logic
only). Verification is a build + manual run:

1. On Style 1, load a config from the browser. Confirm Start plays it and the browser
   row shows the check + "active".
2. Switch to Style 2 (no saved config). Confirm the dropdown shows a live play-mode and
   **no** configuration is selected anywhere.
3. Open the browser on Style 2. Confirm **no** row is marked active.
4. Switch back to Style 1. Confirm its config is active again (restored from saved JSON)
   and the browser marks it.
5. On a style with an active config, pick "All tracks" → config clears, browser marker
   clears, Start plays live tracks.
