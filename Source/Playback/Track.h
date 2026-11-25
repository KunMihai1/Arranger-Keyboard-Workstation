#pragma once
#include <JuceHeader.h>
#include "SubjectInterface.h"
#include "TrackListener.h"
#include "CustomToolTip.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageDialog.h"

/** Component IDs of a track strip's parts (findChildWithID). */
namespace TrackStripIds
{
    inline const juce::String name       = "trackName";
    inline const juce::String instrument = "trackInstrument";
    inline const juce::String fader      = "trackFader";
    inline const juce::String volume     = "trackVolume";
    inline const juce::String notes      = "trackNotes";
}

namespace TrackStripText
{
    inline const juce::String notes = "Notes";
}

/**
 * @class Track
 * @brief Represents a single track in the piano application.
 *
 * The `Track` component encapsulates both the visual and logical representation
 * of a single MIDI track. It provides UI controls for volume, instrument selection,
 * and track name, while also handling user interactions such as renaming,
 * copying, pasting, deleting, and displaying note information.
 *
 * It acts as a Subject for `TrackListener`, allowing other components to react
 * to updates such as instrument or volume changes.
 *
 * @see TrackListener, Subject, InstrumentChooserComponent
 */
class Track : public juce::Component,
              private juce::MouseListener,
              public Subject<TrackListener>
{
public:
    //==============================================================================
    /** @brief Callback invoked whenever the track's state changes (e.g., rename, delete). */
    std::function<void()> onChange;

    /**
     * @brief Callback for requesting track selection.
     *
     * This function accepts another callback which is called once a user selects a track,
     * providing the selected name, UUID, and track type.
     */
    std::function<void(std::function<void(const juce::String&, const juce::Uuid&, const juce::String&)>)> onRequestTrackSelection;

    /** @brief Returns whether the application is currently playing. */
    std::function<bool()> isPlaying;

    /** @brief Synchronizes volume changes across percussion tracks. */
    std::function<void(double newVolume)> syncVolumePercussionTracks;

    /** @brief Called when a track is copied. */
    std::function<void(Track* copiedTrack)> onCopy;

    /** @brief Called when a track is pasted. */
    std::function<void(Track* toPaste)> onPaste;

    /** @brief Displays note information for the given track UUID and channel. */
    std::function<void(const juce::Uuid& uuid, int channel)> onShowInformation;

    /** A short message for the Stage screen (e.g. a drum track's instrument cannot change). */
    std::function<void(const juce::String& text)> onInfo;

    /** The in-app Rename / Delete dialog on screen, or nullptr. */
    StageDialog* getOpenDialog() const;

    /** The user chose this strip's instrument (GM program). */
    std::function<void(int program)> onInstrumentChosen;

    /** Slot number (1-8) drawn at the top of the strip, and the Stage scale. */
    void setSlotNumber(int oneBasedSlot);
    void setScale(float newScale);

    /** What the instrument button reads: the General MIDI name when one is set, "Drums" on a percussion
        track, "As recorded" with none set, an em dash on an empty slot. */
    juce::String instrumentCaption() const;

    /** What picking an instrument does: sets and sends it. The track keeps its name (owner, 2026-11-23). */
    void chooseInstrument(int instrumentIndex);

    //==============================================================================
    /** @brief Constructs an empty Track component with default UI elements. */
    Track();

    /** @brief Destructor — safely releases resources and callbacks. */
    ~Track();

    /** @brief Lays out all UI elements within the component. */
    void resized() override;

    /** @brief Paints the outline and background of the track component. */
    void paint(juce::Graphics& g) override;

    /** @brief Handles mouse entry events (used for displaying tooltips). */
    void mouseEnter(const juce::MouseEvent& ev) override;

    /** @brief Handles mouse exit events (hides tooltips). */
    void mouseExit(const juce::MouseEvent& ev) override;

    /**
     * @brief Serializes the track's data to a JSON-style DynamicObject.
     * @return Pointer to a dynamically allocated `juce::DynamicObject` representing this track.
     */
    juce::DynamicObject* getJson() const;

    /**
     * @brief Loads track data from a JSON file.
     * @param file The file containing JSON track data.
     * @return A parsed `juce::var` object containing the track properties.
     */
    juce::var loadJson(const juce::File& file);

    /**
     * @brief Displays a temporary info message in a callout box.
     * @param text The message to display.
     */
    void showInstantInfo(const juce::String& text);

    //==============================================================================
    /** @brief Sets the volume slider's value. */
    void setVolumeSlider(double value);

    /**
     * @brief Updates the volume label.
     * @param value The new volume string.
     * @param shouldNotify Whether listeners should be notified.
     */
    void setVolumeLabel(const juce::String& value, bool shouldNotify = false);

    /** @brief Updates the track's display name. */
    void setNameLabel(const juce::String& name);

    /** @brief Returns the track's display name. */
    juce::String getName() const;

    /** @brief Opens the instrument chooser popup. */
    void openInstrumentChooser();

    /** @brief Builds a list of all available instrument names. */
    juce::StringArray instrumentListBuild();

    /**
     * @brief Sets the track's MIDI instrument number.
     * @param newInstrumentNumber The new instrument number (0–127).
     * @param shouldNotify Whether listeners should be notified.
     */
    void setInstrumentNumber(int newInstrumentNumber, bool shouldNotify = false);

    /** @brief Returns the track's current instrument number. */
    int getInstrumentNumber() const;

    /** @brief Assigns a unique identifier (UUID) to the track. */
    void setUUID(const juce::Uuid& newUUID);

    /** @brief Sets the type of this track (e.g., "melody", "percussion"). */
    void setTypeOfTrack(const juce::String& newType);

    /** @brief Returns the type of this track. */
    juce::String getTypeOfTrack() const;

    /** @brief Returns the UUID of this track. */
    juce::Uuid getUsedID() const;

    /** @brief Sets the MIDI channel used by this track. */
    void setChannel(int newChannel);

    /** @brief Returns the MIDI channel currently used by this track. */
    int getChannel() const;

    /** @brief Returns the track's current volume value. */
    double getVolume() const;

    //==============================================================================
    /**
     * @brief Copies all track parameters from another Track instance.
     * @param other The source track to copy from.
     */
    void copyFrom(const Track& other);

    /**
     * @brief Pastes all stored parameters into this track.
     * @param source The copied track to paste data from.
     */
    void pasteFrom(const Track& source);

    /** @brief Opens a rename dialog for this track. */
    void renameOneTrack();

    /** @brief Opens a confirmation dialog and removes the track if confirmed. */
    void deleteOneTrack();

    /** @brief Copies the track's current state to the clipboard. */
    void copyOneTrack();

    /** @brief Pastes previously copied track data onto this one. */
    void pasteOneTrack();

    /** @brief Displays note information for the current track. */
    void showNotesInformation();

private:
    //==============================================================================
    /** @brief Handles mouse clicks on the name label for left/right-click menu actions. */
    void mouseDown(const juce::MouseEvent& event) override;

    //==============================================================================
    static constexpr int noInstrument = -1;
    static constexpr int defaultChannel = 1;
    int usedInstrumentNumber = noInstrument;     ///< Currently selected MIDI instrument number.
    int channel = defaultChannel;                  ///< Assigned MIDI channel.
    juce::String typeOfTrack;          ///< Type of track ("melody", "percussion", etc.).
    juce::Uuid uniqueIdentifierTrack;  ///< Unique ID for identifying this track.

    //==============================================================================
    StageFader volumeSlider;               ///< Vertical slider controlling track volume.
    juce::Label volumeLabel;                 ///< Displays numeric volume value.
    juce::Label nameLabel;                   ///< Displays the track's name.
    SegButton instrumentChooserButton { {}, SegButton::Style::instrument };
    SegButton notesButton { TrackStripText::notes, SegButton::Style::seg };   ///< Opens the notes page (as "Notes information").
    int slotNumber {};
    float scale = StageMetrics::referenceScale;
    void refreshInstrumentCaption();///< Opens instrument selection popup.
    juce::StringArray instrumentlist;        ///< List of all MIDI instrument names.
    std::unique_ptr<CustomToolTip> toolTipWindow = nullptr; ///< Tooltip window for displaying track name.
    std::unique_ptr<StageDialog> dialog;   ///< Rename / Delete (in-app; juce::AlertWindow stalled the UI ~1 s)
};
