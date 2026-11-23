/*
  ==============================================================================

    chordBrowser.cpp
    Created: 22 Feb 2026 9:52:13pm
    Author:  Kisuke

  ==============================================================================
*/

#include "chordBrowser.h"

namespace
{
    constexpr int   chordShapes        = 3;       // root position and two inversions
    constexpr float labelFontHeight    = 13.0f;
    constexpr int   previewPadding     = 6;
    constexpr int   labelHeight        = 22;
    constexpr int   labelInsetX        = 4;
    constexpr int   labelInsetY        = 2;
    constexpr int   imageInset         = 4;
    const juce::Colour hoverColour { 0xffe8f0ff };
    constexpr float rowFontHeight      = 20.0f;
    constexpr int   rowTextInsetX      = 6;
    constexpr int   rowTextInsetY      = 4;
    const juce::String searchHint      = "Search chords...";
    constexpr int   browserWidth       = 1200;
    constexpr int   browserHeight      = 750;
    constexpr int   firstRow           = 0;
    constexpr int   outerPadding       = 8;
    constexpr int   searchHeight       = 28;
    constexpr int   listWidthDivisor   = 3;       // the list takes a third of the width
    constexpr int   listInset          = 2;
    constexpr int   previewInset       = 4;
}

ChordPreview::ChordPreview()
{
    const juce::String labels[] = { "Root", "1st Inversion", "2nd Inversion" };

    for (int shapeIndex {}; shapeIndex < chordShapes; ++shapeIndex)
    {
        auto* container = new juce::Component();
        addAndMakeVisible(container);
        containers.add(container);

        auto* lbl = new juce::Label();
        lbl->setText(labels[shapeIndex], juce::dontSendNotification);
        lbl->setJustificationType(juce::Justification::centred);
        lbl->setFont(juce::Font(labelFontHeight, juce::Font::bold));
        container->addAndMakeVisible(lbl);
        labelsArray.add(lbl);

        auto* imageComp = new juce::ImageComponent();
        imageComp->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        imageComp->setOpaque(true);
        imageComp->setInterceptsMouseClicks(false, false);
        container->addAndMakeVisible(imageComp);
        imageComponents.add(imageComp);
    }
}

void ChordPreview::resized()
{
    auto area = getLocalBounds().reduced(previewPadding);
    int singleH = area.getHeight() / chordShapes;

    for (int shapeIndex {}; shapeIndex < containers.size(); ++shapeIndex)
    {
        auto row = area.removeFromTop(singleH);
        containers[shapeIndex]->setBounds(row);

        
        auto containerLocal = containers[shapeIndex]->getLocalBounds();

        
        auto labelArea = containerLocal.removeFromTop(labelHeight).reduced(labelInsetX, labelInsetY);
        labelsArray[shapeIndex]->setBounds(labelArea);

        
        imageComponents[shapeIndex]->setBounds(containerLocal.reduced(imageInset));
    }
}

void ChordPreview::showChordImages(Chord& chord)
{
    ChordHelper::loadChordNeeded(chord);

    const juce::Image images[] = { chord.imgRoot,chord.imgInv1,chord.imgInv2 };

    for (int shapeIndex {}; shapeIndex < imageComponents.size(); ++shapeIndex)
    {
        if (!images[shapeIndex].isNull())
        {
            ChordHelper::loadChordNeeded(chord);
        }

        imageComponents[shapeIndex]->setImage(images[shapeIndex]);
    }
}

void ChordPreview::clearImages()
{
    for (auto* img : imageComponents)
        img->setImage(juce::Image());
}


