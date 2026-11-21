/*
  ==============================================================================

    TrackPlayer.h
    Created: 7 Jul 2025 9:02:50pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "TrackEntry.h"
#include "TrackListener.h"
#include "TrackPlayerListener.h"
#include "SubjectInterface.h"
#include "StyleSection.h"

/**
 * @class MultipleTrackPlayer
 * @brief Plays multiple MIDI tracks with support for real-time playback control.
 *
 * Features:
 *   - High-resolution timing for accurate MIDI playback.
 *   - Individual track playback with volume, instrument, and channel management.
 *   - BPM changes during playback.
 *   - Time signature configuration.
 *   - Callback support for elapsed time updates.
 *
 * Inherits from juce::HighResolutionTimer, TrackListener, and Subject for TrackPlayerListener.
 */
class MultipleTrackPlayer: private juce::HighResolutionTimer, public TrackListener
{
public:
    /**
     * @struct TrackPlaybackData
     * @brief Holds the playback state of a single track.
     */
    struct TrackPlaybackData {
        int filteredSequenceIndex = -1; /**< Index of the filtered MIDI sequence */
        int nextEventIndex = 0;         /**< Index of the next MIDI event to play */
    };

    /** 
     * @brief Callback called on elapsed time updates.
     * Provides elapsed time in beats.
     */
    std::function<void(double)> onElapsedUpdate;
    std::function<void()> onStopTriggerClickFromPlayer;
    std::function<void(const juce::MidiMessage&)> onMidiMessage;

    /**
     * @brief Constructor.
     * @param out Weak pointer to a JUCE MIDI output device.
     */
    MultipleTrackPlayer(std::weak_ptr<juce::MidiOutput> out);
    
    /**
     * @brief Changes MIDI playback settings for volume and instrument.
     * @param volume Volume value (0–127).
     * @param instrument Instrument (program change) number.
     */
    void changingMidPlaySettings(int volume, int instrument);

    /**
     * @brief Sets the tracks to be played.
     * @param newTracks Vector of TrackEntry objects representing the tracks.
     */
    void setTracks(const std::vector<TrackEntry>& newTracks);

    /**
     * @brief Sets the current BPM for playback.
     * @param newBPM Beats per minute.
     */
    void setCurrentBPM(int newBPM);

    /** @brief Stops playback and all MIDI notes. */
    void stop(bool shouldModify=true);

    /** @brief True while playback is running (between start() and stop()). */
    bool isPlaying() const { return isTimerRunning(); }

    /** @brief Starts playback from the beginning or current elapsed time. */
    void start();

    /**
     * @brief Updates playback settings (volume, instrument) for a specific channel.
     * @param channel MIDI channel to update.
     * @param newVolume Optional volume (0–127), default -1 = no change.
     * @param newInstrument Optional instrument/program change, default -1 = no change.
     */
    void updatePlaybackSettings(int channel, int newVolume=-1, int newInstrument=-1) override;

    /** @brief Destructor. */
    ~MultipleTrackPlayer();

    /**
     * @brief Applies a BPM change during playback, adjusting MIDI event timings.
     * @param newBPM New BPM value.
     */
    void applyBPMchangeDuringPlayback(double newBPM);

    /**
     * @brief Finds the next event index in a MIDI sequence after a given time.
     * @param seq The MIDI sequence.
     * @param currentTime Current elapsed time in seconds.
     * @return Index of the next MIDI event to play.
     */
    int findNextEventIndex(const juce::MidiMessageSequence& seq, double currentTime);

    /** @brief Synchronizes playback settings (volume, instrument) for all tracks. */
    void syncPlaybackSettings();

    /** @brief Sets the MIDI output device. */
    void setDeviceOutputTrackPlayer(std::weak_ptr<juce::MidiOutput> newOutput);

    /** @brief Sets the time signature denominator. */
    void setTimeSignatureDenominator(int newDenominator);

    /** @brief Sets the time signature numerator. */
    void setTimeSignatureNumerator(int newNumerator);

    void setLastSectionUsed(const StyleSection& s);

    void resetLastSectionUsed();

    const std::vector<TrackEntry>& getCurrentTracks() const;

    void addSubjectTrackPlayerListener(TrackPlayerListener* l);
    void addSubjectTrackPlayerModifyListener(TrackPlayerListenerModifyStateObjects* l);

    void removeSubjectTrackPlayerListener(TrackPlayerListener* l);
    void removeSubjectTrackPlayerModifyListener(TrackPlayerListenerModifyStateObjects* l);



private:
    /** @brief High-resolution timer callback for playback events. */
    void hiResTimerCallback() override;

    // Member variables
    std::vector<juce::MidiMessageSequence> filteredSequences; /**< Filtered MIDI sequences ready for playback */
    int baseChannelTrack = 2;                                  /**< Starting MIDI channel for non-percussion tracks */
    std::vector<TrackEntry> currentTracks;                     /**< Tracks currently loaded */
    std::weak_ptr<juce::MidiOutput> outputDevice;              /**< MIDI output device */
    std::vector<TrackPlaybackData> tracks;                     /**< Playback state for each track */
    std::vector<int> eventIndices;                              /**< Event indices for each track */
    double startTime = 0.0;                                     /**< Start time of playback */
    double currentElapsedTime;                                  /**< Current elapsed time in seconds */
    double currentBPM = 120.0;                                  /**< Current BPM */
    double lastKnownSequenceTime = 0.0;                        /**< Last known time in the sequence */
    int timeSignatureDenominator = 4;                           /**< Time signature denominator */
    int timeSignatureNumerator = 4;                             /**< Time signature numerator */

    Subject<TrackPlayerListener> trackPlayerSubjects;
    Subject<TrackPlayerListenerModifyStateObjects> trackPlayerModifyObjectsSubjects;

    std::optional<StyleSection> lastStyleSectionUsed;
    bool sectionApplied=false;
};
