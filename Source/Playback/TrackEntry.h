/*
  ==============================================================================

    TrackEntry.h
    Created: 7 Jul 2025 9:19:32pm
    Author: Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map>

/**
 * @enum TrackType
 * @brief Specifies the type of track.
 *
 * - Melodic: Tracks that contain pitched notes (e.g., piano, strings)
 * - Percussion: Tracks that contain drums or percussive elements
 */
enum class TrackType { Melodic, Percussion };

/** Track type names as stored in myTracks.json: the text must never change. */
namespace TrackTypeNames
{
    inline const juce::String melodic    = "melodic";
    inline const juce::String percussion = "percussion";
    inline const juce::String unknown    = "unknown";
}

namespace TrackEntryDefaults
{
    constexpr int    notSet      = -1;       // no note / velocity / instrument recorded
    constexpr double bpmNotSet   = -120.0;   // marks a change entry whose BPM was never set
    constexpr int    keepCurrent = -1;       // playback settings: leave this value unchanged
}

/**
 * @struct MidiChangeInfo
 * @brief Stores information about a single MIDI note change.
 *
 * Contains both the old and new values, which can be useful for undo/redo
 * or style/arrangement changes.
 */
struct MidiChangeInfo
{
    double oldBPMchange = TrackEntryDefaults::bpmNotSet;
    double newBPMchange = TrackEntryDefaults::bpmNotSet;

    int oldNumber = TrackEntryDefaults::notSet;       /**< Original MIDI note number */
    double oldTimeStamp {};   /**< Original time stamp (seconds) */
    int oldVelocity = TrackEntryDefaults::notSet;     /**< Original velocity */

    int newNumber = TrackEntryDefaults::notSet;       /**< Updated MIDI note number */
    double newTimeStamp {};   /**< Updated time stamp */
    int newVelocity = TrackEntryDefaults::notSet;     /**< Updated velocity */
};

/**
 * @struct TrackEntry
 * @brief Represents a single track with MIDI data and metadata.
 *
 * Stores sequences, file references, instrument/volume info, and any MIDI changes.
 */
struct TrackEntry
{
    juce::String folderName;  /**< Folder containing this track */
    juce::Uuid uuid;          /**< Unique identifier for the track */
    juce::File file;          /**< Associated file (e.g., MIDI file) */
    int trackIndex {};        /**< Index in a track list */
    juce::String displayName; /**< User-visible name of the track */
    int instrumentAssociated = TrackEntryDefaults::notSet; /**< MIDI instrument assigned */
    double volumeAssociated {}; /**< Track volume */
    double originalBPM {};      /**< Original tempo */

    /**
     * @brief Map of style/arrangement changes.
     *
     * Keyed by a style name string, then by an int (possibly note index), 
     * mapping to `MidiChangeInfo`.
     */
    std::unordered_map<juce::String, std::unordered_map<int, MidiChangeInfo>> styleChangesMap;

    juce::MidiMessageSequence sequence;             /**< Current MIDI sequence */
    juce::MidiMessageSequence originalSequenceTicks; /**< Original sequence in ticks */
    TrackType type = TrackType::Melodic;           /**< Track type */

    /**
     * @brief Returns the display name, or falls back to file name without extension.
     */
    juce::String getDisplayName() const
    {
        return displayName.isNotEmpty() ? displayName : file.getFileNameWithoutExtension();
    }

    /**
     * @brief Returns the UUID as a string.
     */
    juce::String getUniqueID() const
    {
        return uuid.toString();
    }

    /**
     * @brief Generates a new UUID.
     */
    static juce::Uuid generateUUID()
    {
        return juce::Uuid();
    }
};

/**
 * @namespace TrackTypeConversion
 * @brief Helper functions for converting TrackType to/from strings.
 */
namespace TrackTypeConversion
{
    inline juce::String toString(TrackType type)
    {
        if (type == TrackType::Melodic) return TrackTypeNames::melodic;
        else if (type == TrackType::Percussion) return TrackTypeNames::percussion;
        return TrackTypeNames::unknown;
    }

    inline TrackType fromString(const juce::String& name)
    {
        if (name.equalsIgnoreCase(TrackTypeNames::percussion))
            return TrackType::Percussion;
        else
            return TrackType::Melodic;
    }
}
