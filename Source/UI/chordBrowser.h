/*
  ==============================================================================

    chordBrowser.h
    Created: 22 Feb 2026 9:52:13pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include <functional>
#include <vector>
#include "HelperFunctions.h"
#include "UI/Chords/ChordHelperModel.h"
#include "UI/Stage/StagePageWidgets.h"

//==============================================================================
/*  The Chord helper page's content (board StageRevised openPage=chords, PageChords): a search and the chord list
    ("name · notes") at the left; the chosen chord's name, notes and three voicings on mini keyboards at the right.
    A click on a row shows that chord and reports it (the host plays it). */
class ChordBrowserComponent : public juce::Component
{
public:
    std::function<void(const Chord& chord)> onChordChosen;
    std::function<void()> onEscape;   ///< Esc in the search box (which keeps the key to itself): the host closes the page

    explicit ChordBrowserComponent(const std::vector<Chord>& allChords);
    ~ChordBrowserComponent() override;

    void setScale(float newScale);

    void setSearchText(const juce::String& text);
    int getNumShown() const;
    /** Shows the shown row's chord and reports it; a row that isn't there changes nothing. */
    void chooseShownRow(int row);
    juce::String getChosenName() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct Rows;

    void rebuildFilteredList();
    ChordHelperRects layout() const;
    void paintVoicing(juce::Graphics& g, const ChordHelperRects& rects, size_t column,
                      const juce::String& title, const std::vector<int>& voicing) const;

    std::vector<Chord> allChords;
    std::vector<size_t> filteredIndices;
    size_t chosen {};
    float scale { StageMetrics::referenceScale };

    StageLcdInput search;
    std::unique_ptr<Rows> rows;
    juce::ListBox listBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordBrowserComponent)
};
