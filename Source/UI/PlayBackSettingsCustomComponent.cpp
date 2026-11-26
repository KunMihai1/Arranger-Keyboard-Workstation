/*
  ==============================================================================

    PlayBackSettingsCustomComponent.cpp
    Created: 10 Oct 2025 10:16:25pm
    Author:  Kisuke

  ==============================================================================
*/

#include "PlayBackSettingsCustomComponent.h"
#include "IOHelper.h"
#include "UI/Voices/VoicesPageLayout.h"

namespace
{
    namespace Text
    {
        const juce::String noneItem        = "None";
        const juce::String heading         = "Range, split & transpose";
        const char* const  rangeUtf8       = "Playing range \xc2\xb7 start / end note";
        const char* const  leftHandUtf8    = "Left hand \xc2\xb7 up to";
        const char* const  rightHandUtf8   = "Right hand \xc2\xb7 from";
        const juce::String transpose       = "Transpose";
        const juce::String semitonesSuffix = " semitones";
        const juce::String plusSign        = "+";
    }

    constexpr int firstItemId = 1;   // "None"; the notes follow
    constexpr int noTranspose = 0;
}

namespace TransposeField
{
    juce::String itemText (int semitones)
    {
        return (semitones > noTranspose ? Text::plusSign : juce::String()) + juce::String (semitones) + Text::semitonesSuffix;
    }

    int itemId (int semitones)
    {
        return semitones + maxSemitones + firstItemId;
    }

    int semitonesFor (int id)
    {
        return id - firstItemId - maxSemitones;
    }
}

PlayBackSettingsComponent::PlayBackSettingsComponent(int lowestNote, int highestNote, PlayBackSettings& settingsGiven)
    : lowestNote{ lowestNote }, highestNote{ highestNote }, settings{ settingsGiven },
      rangeLabel (juce::String::fromUTF8 (Text::rangeUtf8), SilkLabel::Tone::dim),
      leftHandBoundLabel (juce::String::fromUTF8 (Text::leftHandUtf8), SilkLabel::Tone::dim),
      rightHandBoundLabel (juce::String::fromUTF8 (Text::rightHandUtf8), SilkLabel::Tone::dim),
      transposeLabel (Text::transpose, SilkLabel::Tone::dim)
{
    for (auto* label : { &rangeLabel, &leftHandBoundLabel, &rightHandBoundLabel, &transposeLabel })
        addAndMakeVisible (*label);
    for (auto* field : { &startNoteBox, &endNoteBox, &leftHandBoundBox, &rightHandBoundBox })
    {
        addAndMakeVisible (*field);
        field->addListener (this);
    }

    addAndMakeVisible (transposeField);
    for (int semitones = -TransposeField::maxSemitones; semitones <= TransposeField::maxSemitones; ++semitones)
        transposeField.addItem (TransposeField::itemText (semitones), TransposeField::itemId (semitones));
    transposeField.setSelectedId (TransposeField::itemId (noTranspose), juce::dontSendNotification);
    transposeField.onChange = [this]
    {
        if (onChangingTranspose)
            onChangingTranspose (getTransposeSemitones());
    };

    refreshFields();
}

PlayBackSettingsComponent::~PlayBackSettingsComponent()
{
    for (auto* field : { &startNoteBox, &endNoteBox, &leftHandBoundBox, &rightHandBoundBox })
        field->removeListener (this);
}

void PlayBackSettingsComponent::setScale (float newScale)
{
    scale = newScale;
    for (auto* field : { &startNoteBox, &endNoteBox, &leftHandBoundBox, &rightHandBoundBox, &transposeField })
        field->setScale (scale);
    for (auto* label : { &rangeLabel, &leftHandBoundLabel, &rightHandBoundLabel, &transposeLabel })
        label->setScale (scale);
    resized();
    repaint();
}

void PlayBackSettingsComponent::setStyleOpen (bool open)
{
    for (auto* field : { &startNoteBox, &endNoteBox, &leftHandBoundBox, &rightHandBoundBox })
        field->setEnabled (open);
    for (auto* label : { &rangeLabel, &leftHandBoundLabel, &rightHandBoundLabel })
        label->setAlpha (open ? StageMetrics::enabledAlpha : StageMetrics::disabledLabelAlpha);
}

void PlayBackSettingsComponent::paint (juce::Graphics& g)
{
    StageLcd::paintHeading (g, rangeColumnLayout (getLocalBounds(), scale).heading.toFloat(), scale, Text::heading, {});
}

void PlayBackSettingsComponent::resized()
{
    const auto rects = rangeColumnLayout (getLocalBounds(), scale);
    rangeLabel.setBounds (rects.rangeLabel);
    startNoteBox.setBounds (rects.startField);
    endNoteBox.setBounds (rects.endField);
    leftHandBoundLabel.setBounds (rects.leftLabel);
    leftHandBoundBox.setBounds (rects.leftField);
    rightHandBoundLabel.setBounds (rects.rightLabel);
    rightHandBoundBox.setBounds (rects.rightField);
    transposeLabel.setBounds (rects.transposeLabel);
    transposeField.setBounds (rects.transposeField);
}

void PlayBackSettingsComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    // "None" reads as notSet (no octave digit), as before.
    const auto note = MapHelper::stringToIntNote (comboBoxThatHasChanged->getText());
    if (comboBoxThatHasChanged == &startNoteBox)
        settings.startNote = note;
    else if (comboBoxThatHasChanged == &endNoteBox)
        settings.endNote = note;
    else if (comboBoxThatHasChanged == &leftHandBoundBox)
        settings.leftHandBound = note;
    else if (comboBoxThatHasChanged == &rightHandBoundBox)
        settings.rightHandBound = note;

    if(onChangingSettings)
        onChangingSettings(settings); //this is to call the listeners of the display, midiHandler for example. MidiHandler could've had a reference to the structure PlayBackSettings
                                      //that lives in the Display class, but it's more flexible this way, the design of the midiHandler.
}

void PlayBackSettingsComponent::refreshFields()
{
    juce::StringArray availableNotes = Text::noneItem;
    for (int noteNumber = lowestNote; noteNumber <= highestNote; noteNumber++)
        availableNotes.add(MapHelper::intToStringNote(noteNumber));

    auto show = [&availableNotes] (StageField& field, int note)
    {
        field.clear (juce::dontSendNotification);
        field.addItemList (availableNotes, firstItemId);
        field.setText (note != PlaybackSettingsKeys::notSet ? MapHelper::intToStringNote (note) : Text::noneItem, juce::dontSendNotification);
    };
    show (startNoteBox, settings.startNote);
    show (endNoteBox, settings.endNote);
    show (leftHandBoundBox, settings.leftHandBound);
    show (rightHandBoundBox, settings.rightHandBound);
    repaint();
}

void PlayBackSettingsComponent::showSettings()
{
    refreshFields();
}

void PlayBackSettingsComponent::setLowestHighest(int lowest, int highest)
{
    this->lowestNote = lowest;
    this->highestNote = highest;
    refreshFields();
}

int PlayBackSettingsComponent::getTransposeSemitones() const
{
    return TransposeField::semitonesFor (transposeField.getSelectedId());
}
