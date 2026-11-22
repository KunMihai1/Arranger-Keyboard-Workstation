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

//==============================================================================
// ChordPreview: shows Root, 1st inv, 2nd inv images
class ChordPreview : public juce::Component
{
public:
    ChordPreview();
    ~ChordPreview() override = default;

    void resized() override;
    void showChordImages(Chord& chord);
    void clearImages();

private:
    juce::OwnedArray<juce::Component> containers;
    juce::OwnedArray<juce::Label> labelsArray;
    juce::OwnedArray<juce::ImageComponent> imageComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordPreview)
};

//==============================================================================
// ChordRowComponent: single row in the list
class ChordRowComponent : public juce::Component
{
public:
    ChordRowComponent(Chord& chord,
        std::function<void(Chord& chord)> onClick,
        std::function<void(Chord& chord)> onHoverEnter,
        std::function<void()> onHoverExit);

    ~ChordRowComponent() override;
    void updateChordData(const Chord& chord);
    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& ev) override;
    void mouseExit(const juce::MouseEvent& ev) override;
    void mouseDown(const juce::MouseEvent& ev) override;

private:
    Chord chordData;
    std::function<void(Chord& chord)> onClick;
    std::function<void(Chord& chord)> onHoverEnter;
    std::function<void()> onHoverExit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordRowComponent)
};

//==============================================================================
// ChordBrowserComponent: main component with search + list + preview
class ChordBrowserComponent : public juce::Component,
    public juce::ListBoxModel,
    private juce::TextEditor::Listener
{
public:
    std::function<void(const Chord& chord)> onChordChosen;

    ChordBrowserComponent(const std::vector<Chord>& allChords);
    ~ChordBrowserComponent() override;

    int getNumRows() override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void paintListBoxItem(int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;

    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

private:
    juce::TextEditor search;
    juce::ListBox listBox;
    ChordPreview preview;

    std::vector<Chord> allChords;
    std::vector<int> filteredIndices;
    juce::String filterText;

    void rebuildFilteredList();
    void showPreviewForChord(Chord& chord);
    void clearPreview();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordBrowserComponent)
};