/*
  ==============================================================================

    InstrumentTreeItem.cpp
    Created: 23 May 2025 11:08:19pm
    Author:  Kisuke

  ==============================================================================
*/

#include "InstrumentTreeItem.h"

namespace
{
    constexpr int textLeft          = 4;
    constexpr int rowTop            = 0;
    constexpr int notMeasured       = 0;    // textBounds starts empty at x 0
    constexpr int widthPadding      = 12;
    constexpr int firstProgram      = 0;
    constexpr int holderWidth       = 600;
    constexpr int holderHeight      = 300;
    constexpr int treeIndent        = 12;
}

InstrumentTreeItem::InstrumentTreeItem(const juce::Image& img, const juce::String& name, int program) : instrumentName{name}, program{program}, img{img}
{
}

InstrumentTreeItem::InstrumentTreeItem(const juce::String& name, int program): instrumentName{name}, program{program}
{
}

bool InstrumentTreeItem::mightContainSubItems()
{
    return getNumSubItems() > int {};
}

bool InstrumentTreeItem::isSelectable()
{
    return instrumentName != InstrumentGroupNames::root && instrumentName != InstrumentGroupNames::pianos && instrumentName != InstrumentGroupNames::basses && instrumentName != InstrumentGroupNames::guitars && 
        instrumentName != InstrumentGroupNames::woodWinds && instrumentName!=InstrumentGroupNames::brass && instrumentName!=InstrumentGroupNames::strings && instrumentName!=InstrumentGroupNames::reeds && instrumentName!=InstrumentGroupNames::organs;
}

void InstrumentTreeItem::paintItem(juce::Graphics& g, int width, int height)
{
    if (shouldHiglight)
    {
        if (isSelected() && isSelectable())
            g.setColour(juce::Colours::goldenrod);
        else g.setColour(juce::Colours::black);
    }
    //g.fillRect(0, 0, width, height);


    g.drawText(instrumentName, textLeft, rowTop, width - textLeft, height, juce::Justification::centredLeft);
    if (textBounds.getX() == notMeasured)
    {
        textBounds.setX(textLeft);
        textBounds.setY(rowTop);
        textBounds.setHeight(height);
        textBounds.setWidth(width);
    }
    shouldHiglight = true;
}

int InstrumentTreeItem::getItemWidth() const
{
    juce::Font font;
    int textWidth = juce::GlyphArrangement::getStringWidthInt(font, instrumentName);
    return textWidth + widthPadding;
}

std::unique_ptr<juce::Component> InstrumentTreeItem::createItemComponent()
{
    return nullptr;
}

juce::String InstrumentTreeItem::getUniqueName() const
{
    return instrumentName;
}

void InstrumentTreeItem::itemClicked(const juce::MouseEvent& event)
{
    auto clickPos = event.getPosition();
    //DBG("ccc" << clickPos.toString() << " " << textBounds.toString());
    if (program >= firstProgram )
    {
        if (clickPos.getX() >= textBounds.getX() && clickPos.getX() <= textBounds.getX() + textBounds.getWidth() &&
            clickPos.getY() >= textBounds.getY() && clickPos.getY() <= textBounds.getY() + textBounds.getHeight() &&
            onProgramSelected)
        {
            onProgramSelected(program,instrumentName);
            shouldHiglight = true;
        }
        else shouldHiglight = false;
    }
    repaintItem();
}

InstrumentTreeItem::~InstrumentTreeItem()
{
    //clearSubItems();
}

TreeViewHolder::TreeViewHolder(juce::TreeView* treeToShow): toShow{treeToShow}
{
    addAndMakeVisible(toShow);
    setSize(holderWidth, holderHeight);
}

void TreeViewHolder::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void TreeViewHolder::resized()
{
    toShow->setBounds(getLocalBounds());
    toShow->setIndentSize(treeIndent);
}



