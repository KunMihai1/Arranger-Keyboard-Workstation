/*
  ==============================================================================

    InstrumentChooser.cpp
    Created: 9 Jul 2025 12:46:50am
    Author:  Kisuke

  ==============================================================================
*/

#include "InstrumentChooser.h"

namespace
{
    const juce::String searchHint = "Click HERE to Search";
    constexpr int   outerPadding    = 6;
    constexpr int   searchHeight    = 18;
    constexpr int   searchGap       = 4;
    constexpr float rowFontHeight   = 16.0f;
    constexpr int   textLeft        = 4;
    constexpr int   rowTop          = 0;
}

InstrumentChooserComponent::InstrumentChooserComponent(juce::StringArray& arr): allInstruments{arr}
{
    searchBox.setTextToShowWhenEmpty(searchHint, juce::Colours::darkgrey);
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    instrumentListBox.setModel(this);
    addAndMakeVisible(instrumentListBox);

    updateListBasedOnSearch();
}

InstrumentChooserComponent::~InstrumentChooserComponent()
{
    searchBox.removeListener(this);
}

void InstrumentChooserComponent::resized()
{
    auto area = getLocalBounds().reduced(outerPadding);
    searchBox.setBounds(area.removeFromTop(searchHeight));
    area.removeFromTop(searchGap);
    instrumentListBox.setBounds(area);
}

int InstrumentChooserComponent::getNumRows()
{
    return filteredInstruments.size();
}

void InstrumentChooserComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::coral);
    g.setColour(juce::Colours::peachpuff);
    g.setFont(rowFontHeight);
    g.drawText(filteredInstruments[rowNumber], textLeft, rowTop, width, height, juce::Justification::centredLeft);
}

void InstrumentChooserComponent::listBoxItemClicked(int row, const juce::MouseEvent& ev)
{
    juce::String name = filteredInstruments[row];
    for (int instrumentIndex {}; instrumentIndex < allInstruments.size(); instrumentIndex++)
    {
        if (name == allInstruments[instrumentIndex])
        {
            if (instrumentSelectedFunction)
                instrumentSelectedFunction(instrumentIndex, name);
            break;
        }
    }
}

void InstrumentChooserComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    updateListBasedOnSearch();
}

void InstrumentChooserComponent::updateListBasedOnSearch()
{
    filteredInstruments.clear();
    auto text = searchBox.getText().trim().toLowerCase();
    for (auto& instr : allInstruments)
    {
        if (text.isEmpty() || instr.toLowerCase().contains(text))
            filteredInstruments.add(instr);
    }

    instrumentListBox.updateContent();
    instrumentListBox.repaint();
}
