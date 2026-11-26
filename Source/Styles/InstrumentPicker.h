/*
  ==============================================================================

    InstrumentPicker.h

    The Style page's "Choose instrument - Slot N" sub-page (owner, 2026-11-25:
    "like the new UI", the Voices page's look): an LCD search box, the 16
    General MIDI families as a 2 x 8 LCD grid and the chosen family's 8
    programs as an LCD list, the current program selected. Typing searches every
    family; a click on an instrument chooses it.

    It replaces the plain list of 128 names in a CallOutBox.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "UI/Stage/StagePageWidgets.h"
#include "SFZlibrary.h"

namespace GmInstruments
{
    inline constexpr int programsPerFamily = 8;

    /** The 16 General MIDI family names, in program order (family f = programs 8f .. 8f + 7). */
    const juce::StringArray& families();
    int familyOf (int program);
    /** Programs whose name contains the text, any case, in program order; empty text finds nothing. */
    std::vector<int> search (const juce::String& text);
}

struct InstrumentPickerRects
{
    juce::Rectangle<int> search;
    juce::Rectangle<int> familyHeading;
    juce::Rectangle<int> familyList;       ///< the LCD panel: two columns of 8 families
    juce::Rectangle<int> instrumentHeading;
    juce::Rectangle<int> instrumentList;   ///< the LCD panel: 8 rows
};

InstrumentPickerRects instrumentPickerLayout (juce::Rectangle<int> content, float scale);

class InstrumentPicker : public juce::Component
{
public:
    explicit InstrumentPicker (int currentProgram);

    /** A program was clicked. */
    std::function<void (int program)> onChosen;

    void setScale (float newScale);
    void setSearchText (const juce::String& text);
    juce::String getSearchText() const;
    void selectFamily (int family);
    void chooseRow (int row);   ///< what a click on an instrument row does

    int getSelectedFamily() const;
    std::vector<int> getShownPrograms() const;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    void refreshShown();
    juce::Rectangle<int> familyCell (const InstrumentPickerRects& rects, int family) const;
    juce::Rectangle<int> instrumentRow (const InstrumentPickerRects& rects, int row) const;

    int currentProgram;
    int selectedFamily {};
    int firstShownRow {};   ///< search results scroll by the wheel
    std::vector<int> shown;
    float scale { StageMetrics::referenceScale };
    StageLcdInput searchBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPicker)
};