ChordRowComponent::ChordRowComponent(Chord& chord, std::function<void(Chord& clicked)> onClick, std::function<void(Chord& hovered)> onHoverEnter, std::function<void()> onHoverExit)
    :chordData{chord}, onClick{onClick}, onHoverEnter{onHoverEnter}, onHoverExit{onHoverExit}
{
    setInterceptsMouseClicks(true, true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

ChordRowComponent::~ChordRowComponent()
{
}

void ChordRowComponent::updateChordData(const Chord& chord)
{
    chordData = chord;
    repaint();
}

void ChordRowComponent::paint(juce::Graphics& g)
{
    auto rowBounds = getLocalBounds();
    if (isMouseOver())
        g.fillAll(hoverColour);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(rowFontHeight);
    g.drawText(chordData.name, rowBounds.reduced(rowTextInsetX, rowTextInsetY), juce::Justification::centredLeft);
}

void ChordRowComponent::mouseEnter(const juce::MouseEvent& ev)
{
    if (onHoverEnter) onHoverEnter(chordData);
    repaint();
}

void ChordRowComponent::mouseExit(const juce::MouseEvent& ev)
{
    if (onHoverExit) onHoverExit();
    repaint();
}

void ChordRowComponent::mouseDown(const juce::MouseEvent& ev)
{
    if (onClick) onClick(chordData);
}

ChordBrowserComponent::ChordBrowserComponent(const std::vector<Chord>& allChords): allChords{allChords}
{
    addAndMakeVisible(search);
    search.setTextToShowWhenEmpty(searchHint, juce::Colours::grey);
    search.addListener(this);

    addAndMakeVisible(listBox);


    listBox.setModel(this);


    addAndMakeVisible(preview);

    rebuildFilteredList();
    setSize(browserWidth, browserHeight);
}

ChordBrowserComponent::~ChordBrowserComponent()
{
    listBox.setModel(nullptr);
    listBox.updateContent();


    search.removeListener(this);
}

int ChordBrowserComponent::getNumRows()
{
    return (int)filteredIndices.size();
}

juce::Component* ChordBrowserComponent::refreshComponentForRow(
    int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    if (rowNumber < firstRow || rowNumber >= (int)filteredIndices.size())
    {
        if (existingComponentToUpdate != nullptr)
            delete existingComponentToUpdate;

        return nullptr;
    }
    Chord& rowChord = allChords[filteredIndices[(size_t)rowNumber]];

    
    auto* row = dynamic_cast<ChordRowComponent*>(existingComponentToUpdate);

    if (row == nullptr)
    {
        
        if (existingComponentToUpdate != nullptr)
            delete existingComponentToUpdate;

        

        row = new ChordRowComponent(rowChord,
            [this](Chord& chosen) { if (onChordChosen) onChordChosen(chosen); },
            [this](Chord& hovered) { showPreviewForChord(hovered); },
            [this]() { clearPreview(); });
    }
    else
    {
        
        row->updateChordData(rowChord);
    }

    return row;
}

void ChordBrowserComponent::paint(juce::Graphics& g)
{
}

void ChordBrowserComponent::resized()
{
    auto area = getLocalBounds().reduced(outerPadding);
    auto top = area.removeFromTop(searchHeight);
    search.setBounds(top.removeFromLeft(area.getWidth()));

    
    auto left = area.removeFromLeft(getWidth() / listWidthDivisor);
    listBox.setBounds(left.reduced(listInset));

    preview.setBounds(area.reduced(previewInset));
}

void ChordBrowserComponent::paintListBoxItem(int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected)
{
}


void ChordBrowserComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    filterText = editor.getText();
    rebuildFilteredList();
}

void ChordBrowserComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
}

void ChordBrowserComponent::textEditorEscapeKeyPressed(juce::TextEditor& editor)
{
    if (onEscape) onEscape();
}

void ChordBrowserComponent::textEditorFocusLost(juce::TextEditor& editor)
{
}

void ChordBrowserComponent::rebuildFilteredList()
{
    filteredIndices.clear();
    for (int chordIndex {}; chordIndex < (int)allChords.size(); ++chordIndex)
    {
        if (filterText.isEmpty()
            || allChords[(size_t)chordIndex].name.toLowerCase().contains(filterText.toLowerCase()))
        {
            filteredIndices.push_back(chordIndex);
        }
    }
    listBox.updateContent();
}

void ChordBrowserComponent::showPreviewForChord(Chord& chord)
{
    preview.showChordImages(chord);

}

void ChordBrowserComponent::clearPreview()
{
    preview.clearImages();
}
