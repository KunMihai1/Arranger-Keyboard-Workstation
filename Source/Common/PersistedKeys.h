#pragma once
#include <JuceHeader.h>

/*  The keys of every file and settings format the app writes to disk (myTracks.json, mySections.json,
    SFZLibrary.json, the settings file). Users already have files with these exact texts: rename a
    constant if you like, but never change the quoted text - saved data would stop loading. */

// Every key below is part of a file or settings format users already have on disk: the constant names
// may change, the quoted text must not.
namespace DeviceSettingsKeys
{
    inline const juce::String vidPidSeparator  = "_";
    inline const juce::String noDeviceKey      = "_";         // what VID + separator + PID gives with no device
    inline const juce::String defaultDeviceKey = "default";
    inline const juce::String prefixSeparator  = ".";
    inline const juce::String firstHandSuffix  = "First";
    inline const juce::String secondHandSuffix = "Second";
    inline constexpr int firstHandChannel      = 1;
}

namespace AppDataNames
{
    inline const juce::String arrangerStylesFolder = "ArrangerStyles";
}

// myTracks.json
namespace TrackJsonKeys
{
    inline const juce::String folderName  = "folderName";
    inline const juce::String filePath    = "filePath";
    inline const juce::String tracks      = "Tracks";
    inline const juce::String trackIndex  = "trackIndex";
    inline const juce::String displayName = "displayName";
    inline const juce::String uuid        = "uuid";
    inline const juce::String styles      = "Styles";
}

// Per-style note edits inside myTracks.json ("Notes information").
namespace NoteChangeJsonKeys
{
    inline const juce::String oldBpmChange = "oldBPMchange";
    inline const juce::String oldNumber    = "oldNumber";
    inline const juce::String oldTimeStamp = "oldTimeStamp";
    inline const juce::String oldVelocity  = "oldVelocity";
    inline const juce::String newBpmChange = "newBPMchange";
    inline const juce::String newNumber    = "newNumber";
    inline const juce::String newTimeStamp = "newTimeStamp";
    inline const juce::String newVelocity  = "newVelocity";
}

// Settings file: playback zone per style and device.
namespace PlaybackSettingsKeys
{
    inline const juce::String startNote      = "startNote";
    inline const juce::String endNote        = "endNote";
    inline const juce::String leftHandBound  = "leftHandBound";
    inline const juce::String rightHandBound = "rightHandBound";

    inline constexpr int notSet = -1;
    // A range narrower than the smallest common hardware keyboard is the PC-keyboard input, whose notes
    // are stored by pitch class from middle C.
    inline constexpr int smallestHardwareKeyboardKeys = 49;
    inline constexpr int middleC = 60;
    inline constexpr int semitonesPerOctave = 12;
}

// mySections.json
namespace SectionJsonKeys
{
    inline const juce::String id               = "id";
    inline const juce::String name             = "name";
    inline const juce::String startTimeSeconds = "startTimeSeconds";
    inline const juce::String endTimeSeconds   = "endTimeSeconds";
    inline const juce::String startBar         = "startBar";
    inline const juce::String endBar           = "endBar";
}

// Settings file: effects per style, device and hand.
namespace EffectSettingsKeys
{
    inline const juce::String reverb        = "reverb";
    inline const juce::String volume        = "volume";
    inline const juce::String brightness    = "brightness";
    inline const juce::String chorus        = "chorus";
    inline const juce::String expression    = "expression";
    inline const juce::String resonance     = "resonance";
    inline const juce::String sustainToggle = "sustainToggle";
    inline const juce::String attack        = "attack";
    inline const juce::String decay         = "decay";
    inline const juce::String release       = "release";
    inline const juce::String vibrato       = "vibrato";
    inline const juce::String delay         = "delay";
    inline const juce::String pan           = "pan";
    inline const juce::String distortion    = "distortion";
    inline const juce::String filterTrack   = "filterTrack";
    inline const juce::String tremolo       = "tremolo";
    inline const juce::String randomMod     = "randomMod";

    inline constexpr int sustainOn  = 1;
    inline constexpr int sustainOff = 0;
}

// SFZLibrary.json
namespace SfzLibraryJsonKeys
{
    inline const juce::String id            = "id";
    inline const juce::String name          = "name";
    inline const juce::String sfzPath       = "sfzPath";
    inline const juce::String library       = "library";
    inline const juce::String styleMappings = "styleMappings";
}
