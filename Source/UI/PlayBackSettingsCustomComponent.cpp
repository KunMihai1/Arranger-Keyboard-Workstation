/*
  ==============================================================================

    PlayBackSettingsCustomComponent.cpp
    Created: 10 Oct 2025 10:16:25pm
    Author:  Kisuke

  ==============================================================================
*/

#include "PlayBackSettingsCustomComponent.h"
#include "IOHelper.h"

namespace
{
    namespace Text
    {
        const juce::String noneItem        = "None";
        const juce::String rangeSeparator  = "-";      // "C2-G4"
        const juce::String leftHand        = "Left hand";
        const juce::String rightHand       = "Right hand";
        const juce::String startNote       = "Start note";
        const juce::String endNote         = "End note";
        const juce::String semitonesSuffix = " semitones";
    }

    constexpr int outerPadding          = 10;
    constexpr int comboWidth            = 85;
    constexpr int comboHeight           = 20;
    constexpr int labelHeight           = 10;
    constexpr int spacing               = 5;
    constexpr int sliderHeight          = 30;
    constexpr int spacingsAboveSlider   = 2;
    constexpr int firstItemId           = 1;
    constexpr int maxTransposeSemitones = 12;
    constexpr int transposeStep         = 1;
    constexpr int noTranspose           = 0;
    constexpr int transposeBoxWidth     = 60;
    constexpr int transposeBoxHeight    = 20;
}

PlayBackSettingsComponent::PlayBackSettingsComponent(int lowestNote, int highestNote, PlayBackSettings& settingsGiven): settings{settingsGiven}
{
    this->lowestNote = lowestNote;
    this->highestNote = highestNote;

    startStopInit();
    handIntervalsInit();
    transposeSliderInit();


}

PlayBackSettingsComponent::~PlayBackSettingsComponent()
{
    startNoteBox.removeListener(this);
    endNoteBox.removeListener(this);
}

void PlayBackSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(outerPadding);

    startNoteLabel.setBounds(area.removeFromLeft(comboWidth).withHeight(labelHeight));
    startNoteBox.setBounds(startNoteLabel.getX(), startNoteLabel.getBottom() + spacing, comboWidth, comboHeight);

    transposeSlider.setBounds(startNoteBox.getX(), startNoteBox.getBottom() + spacing*spacingsAboveSlider, comboWidth, sliderHeight);

    area.removeFromLeft(spacing);

    endNoteLabel.setBounds(area.removeFromLeft(comboWidth).withHeight(labelHeight));
    endNoteBox.setBounds(endNoteLabel.getX(), endNoteLabel.getBottom() + spacing, comboWidth, comboHeight);

    area.removeFromLeft(spacing);

    leftHandBoundLabel.setBounds(area.removeFromLeft(comboWidth).withHeight(labelHeight));
    leftHandBoundBox.setBounds(leftHandBoundLabel.getX(), leftHandBoundLabel.getBottom() + spacing, comboWidth, comboHeight);

    area.removeFromLeft(spacing);

    rightHandBoundLabel.setBounds(area.removeFromLeft(comboWidth).withHeight(labelHeight));
    rightHandBoundBox.setBounds(rightHandBoundLabel.getX(), rightHandBoundLabel.getBottom() + spacing, comboWidth, comboHeight);

}

void PlayBackSettingsComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{   
    if (comboBoxThatHasChanged == &startNoteBox)
    {
        settings.startNote=MapHelper::stringToIntNote(startNoteBox.getText());
    }
    else if (comboBoxThatHasChanged == &endNoteBox)
    {
        settings.endNote =MapHelper::stringToIntNote(endNoteBox.getText());
    }
    else if (comboBoxThatHasChanged == &leftHandBoundBox)
    {
        juce::String text = leftHandBoundBox.getText();
        settings.leftHandBound = MapHelper::stringToIntNote(text.fromFirstOccurrenceOf(Text::rangeSeparator,false,false));
    }
    else if (comboBoxThatHasChanged == &rightHandBoundBox)
    {
        juce::String text = rightHandBoundBox.getText();
        settings.rightHandBound = MapHelper::stringToIntNote(text.upToFirstOccurrenceOf(Text::rangeSeparator, false, false));
    }

    if(onChangingSettings)
        onChangingSettings(settings); //this is to call the listeners of the display, midiHandler for example. MidiHandler could've had a reference to the structure PlayBackSettings
                                      //that lives in the Display class, but it's more flexible this way, the design of the midiHandler.
}

void PlayBackSettingsComponent::handIntervalsInit()
{
    juce::StringArray availableRangeLeft= Text::noneItem;
    juce::StringArray availableRangeRight = Text::noneItem;

    addAndMakeVisible(leftHandBoundBox);
    addAndMakeVisible(rightHandBoundBox);

    leftHandBoundBox.clear();
    rightHandBoundBox.clear();

    addAndMakeVisible(leftHandBoundLabel);

    leftHandBoundLabel.setText(Text::leftHand, juce::dontSendNotification);

    addAndMakeVisible(rightHandBoundLabel);

    rightHandBoundLabel.setText(Text::rightHand, juce::dontSendNotification);

    leftHandBoundBox.addListener(this);
    rightHandBoundBox.addListener(this);

    juce::String lowestNoteString = MapHelper::intToStringNote(lowestNote);
    juce::String highestNoteString = MapHelper::intToStringNote(highestNote);

    for (int noteNumber = lowestNote; noteNumber <= highestNote; noteNumber++)
    {
        availableRangeLeft.add(lowestNoteString + Text::rangeSeparator + MapHelper::intToStringNote(noteNumber));
        availableRangeRight.add(MapHelper::intToStringNote(noteNumber) + Text::rangeSeparator + highestNoteString);
    }
    

    leftHandBoundBox.addItemList(availableRangeLeft, firstItemId);
    rightHandBoundBox.addItemList(availableRangeRight, firstItemId);

    if (settings.leftHandBound != PlaybackSettingsKeys::notSet)
        leftHandBoundBox.setText(lowestNoteString + Text::rangeSeparator + MapHelper::intToStringNote(settings.leftHandBound), juce::dontSendNotification);
    else leftHandBoundBox.setText(Text::noneItem, juce::dontSendNotification);

    if (settings.rightHandBound != PlaybackSettingsKeys::notSet)
        rightHandBoundBox.setText(MapHelper::intToStringNote(settings.rightHandBound) + Text::rangeSeparator + highestNoteString, juce::dontSendNotification);
    else rightHandBoundBox.setText(Text::noneItem, juce::dontSendNotification);
}

void PlayBackSettingsComponent::startStopInit()
{
    juce::StringArray availableNotes = Text::noneItem;
    for (int noteNumber = lowestNote; noteNumber <= highestNote; noteNumber++)
    {
        availableNotes.add(MapHelper::intToStringNote(noteNumber));
    }
    addAndMakeVisible(startNoteBox);
    addAndMakeVisible(endNoteBox);

    startNoteBox.clear();
    endNoteBox.clear();

    startNoteBox.addItemList(availableNotes, firstItemId);

    addAndMakeVisible(startNoteLabel);
    startNoteLabel.setText(Text::startNote, juce::dontSendNotification);
    endNoteBox.addItemList(availableNotes, firstItemId);

    addAndMakeVisible(endNoteLabel);
    endNoteLabel.setText(Text::endNote, juce::dontSendNotification);


    if (settings.startNote != PlaybackSettingsKeys::notSet)
    {
        startNoteBox.setText(MapHelper::intToStringNote(settings.startNote), juce::dontSendNotification);
    }
    else startNoteBox.setText(Text::noneItem, juce::dontSendNotification);
    if (settings.endNote != PlaybackSettingsKeys::notSet)
    {
        endNoteBox.setText(MapHelper::intToStringNote(settings.endNote), juce::dontSendNotification);
    }
    else endNoteBox.setText(Text::noneItem, juce::dontSendNotification);
    startNoteBox.addListener(this);
    endNoteBox.addListener(this);
}

