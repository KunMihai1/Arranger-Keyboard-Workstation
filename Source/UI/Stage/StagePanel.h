/*
  ==============================================================================

    StagePanel.h

    The Stage panel (spec 2026-11-23 §4.2, board StageRevised), laid out as the
    1200 x 124 board scaled by one factor and centred, so it looks the same on
    every window shape:

      left    section buttons: Intro 1-3, End 1-3 / Var 1-4, Break / Fill 1-4, Auto Fill
      centre  Home, Styles, Voices, Tempo -  | screen |  Effects, Settings, Chords, Tempo +
      right   Rec, File / Sync Start, Stop, Start  | Quick FX: Volume, Reverb, Part Left / Right

    The panel only reports presses and shows state; MainComponent wires it to
    today's handlers. Section buttons report the arranger's own names
    ("Ending 1", "Var 2", ...), whatever their printed label.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageModels.h"
#include "StageScreen.h"
#include "StageWidgets.h"

namespace StagePanelNames
{
    // Component names (and the names reported for section buttons).
    inline const juce::String autoFill  = "Auto Fill";
    inline const juce::String rec       = "Rec";
    inline const juce::String file      = "File";
    inline const juce::String syncStart = "Sync Start";
    inline const juce::String stop      = "Stop";
    inline const juce::String start     = "Start";
    inline const juce::String home      = "Home";
    inline const juce::String styles    = "Styles";
    inline const juce::String voices    = "Voices";
    inline const juce::String effects   = "Effects";
    inline const juce::String settings  = "Settings";
    inline const juce::String chords    = "Chords";
    inline const juce::String tempoDown = "Tempo down";
    inline const juce::String tempoUp   = "Tempo up";
    inline const juce::String partLeft  = "Left";
    inline const juce::String partRight = "Right";
    inline const juce::String volume    = "Volume";
    inline const juce::String reverb    = "Reverb";
}

class StagePanel : public juce::Component, private juce::Timer
{
public:
    enum class QuickKnob { volume, reverb };

    explicit StagePanel (double styleTempo);
    ~StagePanel() override;

    void setScale (float newScale);
    StageScreen& screen();

    // --- State shown on the panel ---
    void setSectionLit (const juce::String& sectionName);   // empty = none
    void setAutoFill (bool on);
    void setSyncStart (bool on);
    void setRecording (bool on);
    void setPlaying (bool on);
    void setPageLed (StagePage openPage);
    void setPartLeds (bool leftOn, bool rightOn);
    void setQuickKnobValues (int volumeValue, int reverbValue);

    // --- Presses ---
    std::function<void (const juce::String& sectionName)> onSection;
    std::function<void()> onAutoFill, onSyncStart, onRec, onFile, onStop, onStart;
    std::function<void (StagePage)> onPage;
    std::function<void (bool left)> onPart;
    std::function<void (QuickKnob, int value)> onQuickKnob;
    std::function<void (QuickKnob, int value)> onQuickKnobReleased;   ///< once per turn (and per wheel step): save then

    /** True while a Tempo -/+ button is held and repeating. */
    bool isHoldingTempo() const;

    /** The panel's controls are laid out inside this area (the scaled board, centred). */
    juce::Rectangle<int> getContentBounds() const;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    RubberButton& addKey (const juce::String& name, const juce::String& label, RubberButton::LedColour colour,
                          RubberButton::LabelTone tone);
    RubberButton& addScreenButton (const juce::String& name, const juce::String& label, RubberButton::LabelSide side);
    void startTempoHold (int direction);
    void stopTempoHold();
    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;

    StageScreen stageScreen;
    juce::OwnedArray<RubberButton> buttons;
    std::vector<RubberButton*> sectionButtons;   // in board order, row by row
    std::vector<int> sectionRowLengths;
    std::vector<RubberButton*> transportButtons; // Rec, File / Sync Start, Stop, Start
    std::vector<RubberButton*> leftScreenButtons, rightScreenButtons;
    RubberButton* autoFillButton {};
    RubberButton* recButton {};
    RubberButton* fileButton {};
    RubberButton* syncStartButton {};
    RubberButton* startButton {};
    RubberButton* partLeftButton {};
    RubberButton* partRightButton {};
    std::vector<std::pair<RubberButton*, StagePage>> pageButtons;
    StageKnob volumeKnob { StagePanelNames::volume, StageKnob::Size::quick };
    StageKnob reverbKnob { StagePanelNames::reverb, StageKnob::Size::quick };

    float scale { StageMetrics::referenceScale };
    int holdDirection {};
    int holdStepIndex {};

    // Painted parts, kept from the last layout.
    juce::Rectangle<float> quickFxTitleArea, quickFxRule, partLabelArea, rightRule;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StagePanel)
};
