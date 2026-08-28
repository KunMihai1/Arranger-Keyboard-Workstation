---
type: module
title: "Supabase Backend"
path: "Source/Backend/ (SupabaseClient, LoginComponent, ValidatorUI, PlaytimeTracker, addDeviceWindow)"
status: active
language: cpp
purpose: "Cloud auth, device registration, and playtime/currency tracking via Supabase"
depends_on: []
used_by:
  - "[[Keyboard & Main UI]]"
  - "[[MIDI Handling]]"
created: 2026-06-06
updated: 2026-06-06
tags:
  - module
  - backend
  - auth
related: []
sources: []
---

# Supabase Backend

## Purpose
Connects the desktop app to a Supabase backend for user accounts, device registration, and usage tracking (playtime / in-app currency).

## Key files
- `SupabaseClient.h/.cpp` — thin HTTP client. Returns `HttpResult{ statusCode, body }`. Methods: `login`, `signup`, `incrementPlaytime(seconds, VID, PID)`, `addCurrency`, `addOrUpdateDevice(VID, PID, name, nrKeys)`. Holds `userId` + `accessToken` (mutex-guarded).
- `LoginComponent.h/.cpp` — login/signup UI, calls `SupabaseClient`.
- `ValidatorUI.h/.cpp` — input validation for the auth forms.
- `PlaytimeTracker.h/.cpp` — accumulates session playtime and pushes it via `incrementPlaytime`.
- `addDeviceWindow.h/.cpp` — UI to register a new MIDI device (feeds both Supabase and the local [[MIDI Handling]] device DB).

## How it works
On login/signup `SupabaseClient` stores the access token and user id. While the user plays, `PlaytimeTracker` periodically reports elapsed seconds (tagged with the active device's VID/PID). When a new controller is connected, `addOrUpdateDevice` registers it server-side and the local `MidiDevicesDataBase` caches its key count.

## Connects to
- Used by: [[Keyboard & Main UI]] (`MainComponent` owns `LoginComponent` + `PlaytimeTracker`), [[MIDI Handling]] (device VID/PID, key counts).

## Notes / gotchas
> [!key-insight] Tokens in memory only
> `userId`/`accessToken` live in the client instance (mutex-guarded), set after login. All calls are synchronous HTTP returning `HttpResult`.

> [!gap] Secrets
> Supabase URL/anon key location not yet documented here — check `SupabaseClient.cpp` before changing endpoints.