void PlayBackSettingsComponent::transposeSliderInit()
{
    addAndMakeVisible(transposeSlider);
    transposeSlider.setRange(-maxTransposeSemitones, maxTransposeSemitones, transposeStep);
    transposeSlider.setValue(noTranspose);
    transposeSlider.setTextValueSuffix(Text::semitonesSuffix);
    transposeSlider.setSliderStyle(juce::Slider::IncDecButtons);
    transposeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, transposeBoxWidth, transposeBoxHeight);

    transposeSlider.onDragEnd = [this]()
    {
        if (onChangingTranspose)
            onChangingTranspose(transposeSlider.getValue());
    };
}

void PlayBackSettingsComponent::setNewSettings(PlayBackSettings& newSettings)
{
    juce::StringArray availableNotes = Text::noneItem;
    for (int noteNumber = lowestNote; noteNumber <= highestNote; noteNumber++)
    {
        availableNotes.add(MapHelper::intToStringNote(noteNumber));
    }

    startNoteBox.clear();
    endNoteBox.clear();

    startNoteBox.addItemList(availableNotes, firstItemId);
    endNoteBox.addItemList(availableNotes, firstItemId);

    this->settings = newSettings;
    if (settings.startNote != PlaybackSettingsKeys::notSet)
    {
        startNoteBox.setText(MapHelper::intToStringNote(settings.startNote), juce::dontSendNotification);
    }
    else startNoteBox.setText(Text::noneItem, juce::dontSendNotification);
    if (settings.endNote != PlaybackSettingsKeys::notSet)
    {
        endNoteBox.setText(MapHelper::intToStringNote(settings.endNote), juce::dontSendNotification);
    }
    else endNoteBox.setText(Text::noneItem, juce::dontSendNotification);


    juce::String lowestNoteString = MapHelper::intToStringNote(lowestNote);
    juce::String highestNoteString = MapHelper::intToStringNote(highestNote);

    leftHandBoundBox.clear();
    rightHandBoundBox.clear();

    juce::StringArray availableRangeLeft = Text::noneItem;
    juce::StringArray availableRangeRight = Text::noneItem;

    for (int noteNumber = lowestNote; noteNumber <= highestNote; noteNumber++)
    {
        availableRangeLeft.add(lowestNoteString + Text::rangeSeparator + MapHelper::intToStringNote(noteNumber));
        availableRangeRight.add(MapHelper::intToStringNote(noteNumber) + Text::rangeSeparator + highestNoteString);
    }


    leftHandBoundBox.addItemList(availableRangeLeft, firstItemId);
    rightHandBoundBox.addItemList(availableRangeRight, firstItemId);

    if (settings.leftHandBound != PlaybackSettingsKeys::notSet)
        leftHandBoundBox.setText(lowestNoteString + Text::rangeSeparator + MapHelper::intToStringNote(settings.leftHandBound), juce::dontSendNotification);
    else leftHandBoundBox.setText(Text::noneItem, juce::dontSendNotification);

    if (settings.rightHandBound != PlaybackSettingsKeys::notSet)
        rightHandBoundBox.setText(MapHelper::intToStringNote(settings.rightHandBound) + Text::rangeSeparator + highestNoteString, juce::dontSendNotification);
    else rightHandBoundBox.setText(Text::noneItem, juce::dontSendNotification);

    if (onChangingSettings)
        onChangingSettings(settings);
}

void PlayBackSettingsComponent::setLowestHighest(int lowest, int highest)
{
    this->lowestNote = lowest;
    this->highestNote = highest;
}