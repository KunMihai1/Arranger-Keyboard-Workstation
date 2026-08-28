/*
  ==============================================================================

    AppInfo.h

    Single source of truth for the application's user-facing name. Used for the
    window/app name and for the %APPDATA% data folder that holds settings,
    styles and the device database.

    Header-only and dependency-free on purpose, so decoupled units (e.g.
    MidiDevicesDataBase, which stays testable via IFileSystem) can use it
    without pulling in IOHelper's transitive includes.

  ==============================================================================
*/

#pragma once

namespace AppInfo
{
    /** The application's display name, and the name of its %APPDATA% folder. */
    inline constexpr const char* appName = "Arranger Workstation";
}
